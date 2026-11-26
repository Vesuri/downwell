/* Downwell.terrain: tools/generate-pattern-terrain.py (DWPT v4). Cavern
   wall rows for every autotile neighbourhood the level patterns can make,
   the decoration overlays and the four-cell tile groups. */
#include "derived_parts.h"

#define CORRIDOR ((1u << 1) | (1u << 11))
#define KEYS_MAX 2048

typedef struct {
    const u8* p;
    u32 n, count, rowBase, cellBase;
} Patterns;

static int patterns_open(Patterns* t, const Buf* b)
{
    u32 rows;
    REQUIRE(b->n >= 32, "Downwell.patterns is damaged");
    t->p = b->p; t->n = b->n;
    t->count = rd_le32(b->p + 12);
    rows = rd_le32(b->p + 28);
    REQUIRE(t->count < 65536 && rows < (1ul << 24), "Downwell.patterns is damaged");
    t->rowBase = 32 + t->count * 20;
    t->cellBase = t->rowBase + rows * 8;
    REQUIRE(t->cellBase <= b->n, "Downwell.patterns is damaged");
    return 1;
}

/* Pattern i's family, height and first row. */
static void pattern_info(const Patterns* t, u32 i, u32* family, u32* height, u32* first)
{
    const u8* p = t->p + 32 + i * 20;
    *family = rd_le16(p); *height = rd_le16(p + 6); *first = rd_le32(p + 8);
}

/* Row `row`'s cells (bounds checked by the caller's REQUIRE). */
static int pattern_row(const Patterns* t, u32 row, const u8** cells, u32* width)
{
    const u8* p = t->p + t->rowBase + row * 8;
    u32 offset = rd_le32(p);
    *width = rd_le32(p + 4);
    REQUIRE(t->rowBase + row * 8 + 8 <= t->cellBase && offset <= t->n - t->cellBase && *width <= t->n - t->cellBase - offset,
            "Downwell.patterns is damaged");
    *cells = t->p + t->cellBase + offset;
    return 1;
}

/* The occupancy values one pattern row can take (bit 1 + x, mirrored when
   reversed), at most 64: fixed walls plus each subset of up to six optional
   cells. */
typedef struct { u32 n; u16 v[64]; } Values;

static void values_add(Values* s, u32 v)
{
    u32 i;
    for (i = 0; i < s->n; ++i) if (s->v[i] == v) return;
    s->v[s->n++] = (u16)v;
}

static int row_values(const Patterns* t, u32 row, int reverse, Values* out)
{
    const u8* cells;
    u32 width, x, fixed = CORRIDOR, optional[16], nopt = 0, i;
    TRY(pattern_row(t, row, &cells, &width));
    for (x = 0; x < width; ++x) {
        int bit = reverse ? 11 - (int)(x + 1) : (int)x + 2;
        if (bit < 0 || bit >= 13) continue;
        if (cells[x] == '1' || cells[x] == '|') fixed |= 1u << bit;
        else if (cells[x] == '2' && nopt < 16) optional[nopt++] = 1u << bit;
        else if (cells[x] == '2') ++nopt;
    }
    out->n = 0;
    values_add(out, fixed);
    if (nopt <= 6)
        for (i = 0; i < nopt; ++i) {
            u32 k, n = out->n;
            for (k = 0; k < n; ++k) values_add(out, out->v[k] | optional[i]);
        }
    return 1;
}

/* The output streams to its file through a small buffer: no 340 KB image. */
typedef struct { File f; u32 n; int ok, open; u8 b[4096]; } Writer;

static void w_flush(Writer* w)
{
    if (w->ok && w->n && !io_write(&w->f, w->b, w->n)) w->ok = dw_fail("cannot write an output file (disk full?)");
    w->n = 0;
}
static void w_u16(Writer* w, u32 v)
{
    if (w->n + 2 > sizeof w->b) w_flush(w);
    w->b[w->n++] = (u8)(v >> 8); w->b[w->n++] = (u8)v;
}
static void w_u32(Writer* w, u32 v) { w_u16(w, v >> 16); w_u16(w, v & 0xffff); }

typedef struct { u16 m, a, b; } Key;

static int key_less(const Key* x, const Key* y)
{
    if (x->m != y->m) return x->m < y->m;
    if (x->a != y->a) return x->a < y->a;
    return x->b < y->b;
}

/* Insert into the sorted, distinct keys. */
static int key_add(Key* keys, u32* n, u32 m, u32 a, u32 b)
{
    Key k;
    u32 lo = 0, hi = *n;
    k.m = (u16)m; k.a = (u16)a; k.b = (u16)b;
    while (lo < hi) {
        u32 mid = (lo + hi) / 2;
        if (key_less(&keys[mid], &k)) lo = mid + 1; else hi = mid;
    }
    if (lo < *n && !key_less(&k, &keys[lo])) return 1;
    REQUIRE(*n < KEYS_MAX, "too many wall neighbourhoods");
    {
        u32 i;
        for (i = *n; i > lo; --i) keys[i] = keys[i - 1];
    }
    keys[lo] = k;
    ++*n;
    return 1;
}

static int middle_keys(Key* keys, u32* n, const Values* above, const Values* middle, const Values* below)
{
    u32 i, j, k;
    for (i = 0; i < above->n; ++i)
        for (j = 0; j < middle->n; ++j)
            for (k = 0; k < below->n; ++k) {
                u32 m = middle->v[j];
                TRY(key_add(keys, n, m, above->v[i] & m & 0xffe, below->v[k] & m & 0xffe));
            }
    return 1;
}

static int neighbourhoods(const Patterns* t, Key* keys, u32* nkeys, Values* work)
{
    Values *firsts = work, *lasts = work + 1, *above = work + 2, *middle = work + 3, *below = work + 4;
    u32 i, family, height, first, y;
    int reverse;
    firsts->n = lasts->n = 0;
    values_add(firsts, CORRIDOR); values_add(firsts, 0);
    values_add(lasts, CORRIDOR); values_add(lasts, 0);
    for (i = 0; i < t->count; ++i) {
        pattern_info(t, i, &family, &height, &first);
        if (family > 5 || !height) continue;
        for (reverse = 0; reverse < 2; ++reverse) {
            u32 k;
            TRY(row_values(t, first, reverse, middle));
            for (k = 0; k < middle->n; ++k) values_add(firsts, middle->v[k]);
            TRY(row_values(t, first + height - 1, reverse, middle));
            for (k = 0; k < middle->n; ++k) values_add(lasts, middle->v[k]);
        }
    }
    *nkeys = 0;
    for (i = 0; i < t->count; ++i) {
        pattern_info(t, i, &family, &height, &first);
        if (family > 5) continue;
        for (reverse = 0; reverse < 2; ++reverse)
            for (y = 0; y < height; ++y) {
                if (y) *above = *middle; else *above = *lasts;
                if (y) *middle = *below; else TRY(row_values(t, first, reverse, middle));
                if (y + 1 < height) TRY(row_values(t, first + y + 1, reverse, below)); else *below = *firsts;
                TRY(middle_keys(keys, nkeys, above, middle, below));
                if (y + 1 == height) break;
            }
    }
    middle->n = 0;
    values_add(middle, CORRIDOR);
    return middle_keys(keys, nkeys, lasts, middle, firsts);
}

/* Sprite 51's wall frames: 32 words each (16 rows, two planes). */
static int wall_frames(const Buf* game, u16 frames[26][32])
{
    u32 n = dwap_count(game), i, seen = 0, k;
    REQUIRE(n, "Downwell.game is not a DWAP v2 bank");
    for (i = 0; i < n; ++i) {
        DwapRecord r;
        dwap_record(game, i, &r);
        if (r.sprite != 51 || r.frame >= 26) continue;
        REQUIRE(r.ox == 8 && r.oy == 8 && r.width == 16 && r.height == 16 && r.stride == 16 && r.flags == 3 &&
                r.offset + 64 <= game->n, "Downwell.game: unexpected wall tiles");
        for (k = 0; k < 32; ++k) frames[r.frame][k] = rd_be16(game->p + r.offset + 2 * k);
        seen |= 1ul << r.frame;
    }
    REQUIRE(seen == 0x3fffffful, "Downwell.game lacks a wall tile");
    return 1;
}

static void render(Writer* out, const Key* key, u16 frames[26][32])
{
    u32 y, cell, plane, pixel;
    for (y = 0; y < 16; ++y) {
        u16 row[22];
        memset(row, 0, sizeof row);
        for (cell = 0; cell < 11; ++cell) {
            u32 bit = 2u << cell, frame;
            if (!(key->m & bit)) continue;
            frame = (key->a & bit ? 1 : 0) | (key->m & (bit << 1) ? 2 : 0) | (key->b & bit ? 4 : 0) | (key->m & (bit >> 1) ? 8 : 0);
            for (plane = 0; plane < 2; ++plane) {
                u32 value = frames[frame][y + plane * 16];
                for (pixel = 0; pixel < 16; ++pixel) {
                    u32 x = 102 + cell * 16 + pixel;
                    if (x >= 107 && x < 272 && (value & (0x8000u >> pixel)))
                        row[plane * 11 + (x - 96) / 16] |= (u16)(0x8000u >> ((x - 96) & 15));
                }
            }
        }
        for (pixel = 0; pixel < 22; ++pixel) w_u16(out, row[pixel]);
    }
}

/* The decoration overlays, plain and mirrored, aligned to the playfield. */
/* Records (with payload offsets) to `records`, or the payload to `payload`. */
static int overlays(const Buf* game, Writer* records, Writer* payload, u32* count)
{
    static const u16 sprites[6] = {139, 550, 551, 568, 569, 570};
    u32 n = dwap_count(game), i, s;
    int flip;
    u32 at = 0;
    *count = 0;
    for (i = 0; i < n; ++i) {
        DwapRecord r;
        dwap_record(game, i, &r);
        for (s = 0; s < 6 && sprites[s] != r.sprite; ++s) continue;
        if (s == 6) continue;
        REQUIRE(r.flags == 3 && r.width <= 32 && r.height <= 32 &&
                r.offset + (u32)(r.stride / 8) * r.height * 2 <= game->n, "Downwell.game: unexpected decoration");
        for (flip = 0; flip < 2; ++flip) {
            int origin = flip ? r.width - r.ox : r.ox;
            u32 phase = (u32)(110 - origin) & 15, words = (phase + r.width + 15) / 16, y, x, plane, w;
            ++*count;
            if (records) {
                w_u16(records, r.sprite); w_u16(records, r.frame); w_u16(records, (u32)flip);
                w_u16(records, (u32)origin & 0xffff); w_u16(records, (u32)r.oy & 0xffff);
                w_u16(records, words); w_u16(records, r.height); w_u16(records, phase);
                w_u32(records, at);
                at += r.height * words * 6;
                continue;
            }
            if (!payload) continue;
            for (y = 0; y < r.height; ++y) {
                u16 planes[2][3];
                memset(planes, 0, sizeof planes);
                for (plane = 0; plane < 2; ++plane)
                    for (x = 0; x < r.width; ++x) {
                        u32 sx = flip ? r.width - 1 - x : x;
                        if (game->p[r.offset + (plane * r.height + y) * (r.stride / 8) + sx / 8] & (128 >> (sx & 7)))
                            planes[plane][(phase + x) / 16] |= (u16)(0x8000u >> ((phase + x) & 15));
                    }
                for (w = 0; w < words; ++w) {
                    w_u16(payload, planes[0][w] | planes[1][w]); w_u16(payload, planes[0][w]); w_u16(payload, planes[1][w]);
                }
            }
        }
    }
    return 1;
}

/* Every four-cell group of tile glyphs (box colours, deletions) the patterns allow. */
static const u16 power[4] = {1, 5, 25, 125};

static int group_codes(const Patterns* t, u8 codes[625])
{
    u32 i, family, height, first, y, x, code;
    int reverse;
    memset(codes, 0, 625);
    codes[0] = 1;
    for (i = 0; i < t->count; ++i) {
        pattern_info(t, i, &family, &height, &first);
        if (family > 5) continue;
        for (reverse = 0; reverse < 2; ++reverse)
            for (y = 0; y < height; ++y) {
                const u8* cells;
                u32 width, start;
                u8 choices[12]; /* 0: blank only, 1: 0..3, 2: 0 or 4 */
                memset(choices, 0, sizeof choices);
                TRY(pattern_row(t, first + y, &cells, &width));
                for (x = 0; x < width; ++x) {
                    int cell = reverse ? 9 - (int)x : (int)x + 1;
                    if (cell < 0 || cell >= 11) continue;
                    choices[cell] = cells[x] == 'B' || cells[x] == '3' ? 1 : cells[x] == '-' ? 2 : 0;
                }
                for (start = 0; start < 12; start += 4) {
                    u32 combo, combos = 1, j;
                    for (j = 0; j < 4; ++j) combos *= choices[start + j] == 1 ? 4 : choices[start + j] == 2 ? 2 : 1;
                    for (combo = 0; combo < combos; ++combo) {
                        u32 rest = combo;
                        code = 0;
                        for (j = 0; j < 4; ++j) {
                            u32 c = choices[start + j], digit = 0;
                            if (c == 1) { digit = rest % 4; rest /= 4; }
                            else if (c == 2) { digit = rest % 2 ? 4 : 0; rest /= 2; }
                            code += digit * power[j];
                        }
                        codes[code] = 1;
                    }
                }
            }
    }
    return 1;
}

static void group_write(Writer* out, u16 frames[26][32], const u8 codes[625])
{
    static const u8 tileFrame[4] = {16, 17, 22, 23};
    u32 code, y, x, index = 0;
    for (code = 0; code < 625; ++code) w_u16(out, codes[code] ? index++ : 65535);
    for (code = 0; code < 625; ++code) {
        u16 pixels[16][10];
        u32 cell, plane;
        if (!codes[code]) continue;
        memset(pixels, 0, sizeof pixels);
        for (cell = 0; cell < 4; ++cell) {
            u32 digit = code / power[cell] % 5, frame;
            if (!digit) continue;
            frame = tileFrame[digit - 1];
            for (y = 0; y < 16; ++y)
                for (plane = 0; plane < 2; ++plane) {
                    u32 value = frames[frame][plane * 16 + y];
                    for (x = 0; x < 16; ++x)
                        if (value & (0x8000u >> x)) {
                            u32 px = 6 + cell * 16 + x;
                            pixels[y][plane * 5 + px / 16] |= (u16)(0x8000u >> (px & 15));
                        }
                }
        }
        for (y = 0; y < 16; ++y) for (x = 0; x < 10; ++x) w_u16(out, pixels[y][x]);
    }
}

int derived_pattern_terrain(Context* c, const Buf* game, const Buf* patterns)
{
    Patterns t;
    u16 (*frames)[32] = 0;
    Key* keys = 0;
    Values* work = 0;
    Writer* w = 0;
    u8 codes[625];
    u32 nkeys = 0, noverlays = 0, ngroups = 0, i;
    int ok;
    TRY(patterns_open(&t, patterns));
    frames = dw_alloc(26 * 32 * 2);
    keys = dw_alloc(KEYS_MAX * sizeof(Key));
    work = dw_alloc(5 * sizeof(Values));
    w = dw_alloc(sizeof(Writer));
    ok = frames && keys && work && w && wall_frames(game, frames) && neighbourhoods(&t, keys, &nkeys, work) &&
         overlays(game, 0, 0, &noverlays) && group_codes(&t, codes);
    if (ok && nkeys * 16 >= 65536) ok = dw_fail("too many wall neighbourhoods");
    if (ok) {
        char path[512];
        for (i = 0; i < 625; ++i) ngroups += codes[i];
        dw_path(path, sizeof path, c->dest, "Downwell.terrain");
        w->n = 0;
        w->ok = io_open(&w->f, path, 1) ? 1 : dw_fail("cannot create an output file");
        if (w->ok) {
            w_u32(w, 0x44575054ul); /* DWPT */
            w_u16(w, 4); w_u16(w, nkeys); w_u16(w, nkeys * 16); w_u16(w, noverlays); w_u16(w, ngroups);
            for (i = 0; i < nkeys; ++i) { w_u16(w, keys[i].m); w_u16(w, keys[i].a); w_u16(w, keys[i].b); w_u16(w, i * 16); }
            for (i = 0; i < nkeys; ++i) render(w, &keys[i], frames);
            ok = overlays(game, w, 0, &noverlays) && overlays(game, 0, w, &noverlays);
            group_write(w, frames, codes);
            w_flush(w);
            if (!io_close(&w->f) && w->ok) w->ok = dw_fail("cannot write an output file (disk full?)");
            ok = ok && w->ok;
        } else ok = 0;
    }
    dw_free(w); dw_free(work); dw_free(keys); dw_free(frames);
    return ok;
}

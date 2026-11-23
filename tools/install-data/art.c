/* data.win TXTR, TPAG, SPRT, FONT: the scene banks (Downwell.boot, .front,
   .game, .cavern, .results, .shop, .pause, .clear, .hud) and the entry, exit
   and selection masks: tools/generate-amiga-hud.py, every profile as the
   Makefile runs it (hud, results, pause, shop, clear, game). */
#include "art.h"
#include "art_tex.h"
#include "art_pack.h"
#include "art_tables.h"

enum { P_HUD, P_RESULTS, P_PAUSE, P_SHOP, P_CLEAR, P_GAME };

typedef struct {
    Context* c;
    Win* w;
    Tex tex;
    Font* font;
    Img fontImg;
    Recs rs;
    u8* dead;      /* records the clear profile drops */
    int profile;
    char* ini;     /* language.ini, english section parsed in place */
    u32 nkeys;
    char* keys[400];
    char* values[400];
} Art;

static int in(u32 id, const u16* set, u32 n)
{
    u32 lo = 0, hi = n;
    while (lo < hi) { u32 mid = (lo + hi) >> 1; if (set[mid] < id) lo = mid + 1; else hi = mid; }
    return lo < n && set[lo] == id;
}
/* Small unsorted literal lists of the Python. */
static int any(u32 id, const u16* list, u32 n) { u32 i; for (i = 0; i < n; ++i) if (list[i] == id) return 1; return 0; }
#define IN(id, set) in(id, set, COUNT(set))
#define ANY(id, list) any(id, list, COUNT(list))

static const u16 ORIENT[] = {84, 85, 96, 101, 153, 154, 157, 162, 163, 164, 166, 168, 170, 173, 175, 176, 177, 178,
    181, 182, 183, 184, 185, 186, 187, 188, 205, 207, 208, 278, 702, 703, 289, 352, 458, 459, 460, 461, 463, 470, 471,
    485, 486, 487, 496, 497, 602, 603, 605, 83, 118, 571, 572, 704, 711, 712, 472, 473, 601, 111, 457, 467, 112, 94, 95};
static const u16 BLANK_OK[] = {83, 94, 95, 112, 118, 571, 572, 704, 711, 712, 472, 473, 601, 111};
static const u16 ANCHORED[] = {111, 83, 84, 85, 101, 118, 181, 182, 183, 184, 185, 571, 572, 702, 703, 352, 704, 711, 712};
static const u16 UNFLIPPED[] = {111, 112, 95, 457, 467, 83, 84, 85, 101, 118, 485, 486, 487, 496, 497, 571, 572, 605, 704, 711, 712};
static const u16 FOUR_TURNS[] = {278, 463, 470, 704};
static const u16 MIRRORED[] = {127, 139, 550, 551, 568, 569, 0, 1, 3, 12, 15, 20, 27, 31, 37, 25, 26, 28, 29, 32, 33, 34,
    35, 38, 40, 42, 153, 154, 163, 164, 166, 168, 170, 172, 173, 177, 186, 187, 188, 206, 278, 158, 165, 179, 281, 337, 291};
static const u16 ACTORS[] = {0, 1, 3, 12, 15, 20, 27, 31, 37, 25, 26, 28, 29, 32, 33, 34, 35, 38, 40, 42, 84, 85, 96, 101,
    133, 134, 153, 154, 157, 162, 163, 164, 166, 168, 170, 173, 175, 176, 177, 178, 181, 182, 183, 184, 185, 186, 187, 188,
    205, 207, 208, 278, 458, 459, 460, 702, 703, 289, 352, 485, 486, 487, 496, 497, 602, 603, 707};
static const u16 GARDEN[] = {1, 12, 15, 20, 27, 31, 37};
static const u16 CLEAR_BORDERS[] = {51, 74, 72, 70, 55};
static const u16 SHADOWED[] = {65000, 65001, 65003, 65021, 65022, 65023, 65024, 65030};
static const u16 GUN_SPREAD[] = {458, 459, 602, 603};
static const u16 W14_TURNED[] = {472, 473, 601};

/* encode_planar: two colour planes and the mask, (width + 15) & ~15 wide. */
static int add_pens(Art* a, const Rec* meta, const u8* pens, int w, int h)
{
    u32 dma = ((u32)w + 15) & ~15u, row = dma / 8, plane = row * (u32)h, x, y;
    u8* out = dw_zalloc(plane * 3 + 1);
    Rec r = *meta;
    int ok;
    if (!out) return 0;
    for (y = 0; y < (u32)h; ++y)
        for (x = 0; x < (u32)w; ++x) {
            u8 pen = pens[y * (u32)w + x], m = (u8)(0x80 >> (x & 7));
            u32 at = y * row + x / 8;
            if (pen & 1) out[at] |= m;
            if (pen & 2) out[plane + at] |= m;
            if (pen) out[2 * plane + at] |= m;
        }
    r.dma = (u16)dma;
    ok = rec_add(&a->rs, &r, out, plane * 3);
    dw_free(out);
    return ok;
}

static Rec meta(u32 id, u32 frame, int ox, int oy, int w, int h)
{
    Rec r;
    memset(&r, 0, sizeof r);
    r.id = (u16)id; r.frame = (u16)frame; r.ox = (s16)ox; r.oy = (s16)oy; r.w = (u16)w; r.h = (u16)h;
    return r;
}

static int add_image(Art* a, const Img* m, u32 id, u32 frame, int ox, int oy)
{
    u8* pens = dw_alloc((u32)m->w * (u32)m->h + 1);
    Rec r = meta(id, frame, ox, oy, m->w, m->h);
    int ok;
    if (!pens) return 0;
    ok = img_pens(m, pens) && add_pens(a, &r, pens, m->w, m->h);
    dw_free(pens);
    return ok;
}

/* decode_planar of a record: its pens, width x height. */
static int record_pens(const Art* a, const Rec* r, u8* pens)
{
    const u8* d = stream_data(&a->rs, r->stream);
    u32 row = r->dma / 8u, plane = row * r->h, x, y;
    for (y = 0; y < r->h; ++y)
        for (x = 0; x < r->w; ++x) {
            u32 at = y * row + x / 8, m = 0x80u >> (x & 7);
            pens[y * r->w + x] = (u8)(!!(d[at] & m) | !!(d[plane + at] & m) << 1);
        }
    return 1;
}

static int frame_canvas(Art* a, u32 spriteId, u32 frame, Img* out)
{
    Sprite s;
    Img src;
    const Tpag* t;
    u32 ptr;
    TRY(tex_sprite(a->w, spriteId, &s));
    REQUIRE(frame < s.nframes, "a scene sprite lacks a frame");
    ptr = tex_frame(a->w, &s, frame);
    t = tex_item(&a->tex, ptr);
    REQUIRE(t, "a scene texture item was not decoded");
    TRY(tex_image(&a->tex, ptr, &src));
    if (!img_new(out, s.w, s.h)) { img_free(&src); return 0; }
    img_composite(out, &src, t->tx, t->ty);
    img_free(&src);
    return 1;
}

/* ---- language.ini (configparser: [english], key = value, keys lowercase) */

static int lower(int ch) { return ch >= 'A' && ch <= 'Z' ? ch + 32 : ch; }
static int ini_load(Art* a)
{
    File f;
    char* p;
    char* end;
    int section = 0;
    TRY(ctx_open(a->c, "language.ini", &f));
    a->ini = dw_alloc(f.size + 1);
    if (!a->ini) { io_close(&f); return 0; }
    if (!io_read(&f, a->ini, f.size)) { io_close(&f); return dw_fail("cannot read language.ini"); }
    io_close(&f);
    a->ini[f.size] = 0;
    p = a->ini; end = a->ini + f.size;
    if (f.size >= 3 && (u8)p[0] == 0xef && (u8)p[1] == 0xbb && (u8)p[2] == 0xbf) p += 3;
    while (p < end) {
        char* line = p;
        char *s, *e, *d;
        while (p < end && *p != '\n' && *p != '\r') ++p;
        if (p < end) *p++ = 0;
        s = line; e = s + strlen(s);
        while (*s == ' ' || *s == '\t') ++s;
        while (e > s && (e[-1] == ' ' || e[-1] == '\t')) --e;
        *e = 0;
        if (!*s || *s == '#' || *s == ';') continue;
        if (*s == '[') { section = !dw_strcmp(s, "[english]"); continue; }
        if (!section) continue;
        for (d = s; *d && *d != '=' && *d != ':'; ++d) continue;
        REQUIRE(*d, "language.ini has an unexpected line");
        { char* k = d; while (k > s && (k[-1] == ' ' || k[-1] == '\t')) --k; *k = 0; }
        for (++d; *d == ' ' || *d == '\t'; ++d) continue;
        { char* k; for (k = s; *k; ++k) *k = (char)lower(*k); }
        REQUIRE(a->nkeys < COUNT(a->keys), "language.ini has more keys than expected");
        a->keys[a->nkeys] = s; a->values[a->nkeys] = d; ++a->nkeys;
    }
    return 1;
}

static const char* ini_get(const Art* a, const char* key)
{
    u32 i;
    for (i = 0; i < a->nkeys; ++i) {
        const char* k = a->keys[i];
        const char* q = key;
        while (*k && *k == lower(*q)) ++k, ++q;
        if (!*k && !*q) return a->values[i];
    }
    return 0;
}

/* mode 0: the value as is; 1: '*' becomes a newline (langString); 2: '#'
   too (GameMaker's draw_text newline). */
static int ini_text(const Art* a, const char* key, const char* missing, int mode, char* out, u32 cap)
{
    const char* v = ini_get(a, key);
    u32 n = 0;
    if (!v) { REQUIRE(missing, "language.ini lacks a text the banks use"); v = missing; }
    for (; *v; ++v) {
        REQUIRE(n + 1 < cap, "a language.ini text is longer than expected");
        out[n++] = (mode >= 1 && *v == '*') || (mode == 2 && *v == '#') ? '\n' : *v;
    }
    out[n] = 0;
    return 1;
}

static void key_n(char* out, const char* prefix, u32 n)
{
    char digits[12];
    u32 k = 0;
    while (*prefix) *out++ = *prefix++;
    do digits[k++] = (char)('0' + n % 10); while (n /= 10);
    while (k) *out++ = digits[--k];
    *out = 0;
}

/* ---- render_text: font 2, lines 12 pixels apart, glyph shifts, no kerning */

static u32 utf8(const char** s)
{
    const u8* p = (const u8*)*s;
    u32 c = *p++;
    if (c >= 0xc0) {
        int more = c >= 0xf0 ? 3 : c >= 0xe0 ? 2 : 1;
        c &= 0x3f >> more;
        while (more-- && (*p & 0xc0) == 0x80) c = c << 6 | (*p++ & 0x3f);
    }
    *s = (const char*)p;
    return c;
}

static int font_ready(Art* a)
{
    if (a->fontImg.p) return 1;
    return tex_image(&a->tex, a->font->tpag, &a->fontImg);
}

static int render_text(Art* a, const char* text, int centered, Img* out)
{
    int widths[64], nlines = 0, width = 0, line, i;
    const char* s;
    TRY(font_ready(a));
    widths[0] = 0;
    for (s = text; *s;) {
        u32 ch = utf8(&s);
        const Glyph* g;
        if (ch == '\n') { REQUIRE(++nlines < 64, "a text has too many lines"); widths[nlines] = 0; continue; }
        g = font_glyph(a->font, ch);
        REQUIRE(g, "font 2 lacks a character a text uses");
        widths[nlines] += g->shift;
    }
    ++nlines;
    for (i = 0; i < nlines; ++i) if (widths[i] > width) width = widths[i];
    TRY(img_new(out, width, 10 + 12 * (nlines - 1)));
    for (s = text, line = 0; line < nlines; ++line) {
        int x = centered ? (width - widths[line]) / 2 : 0;
        while (*s && *s != '\n') {
            const Glyph* g = font_glyph(a->font, utf8(&s));
            Img glyph;
            REQUIRE((u32)g->x + g->w <= (u32)a->fontImg.w && (u32)g->y + g->h <= (u32)a->fontImg.h, "a glyph lies outside font 2's item");
            if (!img_crop(&glyph, &a->fontImg, g->x, g->y, g->x + g->w, g->y + g->h)) { img_free(out); return 0; }
            img_composite(out, &glyph, x + g->offset, line * 12);
            img_free(&glyph);
            x += g->shift;
        }
        if (*s) ++s;
    }
    return 1;
}

static int render_bordered(Art* a, const char* text, int red, int centered, Img* out)
{
    Img glyphs, ink;
    static const int at[4][2] = {{0, 1}, {2, 1}, {1, 0}, {1, 2}};
    int i;
    TRY(render_text(a, text, centered, &glyphs));
    if (!img_new(out, glyphs.w + 2, glyphs.h + 2)) { img_free(&glyphs); return 0; }
    if (!img_tinted(&ink, &glyphs, 0, 0, 0)) { img_free(&glyphs); img_free(out); return 0; }
    for (i = 0; i < 4; ++i) img_composite(out, &ink, at[i][0], at[i][1]);
    img_free(&ink);
    if (!img_tinted(&ink, &glyphs, 255, red ? 0 : 255, red ? 0 : 255)) { img_free(&glyphs); img_free(out); return 0; }
    img_composite(out, &ink, 1, 1);
    img_free(&ink); img_free(&glyphs);
    return 1;
}

/* ---- build_records */

static int find_rec(const Art* a, u32 id, u32 frame)
{
    u32 i;
    for (i = 0; i < a->rs.n; ++i) if (a->rs.r[i].id == id && a->rs.r[i].frame == frame) return (int)i;
    return -1;
}

static void recolour(Img* m, int flash)
{
    u32 i, n = (u32)m->w * (u32)m->h;
    for (i = 0; i < n; ++i) {
        u8* p = m->p + 4 * i;
        if (!p[3]) continue;
        if (flash) { u8 v = p[0] == 254 ? 255 : 0; p[0] = p[1] = p[2] = v; p[3] = 255; }
        else if (p[0] == 255 && p[1] == 1 && p[2] == 1) p[1] = p[2] = 0;
    }
}

static int check_flash(const Img* m)
{
    u32 i, n = (u32)m->w * (u32)m->h;
    for (i = 0; i < n; ++i) {
        const u8* p = m->p + 4 * i;
        if (p[3] && !(p[3] == 255 && ((p[0] == 254 && p[1] == 254 && p[2] == 254) || (p[0] == 7 && p[1] == 7 && p[2] == 7))))
            return dw_fail("unreviewed flash-ball color");
    }
    return 1;
}

/* Prepared orientations of gameplay actors (snail flips, quarter turns),
   cropped around their pixels and anchor. */
static int oriented(Art* a, const Sprite* s, u32 id, u32 fi, const Img* canvas)
{
    static const u8 all4[] = {0, 1, 2, 3}, t013[] = {0, 1, 3}, t03[] = {0, 3}, t3[] = {3};
    int snail = id == 177 || id == 178, spread = ANY(id, GUN_SPREAD), w14 = ANY(id, W14_TURNED);
    const u8* turns;
    u32 nturns, t;
    if (id == 601) { turns = t3; nturns = 1; }
    else if (id == 459 || id == 605) { turns = t013; nturns = 3; }
    else if (spread || w14) { turns = t03; nturns = 2; }
    else { turns = all4; nturns = snail ? 2 : ANY(id, FOUR_TURNS) ? 4 : 1; }
    REQUIRE(!spread || !s->smooth, "spread proof requires nearest-sampled source artwork");
    for (t = 0; t < nturns; ++t) {
        u32 turn = turns[t], flip;
        int ox = s->ox, oy = s->oy, box[4], k;
        Img variant, cropped;
        if (!turn) { if (!img_crop(&variant, canvas, 0, 0, canvas->w, canvas->h)) return 0; }
        else if (!img_transpose(&variant, canvas, snail ? FLIP_TB : turn == 1 ? ROT_90 : turn == 2 ? ROT_180 : ROT_270)) return 0;
        if (snail) { if (turn) oy = canvas->h - oy; }
        else if (id == 605 && turn == 3) { int nx = canvas->h - oy; oy = ox; ox = nx; }
        else for (k = 0; k < (int)turn; ++k) { int nx = oy; oy = canvas->w - ox; ox = nx; }
        if (!img_bbox(&variant, box)) {
            if (!ANY(id, BLANK_OK)) { img_free(&variant); return dw_fail("empty oriented actor frame"); }
            box[0] = ox; box[1] = oy; box[2] = ox + 1; box[3] = oy + 1;
        }
        if (spread || w14 || ANY(id, ANCHORED)) {
            if (ox < box[0]) box[0] = ox;
            if (oy < box[1]) box[1] = oy;
            if (ox + 1 > box[2]) box[2] = ox + 1;
            if (oy + 1 > box[3]) box[3] = oy + 1;
        }
        k = img_crop(&cropped, &variant, box[0], box[1], box[2], box[3]);
        img_free(&variant);
        TRY(k);
        ox -= box[0]; oy -= box[1];
        if (cropped.w < 16) {
            Img padded;
            if (!img_crop(&padded, &cropped, 0, 0, 16, cropped.h)) { img_free(&cropped); return 0; }
            img_free(&cropped); cropped = padded;
        }
        for (flip = 0; flip < ((spread || w14 || ANY(id, UNFLIPPED)) ? 1u : 2u); ++flip) {
            Img m;
            int ok;
            if (flip) { if (!img_transpose(&m, &cropped, FLIP_LR)) { img_free(&cropped); return 0; } }
            else m = cropped;
            ok = add_image(a, &m, flip ? 62000 + id : id, turn * 256 + fi, flip ? m.w - ox : ox, oy);
            if (flip) img_free(&m);
            if (!ok) { img_free(&cropped); return 0; }
        }
        img_free(&cropped);
    }
    return 1;
}

static int crop_to(const Img* m, const int box[4], Img* out) { return img_crop(out, m, box[0], box[1], box[2], box[3]); }

/* W1.7 gem-high afterimages and W1.11's low-health red silhouettes. */
static int afterimages(Art* a, const Sprite* s, u32 id, u32 fi, const Img* canvas)
{
    u32 flipped;
    for (flipped = 0; flipped < 2; ++flipped) {
        Img source;
        u8* pens;
        int originX = flipped ? s->w - s->ox : s->ox, parity, x, y;
        if (flipped) TRY(img_transpose(&source, canvas, FLIP_LR));
        else TRY(img_crop(&source, canvas, 0, 0, canvas->w, canvas->h));
        pens = dw_alloc((u32)source.w * (u32)source.h + 1);
        if (!pens || !img_pens(&source, pens)) { dw_free(pens); img_free(&source); return 0; }
        for (parity = 0; parity < 3; ++parity) {
            Img mark, cropped;
            int box[4], ok;
            if (!img_new(&mark, source.w, source.h)) { dw_free(pens); img_free(&source); return 0; }
            for (y = 0; y < source.h; ++y)
                for (x = parity < 2 ? parity : 0; x < source.w; x += parity < 2 ? 2 : 1) {
                    u8 pen = pens[y * source.w + x];
                    if (parity < 2 ? pen == 1 : pen == 1 || pen == 2) { u8* p = PIX(&mark, x, y); p[0] = 255; p[3] = 255; }
                }
            if (!img_bbox(&mark, box)) { box[0] = 0; box[1] = 0; box[2] = 1; box[3] = 1; }
            if (parity < 2) box[0] -= ((box[0] - parity) % 2 + 2) % 2;
            ok = crop_to(&mark, box, &cropped);
            img_free(&mark);
            if (ok) {
                ok = add_image(a, &cropped, id, parity < 2 ? 1024 + 512 * flipped + 256 * (u32)parity + fi : 2048 + 256 * flipped + fi,
                               originX - box[0], s->oy - box[1]);
                img_free(&cropped);
            }
            if (!ok) { dw_free(pens); img_free(&source); return 0; }
        }
        dw_free(pens);
        img_free(&source);
    }
    return 1;
}

static int sprite_frames(Art* a, const Selection* sel)
{
    u32 id = sel->id, fi;
    Sprite s;
    TRY(tex_sprite(a->w, id, &s));
    for (fi = 0; fi < s.nframes; ++fi) {
        Img canvas;
        int ok = 1;
        if (sel->frames != 0xffffffffu && (fi >= 32 || !(sel->frames & (1u << fi)))) continue;
        TRY(frame_canvas(a, id, fi, &canvas));
        if (a->profile == P_CLEAR && ANY(id, CLEAR_BORDERS)) {
            /* Nine repetitions 16 rows apart, painted in order. */
            Img repeated;
            int row;
            REQUIRE((fi == 7 || fi == 13) && (canvas.h == 16 || canvas.h == 32), "unexpected selection border geometry");
            ok = img_new(&repeated, canvas.w, 9 * 16 + canvas.h - 16);
            for (row = 0; ok && row < 9; ++row) img_composite(&repeated, &canvas, 0, row * 16);
            if (ok) {
                ok = add_image(a, &repeated, id, 256 + fi, s.ox, s.oy);
                img_free(&repeated);
            }
        }
        if (ok && a->profile == P_GAME && id == 96) { ok = check_flash(&canvas); recolour(&canvas, 1); }
        if (ok && a->profile == P_GAME && id == 704) recolour(&canvas, 0);
        if (ok && a->profile == P_GAME && ANY(id, ORIENT)) {
            ok = oriented(a, &s, id, fi, &canvas);
            img_free(&canvas);
            TRY(ok);
            continue;
        }
        if (ok) ok = add_image(a, &canvas, id, fi, s.ox, s.oy);
        if (ok && a->profile == P_SHOP && id == 484) {
            Img turned;
            ok = img_transpose(&turned, &canvas, ROT_270);
            if (ok) { ok = add_image(a, &turned, id, 768 + fi, canvas.h - s.oy, s.ox); img_free(&turned); }
        }
        if (ok && ((a->profile == P_HUD && id == 408) || (a->profile == P_GAME && ANY(id, MIRRORED)))) {
            Img mirrored;
            ok = img_transpose(&mirrored, &canvas, FLIP_LR);
            if (ok) {
                ok = add_image(a, &mirrored, 62000 + id, fi, s.w - s.ox, s.oy);
                img_free(&mirrored);
                if (ok && a->profile == P_HUD) {
                    /* Expose the transparent DMA padding: width = stride. */
                    Rec* r = &a->rs.r[a->rs.n - 1];
                    r->w = r->dma;
                }
            }
        }
        if (ok && a->profile == P_GAME && IN(id, AFTERIMAGE_IDS)) ok = afterimages(a, &s, id, fi, &canvas);
        if (ok && a->profile == P_GAME && (id == 27 || id == 43 || id == 44 || id == 25 || id == 26 || id == 28 || id == 29)) {
            static const u16 from[] = {27, 25, 26, 28, 29, 43, 44}, to[] = {65500, 65504, 65505, 65506, 65507, 65501, 65502};
            int box[4], k;
            Img cropped;
            for (k = 0; from[k] != id; ++k) continue;
            REQUIRE(img_bbox(&canvas, box), "empty style bob");
            ok = crop_to(&canvas, box, &cropped);
            if (ok) { ok = add_image(a, &cropped, to[k], fi, s.ox - box[0], s.oy - box[1]); img_free(&cropped); }
        }
        if (ok && a->profile == P_GAME && id == 44) {
            Img mirrored, cropped;
            int box[4], mox = s.w - s.ox;
            ok = img_transpose(&mirrored, &canvas, FLIP_LR);
            if (ok) {
                ok = add_image(a, &mirrored, 65401, fi, mox, s.oy);
                if (ok) REQUIRE(img_bbox(&mirrored, box), "empty style arrow");
                if (ok) ok = crop_to(&mirrored, box, &cropped);
                if (ok) { ok = add_image(a, &cropped, 65503, fi, mox - box[0], s.oy - box[1]); img_free(&cropped); }
                img_free(&mirrored);
            }
        }
        if (ok && a->profile == P_GAME && id == 654) {
            Img aligned;
            ok = img_new(&aligned, s.w + 8, s.h);
            if (ok) {
                img_composite(&aligned, &canvas, 8, 0);
                ok = add_image(a, &aligned, 65400, fi, s.ox + 8, s.oy);
                img_free(&aligned);
            }
        }
        img_free(&canvas);
        TRY(ok);
    }
    return 1;
}

/* The HUD's composed ammo digits (65010) and health glyphs (65011). */
static int hud_glyphs(Art* a)
{
    static const s8 offsets[14][2] = {{-1, 0}, {0, -1}, {1, 0}, {0, 1}, {-1, 1}, {0, 0}, {1, 1}, {0, 2}, {0, 0}, {1, -1}, {2, 0}, {1, 1}, {0, 1}, {1, 0}};
    u8 pens[64 * 64], canvas[64 * 64], cropped[64 * 64];
    int digit, glyph;
    for (digit = 0; digit < 10; ++digit) {
        int layer, left = 32, top = 24, right = -1, bottom = -1, x, y;
        Rec r;
        memset(canvas, 0, 32 * 24);
        for (layer = 0; layer < 15; ++layer) {
            int frame = layer < 14 ? digit + (layer < 12 ? 20 : 10) : digit;
            int dx = layer < 14 ? offsets[layer][0] : 0, dy = layer < 14 ? offsets[layer][1] : 0, k = find_rec(a, 413, (u32)frame);
            const Rec* src;
            REQUIRE(k >= 0, "the HUD lacks a digit frame");
            src = &a->rs.r[k];
            REQUIRE((u32)src->w * src->h <= sizeof pens, "a HUD digit is larger than expected");
            record_pens(a, src, pens);
            for (y = 0; y < src->h; ++y)
                for (x = 0; x < src->w; ++x) {
                    u8 pen = pens[y * src->w + x];
                    int px = 8 + dx + x - src->ox, py = 8 + dy + y - src->oy;
                    if (!pen) continue;
                    REQUIRE(px >= 0 && px < 32 && py >= 0 && py < 24, "ammo digit bounds");
                    canvas[py * 32 + px] = pen;
                }
        }
        for (y = 0; y < 24; ++y)
            for (x = 0; x < 32; ++x)
                if (canvas[y * 32 + x]) {
                    if (x < left) left = x;
                    if (x > right) right = x;
                    if (y < top) top = y;
                    if (y > bottom) bottom = y;
                }
        for (y = top; y <= bottom; ++y)
            for (x = left; x <= right; ++x) cropped[(y - top) * (right + 1 - left) + x - left] = canvas[y * 32 + x];
        r = meta(65010, (u32)digit, 8 - left, 8 - top, right + 1 - left, bottom + 1 - top);
        TRY(add_pens(a, &r, cropped, r.w, r.h));
    }
    for (glyph = 0; glyph < 11; ++glyph) {
        static const s8 at[4][2] = {{1, 0}, {1, 1}, {0, 1}, {0, 0}};
        u32 id = glyph < 10 ? 413 : 414;
        int fg = glyph < 10 ? glyph : 0, shadow = glyph < 10 ? glyph + 20 : 2;
        int layer, left = 1000, top = 1000, right = -1000, bottom = -1000, x, y, pass;
        Rec r;
        /* Two passes: the bounds, then the pixels (later layers win). */
        for (pass = 0; pass < 2; ++pass) {
            if (pass) {
                REQUIRE((right + 1 - left) * (bottom + 1 - top) <= (int)sizeof canvas, "a health glyph is larger than expected");
                memset(canvas, 0, (u32)((right + 1 - left) * (bottom + 1 - top)));
            }
            for (layer = 0; layer < 4; ++layer) {
                int k = find_rec(a, id, (u32)(layer < 3 ? shadow : fg));
                const Rec* src;
                REQUIRE(k >= 0, "the HUD lacks a health glyph frame");
                src = &a->rs.r[k];
                REQUIRE((u32)src->w * src->h <= sizeof pens, "a health glyph is larger than expected");
                record_pens(a, src, pens);
                for (y = 0; y < src->h; ++y)
                    for (x = 0; x < src->w; ++x) {
                        u8 pen = pens[y * src->w + x];
                        int px = x - src->ox + at[layer][0], py = y - src->oy + at[layer][1];
                        if (!pen) continue;
                        if (!pass) {
                            if (px < left) left = px;
                            if (px > right) right = px;
                            if (py < top) top = py;
                            if (py > bottom) bottom = py;
                        } else canvas[(py - top) * (right + 1 - left) + px - left] = pen;
                    }
            }
        }
        r = meta(65011, (u32)glyph, -left, -top, right + 1 - left, bottom + 1 - top);
        TRY(add_pens(a, &r, canvas, r.w, r.h));
    }
    return 1;
}

/* One text record set (render_text and the profile's derived forms). */
static int text_record(Art* a, u32 id, const char* text)
{
    Img canvas;
    int centered = a->profile == P_SHOP || a->profile == P_CLEAR || (id >= 65025 && id < 65030), ox, oy, ok;
    TRY(render_text(a, text, centered, &canvas));
    if (ANY(id, SHADOWED)) {
        Img shadow, out;
        ok = img_tinted(&shadow, &canvas, 255, 0, 0);
        if (ok && !img_new(&out, canvas.w, canvas.h + 1)) { img_free(&shadow); ok = 0; }
        if (!ok) { img_free(&canvas); return 0; }
        img_composite(&out, &shadow, 0, 1);
        img_composite(&out, &canvas, 0, 0);
        img_free(&shadow); img_free(&canvas);
        canvas = out;
    }
    ox = a->profile == P_PAUSE || (id >= 64800 && id < 64928) ? 0 : canvas.w / 2;
    oy = a->profile == P_SHOP ? canvas.h / 2 : 0;
    ok = add_image(a, &canvas, id, 0, ox, oy);
    if (ok && a->profile == P_CLEAR && (id == 64001 || (id >= 64500 && id < 64512))) {
        Img red;
        int offset;
        ok = img_tinted(&red, &canvas, 255, 0, 0);
        for (offset = 1; ok && offset < 7; ++offset) {
            Img composed;
            int shadow;
            ok = img_new(&composed, canvas.w + offset, canvas.h + offset);
            if (!ok) break;
            for (shadow = 0; shadow < offset; ++shadow) img_composite(&composed, &red, offset - shadow, offset - shadow);
            img_composite(&composed, &canvas, 0, 0);
            ok = add_image(a, &composed, id, (u32)offset + 1, ox + offset, offset);
            img_free(&composed);
        }
        img_free(&red);
    }
    if (ok && a->profile == P_CLEAR && id != 64000 && id != 64001 && id != 64700 && id != 64701 && !(id >= 64500 && id < 64512)) {
        Img bordered;
        ok = render_bordered(a, text, 0, 1, &bordered);
        if (ok) { ok = add_image(a, &bordered, id, 1, ox + 1, 1); img_free(&bordered); }
    }
    img_free(&canvas);
    return ok;
}

static int texts(Art* a)
{
    char text[256], key[32], cleared[256];
    u32 i;
    if (a->profile == P_SHOP) {
        static const u16 ids[] = {65410, 65412, 65413};
        static const char* const keys[] = {"shopExit", "shopPurchased", "menuBack"};
        for (i = 0; i < 3; ++i) { TRY(ini_text(a, keys[i], 0, 2, text, sizeof text)); TRY(text_record(a, ids[i], text)); }
        for (i = 0; i < 6; ++i) { key_n(key, "ugName", 100 + i); TRY(ini_text(a, key, 0, 2, text, sizeof text)); TRY(text_record(a, 65420 + i, text)); }
        for (i = 0; i < 6; ++i) { key_n(key, "ugDesc", 100 + i); TRY(ini_text(a, key, 0, 2, text, sizeof text)); TRY(text_record(a, 65430 + i, text)); }
        TRY(text_record(a, 65411, "BUY"));
        for (i = 0; i < 10; ++i) { text[0] = (char)('0' + i); text[1] = 0; TRY(text_record(a, 65440 + i, text)); }
        return text_record(a, 65450, "G");
    }
    if (a->profile == P_CLEAR) {
        static const u8 ups[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 100};
        u32 area, level;
        TRY(ini_text(a, "transitionCleared", " ", 2, cleared, sizeof cleared));
        TRY(text_record(a, 64000, cleared));
        TRY(ini_text(a, "transitionChoose", " ", 2, text, sizeof text));
        TRY(text_record(a, 64001, text));
        for (i = 0; i < sizeof ups; ++i) {
            key_n(key, "ugName", ups[i]); TRY(ini_text(a, key, " ", 2, text, sizeof text)); TRY(text_record(a, 64100 + ups[i], text));
            key_n(key, "ugDesc", ups[i]); TRY(ini_text(a, key, " ", 2, text, sizeof text)); TRY(text_record(a, 64300 + ups[i], text));
        }
        for (area = 1; area < 5; ++area)
            for (level = 1; level < 4; ++level) {
                u32 n = strlen(cleared);
                REQUIRE(n + 5 < sizeof text, "a language.ini text is longer than expected");
                text[0] = (char)('0' + area); text[1] = '-'; text[2] = (char)('0' + level); text[3] = '\n';
                memcpy(text + 4, cleared, n + 1);
                TRY(text_record(a, 64500 + (area - 1) * 3 + level - 1, text));
            }
        for (i = 0; i < 23; ++i) {
            u32 n;
            key_n(key, "ugName", i); TRY(ini_text(a, key, " ", 2, text, sizeof text));
            n = strlen(text);
            REQUIRE(n + 3 < sizeof text, "a language.ini text is longer than expected");
            memcpy(text + n, " 2", 3);
            TRY(text_record(a, 64600 + i, text));
        }
        {
            const char* v = ini_get(a, "area2");
            u32 n;
            REQUIRE(v, "language.ini lacks area2");
            n = strlen(v);
            REQUIRE(n + 3 < sizeof text, "a language.ini text is longer than expected");
            memcpy(text, v, n); memcpy(text + n, "-1", 3);
            TRY(text_record(a, 64700, text));
        }
        return text_record(a, 64701, "WORLD 1 COMPLETE\nEND OF SLICE\n\nESC TO QUIT");
    }
    if (a->profile == P_GAME) {
        for (i = 0; i < COUNT(TEXT_GAME); ++i) TRY(text_record(a, TEXT_GAME[i].id, TEXT_GAME[i].text));
        for (i = 1; i < 5; ++i) { key_n(key, "styleName", i); TRY(ini_text(a, key, 0, 0, text, sizeof text)); TRY(text_record(a, 65020 + i, text)); }
        for (i = 1; i < 5; ++i) { key_n(key, "styleDesc", i); TRY(ini_text(a, key, 0, 1, text, sizeof text)); TRY(text_record(a, 65025 + i, text)); }
        TRY(ini_text(a, "menuHardMode", 0, 0, text, sizeof text));
        return text_record(a, 65030, text);
    }
    if (a->profile == P_RESULTS) for (i = 0; i < COUNT(TEXT_RESULTS); ++i) TRY(text_record(a, TEXT_RESULTS[i].id, TEXT_RESULTS[i].text));
    if (a->profile == P_PAUSE) for (i = 0; i < COUNT(TEXT_PAUSE); ++i) TRY(text_record(a, TEXT_PAUSE[i].id, TEXT_PAUSE[i].text));
    return 1;
}

static int credits(Art* a)
{
    u32 i;
    for (i = 0; i < COUNT(CREDITS); ++i) {
        Img canvas;
        int ox, pad, ok;
        TRY(render_bordered(a, CREDITS[i].text, CREDITS[i].red, 0, &canvas));
        ox = canvas.w / 2;
        pad = (CREDITS[i].x - ox) & 15;
        if (pad) {
            Img aligned;
            if (!img_new(&aligned, canvas.w + pad, canvas.h)) { img_free(&canvas); return 0; }
            img_composite(&aligned, &canvas, pad, 0);
            img_free(&canvas);
            canvas = aligned;
            ox += pad;
        }
        ok = add_image(a, &canvas, CREDITS[i].id, 0, ox, 1);
        img_free(&canvas);
        TRY(ok);
    }
    return 1;
}

/* Opacity of the transition sprites' visible edges (65004, 65005) and the
   world dither edge (65006). */
static int dithers(Art* a)
{
    static const u16 strips[2][4] = {{65004, 129, 80, 0}, {65005, 128, 80, 720}};
    Img canvas, edge;
    u8* pens;
    u32 i, n;
    int x, y, ok;
    Rec r;
    for (i = 0; i < 2; ++i) {
        TRY(frame_canvas(a, strips[i][1], 0, &canvas));
        ok = img_crop(&edge, &canvas, strips[i][2], strips[i][3], strips[i][2] + 160, strips[i][3] + 80);
        img_free(&canvas);
        TRY(ok);
        for (n = 0; n < 160u * 80; ++n) {
            u8* p = edge.p + 4 * n;
            u8 v = p[3] >= 128 ? 255 : 0;
            p[0] = p[1] = p[2] = p[3] = v;
        }
        ok = add_image(a, &edge, strips[i][0], 0, 0, 0);
        img_free(&edge);
        TRY(ok);
    }
    TRY(frame_canvas(a, 128, 0, &canvas));
    {
        Sprite s;
        TRY(tex_sprite(a->w, 128, &s));
        REQUIRE(canvas.w == 320 && canvas.h == 800 && s.ox == 160 && s.oy == 720, "authored dither geometry changed");
    }
    pens = dw_alloc(320 * 80);
    if (!pens) { img_free(&canvas); return 0; }
    for (y = 0; y < 80; ++y)
        for (x = 0; x < 320; ++x) pens[y * 320 + x] = PIX(&canvas, x, 720 + y)[3] >= 128 ? 3 : 0;
    img_free(&canvas);
    r = meta(65006, 0, 160, 0, 320, 80);
    ok = add_pens(a, &r, pens, 320, 80);
    dw_free(pens);
    return ok;
}

static int plane_bit(const u8* d, const Rec* r, int x, int y, int plane)
{
    u32 stride = r->dma / 8u;
    return x >= 0 && x < r->w && (d[(u32)plane * stride * r->h + (u32)y * stride + (u32)x / 8] & (0x80 >> (x & 7)));
}

static void opaque_columns(const u8* d, const Rec* r, int* left, int* right)
{
    int x, y;
    *left = -1; *right = 0;
    for (x = 0; x < r->w; ++x)
        for (y = 0; y < r->h; ++y)
            if (plane_bit(d, r, x, y, 0) || plane_bit(d, r, x, y, 1)) {
                if (*left < 0) *left = x;
                *right = x;
                break;
            }
    if (*left < 0) *left = 0;
}

static int clampi(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int max0_min(int hiValue, int v) { int m = v < hiValue ? v : hiValue; return m > 0 ? m : 0; }

/* Gameplay and upgrade-player DMA sprite streams (63000 + id). */
static int actor_streams(Art* a)
{
    u32 count = a->rs.n, i;
    u8 stream[32 * 4];
    for (i = 0; i < count; ++i) {
        Rec r = a->rs.r[i];
        const u8* d;
        int left, right, parts, flip, part, x, y;
        if ((a->profile == P_CLEAR ? !IN(r.id, UPGRADE_PLAYER_IDS) : !ANY(r.id, ACTORS)) || r.frame >= 1024) continue;
        REQUIRE(r.w <= 64 && r.h <= 64 && r.ox >= 0 && r.ox <= r.w && r.oy >= 0 && r.oy <= r.h,
                "gameplay actor exceeds conservative visibility bounds");
        d = stream_data(&a->rs, r.stream);
        opaque_columns(d, &r, &left, &right);
        parts = (right - left + 16) / 16;
        REQUIRE(parts <= 4 && r.h <= 32, "gameplay sprite exceeds DMA slot");
        for (flip = 0; flip < (r.id == 84 ? 1 : 2); ++flip) {
            int lo = flip ? r.w - 1 - right : left, hi = flip ? r.w - 1 - left : right;
            for (part = 0; part < parts; ++part) {
                int crop = parts == 1 ? max0_min(r.w - 16, lo - (16 - (hi - lo + 1)) / 2) : lo + part * 16;
                Rec m = r;
                u32 n = 0;
                for (y = 0; y < r.h; ++y) {
                    int plane;
                    for (plane = 0; plane < 2; ++plane) {
                        u32 bits = 0;
                        for (x = 0; x < 16; ++x)
                            if (plane_bit(d, &r, flip ? r.w - 1 - crop - x : crop + x, y, plane)) bits |= 1u << (15 - x);
                        stream[n++] = (u8)(bits >> 8); stream[n++] = (u8)bits;
                    }
                }
                m.id = (u16)(63000 + r.id); m.w = 16; m.dma = 16;
                m.ox = (s16)((flip ? r.w - r.ox : r.ox) - crop);
                m.hw = 1; m.parts = (u8)parts;
                m.frame = (u16)(r.frame * 8 + flip * 4 + part);
                TRY(rec_add(&a->rs, &m, stream, n));
                d = stream_data(&a->rs, r.stream);
            }
        }
    }
    (void)clampi;
    return 1;
}

/* Style carousel icons (65510..65517): fixed 136-byte slots, twice. */
static int style_icons(Art* a)
{
    u32 count = a->rs.n, i;
    u8 stream[272];
    for (i = 0; i < count; ++i) {
        Rec r = a->rs.r[i];
        const u8* d;
        u32 n = 4, copy, stride, plane;
        int x, y;
        if (r.id < 65500 || r.id >= 65508) continue;
        REQUIRE(r.w <= 16 && r.h <= 32, "style icon exceeds one DMA channel");
        d = stream_data(&a->rs, r.stream);
        stride = r.dma / 8u; plane = stride * r.h;
        memset(stream, 0, sizeof stream);
        for (y = 0; y < r.h; ++y) {
            u32 p;
            for (p = 0; p < 2; ++p) {
                u32 bits = 0;
                for (x = 0; x < r.w; ++x)
                    if (d[p * plane + (u32)y * stride + (u32)x / 8] & (0x80 >> (x & 7))) bits |= 1u << (15 - x);
                stream[n++] = (u8)(bits >> 8); stream[n++] = (u8)bits;
            }
        }
        memcpy(stream + 136, stream, 136);
        for (copy = 0; copy < (r.id == 65501 ? 4u : 1u); ++copy) {
            Rec m = r;
            m.id = (u16)(r.id + 10); m.w = 16; m.dma = 16; m.hw = 1; m.parts = 0;
            m.frame = (u16)(r.id == 65501 ? copy : r.frame);
            TRY(rec_add(&a->rs, &m, stream, 272));
        }
    }
    return 1;
}

/* pixel() of the garden streams indexes the planar bytes as Python does,
   negative indices counting from the end. */
static int garden_bit(const u8* d, u32 len, const Rec* r, int sx, int y, int plane, int* bit)
{
    int stride = r->dma / 8, at = plane * stride * r->h + y * stride + (sx >= 0 ? sx / 8 : -((-sx + 7) / 8));
    if (at < 0) at += (int)len;
    REQUIRE(at >= 0 && (u32)at < len, "a garden sprite reads past its planes");
    *bit = !!(d[at] & (0x80 >> ((unsigned)sx & 7)));
    return 1;
}

static int garden_sprites(Art* a)
{
    u32 count = a->rs.n, i;
    u8 stream[272];
    for (i = 0; i < count; ++i) {
        Rec r = a->rs.r[i];
        const u8* d;
        u32 len, limit = r.id == 15 ? 5 : r.id == 20 ? 1 : 1024;
        int left, right, flip, x, y, bit;
        if (!ANY(r.id, GARDEN) || r.frame >= 1024 || r.frame >= limit) continue;
        d = stream_data(&a->rs, r.stream);
        len = a->rs.s[r.stream].len;
        {
            /* opaque columns over the stored width, no range check */
            int found = 0;
            left = 0; right = 0;
            for (x = 0; x < r.w; ++x)
                for (y = 0; y < r.h; ++y) {
                    int b0, b1;
                    TRY(garden_bit(d, len, &r, x, y, 0, &b0));
                    TRY(garden_bit(d, len, &r, x, y, 1, &b1));
                    if (b0 || b1) { if (!found) left = x; found = 1; right = x; break; }
                }
        }
        REQUIRE(right - left < 16, "garden actor exceeds one sprite");
        for (flip = 0; flip < 2; ++flip) {
            int lo = flip ? r.w - 1 - right : left, hi = flip ? r.w - 1 - left : right;
            int crop = max0_min(r.w - 16, lo - (16 - (hi - lo + 1)) / 2);
            u32 n = 4;
            Rec m = r;
            memset(stream, 0, sizeof stream);
            for (y = 0; y < r.h; ++y) {
                int plane;
                for (plane = 0; plane < 2; ++plane) {
                    u32 bits = 0;
                    for (x = 0; x < 16; ++x) {
                        TRY(garden_bit(d, len, &r, flip ? r.w - 1 - (crop + x) : crop + x, y, plane, &bit));
                        if (bit) bits |= 1u << (15 - x);
                    }
                    REQUIRE(n + 2 <= 136, "a garden sprite exceeds its slot");
                    stream[n++] = (u8)(bits >> 8); stream[n++] = (u8)bits;
                }
            }
            memcpy(stream + 136, stream, 136);
            m.id = (u16)(64000 + r.id); m.w = 16; m.dma = 16; m.hw = 1; m.parts = 0;
            m.ox = (s16)((flip ? r.w - r.ox : r.ox) - crop);
            m.frame = (u16)(r.frame * 2 + flip);
            TRY(rec_add(&a->rs, &m, stream, 272));
            d = stream_data(&a->rs, r.stream);
        }
    }
    return 1;
}

static int build_records(Art* a, const Selection* sel, u32 nsel)
{
    u32 i;
    for (i = 0; i < nsel; ++i) TRY(sprite_frames(a, &sel[i]));
    if (a->profile == P_HUD) TRY(hud_glyphs(a));
    if (a->profile == P_GAME) {
        static const char digits[] = "-0123456789+ABCEGHILMNOPRSTUY";
        for (i = 0; digits[i]; ++i) {
            const Glyph* g = font_glyph(a->font, (u8)digits[i]);
            REQUIRE(g && g->shift == 8 && !g->kernings, "native numeric text requires eight-pixel advances without kerning");
        }
    }
    TRY(texts(a));
    if (a->profile == P_GAME) { TRY(credits(a)); TRY(dithers(a)); }
    if (a->profile == P_GAME || a->profile == P_CLEAR) TRY(actor_streams(a));
    if (a->profile == P_GAME) { TRY(style_icons(a)); TRY(garden_sprites(a)); }
    return 1;
}

/* ---- banks */

static const u8* source_prefix(void)
{
    static u8 p[8];
    static const char hex[] = SOURCE_PREFIX;
    u32 i;
    for (i = 0; i < 8; ++i) {
        u32 hi = (u32)hex[2 * i], lo = (u32)hex[2 * i + 1];
        hi = hi <= '9' ? hi - '0' : hi - 'a' + 10; lo = lo <= '9' ? lo - '0' : lo - 'a' + 10;
        p[i] = (u8)(hi << 4 | lo);
    }
    return p;
}

static int write_bank(Art* a, const char* name, const u32* idx, const u8* life, u32 n, u32 budget, Buf* keep)
{
    Buf out;
    int ok;
    TRY(pack_bank(&a->rs, idx, life, n, source_prefix(), &out));
    REQUIRE(!budget || out.n <= budget, "a scene bank exceeds its admission budget");
    ok = dw_write_file(a->c->dest, name, out.p, out.n);
    if (keep && ok) *keep = out; else buf_free(&out);
    return ok;
}

static int front_frame(u32 id, u32 frame, int* listed)
{
    u32 i;
    for (i = 0; i < COUNT(FRONT_FRAMES); ++i)
        if (FRONT_FRAMES[i].id == id) { *listed = 1; return frame < 32 && (FRONT_FRAMES[i].frames >> frame & 1); }
    *listed = 0;
    return 0;
}

static int game_banks(Art* a)
{
    u32 n = a->rs.n, i, ng = 0, nb = 0, nf = 0, nc = 0;
    u32* game = dw_alloc(4 * n + 4);
    u32* boot = dw_alloc(4 * n + 4);
    u32* front = dw_alloc(4 * n + 4);
    u8* life = dw_alloc(n + 1);
    int ok = 0;
    Buf pack, mask;
    buf_init(&pack);
    if (!game || !boot || !front || !life) goto done;
    for (i = 0; i < n; ++i) {
        const Rec* r = &a->rs.r[i];
        int listed, keep = front_frame(r->id, r->frame, &listed);
        if (r->id == 654) continue;
        if (IN(r->id, AFTERIMAGE_IDS) && r->frame >= 1024) game[ng++] = i;
        else if (IN(r->id, BOOT_IDS)) boot[nb++] = i;
        else if (listed) { if (keep) front[nf++] = i; }
        else if (IN(r->id, FRONT_IDS)) front[nf++] = i;
        else if (IN(r->id, GAME_IDS)) {
            game[ng++] = i;
            if ((r->id == 76 && r->frame == 0) || (r->id == 51 && (r->frame == 7 || r->frame == 13)) || r->id == 27) front[nf++] = i;
        } else { dw_fail("a scene art record has no scene owner"); goto done; }
    }
    /* Cavern: the gameplay records whose IDs it admits. */
    for (i = 0; i < ng; ++i) if (IN(a->rs.r[game[i]].id, CAVERN_IDS)) boot[nb + nc++] = game[i];
    if (!write_bank(a, "Downwell.cavern", boot + nb, 0, nc, BUDGET_CAVERN, 0)) goto done;
    if (!write_bank(a, "Downwell.boot", boot, 0, nb, BUDGET_BOOT, 0)) goto done;
    for (i = 0; i < nf; ++i) {
        const Rec* r = &a->rs.r[front[i]];
        int listed;
        front_frame(r->id, 0, &listed);
        life[i] = IN(r->id, FRONT_STYLE_IDS) ? 0 : (!listed && r->id != 76) ? 1 : 2;
    }
    if (!write_bank(a, "Downwell.front", front, life, nf, BUDGET_FRONT, &pack)) goto done;
    if (!write_bank(a, "Downwell.game", game, 0, ng, BUDGET_GAME, 0)) goto done;
    if (!entry_mask(&pack, &mask)) goto done;
    ok = dw_write_file(a->c->dest, "Downwell.entrymask", mask.p, mask.n);
    buf_free(&mask);
    if (ok && (ok = exit_mask(&pack, 48, 110, &mask))) { ok = dw_write_file(a->c->dest, "Downwell.exitmask", mask.p, mask.n); buf_free(&mask); }
    if (ok && (ok = exit_mask(&pack, 22, 0, &mask))) { ok = dw_write_file(a->c->dest, "Downwell.selectionmask", mask.p, mask.n); buf_free(&mask); }
done:
    buf_free(&pack);
    dw_free(game); dw_free(boot); dw_free(front); dw_free(life);
    return ok;
}

static int profile(Art* a, int which, const Selection* sel, u32 nsel, const char* name)
{
    int ok;
    recs_init(&a->rs);
    a->profile = which;
    ok = build_records(a, sel, nsel);
    if (ok && which == P_GAME) ok = game_banks(a);
    else if (ok) {
        u32* idx = dw_alloc(4 * a->rs.n + 4), i, n = 0;
        ok = idx != 0;
        for (i = 0; ok && i < a->rs.n; ++i)
            if (!(which == P_CLEAR && IN(a->rs.r[i].id, UPGRADE_PLAYER_IDS))) idx[n++] = i;
        ok = ok && write_bank(a, name, idx, 0, n, 0, 0);
        dw_free(idx);
    }
    recs_free(&a->rs);
    return ok && !io_cancelled();
}

static int want_selection(Art* a, const Selection* sel, u32 n)
{
    u32 i, f;
    for (i = 0; i < n; ++i) {
        Sprite s;
        TRY(tex_sprite(a->w, sel[i].id, &s));
        for (f = 0; f < s.nframes; ++f)
            if (sel[i].frames == 0xffffffffu || (f < 32 && (sel[i].frames >> f & 1))) TRY(tex_want(&a->tex, a->w, tex_frame(a->w, &s, f)));
    }
    return 1;
}

int stage_art(Context* c)
{
    static Art art;
    Art* a = &art;
    int ok;
    memset(a, 0, sizeof *a);
    a->c = c; a->w = &c->win;
    tex_init(&a->tex);
    a->font = dw_alloc(sizeof *a->font);
    ok = a->font && tex_font(a->w, 2, a->font) && ini_load(a);
    ok = ok && want_selection(a, SEL_HUD, COUNT(SEL_HUD)) && want_selection(a, SEL_RESULTS, COUNT(SEL_RESULTS)) &&
         want_selection(a, SEL_PAUSE, COUNT(SEL_PAUSE)) && want_selection(a, SEL_SHOP, COUNT(SEL_SHOP)) &&
         want_selection(a, SEL_CLEAR, COUNT(SEL_CLEAR)) && want_selection(a, SEL_GAME, COUNT(SEL_GAME));
    if (ok) {
        static const Selection dither[] = {{128, 1}, {129, 1}};
        ok = want_selection(a, dither, 2) && tex_want(&a->tex, a->w, a->font->tpag);
    }
    ok = ok && tex_decode(&a->tex, a->w);
    ok = ok && profile(a, P_HUD, SEL_HUD, COUNT(SEL_HUD), "Downwell.hud");
    ok = ok && profile(a, P_RESULTS, SEL_RESULTS, COUNT(SEL_RESULTS), "Downwell.results");
    ok = ok && profile(a, P_PAUSE, SEL_PAUSE, COUNT(SEL_PAUSE), "Downwell.pause");
    ok = ok && profile(a, P_SHOP, SEL_SHOP, COUNT(SEL_SHOP), "Downwell.shop");
    ok = ok && profile(a, P_CLEAR, SEL_CLEAR, COUNT(SEL_CLEAR), "Downwell.clear");
    ok = ok && profile(a, P_GAME, SEL_GAME, COUNT(SEL_GAME), 0);
    img_free(&a->fontImg);
    tex_free(&a->tex);
    dw_free(a->font);
    dw_free(a->ini);
    return ok;
}

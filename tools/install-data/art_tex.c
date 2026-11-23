#include "art_tex.h"
#include "art_bz.h"

/* SPRT entry (GMS2, sprite version 3): name, width, height, four margins,
   transparent, smooth, preload, bbox mode, separate masks, origin x and y,
   -1, version, type, playback speed, speed type, sequence and nine-slice
   offsets, then the frame count and its TPAG pointers. */
int tex_sprite(Win* w, u32 id, Sprite* s)
{
    u32 off, e;
    TRY(win_chunk(w, "SPRT", &off, 0));
    REQUIRE(id < win_list_count(w, off), "data.win lacks a scene sprite");
    e = win_list_item(w, off, id);
    REQUIRE(win_s32(w, e + 56) == -1 && win_u32(w, e + 60) == 3 && win_u32(w, e + 64) == 0,
            "data.win has an unexpected sprite layout");
    s->w = (int)win_s32(w, e + 4); s->h = (int)win_s32(w, e + 8);
    s->smooth = (int)win_u32(w, e + 32);
    s->ox = (int)win_s32(w, e + 48); s->oy = (int)win_s32(w, e + 52);
    s->nframes = win_u32(w, e + 84);
    s->framesAt = e + 88;
    return !w->failed;
}

u32 tex_frame(Win* w, const Sprite* s, u32 frame) { return win_u32(w, s->framesAt + 4 * frame); }

int tex_tpag(Win* w, u32 ptr, Tpag* t)
{
    u8 b[22];
    TRY(win_read(w, ptr, b, 22));
    t->sx = rd_le16(b); t->sy = rd_le16(b + 2); t->sw = rd_le16(b + 4); t->sh = rd_le16(b + 6);
    t->tx = rd_le16(b + 8); t->ty = rd_le16(b + 10); t->tw = rd_le16(b + 12); t->th = rd_le16(b + 14);
    t->bw = rd_le16(b + 16); t->bh = rd_le16(b + 18); t->page = rd_le16(b + 20);
    return 1;
}

/* FONT entry: name, display name, size, bold, italic, range start/charset/
   antialiasing, range end, TPAG pointer, scale x and y, ascender offset,
   ascender, then the glyph list. A glyph: character, x, y, width, height,
   shift, offset, kerning count and pairs. */
int tex_font(Win* w, u32 index, Font* f)
{
    u32 off, e, i;
    TRY(win_chunk(w, "FONT", &off, 0));
    REQUIRE(index < win_list_count(w, off), "data.win lacks font 2");
    e = win_list_item(w, off, index);
    f->tpag = win_u32(w, e + 28);
    f->n = win_u32(w, e + 48);
    REQUIRE(f->n <= sizeof f->g / sizeof f->g[0], "font 2 has more glyphs than expected");
    for (i = 0; i < f->n; ++i) {
        u8 b[16];
        Glyph* g = &f->g[i];
        TRY(win_read(w, win_u32(w, e + 52 + 4 * i), b, 16));
        g->ch = rd_le16(b); g->x = rd_le16(b + 2); g->y = rd_le16(b + 4);
        g->w = rd_le16(b + 6); g->h = rd_le16(b + 8);
        g->shift = (s16)rd_le16(b + 10); g->offset = (s16)rd_le16(b + 12);
        g->kernings = rd_le16(b + 14);
    }
    return !w->failed;
}

const Glyph* font_glyph(const Font* f, u32 ch)
{
    u32 i;
    /* A later duplicate would win in Python's dict; take the last. */
    for (i = f->n; i-- > 0;) if (f->g[i].ch == ch) return &f->g[i];
    return 0;
}

void tex_init(Tex* t) { memset(t, 0, sizeof *t); }

void tex_free(Tex* t)
{
    u32 i;
    for (i = 0; i < t->nslabs; ++i) dw_free(t->slab[i]);
    dw_free(t->items);
    tex_init(t);
}

static TexItem* find(Tex* t, u32 ptr, u32* at)
{
    u32 lo = 0, hi = t->n;
    while (lo < hi) { u32 mid = (lo + hi) >> 1; if (t->items[mid].ptr < ptr) lo = mid + 1; else hi = mid; }
    if (at) *at = lo;
    return lo < t->n && t->items[lo].ptr == ptr ? &t->items[lo] : 0;
}

int tex_want(Tex* t, Win* w, u32 ptr)
{
    u32 at, i;
    TexItem* it;
    if (find(t, ptr, &at)) return 1;
    if (t->n == t->cap) {
        u32 cap = t->cap ? t->cap * 2 : 256;
        TexItem* p = dw_alloc(cap * sizeof *p);
        if (!p) return 0;
        if (t->n) memcpy(p, t->items, t->n * sizeof *p);
        dw_free(t->items); t->items = p; t->cap = cap;
    }
    for (i = t->n; i > at; --i) t->items[i] = t->items[i - 1];
    ++t->n;
    it = &t->items[at];
    it->ptr = ptr; it->head = it->tail = 0xffffffffu;
    TRY(tex_tpag(w, ptr, &it->t));
    REQUIRE(it->t.sw == it->t.tw && it->t.sh == it->t.th, "a scene sprite's texture item is scaled");
    return 1;
}

const Tpag* tex_item(Tex* t, u32 ptr) { TexItem* it = find(t, ptr, 0); return it ? &it->t : 0; }

#define SLAB_BITS 15
#define SLAB (1u << SLAB_BITS)
static u8* at_handle(Tex* t, u32 h) { return t->slab[h >> SLAB_BITS] + (h & (SLAB - 1)); }

/* Append one row (palette indices) to the item's run-length coded rows. */
static int keep_row(Tex* t, TexItem* it, const u8* rgba, u32 n)
{
    u32 need = 4 + 2 * n, h, x, c = 0;
    u8* p;
    u8* row = t->row;
    for (x = 0; x < n; ++x, rgba += 4) {
        if (!x || memcmp(rgba, rgba - 4, 4)) {
            for (c = 0; c < (u32)t->npal; ++c) if (!memcmp(t->palette[c], rgba, 4)) break;
            if (c == (u32)t->npal) {
                REQUIRE(t->npal < 16, "a texture page has more colours than expected");
                memcpy(t->palette[t->npal++], rgba, 4);
            }
        }
        row[x] = (u8)c;
    }
    if (!t->nslabs || t->fill + need > SLAB) {
        REQUIRE(t->nslabs < TEX_SLABS, "the scene art needs more texture memory than expected");
        t->slab[t->nslabs] = dw_alloc(SLAB);
        if (!t->slab[t->nslabs]) return 0;
        ++t->nslabs; t->fill = 0;
    }
    h = (t->nslabs - 1) << SLAB_BITS | t->fill;
    p = at_handle(t, h);
    p[0] = p[1] = p[2] = p[3] = 0xff;
    p += 4;
    for (x = 0; x < n;) {
        u32 run = 1, c = row[x];
        while (x + run < n && row[x + run] == c && run < 271) ++run;
        if (run < 16) *p++ = (u8)(c << 4 | (run - 1));
        else { *p++ = (u8)(c << 4 | 15); *p++ = (u8)(run - 16); }
        x += run;
    }
    t->fill = (u32)(p - t->slab[t->nslabs - 1]);
    t->fill = (t->fill + 1) & ~1u;
    if (it->tail != 0xffffffffu) {
        u8* q = at_handle(t, it->tail);
        q[0] = (u8)(h >> 24); q[1] = (u8)(h >> 16); q[2] = (u8)(h >> 8); q[3] = (u8)h;
    } else it->head = h;
    it->tail = h;
    return 1;
}

int tex_image(Tex* t, u32 ptr, Img* out)
{
    TexItem* it = find(t, ptr, 0);
    REQUIRE(it, "a scene texture item was not decoded");
    return tex_rows(t, ptr, 0, it->t.sh, out);
}

int tex_rows(Tex* t, u32 ptr, u32 y0, u32 y1, Img* out)
{
    TexItem* it = find(t, ptr, 0);
    u32 h, y = 0;
    REQUIRE(it && y0 <= y1 && y1 <= it->t.sh, "a scene texture item was not decoded");
    TRY(img_new(out, it->t.sw, (int)(y1 - y0)));
    for (h = it->head; h != 0xffffffffu && y < y1; ++y) {
        const u8* p = at_handle(t, h);
        u8* d;
        u32 x = 0;
        h = rd_be32(p);
        if (y < y0) continue;
        d = PIX(out, 0, y - y0);
        p += 4;
        while (x < it->t.sw) {
            u32 c = *p >> 4, run = (*p & 15) + 1;
            ++p;
            if (run == 16) run = 16 + *p++;
            for (; run--; ++x, d += 4) memcpy(d, t->palette[c], 4);
        }
    }
    REQUIRE(y == y1, "a scene texture item was not decoded");
    return 1;
}

/* The bzip2 output, a buffer at a time. */
typedef struct { Bz z; u8 buf[4096]; u32 at, n; } Source;
static int src_byte(Source* s, u32* b)
{
    if (s->at == s->n) {
        TRY(bz_read(&s->z, s->buf, sizeof s->buf, &s->n));
        s->at = 0;
        REQUIRE(s->n, "a texture page ends early");
    }
    *b = s->buf[s->at++];
    return 1;
}

static int sext(u32 v, int bits) { return (int)(v ^ (1u << (bits - 1))) - (1 << (bits - 1)); }

static int decode_page(Tex* t, Win* w, u32 page, Source* s)
{
    u32 off, e, size, ptr, width, height, length, y, n = 0, i, rows = 0, run = 0;
    u8 head[12], index[64][4], px[4] = {0, 0, 0, 255}, *row = 0;
    u32* order;
    memset(index, 0, sizeof index);
    for (i = 0; i < t->n; ++i)
        if (t->items[i].t.page == page) {
            ++n;
            if ((u32)t->items[i].t.sy + t->items[i].t.sh > rows) rows = t->items[i].t.sy + t->items[i].t.sh;
        }
    if (!n) return 1;
    order = dw_alloc(n * 4);
    if (!order) return 0;
    /* Items by first row (insertion: a thousand at most). */
    for (n = 0, i = 0; i < t->n; ++i)
        if (t->items[i].t.page == page) {
            u32 k = n++;
            while (k && t->items[order[k - 1]].t.sy > t->items[i].t.sy) { order[k] = order[k - 1]; --k; }
            order[k] = i;
        }
    if (!win_chunk(w, "TXTR", &off, 0)) { dw_free(order); return 0; }
    e = win_list_item(w, off, page);
    size = win_u32(w, e + 8); ptr = win_u32(w, e + 12);
    if (!win_read(w, ptr, head, 12) || memcmp(head, "2zoq", 4) || size < 12 || !bz_open(&s->z, w, ptr + 12, size - 12)) {
        dw_free(order);
        return dw_fail("a texture page is not in the expected format");
    }
    s->at = s->n = 0;
    for (i = 0; i < 12; ++i) { u32 b; if (!src_byte(s, &b)) goto fail; head[i] = (u8)b; }
    width = rd_le16(head + 4); height = rd_le16(head + 6); length = rd_le32(head + 8);
    if (memcmp(head, "fioq", 4) || rows > height) { dw_fail("a texture page is not in the expected format"); goto fail; }
    row = dw_alloc(width * 4);
    t->row = dw_alloc(width);
    if (!row || !t->row) goto fail;
    for (y = 0; y < rows; ++y) {
        u32 x, k;
        if (io_cancelled()) { dw_fail("cancelled"); goto fail; }
        for (x = 0; x < width; ++x) {
            if (run) --run;
            else if (length) {
                u32 c, c2, c3;
                if (!src_byte(s, &c)) goto fail;
                --length;
                if ((c & 0xc0) == 0x00) memcpy(px, index[c & 63], 4);
                else if ((c & 0xe0) == 0x40) run = c & 0x1f;
                else if ((c & 0xe0) == 0x60) { if (!src_byte(s, &c2)) goto fail; --length; run = ((c & 0x1f) << 8 | c2) + 32; }
                else if ((c & 0xc0) == 0x80) {
                    px[0] = (u8)(px[0] + sext(c >> 4 & 3, 2)); px[1] = (u8)(px[1] + sext(c >> 2 & 3, 2)); px[2] = (u8)(px[2] + sext(c & 3, 2));
                } else if ((c & 0xe0) == 0xc0) {
                    if (!src_byte(s, &c2)) goto fail;
                    --length;
                    px[0] = (u8)(px[0] + sext(c & 0x1f, 5)); px[1] = (u8)(px[1] + sext(c2 >> 4, 4)); px[2] = (u8)(px[2] + sext(c2 & 15, 4));
                } else if ((c & 0xf0) == 0xe0) {
                    if (!src_byte(s, &c2) || !src_byte(s, &c3)) goto fail;
                    length -= 2;
                    px[0] = (u8)(px[0] + sext((c & 15) << 1 | c2 >> 7, 5));
                    px[1] = (u8)(px[1] + sext((c2 & 0x7c) >> 2, 5));
                    px[2] = (u8)(px[2] + sext((c2 & 3) << 3 | c3 >> 5, 5));
                    px[3] = (u8)(px[3] + sext(c3 & 0x1f, 5));
                } else {
                    for (k = 0; k < 4; ++k)
                        if (c & (8u >> k)) { if (!src_byte(s, &c2)) goto fail; --length; px[k] = (u8)c2; }
                }
                memcpy(index[(px[0] ^ px[1] ^ px[2] ^ px[3]) & 63], px, 4);
            }
            memcpy(row + 4 * x, px, 4);
        }
        for (k = 0; k < n; ++k) {
            TexItem* it = &t->items[order[k]];
            if (it->t.sy > y) break;
            if (y < (u32)it->t.sy + it->t.sh) {
                if ((u32)it->t.sx + it->t.sw > width) { dw_fail("a texture item exceeds its page"); goto fail; }
                if (!keep_row(t, it, row + 4 * it->t.sx, it->t.sw)) goto fail;
            }
        }
    }
    dw_free(row); dw_free(t->row); t->row = 0; dw_free(order); bz_close(&s->z);
    return 1;
fail:
    dw_free(row); dw_free(t->row); t->row = 0; dw_free(order); bz_close(&s->z);
    return 0;
}

int tex_decode(Tex* t, Win* w)
{
    Source* s = dw_alloc(sizeof *s);
    u32 page, off, pages;
    int ok;
    if (!s) return 0;
    ok = win_chunk(w, "TXTR", &off, 0);
    pages = ok ? win_list_count(w, off) : 0;
    for (page = 0; page < pages && ok; ++page) ok = decode_page(t, w, page, s);
    dw_free(s);
    return ok;
}

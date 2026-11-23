#include "art_img.h"

int img_new(Img* m, int w, int h)
{
    u32 n = (u32)w * (u32)h * 4;
    m->w = w; m->h = h;
    m->p = dw_zalloc(n ? n : 4);
    return m->p != 0;
}

void img_free(Img* m) { dw_free(m->p); m->p = 0; m->w = m->h = 0; }

int img_crop(Img* out, const Img* in, int x0, int y0, int x1, int y1)
{
    int y, from, to;
    TRY(img_new(out, x1 - x0, y1 - y0));
    from = x0 < 0 ? 0 : x0;
    to = x1 > in->w ? in->w : x1;
    if (from >= to) return 1;
    for (y = y0 < 0 ? 0 : y0; y < y1 && y < in->h; ++y)
        memcpy(PIX(out, from - x0, y - y0), PIX(in, from, y), 4 * (u32)(to - from));
    return 1;
}

int img_transpose(Img* out, const Img* in, int op)
{
    int x, y, w = in->w, h = in->h;
    int turned = op == ROT_90 || op == ROT_270;
    TRY(img_new(out, turned ? h : w, turned ? w : h));
    for (y = 0; y < out->h; ++y)
        for (x = 0; x < out->w; ++x) {
            int sx, sy;
            switch (op) {
            case FLIP_LR: sx = w - 1 - x; sy = y; break;
            case FLIP_TB: sx = x; sy = h - 1 - y; break;
            case ROT_90: sx = w - 1 - y; sy = x; break;
            case ROT_180: sx = w - 1 - x; sy = h - 1 - y; break;
            default: sx = y; sy = h - 1 - x; break;
            }
            memcpy(PIX(out, x, y), PIX(in, sx, sy), 4);
        }
    return 1;
}

#define DIV255(v) ((((v) >> 8) + (v)) >> 8)
void img_composite(Img* dst, const Img* src, int x0, int y0)
{
    int x, y;
    for (y = 0; y < src->h; ++y) {
        if (y0 + y < 0 || y0 + y >= dst->h) continue;
        for (x = 0; x < src->w; ++x) {
            const u8* s;
            u8* d;
            u32 sa, blend, outa, c1, c2, k;
            if (x0 + x < 0 || x0 + x >= dst->w) continue;
            s = PIX(src, x, y); d = PIX(dst, x0 + x, y0 + y);
            sa = s[3];
            if (!sa) continue;
            blend = d[3] * (255 - sa);
            outa = sa * 255 + blend;
            c1 = sa * 255 * 255 * 128 / outa;
            c2 = 255 * 128 - c1;
            for (k = 0; k < 3; ++k) {
                u32 v = s[k] * c1 + d[k] * c2 + (0x80 << 7);
                d[k] = (u8)(DIV255(v) >> 7);
            }
            d[3] = (u8)DIV255(outa + 0x80);
        }
    }
}

int img_bbox(const Img* m, int box[4])
{
    int x, y, found = 0;
    box[0] = m->w; box[1] = m->h; box[2] = 0; box[3] = 0;
    for (y = 0; y < m->h; ++y)
        for (x = 0; x < m->w; ++x)
            if (PIX(m, x, y)[3]) {
                found = 1;
                if (x < box[0]) box[0] = x;
                if (y < box[1]) box[1] = y;
                if (x + 1 > box[2]) box[2] = x + 1;
                if (y + 1 > box[3]) box[3] = y + 1;
            }
    return found;
}

int img_tinted(Img* out, const Img* like, u8 r, u8 g, u8 b)
{
    u32 i, n = (u32)like->w * (u32)like->h;
    TRY(img_new(out, like->w, like->h));
    for (i = 0; i < n; ++i) { u8* d = out->p + 4 * i; d[0] = r; d[1] = g; d[2] = b; d[3] = like->p[4 * i + 3]; }
    return 1;
}

int img_pens(const Img* m, u8* pens)
{
    u32 i, n = (u32)m->w * (u32)m->h;
    for (i = 0; i < n; ++i) {
        const u8* p = m->p + 4 * i;
        u32 rgb = (u32)p[0] << 16 | (u32)p[1] << 8 | p[2];
        if (!p[3]) pens[i] = 0;
        else if (rgb == 0xffffff) pens[i] = 1;
        else if (rgb == 0xff0000) pens[i] = 2;
        else if (rgb == 0) pens[i] = 3;
        else return dw_fail("a scene art pixel is outside the four-pen palette");
    }
    return 1;
}

/* data.win SPRT, TPAG, FONT and TXTR for the scene art: sprite geometry,
   texture page items, font 2's glyphs, and the pixels of only the texture
   page items the banks use. The pages (2048x2048 and 1024x2048 RGBA) are
   never held whole: the bzip2 stream feeds the sequential QOI-like decoder
   row by row, and each wanted item keeps its rows run-length coded against
   a palette of the colours seen (13 on Downwell's pages). */
#ifndef DW_ART_TEX_H
#define DW_ART_TEX_H
#include "win.h"
#include "art_img.h"
typedef struct { u16 sx, sy, sw, sh, tx, ty, tw, th, bw, bh, page; } Tpag;
typedef struct {
    int w, h, ox, oy, smooth;
    u32 nframes, framesAt;  /* data.win offset of the TPAG pointer list */
} Sprite;
int tex_sprite(Win* w, u32 id, Sprite* s);
u32 tex_frame(Win* w, const Sprite* s, u32 frame);  /* the frame's TPAG pointer */
int tex_tpag(Win* w, u32 ptr, Tpag* t);

typedef struct { u16 ch, x, y, w, h; s16 shift, offset; u16 kernings; } Glyph;
typedef struct { u32 tpag; u32 n; Glyph g[512]; } Font;
int tex_font(Win* w, u32 index, Font* f);
const Glyph* font_glyph(const Font* f, u32 ch);

typedef struct { u32 ptr; Tpag t; u32 head, tail; } TexItem;
#define TEX_SLABS 128
typedef struct {
    TexItem* items;
    u32 n, cap;
    u8* slab[TEX_SLABS];
    u32 nslabs, fill;
    u8 palette[16][4];
    int npal;
    u8* row;  /* palette indices of the row being kept */
} Tex;
void tex_init(Tex* t);
void tex_free(Tex* t);
/* Register an item whose pixels a bank needs (duplicates are fine). */
int tex_want(Tex* t, Win* w, u32 ptr);
/* Decode the texture pages, keeping the wanted items' rows. */
int tex_decode(Tex* t, Win* w);
/* The item's source rectangle as a new image (sw x sh). */
int tex_image(Tex* t, u32 ptr, Img* out);
const Tpag* tex_item(Tex* t, u32 ptr);
#endif

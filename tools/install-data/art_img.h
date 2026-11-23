/* RGBA images and the few PIL operations tools/generate-amiga-hud.py uses,
   reproduced exactly in integer arithmetic (PIL's own are integer too). */
#ifndef DW_ART_IMG_H
#define DW_ART_IMG_H
#include "dw.h"
typedef struct { int w, h; u8* p; } Img;  /* 4 bytes a pixel: r, g, b, a */
/* A new transparent (0, 0, 0, 0) image; w or h may be 0. */
int img_new(Img* m, int w, int h);
void img_free(Img* m);
/* Image.crop: pixels outside the source are transparent black. */
int img_crop(Img* out, const Img* in, int x0, int y0, int x1, int y1);
enum { FLIP_LR, FLIP_TB, ROT_90, ROT_180, ROT_270 };
int img_transpose(Img* out, const Img* in, int op);
/* Image.alpha_composite(src, (x, y)) in place: PIL's integer formula,
   clipped to the destination. */
void img_composite(Img* dst, const Img* src, int x, int y);
/* Image.getbbox() of an RGBA image: the box of nonzero alpha; 0 if empty. */
int img_bbox(const Img* m, int box[4]);
/* Image.new(size, rgb + 255) with putalpha(alpha of `like`). */
int img_tinted(Img* out, const Img* like, u8 r, u8 g, u8 b);
/* pixel_to_pen of every pixel; fails outside the four-pen palette. */
int img_pens(const Img* m, u8* pens);
#define PIX(m, x, y) ((m)->p + 4 * ((u32)(y) * (u32)(m)->w + (u32)(x)))
#endif

/* A bzip2 stream decoder for the texture pages: our own, in the small
   memory layout (2.5 bytes per block byte, 2.25 MB for a 900 KB block).
   The input comes from data.win; the output is read sequentially. */
#ifndef DW_ART_BZ_H
#define DW_ART_BZ_H
#include "win.h"
typedef struct {
    Win* win;
    u32 inPos, inEnd;      /* data.win bytes still to read */
    u8 in[2048];
    u32 inAt, inHave;
    u32 bits;              /* bit buffer, most significant first */
    int nbits;
    u32 blockMax;          /* level * 100000 */
    u16* ll16;             /* low 16 bits of each T entry (first the block bytes) */
    u8* ll4;               /* high 4 bits, two entries a byte */
    u8* selectors;
    u32 cftab[257];
    /* the current block's output */
    u32 nblock, used, tPos;
    int runLength;         /* identical bytes emitted in a row (0..4) */
    u32 repeat;            /* copies of `last` still to emit */
    u8 last;
    int ended;
} Bz;
/* The stream occupies data.win [off, off + len). */
int bz_open(Bz* z, Win* w, u32 off, u32 len);
void bz_close(Bz* z);
/* Up to n bytes into dst; *got is 0 only at the end of the stream. */
int bz_read(Bz* z, u8* dst, u32 n, u32* got);
#endif

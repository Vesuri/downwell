/* The encoded art records of generate-amiga-hud.py and pack_records: the
   DWAP bank layout with its word-aligned payload sharing. */
#ifndef DW_ART_PACK_H
#define DW_ART_PACK_H
#include "dw.h"
typedef struct {
    u16 id, frame;
    s16 ox, oy;
    u16 w, h, dma;
    u8 hw, parts;  /* hardware (DMA) record; hardwareParts */
    u32 stream;
} Rec;
typedef struct { u32 at, len, hash, next; } Stream;  /* at: arena handle */
#define ARENA_SLABS 128
typedef struct {
    Rec* r;
    u32 n, cap;
    Stream* s;
    u32 ns, scap;
    u32* buckets;  /* 4096 hash chains of identical streams */
    u8* slab[ARENA_SLABS];
    u32 nslabs, fill;
} Recs;
void recs_init(Recs* rs);
void recs_free(Recs* rs);
/* Identical streams are stored once. */
int rec_add(Recs* rs, const Rec* meta, const u8* data, u32 len);
const u8* stream_data(const Recs* rs, u32 stream);
/* pack_records over the records idx[0..n) (in encoded order); life[k] is
   record k's lifetime group (the front bank's live prefix), or life is 0. */
int pack_bank(Recs* rs, const u32* idx, const u8* life, u32 n, const u8* sourcePrefix, Buf* out);
/* build_exit_mask(pack, stride, left) and build_entry_mask(pack). */
int exit_mask(const Buf* pack, u32 stride, u32 left, Buf* out);
int entry_mask(const Buf* pack, Buf* out);
#endif

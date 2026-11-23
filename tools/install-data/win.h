/* Random access to the player's data.win (GameMaker FORM, little-endian)
   through a small page cache: the file is 28 MB, the target has 4 MB. */
#ifndef DW_WIN_H
#define DW_WIN_H
#include "dw.h"
#define WIN_PAGE 4096u
typedef struct { char id[4]; u32 off, len; } WinChunk; /* off: after the 8-byte header */
typedef struct {
    File f;
    u32 size;
    WinChunk chunks[32];
    int nchunks;
    int failed;
    u32 slots;     /* cache pages */
    u8* pages;
    u32* tags;     /* page number + 1 held by each slot, 0 when empty */
} Win;
int win_open(Win* w, const char* path, u32 cacheBytes);
void win_close(Win* w);
/* The chunk's data offset and length; fails when it is missing. */
int win_chunk(Win* w, const char* id, u32* off, u32* len);
int win_read(Win* w, u32 off, void* dst, u32 n);
u32 win_u8(Win* w, u32 off);
u32 win_u16(Win* w, u32 off);
u32 win_u32(Win* w, u32 off);
s32 win_s32(Win* w, u32 off);
/* A string reference points at the characters; its length precedes them. */
u32 win_strlen(Win* w, u32 ref);
int win_string(Win* w, u32 ref, char* dst, u32 cap);
int win_streq(Win* w, u32 ref, const char* s);
/* A resource list: count, then absolute pointers. */
u32 win_list_count(Win* w, u32 off);
u32 win_list_item(Win* w, u32 off, u32 index);
#endif

/* DownwellInstallData: shared definitions. The converter builds every file
   the Amiga build installs from the player's own Windows copy of Downwell.
   The same sources build on the host (tests) and the 68020 (the Installer). */
#ifndef DW_H
#define DW_H
#include "io.h"
#ifndef INSTALL_AMIGA
#include <string.h>
#endif
typedef unsigned char u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef signed char s8;
#ifdef INSTALL_AMIGA
typedef long s32;
#else
typedef int32_t s32;
#endif

/* The first failure: a message for the player, set once. */
extern const char* dw_error;
int dw_fail(const char* message);
#define REQUIRE(cond, message) do { if (!(cond)) return dw_fail(message); } while (0)
#define TRY(call) do { if (!(call)) return 0; } while (0)

/* Allocation through io_alloc, with the peak the converter needed. */
void* dw_alloc(u32 n);
void* dw_zalloc(u32 n);
void dw_free(void* p);
u32 dw_alloc_peak(void);

/* A growable big-endian output buffer. */
typedef struct { u8* p; u32 n, cap; int failed; } Buf;
void buf_init(Buf* b);
void buf_free(Buf* b);
u8* buf_grow(Buf* b, u32 n);
void buf_put(Buf* b, const void* p, u32 n);
void buf_u8(Buf* b, u32 v);
void buf_u16(Buf* b, u32 v);
void buf_u32(Buf* b, u32 v);
void buf_align(Buf* b, u32 base, u32 to);
void buf_set_u32(Buf* b, u32 at, u32 v);

u16 rd_le16(const u8* p);
u32 rd_le32(const u8* p);
u16 rd_be16(const u8* p);
u32 rd_be32(const u8* p);
int dw_strcmp(const char* a, const char* b);
char* dw_strcpy(char* d, const char* s);
void dw_path(char* out, u32 cap, const char* dir, const char* name);

/* Write a finished output file into the destination drawer. */
int dw_write_file(const char* dir, const char* name, const void* data, u32 n);
#endif

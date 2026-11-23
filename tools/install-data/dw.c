#include "dw.h"

const char* dw_error;
int dw_fail(const char* message) { if (!dw_error) dw_error = message; return 0; }

static u32 live, peak;
void* dw_alloc(u32 n)
{
    u32* p = io_alloc(n + 4);
    if (!p) { dw_fail("not enough memory"); return 0; }
    p[0] = n; live += n; if (live > peak) peak = live;
    return p + 1;
}
void* dw_zalloc(u32 n) { void* p = dw_alloc(n); if (p) memset(p, 0, n); return p; }
void dw_free(void* p) { u32* q = p; if (!q) return; live -= q[-1]; io_free(q - 1); }
u32 dw_alloc_peak(void) { return peak; }

void buf_init(Buf* b) { b->p = 0; b->n = b->cap = 0; b->failed = 0; }
void buf_free(Buf* b) { dw_free(b->p); buf_init(b); }
u8* buf_grow(Buf* b, u32 n)
{
    if (b->failed) return 0;
    if (b->n + n > b->cap) {
        u32 cap = b->cap ? b->cap : 256;
        u8* p;
        while (cap < b->n + n) cap *= 2;
        p = dw_alloc(cap);
        if (!p) { b->failed = 1; return 0; }
        if (b->n) memcpy(p, b->p, b->n);
        dw_free(b->p); b->p = p; b->cap = cap;
    }
    b->n += n;
    return b->p + b->n - n;
}
void buf_put(Buf* b, const void* p, u32 n) { u8* d = buf_grow(b, n); if (d && n) memcpy(d, p, n); }
void buf_u8(Buf* b, u32 v) { u8* d = buf_grow(b, 1); if (d) d[0] = (u8)v; }
void buf_u16(Buf* b, u32 v) { u8* d = buf_grow(b, 2); if (d) { d[0] = (u8)(v >> 8); d[1] = (u8)v; } }
void buf_u32(Buf* b, u32 v) { u8* d = buf_grow(b, 4); if (d) { d[0] = (u8)(v >> 24); d[1] = (u8)(v >> 16); d[2] = (u8)(v >> 8); d[3] = (u8)v; } }
void buf_align(Buf* b, u32 base, u32 to) { while ((base + b->n) % to) buf_u8(b, 0); }
void buf_set_u32(Buf* b, u32 at, u32 v) { if (!b->failed) { b->p[at] = (u8)(v >> 24); b->p[at + 1] = (u8)(v >> 16); b->p[at + 2] = (u8)(v >> 8); b->p[at + 3] = (u8)v; } }

u16 rd_le16(const u8* p) { return (u16)(p[0] | p[1] << 8); }
u32 rd_le32(const u8* p) { return (u32)p[0] | (u32)p[1] << 8 | (u32)p[2] << 16 | (u32)p[3] << 24; }
u16 rd_be16(const u8* p) { return (u16)(p[0] << 8 | p[1]); }
u32 rd_be32(const u8* p) { return (u32)p[0] << 24 | (u32)p[1] << 16 | (u32)p[2] << 8 | p[3]; }
int dw_strcmp(const char* a, const char* b) { while (*a && *a == *b) ++a, ++b; return (u8)*a - (u8)*b; }
char* dw_strcpy(char* d, const char* s) { char* r = d; while ((*d++ = *s++)) continue; return r; }

void dw_path(char* out, u32 cap, const char* dir, const char* name)
{
    u32 n = 0;
    while (*dir && n + 1 < cap) out[n++] = *dir++;
#ifdef INSTALL_AMIGA
    if (n && out[n - 1] != ':' && out[n - 1] != '/' && n + 1 < cap) out[n++] = '/';
#else
    if (n && out[n - 1] != '/' && n + 1 < cap) out[n++] = '/';
#endif
    while (*name && n + 1 < cap) out[n++] = *name++;
    out[n] = 0;
}

int dw_write_file(const char* dir, const char* name, const void* data, u32 n)
{
    char path[512];
    File f;
    dw_path(path, sizeof path, dir, name);
    REQUIRE(io_open(&f, path, 1), "cannot create an output file");
    if (!io_write(&f, data, n)) { io_close(&f); return dw_fail("cannot write an output file (disk full?)"); }
    REQUIRE(io_close(&f), "cannot write an output file (disk full?)");
    return 1;
}

#include "win.h"

static const u8* page(Win* w, u32 number)
{
    u32 slot = number % w->slots;
    u8* p = w->pages + slot * WIN_PAGE;
    if (w->tags[slot] != number + 1) {
        u32 at = number * WIN_PAGE, n = w->size - at < WIN_PAGE ? w->size - at : WIN_PAGE;
        if (!io_seek(&w->f, at) || !io_read(&w->f, p, n)) { w->failed = 1; dw_fail("cannot read data.win"); w->tags[slot] = 0; memset(p, 0, WIN_PAGE); return p; }
        w->tags[slot] = number + 1;
    }
    return p;
}

int win_read(Win* w, u32 off, void* dst, u32 n)
{
    u8* d = dst;
    if (off > w->size || n > w->size - off) { w->failed = 1; memset(dst, 0, n); return dw_fail("data.win is damaged (read past its end)"); }
    while (n) {
        u32 in = off % WIN_PAGE, take = WIN_PAGE - in;
        if (take > n) take = n;
        memcpy(d, page(w, off / WIN_PAGE) + in, take);
        d += take; off += take; n -= take;
    }
    return !w->failed;
}

u32 win_u8(Win* w, u32 off) { u8 b[1]; win_read(w, off, b, 1); return b[0]; }
u32 win_u16(Win* w, u32 off) { u8 b[2]; win_read(w, off, b, 2); return rd_le16(b); }
u32 win_u32(Win* w, u32 off) { u8 b[4]; win_read(w, off, b, 4); return rd_le32(b); }
s32 win_s32(Win* w, u32 off) { return (s32)win_u32(w, off); }

int win_open(Win* w, const char* path, u32 cacheBytes)
{
    u8 head[8];
    u32 pos;
    memset(w, 0, sizeof *w);
    REQUIRE(io_open(&w->f, path, 0), "cannot open data.win");
    w->size = w->f.size;
    w->slots = cacheBytes / WIN_PAGE;
    if (w->slots < 4) w->slots = 4;
    w->pages = dw_alloc(w->slots * WIN_PAGE);
    w->tags = dw_zalloc(w->slots * 4);
    REQUIRE(w->pages && w->tags, "not enough memory for data.win's cache");
    TRY(win_read(w, 0, head, 8));
    REQUIRE(!memcmp(head, "FORM", 4) && rd_le32(head + 4) == w->size - 8, "data.win is not a GameMaker FORM");
    for (pos = 8; pos < w->size;) {
        WinChunk* c;
        TRY(win_read(w, pos, head, 8));
        REQUIRE(w->nchunks < 32, "data.win has too many chunks");
        c = &w->chunks[w->nchunks++];
        memcpy(c->id, head, 4);
        c->off = pos + 8;
        c->len = rd_le32(head + 4);
        REQUIRE(c->len <= w->size - c->off, "data.win is damaged (a chunk exceeds the file)");
        pos = c->off + c->len;
    }
    return 1;
}

void win_close(Win* w)
{
    if (w->f.handle) io_close(&w->f);
    dw_free(w->pages); dw_free(w->tags);
    w->pages = 0; w->tags = 0;
}

int win_chunk(Win* w, const char* id, u32* off, u32* len)
{
    int i;
    for (i = 0; i < w->nchunks; ++i)
        if (!memcmp(w->chunks[i].id, id, 4)) { *off = w->chunks[i].off; if (len) *len = w->chunks[i].len; return 1; }
    return dw_fail("data.win lacks a chunk the converter needs");
}

u32 win_strlen(Win* w, u32 ref) { return ref >= 4 ? win_u32(w, ref - 4) : 0; }

int win_string(Win* w, u32 ref, char* dst, u32 cap)
{
    u32 n = win_strlen(w, ref);
    if (n + 1 > cap) { dst[0] = 0; return dw_fail("data.win holds a longer string than expected"); }
    win_read(w, ref, dst, n);
    dst[n] = 0;
    return !w->failed;
}

int win_streq(Win* w, u32 ref, const char* s)
{
    char buf[256];
    u32 n = win_strlen(w, ref);
    if (n >= sizeof buf) return 0;
    win_read(w, ref, buf, n); buf[n] = 0;
    return !dw_strcmp(buf, s);
}

u32 win_list_count(Win* w, u32 off) { return win_u32(w, off); }
u32 win_list_item(Win* w, u32 off, u32 index) { return win_u32(w, off + 4 + 4 * index); }

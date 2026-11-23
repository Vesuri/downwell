/* Downwell.planes and Downwell.cavernplanes: tools/generate-amiga-plane-bank.py
   (DWPL v1). Each distinct bitplane stream of the BOB art once, longest
   first, found inside or overlapping what is already interned (2-byte
   aligned), then the source view's three plane offsets. */
#include "derived_parts.h"
#include "sha256.h"

/* tools/amiga_scene_contract.py SURFACE_ONLY_IDS: effect-surface art stays
   in the CPU pack. */
static const u16 surfaceOnly[] = {463, 62463, 470, 62470, 131, 526, 700, 701, 21, 22, 23};

typedef struct { u32 offset, plane; } Source;
typedef struct { u32 start, len, slot; } Stream; /* slot: source * 3 + plane */

static const u8* sortPack;

static int stream_before(const Stream* a, const Stream* b)
{
    int d;
    if (a->len != b->len) return a->len > b->len;
    d = memcmp(sortPack + a->start, sortPack + b->start, a->len);
    return d ? d < 0 : a->slot < b->slot;
}

static int source_before(const Source* a, const Source* b)
{
    return a->offset != b->offset ? a->offset < b->offset : a->plane < b->plane;
}

/* Bottom-up merge sorts (stable, no recursion). */
static void sort_streams(Stream* a, Stream* t, u32 n)
{
    u32 width, i;
    for (width = 1; width < n; width *= 2) {
        for (i = 0; i < n; i += 2 * width) {
            u32 l = i, m = i + width < n ? i + width : n, r = i + 2 * width < n ? i + 2 * width : n, k = i, j = m;
            while (l < m || j < r) t[k++] = j >= r || (l < m && !stream_before(&a[j], &a[l])) ? a[l++] : a[j++];
        }
        memcpy(a, t, n * sizeof *a);
    }
}

static void sort_sources(Source* a, Source* t, u32 n)
{
    u32 width, i;
    for (width = 1; width < n; width *= 2) {
        for (i = 0; i < n; i += 2 * width) {
            u32 l = i, m = i + width < n ? i + width : n, r = i + 2 * width < n ? i + 2 * width : n, k = i, j = m;
            while (l < m || j < r) t[k++] = j >= r || (l < m && !source_before(&a[j], &a[l])) ? a[l++] : a[j++];
        }
        memcpy(a, t, n * sizeof *a);
    }
}

/* Even payload positions (in words) by their first eight bytes, in ascending
   chains. A link is the u16 distance to the next position in its chain (0:
   none); the rare longer link is kept in a small overflow list. */
#define BUCKETS 8192u
#define NONE 0xffffffffu
#define FAR_MAX 1024u
typedef struct { u32 *head, *tail; u16* link; u32 indexed, nfar, far[FAR_MAX][2]; } Index;

static u32 key8(const u8* p)
{
    u32 h = rd_be32(p) * 2654435761u ^ rd_be32(p + 4) * 2246822519u;
    return (h ^ h >> 15) & (BUCKETS - 1);
}

static int index_grow(Index* x, const u8* payload, u32 n)
{
    for (; x->indexed + 8 <= n; x->indexed += 2) {
        u32 k = key8(payload + x->indexed), at = x->indexed / 2;
        x->link[at] = 0;
        if (x->head[k] == NONE) x->head[k] = at;
        else {
            u32 from = x->tail[k], gap = at - from;
            if (gap >= 0xffff) {
                REQUIRE(x->nfar < FAR_MAX, "a scene bank's planes do not intern");
                x->far[x->nfar][0] = from; x->far[x->nfar++][1] = at;
                gap = 0xffff;
            }
            x->link[from] = (u16)gap;
        }
        x->tail[k] = at;
    }
    return 1;
}

static u32 index_next(const Index* x, u32 at)
{
    u32 i;
    if (!x->link[at]) return NONE;
    if (x->link[at] < 0xffff) return at + x->link[at];
    for (i = 0; i < x->nfar; ++i) if (x->far[i][0] == at) return x->far[i][1];
    return NONE;
}

/* The first even offset of s in payload[0..n), or NONE. */
static u32 find_stream(const Index* x, const u8* payload, u32 n, const u8* s, u32 len)
{
    u32 at;
    if (len < 8) {
        for (at = 0; at + len <= n; at += 2)
            if (payload[at] == s[0] && !memcmp(payload + at, s, len)) return at;
        return NONE;
    }
    /* Key on the stream's first even window that is not blank (transparent
       rows make long chains): a match at p has that window at p + k. */
    {
        u32 k = 0, w;
        for (w = 0; w + 8 <= len; w += 2)
            if (rd_be32(s + w) | rd_be32(s + w + 4)) { k = w; break; }
        for (at = x->head[key8(s + k)]; at != NONE; at = index_next(x, at)) {
            u32 p = 2 * at;
            if (p >= k && p - k + len <= n && !memcmp(payload + p - k, s, len)) return p - k;
        }
    }
    return NONE;
}

int derived_plane_bank(Context* c, const Buf* pack, const char* name)
{
    u32 count = dwap_count(pack), nsources = 0, i, j, nstreams, unique = 0, start, n, *offsets = 0;
    Source *sources = 0, *stemp = 0;
    Stream *streams = 0, *temp = 0;
    Index* x = 0;
    u8 *bank = 0, *payload;
    SHA256 sha;
    int ok = 1;
    REQUIRE(count, "a scene bank is not a DWAP v2 bank");
    sources = dw_alloc(count * sizeof *sources);
    stemp = dw_alloc(count * sizeof *stemp);
    if (!sources || !stemp) ok = 0;
    for (i = 0; ok && i < count; ++i) {
        DwapRecord r;
        u32 plane, k;
        dwap_record(pack, i, &r);
        if (r.flags != 3) continue;
        for (k = 0; k < sizeof surfaceOnly / sizeof surfaceOnly[0] && surfaceOnly[k] != r.sprite; ++k) continue;
        if (k < sizeof surfaceOnly / sizeof surfaceOnly[0]) continue;
        plane = (u32)(r.stride / 8) * r.height;
        if (!plane || r.stride % 16 || r.size != plane * 3 || r.offset % 2 || r.offset > pack->n || r.size > pack->n - r.offset) {
            ok = dw_fail("a scene bank has an invalid planar image");
            break;
        }
        sources[nsources].offset = r.offset; sources[nsources++].plane = plane;
    }
    if (ok) {
        sort_sources(sources, stemp, nsources);
        for (i = j = 0; i < nsources; ++i)
            if (!j || sources[i].offset != sources[j - 1].offset || sources[i].plane != sources[j - 1].plane) sources[j++] = sources[i];
        nsources = j;
    }
    dw_free(stemp);
    nstreams = 3 * nsources;
    streams = ok ? dw_alloc(nstreams * sizeof *streams) : 0;
    temp = ok ? dw_alloc(nstreams * sizeof *temp) : 0;
    offsets = ok ? dw_alloc(nstreams * 4) : 0;
    if (!streams || !temp || !offsets) ok = 0;
    if (ok) {
        for (i = 0; i < nstreams; ++i) {
            streams[i].len = sources[i / 3].plane;
            streams[i].start = sources[i / 3].offset + (i % 3) * streams[i].len;
            streams[i].slot = i;
        }
        sortPack = pack->p;
        sort_streams(streams, temp, nstreams);
        /* The payload is at most the distinct streams laid end to end. */
        for (i = 0; i < nstreams; ++i)
            if (!i || streams[i].len != streams[i - 1].len ||
                memcmp(pack->p + streams[i].start, pack->p + streams[i - 1].start, streams[i].len))
                unique += streams[i].len;
    }
    dw_free(temp);
    start = 48 + nsources * 20;
    if (ok) {
        bank = dw_alloc(start + unique + 2);
        x = dw_zalloc(sizeof *x);
        if (x) { x->head = dw_alloc(BUCKETS * 4); x->tail = dw_alloc(BUCKETS * 4); x->link = dw_alloc(unique / 2 * 2 + 2); }
        if (!bank || !x || !x->head || !x->tail || !x->link) ok = 0;
    }
    if (ok) {
        payload = bank + start;
        n = 0;
        memset(x->head, 0xff, BUCKETS * 4);
        for (i = 0; i < nstreams; ++i) {
            const u8* s = pack->p + streams[i].start;
            u32 len = streams[i].len, at;
            if (i && len == streams[i - 1].len && !memcmp(s, pack->p + streams[i - 1].start, len)) {
                offsets[streams[i].slot] = offsets[streams[i - 1].slot];
                continue;
            }
            if ((i & 63) == 0 && io_cancelled()) { ok = dw_fail("cancelled"); break; }
            at = find_stream(x, payload, n, s, len);
            if (at == NONE) {
                u32 overlap = 0, size;
                for (size = (n < len ? n : len) / 2 * 2; size; size -= 2)
                    if (payload[n - size] == s[0] && !memcmp(payload + n - size, s, size)) { overlap = size; break; }
                at = n - overlap;
                memcpy(payload + n, s + overlap, len - overlap);
                n += len - overlap;
                if (!index_grow(x, payload, n)) { ok = 0; break; }
            }
            offsets[streams[i].slot] = start + at;
        }
    }
    if (ok) {
        u8* h = bank;
        memcpy(h, "DWPL", 4);
        h[4] = 0; h[5] = 1; h[6] = 0; h[7] = 48;
        h[8] = (u8)(nsources >> 24); h[9] = (u8)(nsources >> 16); h[10] = (u8)(nsources >> 8); h[11] = (u8)nsources;
        h[12] = (u8)(pack->n >> 24); h[13] = (u8)(pack->n >> 16); h[14] = (u8)(pack->n >> 8); h[15] = (u8)pack->n;
        sha_init(&sha); sha_update(&sha, pack->p, pack->n); sha_final(&sha, h + 16);
        for (i = 0; i < nsources; ++i) {
            u32 v[5], k;
            v[0] = sources[i].offset; v[1] = sources[i].plane;
            v[2] = offsets[3 * i]; v[3] = offsets[3 * i + 1]; v[4] = offsets[3 * i + 2];
            for (k = 0; k < 5; ++k) {
                u8* d = bank + 48 + 20 * i + 4 * k;
                d[0] = (u8)(v[k] >> 24); d[1] = (u8)(v[k] >> 16); d[2] = (u8)(v[k] >> 8); d[3] = (u8)v[k];
            }
        }
        bank[start + n] = bank[start + n + 1] = 0; /* the shifted fetch guard, as in DWAP */
        ok = dw_write_file(c->dest, name, bank, start + n + 2);
    }
    if (x) { dw_free(x->link); dw_free(x->tail); dw_free(x->head); dw_free(x); }
    dw_free(bank);
    dw_free(offsets); dw_free(streams); dw_free(sources);
    return ok;
}

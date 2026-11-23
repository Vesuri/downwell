#include "art_pack.h"

#define SLAB_BITS 16
#define SLAB (1u << SLAB_BITS)
#define NONE 0xffffffffu

void recs_init(Recs* rs) { memset(rs, 0, sizeof *rs); }
void recs_free(Recs* rs)
{
    u32 i;
    for (i = 0; i < rs->nslabs; ++i) dw_free(rs->slab[i]);
    dw_free(rs->r); dw_free(rs->s); dw_free(rs->buckets);
    recs_init(rs);
}

const u8* stream_data(const Recs* rs, u32 stream) { u32 h = rs->s[stream].at; return rs->slab[h >> SLAB_BITS] + (h & (SLAB - 1)); }

static int grow(void** p, u32* cap, u32 n, u32 size)
{
    u32 c;
    u8* q;
    if (n < *cap) return 1;
    c = *cap ? *cap * 2 : 256;
    q = dw_alloc(c * size);
    if (!q) return 0;
    if (*cap) memcpy(q, *p, *cap * size);
    dw_free(*p); *p = q; *cap = c;
    return 1;
}

static u32 hash_bytes(const u8* p, u32 n)
{
    u32 h = 2166136261u;
    while (n--) h = (h ^ *p++) * 16777619u;
    return h;
}

int rec_add(Recs* rs, const Rec* meta, const u8* data, u32 len)
{
    u32 h = hash_bytes(data, len), k;
    if (!rs->buckets) {
        rs->buckets = dw_alloc(4096 * 4);
        if (!rs->buckets) return 0;
        memset(rs->buckets, 0xff, 4096 * 4);
    }
    TRY(grow((void**)&rs->r, &rs->cap, rs->n, sizeof(Rec)));
    for (k = rs->buckets[h & 4095]; k != NONE; k = rs->s[k].next)
        if (rs->s[k].hash == h && rs->s[k].len == len && !memcmp(stream_data(rs, k), data, len)) break;
    if (k == NONE) {
        Stream* s;
        REQUIRE(len <= SLAB, "a scene art frame is larger than expected");
        TRY(grow((void**)&rs->s, &rs->scap, rs->ns, sizeof(Stream)));
        if (!rs->nslabs || rs->fill + len > SLAB) {
            REQUIRE(rs->nslabs < ARENA_SLABS, "the scene art needs more memory than expected");
            rs->slab[rs->nslabs] = dw_alloc(SLAB);
            if (!rs->slab[rs->nslabs]) return 0;
            ++rs->nslabs; rs->fill = 0;
        }
        k = rs->ns++;
        s = &rs->s[k];
        s->at = (rs->nslabs - 1) << SLAB_BITS | rs->fill;
        s->len = len; s->hash = h;
        s->next = rs->buckets[h & 4095]; rs->buckets[h & 4095] = k;
        if (len) memcpy(rs->slab[rs->nslabs - 1] + rs->fill, data, len);
        rs->fill += (len + 3) & ~3u;
    }
    rs->r[rs->n] = *meta;
    rs->r[rs->n].stream = k;
    ++rs->n;
    return 1;
}

/* A stable merge sort of u32 items. */
typedef int (*Less)(void* ctx, u32 a, u32 b);
static int sort_stable(u32* a, u32 n, Less less, void* ctx)
{
    u32 width, i, *t = dw_alloc(n * 4 + 4), *src = a, *dst = t;
    if (!t) return 0;
    for (width = 1; width < n; width *= 2) {
        for (i = 0; i < n; i += 2 * width) {
            u32 lo = i, mid = i + width < n ? i + width : n, hi = i + 2 * width < n ? i + 2 * width : n;
            u32 p = lo, q = mid, k = lo;
            while (p < mid && q < hi) dst[k++] = less(ctx, src[q], src[p]) ? src[q++] : src[p++];
            while (p < mid) dst[k++] = src[p++];
            while (q < hi) dst[k++] = src[q++];
        }
        { u32* x = src; src = dst; dst = x; }
    }
    if (src != a) memcpy(a, src, n * 4);
    dw_free(t);
    return 1;
}

/* Python's bytes.find, restricted to even positions: the first one. */
static u32 find_even(const u8* hay, u32 hn, const u8* needle, u32 nn)
{
    u32 p;
    if (nn > hn) return NONE;
    if (!nn) return 0;
    for (p = 0; p + nn <= hn; p += 2)
        if (hay[p] == needle[0] && !memcmp(hay + p, needle, nn)) return p;
    return NONE;
}

/* planar_overlap: the largest even s <= min(len) with left's last s bytes
   equal to right's first s. */
static u32 overlap(const u8* left, u32 ln, const u8* right, u32 rn)
{
    u32 limit = (ln < rn ? ln : rn) & ~1u, s;
    for (s = limit; s >= 2; s -= 2)
        if (left[ln - s] == right[0] && !memcmp(left + ln - s, right, s)) return s;
    return 0;
}

typedef struct { const Recs* rs; } Ctx;
static int longer_first(void* c, u32 a, u32 b)
{
    const Recs* rs = ((Ctx*)c)->rs;
    u32 la = rs->s[a].len, lb = rs->s[b].len;
    if (la != lb) return la > lb;
    return memcmp(stream_data(rs, a), stream_data(rs, b), la) < 0;
}

/* intern_planar into payload[0..*n): the stream's offset. */
static u32 intern(u8* payload, u32* n, const u8* data, u32 len)
{
    u32 at = find_even(payload, *n, data, len), ov;
    if (at != NONE) return at;
    ov = overlap(payload, *n, data, len);
    memcpy(payload + *n, data + ov, len - ov);
    *n += len - ov;
    return *n - len;
}

static u32 packed_size(const Recs* rs, const u32* order, u32 n, const u32* contained, u32 nc, u8* scratch)
{
    u32 size = 0, i;
    for (i = 0; i < n; ++i) intern(scratch, &size, stream_data(rs, order[i]), rs->s[order[i]].len);
    for (i = 0; i < nc; ++i) intern(scratch, &size, stream_data(rs, contained[i]), rs->s[contained[i]].len);
    return size;
}

typedef struct { u32 ov, i, j; } Edge;
static u32 root(u32* parent, u32 i)
{
    while (parent[i] != i) { parent[i] = parent[parent[i]]; i = parent[i]; }
    return i;
}

/* planar_payload_order: the unique streams (any order) become the interning
   order, written back into list[0..n). */
static int payload_order(const Recs* rs, u32* list, u32 n, u32 total)
{
    Ctx ctx;
    u32 *standalone = 0, *contained = 0, *ordered = 0, *paths = 0, *parent = 0, *before = 0, *after = 0;
    u32 ns = 0, nc = 0, no = 0, np = 0, i, j, plen = 0;
    u8* payload = 0;
    Edge* edges = 0;
    u32 nedges = 0, ecap = 0;
    int ok = 0;
    ctx.rs = rs;
    if (!sort_stable(list, n, longer_first, &ctx)) return 0;
    standalone = dw_alloc(4 * n + 4); contained = dw_alloc(4 * n + 4);
    ordered = dw_alloc(4 * n + 4); paths = dw_alloc(4 * n + 4);
    parent = dw_alloc(4 * n + 4); before = dw_alloc(4 * n + 4); after = dw_alloc(4 * n + 4);
    payload = dw_alloc(total + 4);
    if (!standalone || !contained || !ordered || !paths || !parent || !before || !after || !payload) goto done;
    for (i = 0; i < n; ++i) {
        const u8* d = stream_data(rs, list[i]);
        u32 len = rs->s[list[i]].len;
        for (j = 0; j < ns; ++j)
            if (find_even(stream_data(rs, standalone[j]), rs->s[standalone[j]].len, d, len) != NONE) break;
        if (j < ns) contained[nc++] = list[i]; else standalone[ns++] = list[i];
    }
    /* Greedy tail: the longest overlap with the payload so far, first wins. */
    {
        u32* left = dw_alloc(4 * ns + 4), nl = ns;
        if (!left) goto done;
        memcpy(left, standalone, 4 * ns);
        while (nl) {
            u32 best = 0, bestOv = 0, k;
            int any = 0;
            for (k = 0; k < nl; ++k) {
                u32 ov = overlap(payload, plen, stream_data(rs, left[k]), rs->s[left[k]].len);
                if (!any || ov > bestOv) { any = 1; bestOv = ov; best = k; }
            }
            memcpy(payload + plen, stream_data(rs, left[best]) + bestOv, rs->s[left[best]].len - bestOv);
            plen += rs->s[left[best]].len - bestOv;
            ordered[no++] = left[best];
            for (k = best; k + 1 < nl; ++k) left[k] = left[k + 1];
            --nl;
        }
        dw_free(left);
    }
    /* Disjoint overlap paths: edges by (-overlap, i, j). */
    for (i = 0; i < ns; ++i)
        for (j = 0; j < ns; ++j) {
            u32 ov;
            if (i == j) continue;
            ov = overlap(stream_data(rs, standalone[i]), rs->s[standalone[i]].len, stream_data(rs, standalone[j]), rs->s[standalone[j]].len);
            if (!ov) continue;
            if (!grow((void**)&edges, &ecap, nedges, sizeof(Edge))) goto done;
            edges[nedges].ov = ov; edges[nedges].i = i; edges[nedges].j = j; ++nedges;
        }
    {
        /* Counting sort by overlap, descending, stable in (i, j). */
        u32 maxov = 0, *count, *order, e;
        Edge* sorted;
        for (e = 0; e < nedges; ++e) if (edges[e].ov > maxov) maxov = edges[e].ov;
        count = dw_zalloc(4 * (maxov + 2));
        sorted = dw_alloc(sizeof(Edge) * nedges + 4);
        if (!count || !sorted) { dw_free(count); dw_free(sorted); goto done; }
        for (e = 0; e < nedges; ++e) count[maxov - edges[e].ov + 1]++;
        for (e = 1; e <= maxov + 1; ++e) count[e] += count[e - 1];
        for (e = 0; e < nedges; ++e) sorted[count[maxov - edges[e].ov]++] = edges[e];
        dw_free(count);
        (void)order;
        for (i = 0; i < ns; ++i) { parent[i] = i; before[i] = after[i] = NONE; }
        for (e = 0; e < nedges; ++e) {
            u32 a = sorted[e].i, b = sorted[e].j;
            if (after[a] == NONE && before[b] == NONE && root(parent, a) != root(parent, b)) {
                after[a] = b; before[b] = a;
                parent[root(parent, b)] = root(parent, a);
            }
        }
        dw_free(sorted);
    }
    for (i = 0; i < ns; ++i)
        if (before[i] == NONE)
            for (j = i; j != NONE; j = after[j]) paths[np++] = standalone[j];
    if (packed_size(rs, paths, np, contained, nc, payload) < packed_size(rs, ordered, no, contained, nc, payload))
        memcpy(ordered, paths, 4 * np);
    memcpy(list, ordered, 4 * no);
    memcpy(list + no, contained, 4 * nc);
    ok = 1;
done:
    dw_free(standalone); dw_free(contained); dw_free(ordered); dw_free(paths);
    dw_free(parent); dw_free(before); dw_free(after); dw_free(payload); dw_free(edges);
    return ok;
}

typedef struct { const Recs* rs; const u32* idx; } KeyCtx;
static int by_key(void* c, u32 a, u32 b)
{
    const KeyCtx* k = c;
    const Rec* x = &k->rs->r[k->idx[a]];
    const Rec* y = &k->rs->r[k->idx[b]];
    return x->id != y->id ? x->id < y->id : x->frame < y->frame;
}

static void put16(u8* p, u32 v) { p[0] = (u8)(v >> 8); p[1] = (u8)v; }
static void put32(u8* p, u32 v) { p[0] = (u8)(v >> 24); p[1] = (u8)(v >> 16); p[2] = (u8)(v >> 8); p[3] = (u8)v; }

int pack_bank(Recs* rs, const u32* idx, const u8* life, u32 n, const u8* sourcePrefix, Buf* out)
{
    KeyCtx kc;
    u32 *pos = dw_alloc(4 * n + 4), *offs = dw_alloc(4 * n + 4), *list = dw_alloc(4 * n + 4), *at = dw_alloc(4 * rs->ns + 4);
    u8* mark = dw_zalloc(rs->ns + 1);
    u32 bound = 0, head = 20 + 24 * n, base, k, g;
    u8* p;
    int ok = 0;
    buf_init(out);
    if (!pos || !offs || !list || !at || !mark) goto done;
    for (k = 0; k < n; ++k) { pos[k] = k; bound += rs->s[rs->r[idx[k]].stream].len; }
    kc.rs = rs; kc.idx = idx;
    if (!sort_stable(pos, n, by_key, &kc)) goto done;
    for (k = 1; k < n; ++k) {
        const Rec* x = &rs->r[idx[pos[k - 1]]];
        const Rec* y = &rs->r[idx[pos[k]]];
        if (x->id == y->id && x->frame == y->frame) { dw_fail("two scene art records share an ID and frame"); goto done; }
    }
    p = buf_grow(out, head + bound);
    if (!p) goto done;
    base = head;
    for (g = 0; g < 3; ++g) {
        u32 nl = 0, size = 0, cls;
        for (k = 0; k < n; ++k) {
            const Rec* r = &rs->r[idx[pos[k]]];
            if ((life ? life[pos[k]] : 0) != g || r->hw || mark[r->stream]) continue;
            mark[r->stream] = 1;
            list[nl++] = r->stream;
        }
        if (nl) {
            u32 total = 0;
            for (k = 0; k < nl; ++k) total += rs->s[list[k]].len;
            if (!payload_order(rs, list, nl, total)) goto done;
            for (k = 0; k < nl; ++k) at[list[k]] = intern(p + base, &size, stream_data(rs, list[k]), rs->s[list[k]].len);
            for (k = 0; k < nl; ++k) mark[list[k]] = 0;
        }
        for (k = 0; k < n; ++k) {
            const Rec* r = &rs->r[idx[pos[k]]];
            if ((life ? life[pos[k]] : 0) == g && !r->hw) offs[k] = base + at[r->stream];
        }
        for (cls = 1; cls <= 2; ++cls)
            for (k = 0; k < n; ++k) {
                const Rec* r = &rs->r[idx[pos[k]]];
                if ((life ? life[pos[k]] : 0) != g || !r->hw || (r->parts ? 2u : 1u) != cls) continue;
                offs[k] = base + size;
                memcpy(p + base + size, stream_data(rs, r->stream), rs->s[r->stream].len);
                size += rs->s[r->stream].len;
            }
        base += size;
    }
    out->n = base;
    memcpy(p, "DWAP", 4); put16(p + 4, 2); put16(p + 6, 20); put32(p + 8, n);
    memcpy(p + 12, sourcePrefix, 8);
    for (k = 0; k < n; ++k) {
        const Rec* r = &rs->r[idx[pos[k]]];
        u8* q = p + 20 + 24 * k;
        put16(q, r->id); put16(q + 2, r->frame); put16(q + 4, (u16)r->ox); put16(q + 6, (u16)r->oy);
        put16(q + 8, r->w); put16(q + 10, r->h); put16(q + 12, r->dma);
        put16(q + 14, r->hw ? 4u | (u32)r->parts << 8 : 3u);
        put32(q + 16, offs[k]); put32(q + 20, rs->s[r->stream].len);
    }
    ok = 1;
done:
    dw_free(pos); dw_free(offs); dw_free(list); dw_free(at); dw_free(mark);
    if (!ok) buf_free(out);
    return ok;
}

static int find_record(const Buf* pack, u32 id, u32* offset)
{
    u32 n, k;
    REQUIRE(pack->n >= 20, "a scene bank is damaged");
    n = rd_be32(pack->p + 8);
    for (k = 0; k < n; ++k) {
        const u8* q = pack->p + 20 + 24 * k;
        if (rd_be16(q) == id && rd_be16(q + 2) == 0) {
            REQUIRE(rd_be16(q + 8) == 160 && rd_be16(q + 10) == 80 && rd_be16(q + 12) == 160 && rd_be32(q + 20) >= 1600,
                    "unexpected transition edge layout");
            *offset = rd_be32(q + 16);
            return 1;
        }
    }
    return dw_fail("the front bank lacks a transition edge");
}

int exit_mask(const Buf* pack, u32 stride, u32 left, Buf* out)
{
    u32 offset, x, y;
    u8* o;
    TRY(find_record(pack, 65005, &offset));
    buf_init(out);
    o = buf_grow(out, stride * 648);
    if (!o) return 0;
    memset(o, 0, stride * 648);
    for (y = 0; y < 364; ++y)
        for (x = 0; x < 160; ++x)
            if (y < 284 || (pack->p[offset + (y - 284) * 20 + x / 8] & (0x80 >> (x & 7)))) {
                u32 px = left + x;
                o[y * stride + px / 8] |= (u8)(0x80 >> (px & 7));
            }
    return 1;
}

int entry_mask(const Buf* pack, Buf* out)
{
    u32 offset, x, y;
    u8* o;
    TRY(find_record(pack, 65004, &offset));
    buf_init(out);
    o = buf_grow(out, 48 * 648);
    if (!o) return 0;
    memset(o, 0, 48 * 648);
    for (y = 284; y < 648; ++y)
        for (x = 0; x < 160; ++x)
            if (y >= 364 || (pack->p[offset + (y - 284) * 20 + x / 8] & (0x80 >> (x & 7)))) {
                u32 px = 110 + x;
                o[y * 48 + px / 8] |= (u8)(0x80 >> (px & 7));
            }
    return 1;
}

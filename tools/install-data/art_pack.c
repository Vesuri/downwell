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

/* Python's bytes.find, restricted to even positions: the first one. Every
   planar stream and payload length is even, so all of this is word work. */
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
   equal to right's first s. KMP over right's prefix; the borders of the
   longest match are every shorter match, so follow them to an even one. */
static u32 overlap_kmp(const u8* left, u32 ln, const u8* right, u32 rn, u16* pi)
{
    u32 m = (ln < rn ? ln : rn) & ~1u, i, q;
    const u8* text = left + ln - m;
    if (m < 2) return 0;
    pi[0] = 0;
    for (i = 1, q = 0; i < m; ++i) {
        while (q && right[i] != right[q]) q = pi[q - 1];
        if (right[i] == right[q]) ++q;
        pi[i] = (u16)q;
    }
    for (i = 0, q = 0; i < m; ++i) {
        while (q && text[i] != right[q]) q = pi[q - 1];
        if (text[i] == right[q]) ++q;
    }
    while (q & 1) q = pi[q - 1];
    return q;
}

/* A stream (or the greedy payload's tail) indexed for overlap and
   containment queries: its nonzero positions chained by their four bytes,
   in ascending order, and the zero run before each position. A match of a
   right stream with z leading zeros puts the right's first nonzero byte at
   some q with at least z zeros before it. */
#define IXB 12
#define END16 0xffffu
typedef struct {
    const u8* d;
    u32 n, zt;   /* zt: trailing zeros */
    u16* next;
    u16* zrun;
    u16 head[1 << IXB], tail[1 << IXB];
} Ix;
static u32 kh(u32 key, int bits) { return (key * 2654435761u) >> (32 - bits); }

static void ix_build(Ix* x, const u8* d, u32 n)
{
    u32 q;
    x->d = d; x->n = n;
    memset(x->head, 0xff, sizeof x->head);
    x->zrun[0] = 0;
    for (q = 0; q < n; ++q) x->zrun[q + 1] = d[q] ? 0 : (u16)(x->zrun[q] + (x->zrun[q] < 0xffffu));
    x->zt = x->zrun[n];
    for (q = 0; q + 4 <= n; ++q)
        if (d[q]) {
            u32 b = kh(rd_be32(d + q), IXB);
            x->next[q] = END16;
            if (x->head[b] == END16) x->head[b] = (u16)q; else x->next[x->tail[b]] = (u16)q;
            x->tail[b] = (u16)q;
        }
}

/* Leading zeros z and the four bytes there: a stream's query signature. */
typedef struct { u32 z, key, o, okey; } Sig;  /* o, okey: first even run start (NONE: none) */

static u32 overlap_ix(const Ix* x, const u8* r, u32 rn, const Sig* g)
{
    u32 n = x->n, z = g->z, q, s;
    if (((n < rn ? n : rn) & ~1u) < 2) return 0;
    if (z < rn) {
        if (rn - z >= 4)
            for (q = x->head[kh(g->key, IXB)]; q != END16; q = x->next[q]) {
                if (q < z || ((q ^ z) & 1) || n - q > rn - z || x->zrun[q] < z) continue;
                if (rd_be32(x->d + q) != g->key || memcmp(x->d + q + 4, r + z + 4, n - q - 4)) continue;
                return n - q + z;
            }
        for (q = n > 3 ? n - 3 : 0; q < n; ++q) {
            if (q < z || ((q ^ z) & 1) || n - q > rn - z || x->zrun[q] < z) continue;
            if (!memcmp(x->d + q, r + z, n - q)) return n - q + z;
        }
    }
    /* Only zeros overlap. */
    s = z < x->zt ? z : x->zt;
    if (s > n) s = n;
    if (s > rn) s = rn;
    s &= ~1u;
    return s >= 2 ? s : 0;
}

/* Whether d occurs in the indexed stream at an even offset. */
static int contains_ix(const Ix* x, const u8* d, u32 dn, const Sig* g)
{
    u32 z = g->z, q;
    if (dn > x->n) return 0;
    if (z == dn || dn - z < 4) return find_even(x->d, x->n, d, dn) != NONE;
    for (q = x->head[kh(g->key, IXB)]; q != END16; q = x->next[q]) {
        if (q < z || ((q ^ z) & 1) || q + dn - z > x->n || x->zrun[q] < z) continue;
        if (rd_be32(x->d + q) == g->key && !memcmp(x->d + q + 4, d + z + 4, dn - z - 4)) return 1;
    }
    return 0;
}

static void signature(const u8* d, u32 n, Sig* g)
{
    u32 i;
    for (g->z = 0; g->z < n && !d[g->z]; ++g->z) continue;
    g->key = g->z + 4 <= n ? rd_be32(d + g->z) : 0;
    g->o = NONE; g->okey = 0;
    for (i = 2; i + 4 <= n; i += 2)
        if (d[i] && !d[i - 1]) { g->o = i; g->okey = rd_be32(d + i); break; }
}

/* The interning payload, its even run starts (a nonzero byte after a zero)
   chained by their four bytes in ascending order: an even occurrence of a
   stream puts the stream's first even run start on one of them. */
#define PXB 13
#define PX_CHUNK 4096          /* entries a chunk: position, next */
#define PX_CHUNKS 256
typedef struct {
    u8* p;
    u32 n, scanned, count;
    u32* chunk[PX_CHUNKS];
    u32 head[1 << PXB], tail[1 << PXB];
} Px;
static u32* px_entry(const Px* x, u32 e) { return x->chunk[e / PX_CHUNK] + 2 * (e % PX_CHUNK); }

static int px_update(Px* x)
{
    u32 q;
    for (q = x->scanned ? x->scanned : 2; q + 4 <= x->n; q += 2)
        if (x->p[q] && !x->p[q - 1]) {
            u32 b = kh(rd_be32(x->p + q), PXB), *e;
            if (!(x->count % PX_CHUNK)) {
                REQUIRE(x->count / PX_CHUNK < PX_CHUNKS, "a scene bank is larger than expected");
                x->chunk[x->count / PX_CHUNK] = dw_alloc(8 * PX_CHUNK);
                if (!x->chunk[x->count / PX_CHUNK]) return 0;
            }
            e = px_entry(x, x->count);
            e[0] = q; e[1] = NONE;
            if (x->head[b] == NONE) x->head[b] = x->count; else px_entry(x, x->tail[b])[1] = x->count;
            x->tail[b] = x->count++;
        }
    x->scanned = q;
    return 1;
}

static void px_free(Px* x)
{
    u32 i;
    for (i = 0; i < PX_CHUNKS; ++i) dw_free(x->chunk[i]);
}

static u32 px_find(const Px* x, const u8* d, u32 dn, const Sig* g)
{
    u32 e;
    if (g->o == NONE) return find_even(x->p, x->n, d, dn);
    for (e = x->head[kh(g->okey, PXB)]; e != NONE;) {
        const u32* f = px_entry(x, e);
        u32 q = f[0], p;
        e = f[1];
        if (q < g->o) continue;
        p = q - g->o;
        if (p + dn > x->n) break;
        if (rd_be32(x->p + q) == g->okey && !memcmp(x->p + p, d, dn)) return p;
    }
    return NONE;
}

/* intern_planar: the stream's offset in the payload. */
static int intern(Px* x, const u8* d, u32 dn, const Sig* g, u16* pi, u32* at)
{
    u32 ov;
    *at = px_find(x, d, dn, g);
    if (*at != NONE) return 1;
    ov = overlap_kmp(x->p, x->n, d, dn, pi);
    memcpy(x->p + x->n, d + ov, dn - ov);
    x->n += dn - ov;
    *at = x->n - dn;
    return px_update(x);
}

typedef struct {
    const Recs* rs;
    const Sig* sig;   /* by stream */
    u16* pi;
} Order;

/* Intern list[0..n) in order into payload; offsets by stream into at. */
static int intern_all(const Order* o, const u32* list, u32 n, u8* payload, u32* size, u32* at)
{
    Px* x = dw_alloc(sizeof *x);
    u32 i, off;
    int ok = 1;
    if (!x) return 0;
    memset(x, 0, sizeof *x);
    memset(x->head, 0xff, sizeof x->head);
    x->p = payload;
    for (i = 0; i < n && ok; ++i) {
        ok = intern(x, stream_data(o->rs, list[i]), o->rs->s[list[i]].len, &o->sig[list[i]], o->pi, &off);
        if (at) at[list[i]] = off;
    }
    *size = x->n;
    px_free(x); dw_free(x);
    return ok;
}

typedef struct { const Recs* rs; } Ctx;
static int longer_first(void* c, u32 a, u32 b)
{
    const Recs* rs = ((Ctx*)c)->rs;
    u32 la = rs->s[a].len, lb = rs->s[b].len;
    if (la != lb) return la > lb;
    return memcmp(stream_data(rs, a), stream_data(rs, b), la) < 0;
}

static u32 root(u32* parent, u32 i)
{
    while (parent[i] != i) { parent[i] = parent[parent[i]]; i = parent[i]; }
    return i;
}

/* The disjoint overlap paths. Python sorts every nonzero (-overlap, i, j)
   edge; here each i keeps only its best few successors (recomputed when
   used up) in a heap ordered the same way, so the same edges are visited
   in the same order without holding them all. */
#define TOPK 16
typedef struct {
    const Order* o;
    const u32* sa;   /* standalone streams */
    u32 n;
    Ix* ix;
    u32 *cand, *cnt, *cur, *before, *after, *parent;
    u32 *heapA, *heapB, nheap;  /* A: (65535 - overlap) << 16 | i, B: j */
} Paths;

static void row(Paths* p, u32 i, u32 afterKey)
{
    const Recs* rs = p->o->rs;
    u32 j, *c = p->cand + i * TOPK, n = 0;
    ix_build(p->ix, stream_data(rs, p->sa[i]), rs->s[p->sa[i]].len);
    for (j = 0; j < p->n; ++j) {
        u32 ov, key, k;
        if (j == i || p->before[j] != NONE) continue;
        ov = overlap_ix(p->ix, stream_data(rs, p->sa[j]), rs->s[p->sa[j]].len, &p->o->sig[p->sa[j]]);
        if (!ov) continue;
        key = (65535 - ov) << 16 | j;
        if (afterKey != NONE && key <= afterKey) continue;
        if (n == TOPK && key >= c[TOPK - 1]) continue;
        k = n < TOPK ? n++ : TOPK - 1;
        while (k && c[k - 1] > key) { c[k] = c[k - 1]; --k; }
        c[k] = key;
    }
    p->cnt[i] = n; p->cur[i] = 0;
}

static int heap_less(const Paths* p, u32 a, u32 b) { return p->heapA[a] != p->heapA[b] ? p->heapA[a] < p->heapA[b] : p->heapB[a] < p->heapB[b]; }
static void heap_swap(Paths* p, u32 a, u32 b) { u32 t = p->heapA[a]; p->heapA[a] = p->heapA[b]; p->heapA[b] = t; t = p->heapB[a]; p->heapB[a] = p->heapB[b]; p->heapB[b] = t; }
static void heap_push(Paths* p, u32 i)
{
    u32 key = p->cand[i * TOPK + p->cur[i]], k = p->nheap++;
    p->heapA[k] = (key & 0xffff0000u) | i; p->heapB[k] = key & 0xffff;
    while (k && heap_less(p, k, (k - 1) / 2)) { heap_swap(p, k, (k - 1) / 2); k = (k - 1) / 2; }
}
static void heap_pop(Paths* p)
{
    u32 k = 0;
    heap_swap(p, 0, --p->nheap);
    for (;;) {
        u32 l = 2 * k + 1, r = l + 1, m = k;
        if (l < p->nheap && heap_less(p, l, m)) m = l;
        if (r < p->nheap && heap_less(p, r, m)) m = r;
        if (m == k) break;
        heap_swap(p, k, m); k = m;
    }
}

static int overlap_paths(const Order* o, const u32* sa, u32 n, Ix* ix, u32* out)
{
    Paths p;
    u32 i, j, np = 0;
    int ok = 0;
    p.o = o; p.sa = sa; p.n = n; p.ix = ix; p.nheap = 0;
    p.cand = dw_alloc(4 * TOPK * n + 4); p.cnt = dw_alloc(4 * n + 4); p.cur = dw_alloc(4 * n + 4);
    p.before = dw_alloc(4 * n + 4); p.after = dw_alloc(4 * n + 4); p.parent = dw_alloc(4 * n + 4);
    p.heapA = dw_alloc(4 * n + 4); p.heapB = dw_alloc(4 * n + 4);
    if (!p.cand || !p.cnt || !p.cur || !p.before || !p.after || !p.parent || !p.heapA || !p.heapB) goto done;
    for (i = 0; i < n; ++i) { p.parent[i] = i; p.before[i] = p.after[i] = NONE; }
    for (i = 0; i < n; ++i) {
        if (io_cancelled()) { dw_fail("cancelled"); goto done; }
        row(&p, i, NONE);
        if (p.cnt[i]) heap_push(&p, i);
    }
    while (p.nheap) {
        u32 a = p.heapA[0] & 0xffff, b = p.heapB[0];
        u32 key = p.cand[a * TOPK + p.cur[a]];
        heap_pop(&p);
        if (p.after[a] == NONE && p.before[b] == NONE && root(p.parent, a) != root(p.parent, b)) {
            p.after[a] = b; p.before[b] = a;
            p.parent[root(p.parent, b)] = root(p.parent, a);
            continue;
        }
        if (p.after[a] != NONE) continue;
        if (++p.cur[a] == p.cnt[a]) {
            if (p.cnt[a] < TOPK) continue;
            row(&p, a, key);
            if (!p.cnt[a]) continue;
        }
        heap_push(&p, a);
    }
    for (i = 0; i < n; ++i)
        if (p.before[i] == NONE)
            for (j = i; j != NONE; j = p.after[j]) out[np++] = sa[j];
    ok = np == n || dw_fail("overlap paths lost a stream");
done:
    dw_free(p.cand); dw_free(p.cnt); dw_free(p.cur); dw_free(p.before); dw_free(p.after);
    dw_free(p.parent); dw_free(p.heapA); dw_free(p.heapB);
    return ok;
}

/* planar_payload_order: list[0..n) (unique streams) becomes the interning
   order; payload is scratch of at least the streams' total length. */
static int payload_order(const Order* o, u32* list, u32 n, u8* payload)
{
    const Recs* rs = o->rs;
    Ctx ctx;
    Ix* ix = 0;
    u32 *standalone = 0, *contained = 0, *ordered = 0, *paths = 0, *left = 0;
    u8 *flag = 0, *tail = 0;
    u32 ns = 0, nc = 0, no = 0, i, j, maxlen, sizeO, sizeP;
    int ok = 0;
    ctx.rs = rs;
    if (!sort_stable(list, n, longer_first, &ctx)) return 0;
    maxlen = rs->s[list[0]].len;
    ix = dw_alloc(sizeof *ix);
    standalone = dw_alloc(4 * n + 4); contained = dw_alloc(4 * n + 4);
    ordered = dw_alloc(4 * n + 4); paths = dw_alloc(4 * n + 4); left = dw_alloc(4 * n + 4);
    flag = dw_zalloc(n + 1); tail = dw_alloc(2 * maxlen + 4);
    if (!ix || !standalone || !contained || !ordered || !paths || !left || !flag || !tail) goto done;
    ix->next = dw_alloc(2 * maxlen + 2); ix->zrun = dw_alloc(2 * maxlen + 4);
    if (!ix->next || !ix->zrun) goto done;
    /* Contained: at an even offset of a longer standalone stream. */
    for (i = 0; i < n; ++i) {
        u32 li = rs->s[list[i]].len;
        if (flag[i]) continue;
        if (io_cancelled()) { dw_fail("cancelled"); goto done; }
        ix_build(ix, stream_data(rs, list[i]), li);
        for (j = i + 1; j < n; ++j)
            if (!flag[j] && rs->s[list[j]].len < li &&
                contains_ix(ix, stream_data(rs, list[j]), rs->s[list[j]].len, &o->sig[list[j]])) flag[j] = 1;
    }
    for (i = 0; i < n; ++i) { if (flag[i]) contained[nc++] = list[i]; else standalone[ns++] = list[i]; }
    /* Greedy tail: the longest overlap with the payload so far, first
       wins; only the payload's last bytes matter, kept in `tail`. */
    {
        u32 nl = ns, plen = 0, tlen = 0;
        memcpy(left, standalone, 4 * ns);
        while (nl) {
            u32 best = 0, bestOv = 0, k, w = 0, piece;
            const u8* d;
            if (io_cancelled()) { dw_fail("cancelled"); goto done; }
            for (k = 0; k < nl; ++k) if (rs->s[left[k]].len > w) w = rs->s[left[k]].len;
            if (w > plen) w = plen;
            ix_build(ix, tail + tlen - w, w);
            for (k = 0; k < nl; ++k) {
                u32 ov = overlap_ix(ix, stream_data(rs, left[k]), rs->s[left[k]].len, &o->sig[left[k]]);
                if (!k || ov > bestOv) { bestOv = ov; best = k; }
            }
            d = stream_data(rs, left[best]);
            piece = rs->s[left[best]].len - bestOv;
            if (tlen + piece > 2 * maxlen) {
                u32 drop = tlen - maxlen;
                for (k = 0; k < maxlen; ++k) tail[k] = tail[k + drop];
                tlen = maxlen;
            }
            memcpy(tail + tlen, d + bestOv, piece);
            tlen += piece; plen += piece;
            ordered[no++] = left[best];
            for (k = best; k + 1 < nl; ++k) left[k] = left[k + 1];
            --nl;
        }
    }
    if (!overlap_paths(o, standalone, ns, ix, paths)) goto done;
    memcpy(ordered + no, contained, 4 * nc);
    memcpy(paths + ns, contained, 4 * nc);
    if (!intern_all(o, paths, n, payload, &sizeP, 0) || !intern_all(o, ordered, n, payload, &sizeO, 0)) goto done;
    memcpy(list, sizeP < sizeO ? paths : ordered, 4 * n);
    ok = 1;
done:
    if (ix) { dw_free(ix->next); dw_free(ix->zrun); }
    dw_free(ix); dw_free(standalone); dw_free(contained); dw_free(ordered); dw_free(paths);
    dw_free(left); dw_free(flag); dw_free(tail);
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
    Sig* sig = dw_alloc(sizeof(Sig) * rs->ns + 4);
    u32 bound = 0, head = 20 + 24 * n, base, k, g, maxlen = 2;
    u8* p;
    int ok = 0;
    Order o;
    buf_init(out);
    o.rs = rs; o.sig = sig; o.pi = 0;
    if (!pos || !offs || !list || !at || !mark || !sig) goto done;
    /* The payload is at most every hardware stream and each other stream once. */
    for (k = 0; k < n; ++k) {
        const Rec* r = &rs->r[idx[k]];
        const Stream* s = &rs->s[r->stream];
        pos[k] = k;
        if (r->hw || !mark[r->stream]) bound += s->len;
        if (!r->hw) mark[r->stream] = 1;
        if (s->len > maxlen) maxlen = s->len;
        if (!r->hw && (s->len & 1)) { dw_fail("a scene art stream has an odd length"); goto done; }
    }
    memset(mark, 0, rs->ns);
    for (k = 0; k < rs->ns; ++k) signature(stream_data(rs, k), rs->s[k].len, &sig[k]);
    o.pi = dw_alloc(2 * maxlen);
    if (!o.pi) goto done;
    kc.rs = rs; kc.idx = idx;
    if (!sort_stable(pos, n, by_key, &kc)) goto done;
    for (k = 1; k < n; ++k) {
        const Rec* x = &rs->r[idx[pos[k - 1]]];
        const Rec* y = &rs->r[idx[pos[k]]];
        if (x->id == y->id && x->frame == y->frame) { dw_fail("two scene art records share an ID and frame"); goto done; }
    }
    p = dw_alloc(head + bound);
    if (!p) goto done;
    out->p = p; out->cap = head + bound;
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
            if (!payload_order(&o, list, nl, p + base) || !intern_all(&o, list, nl, p + base, &size, at)) goto done;
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
    dw_free(pos); dw_free(offs); dw_free(list); dw_free(at); dw_free(mark); dw_free(sig); dw_free((void*)o.pi);
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

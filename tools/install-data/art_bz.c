/* bzip2 decoding, written for the converter: Huffman-coded MTF/RLE2
   symbols, the inverse BWT kept in 20-bit entries (16 + 4 bits), then the
   initial run-length coding undone while the caller reads. data.win is
   SHA-256 checked, so the block CRCs are not recomputed. */
#include "art_bz.h"

#define MAX_GROUPS 6
#define MAX_ALPHA 258
#define MAX_SELECTORS 18002
#define MAX_LEN 20
#define TABLE_BITS 10

static int refill(Bz* z)
{
    if (z->inAt == z->inHave) {
        u32 n = z->inEnd - z->inPos;
        if (!n) return 0;
        if (n > sizeof z->in) n = sizeof z->in;
        if (!win_read(z->win, z->inPos, z->in, n)) return 0;
        z->inPos += n; z->inAt = 0; z->inHave = n;
    }
    return 1;
}

/* At least n (<= 24) bits in the buffer; zeros past the end of the input. */
static void need(Bz* z, int n)
{
    while (z->nbits < n) {
        u32 b = 0;
        if (refill(z)) b = z->in[z->inAt++];
        z->bits = z->bits << 8 | b;
        z->nbits += 8;
    }
}
static u32 peek(Bz* z, int n) { need(z, n); return (z->bits >> (z->nbits - n)) & ((1u << n) - 1); }
static u32 take(Bz* z, int n) { u32 v = peek(z, n); z->nbits -= n; return v; }

typedef struct {
    u16 table[1 << TABLE_BITS];  /* symbol | length << 9, 0 for longer codes */
    u32 first[MAX_LEN + 2], count[MAX_LEN + 2], base[MAX_LEN + 2];
    u16 perm[MAX_ALPHA];
} Code;

/* Canonical codes: by length, then by symbol. */
static int code_build(Code* c, const u8* len, int n)
{
    u32 code = 0, at = 0;
    int l, s;
    memset(c->count, 0, sizeof c->count);
    memset(c->table, 0, sizeof c->table);
    for (s = 0; s < n; ++s) c->count[len[s]]++;
    for (l = 1; l <= MAX_LEN; ++l) {
        c->first[l] = code; c->base[l] = at;
        for (s = 0; s < n; ++s)
            if (len[s] == l) {
                c->perm[at++] = (u16)s;
                if (l <= TABLE_BITS) {
                    u32 from = code << (TABLE_BITS - l), k;
                    for (k = 0; k < 1u << (TABLE_BITS - l); ++k) c->table[from + k] = (u16)(s | l << 9);
                }
                ++code;
            }
        REQUIRE(code <= 1u << l, "a texture page is damaged (bad Huffman lengths)");
        code <<= 1;
    }
    return 1;
}

static int decode_symbol(Bz* z, const Code* c)
{
    u32 v = peek(z, TABLE_BITS), e = c->table[v];
    int l;
    if (e) { z->nbits -= (int)(e >> 9); return (int)(e & 511); }
    z->nbits -= TABLE_BITS;
    for (l = TABLE_BITS + 1; l <= MAX_LEN; ++l) {
        v = v << 1 | take(z, 1);
        if (v - c->first[l] < c->count[l]) return c->perm[c->base[l] + v - c->first[l]];
    }
    return -1;
}

static void set_ll(Bz* z, u32 i, u32 v)
{
    u8* p = &z->ll4[i >> 1];
    z->ll16[i] = (u16)v;
    if (i & 1) *p = (u8)((*p & 0x0f) | (v >> 16) << 4);
    else *p = (u8)((*p & 0xf0) | (v >> 16));
}
static u32 get_ll(const Bz* z, u32 i) { return z->ll16[i] | (u32)((z->ll4[i >> 1] >> ((i & 1) << 2)) & 15) << 16; }

static int read_block(Bz* z)
{
    static Code codes[MAX_GROUPS];
    u8 lens[MAX_ALPHA], used[256], mtf[256], order[MAX_GROUPS];
    u32 counts[256], magicHi, magicLo, origPtr, nsel, kept, i, j, nblock = 0, run = 0;
    int nused = 0, alpha, ngroups, g, left = 0, shift = 0, sel = 0, group = 0;
    magicHi = take(z, 24); magicLo = take(z, 24);
    if (magicHi == 0x177245 && magicLo == 0x385090) { z->ended = 1; return 1; }
    REQUIRE(magicHi == 0x314159 && magicLo == 0x265359, "a texture page is damaged (no bzip2 block)");
    take(z, 16); take(z, 16);  /* block CRC */
    REQUIRE(!take(z, 1), "a texture page uses randomised bzip2 blocks");
    origPtr = take(z, 24);
    {
        u32 groups16 = take(z, 16);
        for (i = 0; i < 16; ++i)
            if (groups16 & (0x8000u >> i)) {
                u32 bits = take(z, 16);
                for (j = 0; j < 16; ++j) if (bits & (0x8000u >> j)) used[nused++] = (u8)(i * 16 + j);
            }
    }
    REQUIRE(nused > 0, "a texture page is damaged (empty bzip2 block)");
    alpha = nused + 2;
    ngroups = (int)take(z, 3);
    REQUIRE(ngroups >= 2 && ngroups <= MAX_GROUPS, "a texture page is damaged (bzip2 groups)");
    nsel = take(z, 15);
    REQUIRE(nsel >= 1, "a texture page is damaged (bzip2 selectors)");
    for (g = 0; g < ngroups; ++g) order[g] = (u8)g;
    kept = nsel < MAX_SELECTORS ? nsel : MAX_SELECTORS;
    for (i = 0; i < nsel; ++i) {
        int k = 0;
        u8 t;
        while (take(z, 1)) { ++k; REQUIRE(k < ngroups, "a texture page is damaged (bzip2 selector)"); }
        t = order[k];
        for (; k > 0; --k) order[k] = order[k - 1];
        order[0] = t;
        if (i < kept) z->selectors[i] = t;
    }
    for (g = 0; g < ngroups; ++g) {
        int s, l = (int)take(z, 5);
        for (s = 0; s < alpha; ++s) {
            for (;;) {
                REQUIRE(l >= 1 && l <= MAX_LEN, "a texture page is damaged (bzip2 code length)");
                if (!take(z, 1)) break;
                l += take(z, 1) ? -1 : 1;
            }
            lens[s] = (u8)l;
        }
        TRY(code_build(&codes[g], lens, alpha));
    }
    for (i = 0; i < (u32)nused; ++i) mtf[i] = used[i];
    memset(counts, 0, sizeof counts);
    for (;;) {
        int s;
        if (!left) {
            REQUIRE((u32)sel < kept, "a texture page is damaged (bzip2 selectors run out)");
            group = z->selectors[sel++]; left = 50;
        }
        --left;
        s = decode_symbol(z, &codes[group]);
        REQUIRE(s >= 0, "a texture page is damaged (bzip2 code)");
        if (s <= 1) {
            REQUIRE(shift < 21, "a texture page is damaged (bzip2 run)");
            run += (u32)(s + 1) << shift++;
            continue;
        }
        if (run) {
            u8 b = mtf[0];
            REQUIRE(run <= z->blockMax - nblock, "a texture page is damaged (bzip2 block overflow)");
            counts[b] += run;
            while (run--) z->ll16[nblock++] = b;
            run = 0; shift = 0;
        }
        if (s == alpha - 1) break;
        {
            u32 v = (u32)s - 1;
            u8 b = mtf[v];
            REQUIRE(v < (u32)nused && nblock < z->blockMax, "a texture page is damaged (bzip2 symbol)");
            for (; v > 0; --v) mtf[v] = mtf[v - 1];
            mtf[0] = b;
            counts[b]++;
            z->ll16[nblock++] = b;
        }
    }
    REQUIRE(origPtr < nblock, "a texture page is damaged (bzip2 origin)");
    /* T: each byte's position in the sorted column, then reversed in place
       (it is one cycle) so that walking it yields the bytes in order. */
    {
        u32 next[256];
        z->cftab[0] = 0;
        for (i = 0; i < 256; ++i) { next[i] = z->cftab[i]; z->cftab[i + 1] = z->cftab[i] + counts[i]; }
        for (i = 0; i < nblock; ++i) { u32 b = z->ll16[i]; set_ll(z, i, next[b]++); }
        i = origPtr; j = get_ll(z, i);
        do { u32 t = get_ll(z, j); set_ll(z, j, i); i = j; j = t; } while (i != origPtr);
    }
    z->nblock = nblock; z->used = 0; z->tPos = origPtr;
    z->runLength = 0; z->repeat = 0;
    return 1;
}

int bz_open(Bz* z, Win* w, u32 off, u32 len)
{
    u32 level;
    memset(z, 0, sizeof *z);
    z->win = w; z->inPos = off; z->inEnd = off + len;
    REQUIRE(take(z, 8) == 'B' && take(z, 8) == 'Z' && take(z, 8) == 'h', "a texture page is not bzip2");
    level = take(z, 8) - '0';
    REQUIRE(level >= 1 && level <= 9, "a texture page is not bzip2");
    z->blockMax = level * 100000;
    z->ll16 = dw_alloc(z->blockMax * 2);
    z->ll4 = dw_alloc(z->blockMax / 2 + 1);
    z->selectors = dw_alloc(MAX_SELECTORS);
    if (!z->ll16 || !z->ll4 || !z->selectors) { bz_close(z); return 0; }
    return 1;
}

void bz_close(Bz* z)
{
    dw_free(z->ll16); dw_free(z->ll4); dw_free(z->selectors);
    z->ll16 = 0; z->ll4 = 0; z->selectors = 0;
}

int bz_read(Bz* z, u8* dst, u32 n, u32* got)
{
    u32 k = 0;
    while (k < n) {
        if (z->repeat) { dst[k++] = z->last; --z->repeat; continue; }
        if (z->used == z->nblock) {
            if (z->ended) break;
            TRY(read_block(z));
            if (z->ended) break;
            continue;
        }
        {
            /* The F column entry of tPos, found in the cumulative counts. */
            u32 t = z->tPos, lo = 0, hi = 256;
            u8 b;
            while (hi - lo > 1) { u32 mid = (lo + hi) >> 1; if (t >= z->cftab[mid]) lo = mid; else hi = mid; }
            b = (u8)lo;
            z->tPos = get_ll(z, t);
            ++z->used;
            if (z->runLength == 4) { z->repeat = b; z->runLength = 0; continue; }
            if (z->runLength && b == z->last) ++z->runLength;
            else { z->last = b; z->runLength = 1; }
            dst[k++] = b;
        }
    }
    *got = k;
    return !z->win->failed;
}

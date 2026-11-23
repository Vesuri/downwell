/* The HUD's prepared tables from Downwell.hud (and Downwell.game): AMDV
   (tools/generate-ammo-dividers.py), AMFA (tools/probe-ammo-complete.py
   --factored), HART (tools/generate-amiga-health-art.py) and HUDT
   (tools/generate-amiga-hud-text.py), as tools/install-table-headers.py
   lays their tables out. The binary64 divider positions come from the
   game's own integer HudReal; the art from the game's own renderer. */
#ifdef INSTALL_AMIGA
#define _STRING_H_ /* the NDK's string.h lacks C linkage; io.h declares memcpy and memset */
#else
#include <stddef.h>
#include <string.h>
#endif
#include "derived_parts.h"
#include "platform/amiga/AmigaHudText.h"
#include "core/AmmoDividers.h"
#include "../../src/platform/amiga/AmigaRenderer.c"

/* The renderer's blitter hooks: the converter always takes the CPU paths. */
int amigaBlitterDrawPlanarMasked(const UBYTE*, const UBYTE*, UWORD, UWORD, UWORD, UBYTE*, UWORD, UWORD, UWORD,
                                 UWORD, UWORD, UWORD, UWORD, int) { return 0; }
int amigaBlitterClearPlanar(UBYTE*, UWORD, UWORD) { return 0; }
int amigaBlitterSolidRect(UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD) { return 0; }

using downwell::HudReal;

namespace {
struct Pair { u16 lo, hi; };

/* The last record of (sprite, frame), as the generators' dictionaries keep it. */
bool findRecord(const Buf* pack, unsigned sprite, unsigned frame, DwapRecord* out)
{
    const u32 n = dwap_count(pack);
    bool found = false;
    for (u32 i = 0; i < n; ++i) {
        DwapRecord r;
        dwap_record(pack, i, &r);
        if (r.sprite == sprite && r.frame == frame) { *out = r; found = true; }
    }
    return found && out->offset <= pack->n && (u32)out->stride / 8 * out->height * 2 <= pack->n - out->offset;
}

int finishSection(Context* c, Section* s, const char* tag)
{
    Buf* out = tables_add(&c->tables, tag);
    const int ok = out && sec_finish(s, out);
    sec_free(s);
    return ok;
}

/* math.ceil of a nonnegative binary64. */
unsigned ceiling(HudReal r)
{
    const unsigned f = r.floor();
    return HudReal::integer(f).equal(r) ? f : f + 1;
}

}

/* ---- AMDV ---- */
extern "C" int derived_ammo_dividers(Context* c, const Buf* hud)
{
    Pair stamps[2][8];
    for (unsigned frame = 0; frame < 2; ++frame) {
        DwapRecord r;
        REQUIRE(findRecord(hud, 421, frame, &r) && r.ox == 0 && r.oy == 0 && r.width == 12 && r.height == 8 &&
                r.stride == 16 && r.offset + 32 <= hud->n, "Downwell.hud: unexpected ammo divider");
        for (unsigned y = 0; y < 8; ++y) {
            stamps[frame][y].lo = rd_be16(hud->p + r.offset + 2 * y);
            stamps[frame][y].hi = rd_be16(hud->p + r.offset + 16 + 2 * y);
        }
    }
    Section sec;
    sec_init(&sec);
    Buf* tStamps = sec_table(&sec);
    Buf* tBlocks = sec_table(&sec);
    Buf* tImages = sec_table(&sec);
    for (unsigned f = 0; f < 2; ++f)
        for (unsigned y = 0; y < 8; ++y) { buf_u16(tStamps, stamps[f][y].lo); buf_u16(tStamps, stamps[f][y].hi); }
    Pair* column = static_cast<Pair*>(dw_alloc(288 * sizeof(Pair)));
    Pair* blocks = static_cast<Pair*>(dw_zalloc(256 * 8 * sizeof(Pair)));
    unsigned nblocks = 1;
    int ok = column && blocks;
    for (unsigned capacity = 0; ok && capacity <= 16; ++capacity)
        for (unsigned ammo = 0; ok && ammo <= capacity; ++ammo) {
            memset(column, 0, 288 * sizeof(Pair));
            downwell::drawAmmoDividers(ammo, capacity, 1, 0, 229, [&](unsigned, unsigned frame, int, unsigned y) {
                for (unsigned row = 0; row < 8; ++row) {
                    if (y + row >= 284) continue;
                    const unsigned mask = (stamps[frame][row].lo | stamps[frame][row].hi) & 0xfff0u;
                    Pair& p = column[y + row];
                    p.lo = (u16)((p.lo & ~mask) | (stamps[frame][row].lo & mask));
                    p.hi = (u16)((p.hi & ~mask) | (stamps[frame][row].hi & mask));
                }
            });
            for (unsigned first = 0; first < 288; first += 8) {
                unsigned b = 0;
                while (b < nblocks && memcmp(blocks + 8 * b, column + first, 8 * sizeof(Pair))) ++b;
                if (b == nblocks) {
                    if (nblocks == 256) { ok = dw_fail("ammo divider blocks no longer fit byte indices"); break; }
                    memcpy(blocks + 8 * nblocks++, column + first, 8 * sizeof(Pair));
                }
                buf_u8(tImages, b);
            }
        }
    for (unsigned b = 0; ok && b < nblocks; ++b)
        for (unsigned y = 0; y < 8; ++y) { buf_u16(tBlocks, blocks[8 * b + y].lo); buf_u16(tBlocks, blocks[8 * b + y].hi); }
    dw_free(column);
    dw_free(blocks);
    if (!ok) { sec_free(&sec); return 0; }
    return finishSection(c, &sec, "AMDV");
}

/* ---- AMFA ---- */
namespace {
enum { Rows = 284 };
typedef u16 Row6[6]; /* plane words for x = 272..319: low 0..2, high 3..5 */

/* stamp(): the record's pixels at (x, y), clipped to x 282..319, y 0..rows. */
void stampRows(const Buf* pack, const DwapRecord& r, int x, int y, Row6* out, int rows)
{
    const unsigned bytes = r.stride / 8, plane = bytes * r.height;
    memset(out, 0, sizeof(Row6) * rows);
    for (unsigned sy = 0; sy < r.height; ++sy)
        for (unsigned sx = 0; sx < r.width; ++sx) {
            const u8* at = pack->p + r.offset + sy * bytes + sx / 8;
            const unsigned bit = 7 - sx % 8;
            const unsigned pen = ((at[0] >> bit) & 1) | (((at[plane] >> bit) & 1) << 1);
            const int px = x + (int)sx - r.ox, py = y + (int)sy - r.oy;
            if (!pen || px < 282 || px >= 320 || py < 0 || py >= rows) continue;
            const unsigned word = (unsigned)(px - 272) / 16;
            const u16 mask = (u16)(1u << (15 - px % 16));
            if (pen & 1) out[py][word] |= mask;
            if (pen & 2) out[py][word + 3] |= mask;
        }
}

void overlay6(Row6& t, const Row6& a)
{
    for (unsigned w = 0; w < 3; ++w) {
        const u16 mask = (u16)(a[w] | a[w + 3]);
        t[w] = (u16)((t[w] & ~mask) | a[w]);
        t[w + 3] = (u16)((t[w + 3] & ~mask) | a[w + 3]);
    }
}

inline void overlay1(Pair& t, const Pair& a)
{
    const u16 mask = (u16)(a.lo | a.hi);
    t.lo = (u16)((t.lo & ~mask) | a.lo);
    t.hi = (u16)((t.hi & ~mask) | a.hi);
}

u32 fnv(const u8* p, u32 n)
{
    u32 h = 2166136261u;
    while (n--) h = (h ^ *p++) * 16777619u;
    return h;
}

/* Tiles as tools/probe-ammo-complete.py interns them, kept as their final
   run-length encoding over the row-pair dictionary (pairs in first use). */
struct Factored {
    enum { PairSlots = 1024, TileSlots = 16384, PageSlots = 4096 };
    Pair pairs[256];
    unsigned npairs;
    u16 pairSlot[PairSlots];   /* pair index + 1 */
    Buf runs;                  /* (length, pair) bytes */
    Buf tiles;                 /* u32 first run byte, u32 run count per tile */
    unsigned ntiles;
    u16* tileSlot;             /* tile + 1 */
    Buf pages;                 /* 16 u16 per page */
    unsigned npages;
    u16* pageSlot;             /* page + 1 */
    u8 candidate[2 * 256];
    u8 fresh[256];             /* pairs the candidate added */
};

int pairIndex(Factored& f, Pair p, bool add)
{
    const u32 key = (u32)p.lo << 16 | p.hi;
    unsigned slot = (key * 2654435761u) >> 22;
    for (;; slot = (slot + 1) & (Factored::PairSlots - 1)) {
        const unsigned e = f.pairSlot[slot];
        if (!e) break;
        if (f.pairs[e - 1].lo == p.lo && f.pairs[e - 1].hi == p.hi) return (int)e - 1;
    }
    if (!add) return -1;
    if (f.npairs == 256) return -2;
    f.pairs[f.npairs] = p;
    f.pairSlot[slot] = (u16)++f.npairs;
    return (int)f.npairs - 1;
}

/* The tile id of rows[0..n), adding it (and its new pairs) when new. */
int internTile(Factored& f, const Pair* rows, unsigned n, unsigned* id)
{
    if (n > 256) return dw_fail("an ammo gauge tile is too tall");
    unsigned len = 0;
    bool known = true;
    for (unsigned y = 0; y < n; ++y) {
        int index = pairIndex(f, rows[y], false);
        if (index < 0) {
            known = false;
            index = pairIndex(f, rows[y], true);
            if (index < 0) return dw_fail("the ammo gauge rows no longer fit byte indices");
        }
        if (len && f.candidate[len - 1] == index && f.candidate[len - 2] < 255) ++f.candidate[len - 2];
        else { f.candidate[len++] = 1; f.candidate[len++] = (u8)index; }
    }
    const u32 h = fnv(f.candidate, len);
    unsigned slot = h & (Factored::TileSlots - 1);
    for (;; slot = (slot + 1) & (Factored::TileSlots - 1)) {
        const unsigned e = f.tileSlot[slot];
        if (!e) break;
        if (!known) continue;
        const u8* t = f.tiles.p + 8 * (e - 1);
        if (rd_be32(t + 4) * 2 == len && !memcmp(f.runs.p + rd_be32(t), f.candidate, len)) { *id = e - 1; return 1; }
    }
    if (f.ntiles >= 65535 || f.ntiles >= Factored::TileSlots / 2) return dw_fail("too many ammo gauge tiles");
    buf_u32(&f.tiles, f.runs.n); buf_u32(&f.tiles, len / 2);
    buf_put(&f.runs, f.candidate, len);
    f.tileSlot[slot] = (u16)(f.ntiles + 1);
    *id = f.ntiles++;
    return !f.runs.failed && !f.tiles.failed ? 1 : dw_fail("not enough memory");
}

int internPage(Factored& f, const u16* page, unsigned* id)
{
    const u32 h = fnv(reinterpret_cast<const u8*>(page), 32);
    unsigned slot = h & (Factored::PageSlots - 1);
    for (;; slot = (slot + 1) & (Factored::PageSlots - 1)) {
        const unsigned e = f.pageSlot[slot];
        if (!e) break;
        if (!memcmp(f.pages.p + 32 * (e - 1), page, 32)) { *id = e - 1; return 1; }
    }
    if (f.npages >= Factored::PageSlots / 2) return dw_fail("too many ammo gauge pages");
    buf_put(&f.pages, page, 32);
    f.pageSlot[slot] = (u16)(f.npages + 1);
    *id = f.npages++;
    return !f.pages.failed ? 1 : dw_fail("not enough memory");
}

struct FactoredWork {
    Row6 bases[4][Rows], rims[4][Rows], scratch[Rows];
    Pair rowArt[3][4][8]; /* 418, 419, 421: word 1 of the first eight rows at (288, 0) */
    u8 artRows[3][4][2];  /* their rows with pixels: overlaying blank rows changes nothing */
    Pair base[Rows], divider[Rows], columns[20][216]; /* own * 4 + previous, then the four below */
    u16 starts[16], configurations[4960];
    u8 bounds[16][17];
    u16 page[16];
    u16 outer[256][4];
    u8 frames[4][216];
};
}

extern "C" int derived_ammo_factored(Context* c, const Buf* hud)
{
    static const u16 gauge[5] = {416, 418, 419, 421, 65010};
    FactoredWork* w = static_cast<FactoredWork*>(dw_zalloc(sizeof(FactoredWork)));
    Factored* f = static_cast<Factored*>(dw_zalloc(sizeof(Factored)));
    u16* tileSlot = static_cast<u16*>(dw_zalloc(2 * Factored::TileSlots));
    u16* pageSlot = static_cast<u16*>(dw_zalloc(2 * Factored::PageSlots));
    Section sec;
    sec_init(&sec);
    int ok = w && f && tileSlot && pageSlot;
    unsigned nconfig = 0;
    if (ok) {
        f->tileSlot = tileSlot; f->pageSlot = pageSlot;
        buf_init(&f->runs); buf_init(&f->tiles); buf_init(&f->pages);
    }
    /* The art: bases with their rim's fill, rims, the eight-row stamps. */
    for (unsigned meter = 0; ok && meter < 4; ++meter) {
        DwapRecord r0, r1;
        if (!findRecord(hud, 416, 0, &r0) || !findRecord(hud, 416, 1, &r1)) { ok = dw_fail("Downwell.hud lacks the ammo gauge"); break; }
        for (unsigned y = 40; y < 256; ++y) {
            static const u16 open[6] = {0x003f, 0xffff, 0xc000, 0x003f, 0xffff, 0xc000};
            memcpy(w->bases[meter][y], open, sizeof open);
        }
        stampRows(hud, r0, 284, 43 + (int)meter, w->scratch, Rows);
        for (unsigned y = 0; y < Rows; ++y) overlay6(w->bases[meter][y], w->scratch[y]);
        stampRows(hud, r1, 284, 43 + (int)meter, w->rims[meter], Rows);
    }
    for (unsigned s = 0; ok && s < 3; ++s)
        for (unsigned frame = 0; frame < (s == 2 ? 2u : 4u); ++frame) {
            DwapRecord r;
            if (!findRecord(hud, gauge[1 + s], frame, &r)) { ok = dw_fail("Downwell.hud lacks the ammo gauge"); break; }
            stampRows(hud, r, 288, 0, w->scratch, 8);
            unsigned top = 8, end = 0;
            for (unsigned y = 0; y < 8; ++y) {
                w->rowArt[s][frame][y].lo = w->scratch[y][1]; w->rowArt[s][frame][y].hi = w->scratch[y][4];
                if (w->scratch[y][1] | w->scratch[y][4]) { if (top == 8) top = y; end = y + 1; }
            }
            w->artRows[s][frame][0] = (u8)(top < end ? top : 0); w->artRows[s][frame][1] = (u8)end;
        }
    /* Every (capacity, ammo, meter, segment) selector page. */
    for (unsigned cap = 1; ok && cap <= 16; ++cap) {
        w->starts[cap - 1] = (u16)nconfig;
        unsigned bounds[17];
        for (unsigned a = 0; a <= cap; ++a) {
            bounds[a] = ceiling(HudReal::ratio(a, cap).times184());
            w->bounds[cap - 1][a] = (u8)bounds[a];
        }
        for (unsigned ammo = 0; ok && ammo <= (cap < 9 ? cap : 9); ++ammo) {
            if (io_cancelled()) { ok = dw_fail("cancelled"); break; }
            const HudReal fill = HudReal::ratio(ammo, cap).times184();
            const HudReal five = HudReal::integer(5);
            unsigned fillTop = 232, fillHeight = 0;
            if (fill.mantissa && fill.less(five)) fillHeight = five.subtract(fill).floor();
            else if (ammo) { fillTop = HudReal::integer(237).subtract(fill).floor(); fillHeight = fill.subtract(five).floor(); }
            for (unsigned meter = 0; ok && meter < 4; ++meter) {
                const int baseY = 229 + 4 * (int)meter;
                for (unsigned y = 0; y < Rows; ++y) { w->base[y].lo = w->bases[meter][y][1]; w->base[y].hi = w->bases[meter][y][4]; }
                for (unsigned y = fillTop; y < fillTop + fillHeight; ++y) { w->base[y].lo |= 0x7fe0; w->base[y].hi &= (u16)~0x7fe0; }
                memset(w->divider, 0, sizeof w->divider);
                downwell::drawAmmoDividers(ammo, cap, 1, 0, (unsigned)baseY, [&](unsigned, unsigned frame, int, unsigned y) {
                    for (unsigned row = 0; row < 8; ++row)
                        if (y + row < Rows) overlay1(w->divider[y + row], w->rowArt[2][frame][row]);
                });
                for (unsigned segment = 0; ok && segment < cap; ++segment) {
                    const unsigned lo = bounds[segment], hi = bounds[segment + 1];
                    const int first = segment == cap - 1 ? 40 : baseY - (int)hi + 8;
                    const int last = segment == 0 ? 256 : baseY - (int)lo + 8;
                    const unsigned n = (unsigned)(last - first);
                    /* Rows below the segment take the previous frame, the
                       segment's own rows (drawn after them) its own frame. */
                    auto sprites = [&](Pair* column, unsigned from, unsigned to, unsigned frame) {
                        for (unsigned row = from; row < to; ++row) {
                            const int y = baseY - (int)row;
                            if (y >= 232) continue;
                            const unsigned s = row < bounds[ammo] ? (row ? 1 : 2) : 0, af = s == 2 ? frame % 2 : frame;
                            const Pair* art = w->rowArt[s][af];
                            for (int k = w->artRows[s][af][0]; k < w->artRows[s][af][1]; ++k) {
                                const int t = y - first + k;
                                if (t >= 0 && t < (int)n) overlay1(column[t], art[k]);
                            }
                        }
                    };
                    for (unsigned previous = 0; previous < 4; ++previous) {
                        Pair* below = w->columns[16 + previous];
                        memcpy(below, w->base + first, n * sizeof(Pair));
                        sprites(below, segment ? bounds[segment - 1] : 0, lo, previous);
                        for (unsigned own = 0; own < 4; ++own) {
                            Pair* column = w->columns[own * 4 + previous];
                            memcpy(column, below, n * sizeof(Pair));
                            sprites(column, lo, hi, own);
                            for (unsigned y = 0; y < n; ++y) {
                                const Pair& d = w->divider[first + y];
                                const u16* r = w->rims[meter][first + y];
                                if (d.lo | d.hi) overlay1(column[y], d);
                                if (r[1] | r[4]) { const Pair rim = {r[1], r[4]}; overlay1(column[y], rim); }
                            }
                        }
                    }
                    for (unsigned i = 0; ok && i < 16; ++i) {
                        unsigned id = 0;
                        ok = internTile(*f, w->columns[i], n, &id);
                        w->page[i] = (u16)id;
                    }
                    unsigned page = 0;
                    ok = ok && internPage(*f, w->page, &page);
                    if (ok) {
                        if (nconfig == 4960) { ok = dw_fail("too many ammo gauge configurations"); break; }
                        w->configurations[nconfig++] = (u16)page;
                    }
                }
            }
        }
    }
    if (ok) {
        REQUIRE(nconfig == 4960, "unexpected ammo gauge configuration count");
        Buf* t = sec_table(&sec);
        for (unsigned i = 0; i < 16; ++i) buf_u16(t, w->starts[i]);
        t = sec_table(&sec); buf_put(t, w->bounds, sizeof w->bounds);
        t = sec_table(&sec); for (unsigned i = 0; i < nconfig; ++i) buf_u16(t, w->configurations[i]);
        t = sec_table(&sec);
        for (unsigned i = 0; i < 16 * f->npages; ++i) buf_u16(t, reinterpret_cast<const u16*>(f->pages.p)[i]);
        t = sec_table(&sec);
        for (unsigned i = 0; i < f->ntiles; ++i) {
            const u32 at = rd_be32(f->tiles.p + 8 * i), count = rd_be32(f->tiles.p + 8 * i + 4);
            if (at / 2 > 65535) ok = dw_fail("too many ammo gauge runs");
            buf_u16(t, at / 2); buf_u16(t, count);
        }
        t = sec_table(&sec); buf_put(t, f->runs.p, f->runs.n);
        t = sec_table(&sec); for (unsigned i = 0; i < f->npairs; ++i) { buf_u16(t, f->pairs[i].lo); buf_u16(t, f->pairs[i].hi); }
        /* The outer rim words (0, 2, 3, 5) of each meter, and the patches
           between any two meters. */
        u16 (*outer)[4] = w->outer;
        u8 (*frames)[216] = w->frames;
        unsigned nouter = 0;
        for (unsigned meter = 0; ok && meter < 4; ++meter)
            for (unsigned y = 40; y < 256; ++y) {
                Row6 row;
                memcpy(row, w->bases[meter][y], sizeof row);
                overlay6(row, w->rims[meter][y]);
                const u16 v[4] = {(u16)(row[0] & 0x003f), (u16)(row[2] & 0xc000), (u16)(row[3] & 0x003f), (u16)(row[5] & 0xc000)};
                unsigned i = 0;
                while (i < nouter && memcmp(outer[i], v, sizeof v)) ++i;
                if (i == nouter) {
                    if (nouter == 256) { ok = dw_fail("too many ammo gauge rim rows"); break; }
                    memcpy(outer[nouter++], v, sizeof v);
                }
                frames[meter][y - 40] = (u8)i;
            }
        t = sec_table(&sec); for (unsigned i = 0; i < nouter; ++i) for (unsigned k = 0; k < 4; ++k) buf_u16(t, outer[i][k]);
        t = sec_table(&sec); buf_put(t, frames, sizeof w->frames);
        Buf* directory = sec_table(&sec);
        Buf* patches = sec_table(&sec);
        unsigned npatches = 0;
        for (unsigned old = 0; old < 4; ++old)
            for (unsigned now = 0; now < 4; ++now) {
                const unsigned firstPatch = npatches;
                for (unsigned y = 0; y < 216; ++y)
                    for (unsigned k = 0; k < 4; ++k) {
                        const u16 a = outer[frames[old][y]][k], v = outer[frames[now][y]][k];
                        if (a == v) continue;
                        buf_u16(patches, (y + 40) * 48 + (k % 2 == 0 ? 34 : 38) + (k >= 2 ? 13632 : 0));
                        buf_u16(patches, k % 2 == 0 ? 0x003f : 0xc000);
                        buf_u16(patches, v);
                        ++npatches;
                    }
                buf_u16(directory, firstPatch); buf_u16(directory, npatches - firstPatch);
            }
        /* The single-digit labels: word 1 of rows 238..249. */
        t = sec_table(&sec);
        for (unsigned ammo = 0; ok && ammo < 10; ++ammo)
            for (int offset = 0; offset < 3; ++offset) {
                DwapRecord r;
                if (!findRecord(hud, 65010, ammo, &r)) { ok = dw_fail("Downwell.hud lacks an ammo digit"); break; }
                stampRows(hud, r, 290 + offset, 239 + offset, w->scratch, Rows);
                for (unsigned y = 238; y < 250; ++y) { buf_u16(t, w->scratch[y][1]); buf_u16(t, w->scratch[y][4]); }
            }
        /* The gauge's source records and bytes. */
        Buf* records = sec_table(&sec);
        Buf* bytes = sec_table(&sec);
        const u32 n = dwap_count(hud);
        for (u32 i = 0; ok && i < n; ++i) {
            DwapRecord r;
            dwap_record(hud, i, &r);
            unsigned k = 0;
            while (k < 5 && gauge[k] != r.sprite) ++k;
            if (k == 5) continue;
            if (r.offset > hud->n || r.size > hud->n - r.offset) { ok = dw_fail("Downwell.hud is damaged"); break; }
            const s32 v[10] = {r.sprite, r.frame, r.ox, r.oy, r.width, r.height, r.stride, r.flags, (s32)bytes->n, (s32)r.size};
            for (unsigned j = 0; j < 10; ++j) buf_u32(records, (u32)v[j]);
            buf_put(bytes, hud->p + r.offset, r.size);
        }
    }
    if (f) { buf_free(&f->runs); buf_free(&f->tiles); buf_free(&f->pages); }
    dw_free(pageSlot); dw_free(tileSlot); dw_free(f); dw_free(w);
    if (!ok) { sec_free(&sec); return 0; }
    return finishSection(c, &sec, "AMFA");
}

/* ---- HART ---- */
extern "C" int derived_health_art(Context* c, const Buf* hud)
{
    const UBYTE* pack = hud->p;
    const UBYTE* end = hud->p + hud->n;
    UBYTE* pixels = static_cast<UBYTE*>(dw_zalloc(downwellLayerBytes));
    if (!pixels) return 0;
    Section sec;
    sec_init(&sec);
    AmigaRenderer renderer;
    amigaRendererInit(&renderer, nullptr, pixels);
    bool ok = true;
    auto asset = [&](unsigned s, unsigned f, int x, int y) {
        int left, top, width, height;
        if (!amigaRendererAssetBounds(pack, end, (UWORD)s, (UWORD)f, x, y, 0, &left, &top, &width, &height) ||
            left < 8 || top < 6 || left + width > 100 || top + height > 34) { ok = false; return; }
        amigaRendererDrawAsset(&renderer, pack, end, (UWORD)s, (UWORD)f, x, y, 0);
    };
    auto emit = [&]() {
        Buf* t = sec_table(&sec);
        for (unsigned y = 6; y < 34; ++y)
            for (unsigned w = 0; w < 7; ++w) {
                const UBYTE* p = pixels + y * downwellBytesPerPlaneLine + w * 2;
                buf_u16(t, (u32)p[0] << 8 | p[1]);
                buf_u16(t, (u32)p[downwellPlaneBytes] << 8 | p[downwellPlaneBytes + 1]);
            }
    };
    amigaRendererFillRect(&renderer, 8, 6, 92, 28, 3);
    asset(415, 1, 8, 6); emit();
    amigaRendererFillRect(&renderer, 8, 6, 92, 28, 0);
    downwell::drawHeartPieceMeter(8, 6, 0, 4, [](int, int, int, int) {}, asset);
    asset(415, 0, 8, 6); emit();
    dw_free(pixels);
    Buf* t = sec_table(&sec);
    for (unsigned index = 0; ok && index < 11; ++index) {
        AmigaHudGlyph glyph;
        memset(&glyph, 0, sizeof glyph);
        if (!amigaRendererPrepareHudGlyph(&glyph, pack, end, 65011, (UWORD)index)) { ok = false; break; }
        for (unsigned i = 0; i < 16; ++i) buf_u16(t, glyph.low[i]);
        for (unsigned i = 0; i < 16; ++i) buf_u16(t, glyph.high[i]);
        buf_u32(t, (u32)glyph.left); buf_u32(t, (u32)glyph.top); buf_u32(t, glyph.width); buf_u32(t, glyph.height);
    }
    /* The admitted pack bytes: the header, then each heart record and its pixels. */
    u32 spans[7][2] = {{0, 20}};
    unsigned nspans = 1, wanted = 0;
    const u32 n = dwap_count(hud);
    for (u32 i = 0; ok && i < n; ++i) {
        DwapRecord r;
        dwap_record(hud, i, &r);
        const unsigned bit = r.sprite == 415 && r.frame < 2 ? 1u << r.frame : r.sprite == 378 && r.frame == 2 ? 4u : 0u;
        if (!bit || (wanted & bit)) continue;
        wanted |= bit;
        if (r.offset > hud->n || r.size > hud->n - r.offset) { ok = false; break; }
        spans[nspans][0] = 20 + 24 * i; spans[nspans++][1] = 24;
        spans[nspans][0] = r.offset; spans[nspans++][1] = r.size;
    }
    ok = ok && wanted == 7;
    int first = 0;
    for (unsigned i = 0; ok && i < 7; ++i) {
        Buf* b = sec_table(&sec);
        if (!i) first = sec.ntables - 1;
        buf_put(b, hud->p + spans[i][0], spans[i][1]);
    }
    if (ok) {
        const int table = sec.ntables;
        t = sec_table(&sec);
        for (unsigned i = 0; i < 7; ++i) {
            buf_u32(t, spans[i][0]); buf_u32(t, spans[i][1]); buf_u32(t, 0);
            sec_ptr_table(&sec, table, 12 * i + 8, first + (int)i);
        }
    }
    if (!ok) { sec_free(&sec); return dw_fail("Downwell.hud: unexpected health art"); }
    return finishSection(c, &sec, "HART");
}

/* ---- HUDT ---- */
/* tools/generate-amiga-hud-text.cpp's tables, without the five streak tables
   install-table-headers.py leaves out. AmigaHudText befriends this name. */
struct AmigaHudTextTables {
    static int run(Context* c, AmigaHudText& t)
    {
        Section sec;
        sec_init(&sec);
        Buf* tb = sec_table(&sec);
        const AmigaHudStage& s = t.currencyBackground_;
        for (unsigned p = 0; p < 3; ++p)
            for (unsigned y = 0; y < s.rows; ++y)
                for (unsigned x = 0; x < s.words; ++x)
                    buf_u16(tb, (UWORD)((p == 0 ? s.low[y][x] : p == 1 ? s.high[y][x] : s.mask[y][x]) & s.mask[y][x]));
        typedef UWORD Pixels[2][3][10][2];
        Pixels* pixels = static_cast<Pixels*>(dw_alloc(10 * sizeof(Pixels)));
        if (!pixels) { sec_free(&sec); return 0; }
        Buf* tc = sec_table(&sec);
        int ok = 1;
        for (unsigned digit = 0; digit < 10; ++digit)
            for (unsigned phase = 0; phase < 2; ++phase) {
                if (!t.compose(digit, phase)) ok = 0;
                const auto& comp = t.currencyDigits_[digit][phase];
                for (unsigned p = 0; p < 3; ++p)
                    for (unsigned y = 0; y < 10; ++y)
                        for (unsigned x = 0; x < 2; ++x) {
                            const UWORD v = p == 0 ? comp.low[y][x] : p == 1 ? comp.high[y][x] : comp.mask[y][x];
                            pixels[digit][phase][p][y][x] = v;
                            buf_u16(tc, v);
                        }
            }
        /* currencyPairs[1][2][11][11][10][3]: a byte of each word spans two digits. */
        Buf* tp = sec_table(&sec);
        for (unsigned half = 0; half < 2; ++half)
            for (unsigned a = 0; a < 11; ++a)
                for (unsigned b = 0; b < 11; ++b) {
                    const unsigned digits[3] = {half ? 10 : a, half ? a : b, half ? b : 10};
                    UWORD column[30];
                    memset(column, 0, sizeof column);
                    for (unsigned y = 0; y < 10; ++y)
                        for (unsigned i = 0; i < 3; ++i)
                            if (digits[i] < 10) {
                                const auto& sp = pixels[digits[i]][i == 1 ? 1 : 0];
                                const unsigned x = i == 0 ? 1 : 0;
                                const UWORD mask = sp[2][y][x];
                                column[y * 3] = (UWORD)((column[y * 3] & ~mask) | sp[0][y][x]);
                                column[y * 3 + 1] = (UWORD)((column[y * 3 + 1] & ~mask) | sp[1][y][x]);
                                column[y * 3 + 2] |= mask;
                            }
                    for (unsigned i = 0; i < 30; ++i) buf_u16(tp, column[i] & (half ? 0xffu : 0xff00u));
                }
        dw_free(pixels);
        if (!ok) { sec_free(&sec); return dw_fail("cannot compose the gem counter digits"); }
        return finishSection(c, &sec, "HUDT");
    }
};

extern "C" int derived_hud_text(Context* c, const Buf* hud, const Buf* game)
{
    /* All of AmigaHudText's defaults are zero. */
    AmigaHudText* text = static_cast<AmigaHudText*>(dw_zalloc(sizeof(AmigaHudText)));
    if (!text) return 0;
    int ok = text->bind(hud->p, hud->p + hud->n, game->p, game->p + game->n);
    ok = ok ? AmigaHudTextTables::run(c, *text) : dw_fail("cannot bind the HUD text art");
    dw_free(text);
    return ok;
}

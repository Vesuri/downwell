/* the scene banks: AMDV, AMFA, HUDT, HART, DMGT, STMP, TPH6, TINI, Downwell.terrain, Downwell.planes, Downwell.cavernplanes. */
#include "derived.h"
#include "derived_parts.h"
#include "sha256.h"

u32 dwap_count(const Buf* pack)
{
    if (pack->n < 20 || memcmp(pack->p, "DWAP", 4) || rd_be16(pack->p + 4) != 2 || rd_be16(pack->p + 6) != 20) return 0;
    u32 n = rd_be32(pack->p + 8);
    return n <= (pack->n - 20) / 24 ? n : 0;
}

void dwap_record(const Buf* pack, u32 i, DwapRecord* r)
{
    const u8* p = pack->p + 20 + 24 * i;
    r->sprite = rd_be16(p); r->frame = rd_be16(p + 2);
    r->ox = (s16)rd_be16(p + 4); r->oy = (s16)rd_be16(p + 6);
    r->width = rd_be16(p + 8); r->height = rd_be16(p + 10);
    r->stride = rd_be16(p + 12); r->flags = rd_be16(p + 14);
    r->offset = rd_be32(p + 16); r->size = rd_be32(p + 20);
}

/* tools/generate-terrain-phase.py: sprite 51's 26 frames shifted to the
   six-pixel phase, 16 rows of (coverage, low, high) each. */
static int terrain_phase(Context* c, const Buf* game, u8* out)
{
    u32 n = dwap_count(game), i, seen = 0;
    Buf* section;
    REQUIRE(n, "Downwell.game is not a DWAP v2 bank");
    for (i = 0; i < n; ++i) {
        DwapRecord r;
        u32 row;
        dwap_record(game, i, &r);
        if (r.sprite != 51) continue;
        REQUIRE(r.ox == 8 && r.oy == 8 && r.width == 16 && r.height == 16 && r.stride == 16 && r.flags == 3 &&
                r.frame < 26 && r.offset + 64 <= game->n && !(seen >> r.frame & 1), "Downwell.game: unexpected wall tiles");
        seen |= 1ul << r.frame;
        for (row = 0; row < 16; ++row) {
            u32 low = (u32)rd_be16(game->p + r.offset + 2 * row) << 10, high = (u32)rd_be16(game->p + r.offset + 32 + 2 * row) << 10;
            u8* d = out + r.frame * 192 + row * 12;
            u32 v[3], k;
            v[0] = low | high; v[1] = low; v[2] = high;
            for (k = 0; k < 3; ++k) { d[4 * k] = (u8)(v[k] >> 24); d[4 * k + 1] = (u8)(v[k] >> 16); d[4 * k + 2] = (u8)(v[k] >> 8); d[4 * k + 3] = (u8)v[k]; }
        }
    }
    REQUIRE(seen == 0x3fffffful, "Downwell.game lacks a wall tile");
    section = tables_add(&c->tables, "TPH6");
    if (!section) return 0;
    buf_put(section, out, 4992);
    return 1;
}

/* tools/generate-effect-stamps.py: alpha/low/high rows of the currency and
   smoke effects, sorted by (sprite, frame). */
static int effect_stamps(Context* c, const Buf* game)
{
    static const u16 sprites[3] = {84, 133, 134};
    static const u8 frames[3] = {9, 4, 4};
    u32 n = dwap_count(game), found[17], k = 0, s, f, i;
    SHA256 sha;
    u8 digest[32];
    Buf* out;
    u32 payload;
    REQUIRE(n, "Downwell.game is not a DWAP v2 bank");
    for (s = 0; s < 3; ++s)
        for (f = 0; f < frames[s]; ++f) {
            u32 hit = n;
            for (i = 0; i < n; ++i) {
                DwapRecord r;
                dwap_record(game, i, &r);
                if (r.sprite != sprites[s] || r.frame != f) continue;
                REQUIRE(hit == n, "Downwell.game: duplicate effect frame");
                REQUIRE(r.flags == 3 && r.width && r.height && !(r.stride % 16) && r.width <= r.stride &&
                        r.offset >= 20 + n * 24 && !(r.offset % 2) && r.offset + r.size <= game->n &&
                        r.size == (u32)r.stride / 8 * r.height * 3, "Downwell.game: invalid effect frame");
                hit = i;
            }
            REQUIRE(hit < n, "Downwell.game lacks a currency or smoke frame");
            found[k++] = hit;
        }
    sha_init(&sha); sha_update(&sha, game->p, game->n); sha_final(&sha, digest);
    out = tables_add(&c->tables, "STMP");
    if (!out) return 0;
    buf_put(out, "DWEF", 4); buf_u16(out, 1); buf_u16(out, 17); buf_u32(out, 20); buf_u32(out, 48 + 17 * 20);
    buf_put(out, digest, 32);
    payload = 48 + 17 * 20;
    for (k = 0; k < 17; ++k) {
        DwapRecord r;
        u32 stride, bytes;
        dwap_record(game, found[k], &r);
        stride = r.stride / 8; bytes = stride * r.height * 3;
        buf_u16(out, r.sprite); buf_u16(out, r.frame); buf_u16(out, r.width); buf_u16(out, r.height);
        buf_u32(out, stride); buf_u32(out, payload); buf_u32(out, payload + bytes);
        payload += 2 * bytes;
    }
    for (k = 0; k < 17; ++k) {
        DwapRecord r;
        u32 stride, plane, y;
        const u8 *low, *high, *mask;
        dwap_record(game, found[k], &r);
        stride = r.stride / 8; plane = stride * r.height;
        low = game->p + r.offset; high = low + plane; mask = high + plane;
        for (y = 0; y < r.height; ++y) {
            buf_put(out, mask + y * stride, stride); buf_put(out, low + y * stride, stride); buf_put(out, high + y * stride, stride);
        }
        for (y = 0; y < r.height; ++y) {
            buf_put(out, mask + y * stride, stride); buf_put(out, mask + y * stride, stride); buf_put(out, mask + y * stride, stride);
        }
    }
    REQUIRE(!out->failed, "not enough memory");
    return 1;
}

/* tools/generate-amiga-damage-text.py: the damage digit glyphs 65100..65110
   and the ten "-N" labels, ink and its one-pixel cross outline. */
static int damage_text(Context* c, const Buf* game)
{
    u8 glyphs[11][10];
    u32 n = dwap_count(game), g, i, value;
    Section sec;
    Buf *t0, *t1, *out;
    int ok;
    REQUIRE(n, "Downwell.game is not a DWAP v2 bank");
    for (g = 0; g < 11; ++g) {
        u32 matches = 0, y;
        for (i = 0; i < n; ++i) {
            DwapRecord r;
            dwap_record(game, i, &r);
            if (r.sprite != 65100 + g || r.frame) continue;
            ++matches;
            REQUIRE(r.ox == 4 && r.oy == 0 && r.width == 8 && r.height == 10 && r.stride == 16 && r.offset + 40 <= game->n,
                    "Downwell.game: unexpected damage digit");
            for (y = 0; y < 10; ++y) glyphs[g][y] = game->p[r.offset + 2 * y] | game->p[r.offset + 20 + 2 * y];
        }
        REQUIRE(matches == 1, "Downwell.game lacks a damage digit");
    }
    sec_init(&sec);
    t0 = sec_table(&sec); t1 = sec_table(&sec);
    for (g = 0; g < 11; ++g) buf_put(t0, glyphs[g], 10);
    for (value = 0; value < 10; ++value) {
        /* 12 rows of 48 pixels as three words: ink, then the outline mask. */
        u16 ink[12][3], mask[12][3];
        u32 y, w, first = 0, length = 0, used = 0;
        memset(ink, 0, sizeof ink);
        for (y = 0; y < 10; ++y) {
            ink[y + 1][1] = (u16)(glyphs[10][y] << 8 | glyphs[value][y]); /* x 16..23 and 24..31 */
        }
        for (y = 0; y < 12; ++y)
            for (w = 0; w < 3; ++w) {
                u16 m = ink[y][w];
                m |= (u16)(ink[y][w] << 1) | (u16)(w < 2 ? ink[y][w + 1] >> 15 : 0); /* ink at x + 1 */
                m |= (u16)(ink[y][w] >> 1) | (u16)(w ? ink[y][w - 1] << 15 : 0);    /* ink at x - 1 */
                if (y) m |= ink[y - 1][w];
                if (y < 11) m |= ink[y + 1][w];
                mask[y][w] = m;
                if (m) used |= 1u << w;
            }
        if (used) {
            while (!(used >> first & 1)) ++first;
            for (w = 0; w < 3; ++w) if (used >> w & 1) length = w - first + 1;
        }
        buf_u32(t1, first); buf_u32(t1, length);
        for (y = 0; y < 12; ++y) for (w = 0; w < 3; ++w) buf_u16(t1, mask[y][w]);
        for (y = 0; y < 12; ++y) for (w = 0; w < 3; ++w) buf_u16(t1, ink[y][w]);
    }
    out = tables_add(&c->tables, "DMGT");
    ok = out && sec_finish(&sec, out);
    sec_free(&sec);
    return ok;
}

int stage_derived(Context* c)
{
    Buf game, hud, patterns;
    u8* phase;
    int ok;
    /* Downwell.game alone: the tile phase, the effect stamps, the damage text. */
    TRY(ctx_read_output(c, "Downwell.game", &game));
    phase = dw_alloc(4992);
    ok = phase && terrain_phase(c, &game, phase) && effect_stamps(c, &game) && damage_text(c, &game);
    if (ok) {
        ok = ctx_read_output(c, "Downwell.hud", &hud);
        if (ok) {
            ok = derived_ammo_dividers(c, &hud) && derived_ammo_factored(c, &hud) &&
                 derived_health_art(c, &hud) && derived_hud_text(c, &hud, &game);
            buf_free(&hud);
        }
    }
    if (ok) {
        ok = ctx_read_output(c, "Downwell.patterns", &patterns);
        if (ok) { ok = derived_pattern_terrain(c, &game, &patterns); buf_free(&patterns); }
    }
    ok = ok && !io_cancelled() && derived_plane_bank(c, &game, "Downwell.planes");
    buf_free(&game);
    ok = ok && derived_initial_terrain(c, phase);
    dw_free(phase);
    if (ok) {
        Buf cavern;
        ok = ctx_read_output(c, "Downwell.cavern", &cavern);
        if (ok) { ok = derived_plane_bank(c, &cavern, "Downwell.cavernplanes"); buf_free(&cavern); }
    }
    return ok;
}

/* data.win SPRT and OBJT: sprite collision boxes (SCOL), goal (GOAL) and time-field (TFLD) masks, Downwell.encountermasks. */
#include "masks.h"

/* The first 8 bytes of data.win's SHA-256, as the generators stamp them. */
static const u8 SOURCE_PREFIX[8] = {0xc0, 0x7d, 0xaf, 0x5f, 0xbe, 0x94, 0x0a, 0xe0};
#define SPRITE_COUNT 741
#define MANIFEST_SCHEMA 3

/* An SPRT entry (bytecode 17): name, width, height, marginLeft, marginRight,
   marginBottom, marginTop, transparent, smooth, preload, bboxMode, sepMasks,
   originX, originY; then -1, version, type, playback speed and its type,
   a sequence offset (version 2+) and a nine-slice offset (version 3); then
   the TPAG list and the collision masks (count, rows padded to bytes). */
typedef struct {
    u32 name;
    s32 width, height, marginLeft, marginRight, marginBottom, marginTop, sepMasks, originX, originY;
    u32 frames, masks, maskAt;
} Sprite;

static int sprite_read(Win* w, u32 entry, Sprite* s)
{
    u8 b[56];
    u32 q = entry + 56;
    TRY(win_read(w, entry, b, sizeof b));
    s->name = rd_le32(b);
    s->width = (s32)rd_le32(b + 4); s->height = (s32)rd_le32(b + 8);
    s->marginLeft = (s32)rd_le32(b + 12); s->marginRight = (s32)rd_le32(b + 16);
    s->marginBottom = (s32)rd_le32(b + 20); s->marginTop = (s32)rd_le32(b + 24);
    s->sepMasks = (s32)rd_le32(b + 44);
    s->originX = (s32)rd_le32(b + 48); s->originY = (s32)rd_le32(b + 52);
    if (win_s32(w, q) == -1) {
        u32 version = win_u32(w, q + 4), type = win_u32(w, q + 8);
        REQUIRE(version >= 1 && version <= 3 && type == 0, "data.win holds a sprite of an unexpected kind");
        q += 20 + (version >= 2 ? 4 : 0) + (version >= 3 ? 4 : 0);
    }
    s->frames = win_u32(w, q);
    REQUIRE(s->frames < 65536, "data.win is damaged (a sprite's frames)");
    q += 4 + 4 * s->frames;
    s->masks = win_u32(w, q);
    s->maskAt = q + 4;
    REQUIRE(s->width > 0 && s->height > 0 && s->width <= 4096 && s->height <= 4096 && s->masks < 65536,
            "data.win is damaged (a sprite's masks)");
    REQUIRE(s->sepMasks == 0 || s->sepMasks == 1, "data.win holds an unsupported collision mode");
    return !w->failed;
}

static u32 mask_bytes(const Sprite* s) { return (u32)(s->width + 7) / 8 * (u32)s->height; }

static int sprite_get(Context* c, u32 id, const char* name, Sprite* s)
{
    u32 off;
    TRY(win_chunk(&c->win, "SPRT", &off, 0));
    REQUIRE(win_list_count(&c->win, off) == SPRITE_COUNT, "data.win has an unexpected number of sprites");
    TRY(sprite_read(&c->win, win_list_item(&c->win, off, id), s));
    REQUIRE(win_streq(&c->win, s->name, name), "data.win has an unexpected sprite");
    return 1;
}

/* An object's default sprite and collision mask (OBJT +4, +32). */
static int object_is(Context* c, u32 id, s32 sprite, s32 mask)
{
    u32 off, e;
    TRY(win_chunk(&c->win, "OBJT", &off, 0));
    REQUIRE(id < win_list_count(&c->win, off), "data.win has too few objects");
    e = win_list_item(&c->win, off, id);
    REQUIRE(win_s32(&c->win, e + 4) == sprite && win_s32(&c->win, e + 32) == mask, "data.win has an unexpected object");
    return !c->win.failed;
}

static void put_le32(u8* p, u32 v) { p[0] = (u8)v; p[1] = (u8)(v >> 8); p[2] = (u8)(v >> 16); p[3] = (u8)(v >> 24); }

/* generate-sprite-pack.py's DWSC: header, then per sprite originX, originY,
   marginLeft, marginTop, marginRight, marginBottom, collision mode, mask
   count, frame count (little-endian). */
static int collision_boxes(Context* c)
{
    Win* w = &c->win;
    u32 off, i;
    u8* p;
    Buf* out = tables_add(&c->tables, "SCOL");
    if (!out) return 0;
    TRY(win_chunk(w, "SPRT", &off, 0));
    REQUIRE(win_list_count(w, off) == SPRITE_COUNT, "data.win has an unexpected number of sprites");
    p = buf_grow(out, 24 + 36 * SPRITE_COUNT);
    REQUIRE(p, "not enough memory");
    memcpy(p, "DWSC\1\0\30\0", 8);
    memcpy(p + 8, SOURCE_PREFIX, 8);
    put_le32(p + 16, SPRITE_COUNT);
    put_le32(p + 20, MANIFEST_SCHEMA);
    for (i = 0, p += 24; i < SPRITE_COUNT; ++i, p += 36) {
        Sprite s;
        TRY(sprite_read(w, win_list_item(w, off, i), &s));
        put_le32(p, (u32)s.originX); put_le32(p + 4, (u32)s.originY);
        put_le32(p + 8, (u32)s.marginLeft); put_le32(p + 12, (u32)s.marginTop);
        put_le32(p + 16, (u32)s.marginRight); put_le32(p + 20, (u32)s.marginBottom);
        put_le32(p + 24, (u32)s.sepMasks); put_le32(p + 28, s.masks); put_le32(p + 32, s.frames);
    }
    return 1;
}

/* A precise square mask as row intervals {left, right}, one u8 table
   [size][2] in its own section (install-table-headers.py); every row must be
   one run without holes. */
static int mask_rows(Context* c, u32 id, const char* name, s32 size, const char* tag)
{
    Sprite s;
    Section sec;
    Buf *t, *out;
    u8 row[16];
    u32 stride = (u32)(size + 7) / 8, y, x;
    int ok = 1;
    TRY(sprite_get(c, id, name, &s));
    REQUIRE(s.width == size && s.height == size && s.masks == 1 && s.sepMasks == 1, "data.win has an unexpected field mask");
    sec_init(&sec);
    t = sec_table(&sec);
    for (y = 0; y < (u32)size && ok; ++y) {
        u32 left = 0, right = 0, n = 0;
        ok = win_read(&c->win, s.maskAt + y * stride, row, stride);
        for (x = 0; x < (u32)size; ++x)
            if (row[x >> 3] & (0x80 >> (x & 7))) {
                if (!n++) left = x;
                right = x + 1;
            }
        if (ok && n != right - left) ok = dw_fail("a field mask has a hole");
        buf_u8(t, left); buf_u8(t, right);
    }
    out = ok ? tables_add(&c->tables, tag) : 0;
    ok = out && sec_finish(&sec, out);
    sec_free(&sec);
    return ok;
}

/* generate-collision-masks.py's DWSM, kept to the admitted sprites
   (generate-encounter-masks.py): each has one mask, frame 0. */
static int encounter_masks(Context* c)
{
    static const u32 ids[4] = {31, 128, 585, 616};
    static const char* const names[4] = {"sprPlayerIdle", "sprVertDither", "mask16", "sprCircleArea"};
    Sprite s[4];
    u32 i, total = 24 + 20 * 4, at;
    u8* p;
    int ok;
    TRY(object_is(c, 35, 31, 31) && object_is(c, 60, 51, 585) && object_is(c, 61, 128, -1) &&
        object_is(c, 255, 616, -1) && object_is(c, 371, 616, -1));
    for (i = 0; i < 4; ++i) {
        TRY(sprite_get(c, ids[i], names[i], &s[i]));
        REQUIRE(s[i].masks == 1, "data.win has an unexpected collision mask");
        total += mask_bytes(&s[i]);
    }
    REQUIRE(total == 34256, "the encounter masks changed size");
    p = dw_alloc(total);
    if (!p) return 0;
    memcpy(p, "DWSM\1\0\30\0", 8);
    memcpy(p + 8, SOURCE_PREFIX, 8);
    put_le32(p + 16, 4);
    put_le32(p + 20, 3);
    for (i = 0, at = 24 + 20 * 4; i < 4; ++i) {
        u8* d = p + 24 + 20 * i;
        put_le32(d, ids[i]); put_le32(d + 4, 0);
        put_le32(d + 8, (u32)s[i].width); put_le32(d + 12, (u32)s[i].height); put_le32(d + 16, at);
        if (!win_read(&c->win, s[i].maskAt, p + at, mask_bytes(&s[i]))) { dw_free(p); return 0; }
        at += mask_bytes(&s[i]);
    }
    ok = dw_write_file(c->dest, "Downwell.encountermasks", p, total);
    dw_free(p);
    return ok;
}

int stage_masks(Context* c)
{
    TRY(collision_boxes(c));
    TRY(object_is(c, 255, 616, -1) && mask_rows(c, 616, "sprCircleArea", 128, "GOAL"));
    TRY(object_is(c, 82, 597, -1) && mask_rows(c, 597, "maskCircle24", 48, "TFLD"));
    TRY(encounter_masks(c));
    return 1;
}

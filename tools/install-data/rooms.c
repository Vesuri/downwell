/* data.win OBJT and ROOM: objects (OBJT), runtime rooms (ROOM), room terrain plan (RTPL).
   tools/generate-object-pack.py, generate-room-pack.py and
   generate-room-terrain-plan.py read UndertaleModTool 0.9.2.0's export of
   these chunks (tools/utmt-export.csx, export-room-layer-metadata.csx); the
   fields come straight from data.win here (GMS2 2022.5, bytecode 17). */
#include "rooms.h"

/* The first 8 bytes of data.win's SHA-256, the packs' source hash prefix. */
static const u8 SOURCE_PREFIX[8] = {0xc0, 0x7d, 0xaf, 0x5f, 0xbe, 0x94, 0x0a, 0xe0};

static void le16(u8* p, u32 v) { p[0] = (u8)v; p[1] = (u8)(v >> 8); }
static void le32(u8* p, u32 v) { p[0] = (u8)v; p[1] = (u8)(v >> 8); p[2] = (u8)(v >> 16); p[3] = (u8)(v >> 24); }
static void put_le32(Buf* b, u32 v) { u8* p = buf_grow(b, 4); if (p) le32(p, v); }

/* A resource reference: UTMT's IndexOf() of the resolved resource. */
static u32 ref(Win* w, u32 off) { s32 v = win_s32(w, off); return v < 0 ? 0xffffffffu : (u32)v; }
static u32 flag(Win* w, u32 off) { return win_u32(w, off) != 0; }

/* An integer as an IEEE single (exact for the small values involved). */
static u32 int_float(s32 v)
{
    u32 sign = 0, m, e = 150;
    if (!v) return 0;
    if (v < 0) { sign = 0x80000000u; m = (u32)-v; } else m = (u32)v;
    while (m >= 0x1000000u) { m >>= 1; ++e; } /* truncates beyond 24 bits; not reached by room data */
    while (m < 0x800000u) { m <<= 1; --e; }
    return sign | e << 23 | (m & 0x7fffff);
}

/* ---- Objects ---------------------------------------------------------- */

/* OBJT record: name, sprite, visible, managed, solid, depth, persistent,
   parent, texture mask, uses physics, ... physics vertices at +84 (count
   at +68), then the event-type list. */
enum { OBJ_SPRITE = 4, OBJ_VISIBLE = 8, OBJ_MANAGED = 12, OBJ_SOLID = 16, OBJ_DEPTH = 20, OBJ_PERSISTENT = 24,
       OBJ_PARENT = 28, OBJ_MASK = 32, OBJ_PHYSICS = 36, OBJ_VERTICES = 68, OBJ_VERTEX_DATA = 84 };

static const char DEPTH_SCRIPT[] = "gml_GlobalScript___global_object_depths";

typedef struct { u32 index; s32 value; } Pair;

/* VARI entries named `name` with instance type self: their occurrence
   addresses (the instructions that reference them). */
static int variable_uses(Win* w, const char* name, Buf* out)
{
    u32 off, len, at;
    TRY(win_chunk(w, "VARI", &off, &len));
    for (at = off + 12; at + 20 <= off + len; at += 20) {
        u32 n, i, addr;
        if (win_s32(w, at + 4) != -1 || !win_streq(w, win_u32(w, at), name)) continue;
        n = win_u32(w, at + 12);
        addr = win_u32(w, at + 16);
        for (i = 0; i < n; ++i) {
            put_le32(out, addr);
            addr += win_u32(w, addr + 4) & 0x07ffffffu;
        }
    }
    REQUIRE(!out->failed, "not enough memory");
    return !w->failed;
}

static int uses(const Buf* b, u32 addr)
{
    u32 i;
    for (i = 0; i + 4 <= b->n; i += 4)
        if (rd_le32(b->p + i) == addr) return 1;
    return 0;
}

/* Instruction length in words (UndertaleModLib's Instruction). */
static u32 insn_words(u32 word)
{
    u32 op = word >> 24, t1 = (word >> 16) & 15;
    if (op == 0x45) return t1 == 0x0f ? 1 : 2;               /* pop; popswap has no operand */
    if (op == 0xd9) return 2;                                /* call */
    if (op == 0xc0 || op == 0xc1 || op == 0xc2 || op == 0xc3 || op == 0x84) /* push* */
        return t1 == 0 || t1 == 3 ? 3 : t1 == 0x0f ? 1 : 2;
    if (op == 0xff && t1 == 2) return 2;                     /* break with an int operand */
    return 1;
}

static void add_pair(Buf* b, u32 index, s32 value)
{
    Pair p;
    p.index = index; p.value = value;
    buf_put(b, &p, sizeof p);
}

/* Python's dict of the regex matches: the last value of each index. */
static u32 distinct(Buf* b)
{
    Pair* p = (Pair*)b->p;
    u32 n = b->n / sizeof *p, i, j, k = 0;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < k && p[j].index != p[i].index; ++j) continue;
        if (j < k) p[j].value = p[i].value;
        else p[k++] = p[i];
    }
    b->n = k * sizeof *p;
    return k;
}

/* The global script's __objectDepths/__objectNames tables, as
   generate-object-pack.py's dynamic_depths() matches them in the VM
   assembly: push.i|pushi.e value, conv.i.v, pushi.e -5, pushi.e index,
   pop.v.v [array]self.__objectDepths (and push.s name, conv.s.v, ...). */
static int scan_depths(Win* w, Buf* depths, Buf* names)
{
    u32 code, n, i, entry = 0, start, end, at, targets[64], ntargets = 0;
    u32 addr[5], word[5], operand[5], seen = 0;
    Buf depthUses, nameUses;
    int ok = 0;
    TRY(win_chunk(w, "CODE", &code, 0));
    n = win_list_count(w, code);
    for (i = 0; i < n; ++i) {
        u32 e = win_list_item(w, code, i);
        if (win_streq(w, win_u32(w, e), DEPTH_SCRIPT)) { entry = e; break; }
    }
    REQUIRE(entry, "data.win lacks the object depth script");
    start = entry + 12 + (u32)win_s32(w, entry + 12) + win_u32(w, entry + 16);
    end = start + win_u32(w, entry + 4);
    REQUIRE(end <= w->size, "data.win is damaged (code)");
    /* Branch targets: the assembly puts a label there, which ends a match. */
    for (at = start; at < end; at += 4 * insn_words(win_u32(w, at))) {
        u32 wd = win_u32(w, at), op = wd >> 24;
        if ((op >= 0xb6 && op <= 0xb8) || op == 0xba || (op == 0xbb && (wd & 0xffffff) != 0xf00000)) {
            u32 off = wd & 0x7fffff;
            if (off & 0x400000) off |= 0xff800000u;
            REQUIRE(ntargets < 64, "the object depth script has more branches than expected");
            targets[ntargets++] = at + off * 4;
        }
        if (w->failed) return 0;
    }
    buf_init(&depthUses); buf_init(&nameUses);
    if (!variable_uses(w, "__objectDepths", &depthUses) || !variable_uses(w, "__objectNames", &nameUses)) goto done;
    for (at = start; at < end && !w->failed;) {
        u32 k = seen % 5, words, j, labelled = 0;
        word[k] = win_u32(w, at);
        words = insn_words(word[k]);
        operand[k] = words > 1 ? win_u32(w, at + 4) : 0;
        addr[k] = at;
        at += 4 * words;
        ++seen;
        if (seen < 5) continue;
#define W(back) word[(seen - 1 - (back)) % 5]
#define O(back) operand[(seen - 1 - (back)) % 5]
#define A(back) addr[(seen - 1 - (back)) % 5]
        if ((W(0) & 0xffff0000u) != 0x45550000u || ((O(0) >> 24) & 0xf8) != 0) continue; /* pop.v.v [array] */
        if (W(2) != 0x840ffffbu || (W(1) & 0xffff8000u) != 0x840f0000u) continue;        /* pushi.e -5; pushi.e N */
        for (j = 0; j < ntargets; ++j)
            if (targets[j] == A(0) || targets[j] == A(1) || targets[j] == A(2) || targets[j] == A(3)) labelled = 1;
        if (labelled) continue;
        if ((W(3) & 0xffff0000u) == 0x07520000u && uses(&depthUses, A(0))) {             /* conv.i.v */
            if ((W(4) & 0xffff0000u) == 0xc0020000u) add_pair(depths, W(1) & 0x7fff, (s32)O(4));
            else if ((W(4) & 0xffff0000u) == 0x840f0000u) add_pair(depths, W(1) & 0x7fff, (s16)(W(4) & 0xffff));
        } else if ((W(3) & 0xffff0000u) == 0x07560000u && (W(4) & 0xffff0000u) == 0xc0060000u &&
                   uses(&nameUses, A(0)))                                                  /* conv.s.v; push.s */
            add_pair(names, W(1) & 0x7fff, (s32)O(4));
#undef W
#undef O
#undef A
    }
    ok = !w->failed && !depths->failed && !names->failed;
    if (!ok) dw_fail("not enough memory");
done:
    buf_free(&depthUses); buf_free(&nameUses);
    return ok;
}

/* Every object's creation depth (0 when the table does not name it). */
static int dynamic_depths(Win* w, u32 objt, u32 count, s32* result)
{
    Buf depths, names, objectNames, offsets;
    u32 strg, i, j, nd, nn;
    int ok = 0;
    char name[256];
    buf_init(&depths); buf_init(&names); buf_init(&objectNames); buf_init(&offsets);
    if (!scan_depths(w, &depths, &names) || !win_chunk(w, "STRG", &strg, 0)) goto done;
    nd = distinct(&depths); nn = distinct(&names);
    if (nd != 342 || nn != 342) { dw_fail("data.win's object depth table is incomplete"); goto done; }
    for (i = 0; i < nn; ++i) {
        for (j = 0; j < nd && ((Pair*)depths.p)[j].index != ((Pair*)names.p)[i].index; ++j) continue;
        if (j == nd) { dw_fail("data.win's object depth table is incomplete"); goto done; }
    }
    /* Object names, unique, in memory for the lookups. */
    for (i = 0; i < count; ++i) {
        u32 ref0 = win_u32(w, win_list_item(w, objt, i)), n = win_strlen(w, ref0);
        put_le32(&offsets, objectNames.n);
        if (n + 1 > sizeof name || !win_string(w, ref0, name, sizeof name)) { dw_fail("data.win holds an unexpected object name"); goto done; }
        buf_put(&objectNames, name, n + 1);
    }
    put_le32(&offsets, objectNames.n);
    if (objectNames.failed || offsets.failed) { dw_fail("not enough memory"); goto done; }
    for (i = 0; i < count; ++i)
        for (j = 0; j < i; ++j)
            if (!dw_strcmp((char*)objectNames.p + rd_le32(offsets.p + 4 * i), (char*)objectNames.p + rd_le32(offsets.p + 4 * j))) {
                dw_fail("data.win repeats an object name"); goto done;
            }
    for (i = 0; i < count; ++i) result[i] = 0;
    for (i = 0; i < nn; ++i) {
        const Pair* p = (const Pair*)names.p + i;
        u32 s, k;
        s32 depth = 0;
        if ((u32)p->value >= win_list_count(w, strg)) { dw_fail("data.win's object depth table is damaged"); goto done; }
        s = win_list_item(w, strg, (u32)p->value) + 4;
        if (!win_string(w, s, name, sizeof name)) goto done;
        for (j = 0; j < nd; ++j)
            if (((Pair*)depths.p)[j].index == p->index) depth = ((Pair*)depths.p)[j].value;
        for (k = 0; k < count && dw_strcmp((char*)objectNames.p + rd_le32(offsets.p + 4 * k), name); ++k) continue;
        if (k == count) { dw_fail("data.win's object depth table names an unknown object"); goto done; }
        result[k] = depth;
    }
    ok = !w->failed;
done:
    buf_free(&depths); buf_free(&names); buf_free(&objectNames); buf_free(&offsets);
    return ok;
}

/* generated/downwell-objects.dwob (format 3). */
static int object_pack(Context* c, Buf* out)
{
    Win* w = &c->win;
    u32 objt, count, i, bindings = 0, at;
    s32* depths;
    u8* p;
    int ok = 0;
    TRY(win_chunk(w, "OBJT", &objt, 0));
    count = win_list_count(w, objt);
    REQUIRE(count == 386, "data.win has an unexpected number of objects");
    depths = dw_alloc(4 * count);
    if (!depths) return 0;
    if (!dynamic_depths(w, objt, count, depths)) goto done;
    /* Header, then the objects, then the event bindings. */
    p = buf_grow(out, 40 + 36 * count);
    if (!p) { dw_fail("not enough memory"); goto done; }
    for (i = 0; i < count; ++i) {
        u32 o = win_list_item(w, objt, i), events, types, t, first = bindings, managed, flags = 0;
        u8* r = out->p + 40 + 36 * i;
        events = o + OBJ_VERTEX_DATA + 8 * win_u32(w, o + OBJ_VERTICES);
        types = win_list_count(w, events);
        for (t = 0; t < types; ++t) {
            u32 bucket = win_list_item(w, events, t), n = win_list_count(w, bucket), e;
            for (e = 0; e < n; ++e) {
                u32 ev = win_list_item(w, bucket, e), actions = ev + 4;
                u8* b;
                if (win_list_count(w, actions) != 1) { dw_fail("an object event does not have exactly one action"); goto done; }
                b = buf_grow(out, 16);
                if (!b) { dw_fail("not enough memory"); goto done; }
                le32(b, t);
                le32(b + 4, win_u32(w, ev));
                le32(b + 8, ref(w, win_list_item(w, actions, 0) + 32));
                le32(b + 12, 0);
                ++bindings;
            }
        }
        r = out->p + 40 + 36 * i; /* the buffer may have moved */
        managed = win_u32(w, o + OBJ_MANAGED);
        if (managed > 1) { dw_fail("an object has an unexpected managed property"); goto done; }
        if (flag(w, o + OBJ_VISIBLE)) flags |= 1;
        if (flag(w, o + OBJ_SOLID)) flags |= 2;
        if (flag(w, o + OBJ_PERSISTENT)) flags |= 4;
        if (flag(w, o + OBJ_PHYSICS)) flags |= 8;
        if (managed) flags |= 16;
        le32(r, i);
        le32(r + 4, ref(w, o + OBJ_SPRITE));
        le32(r + 8, ref(w, o + OBJ_PARENT));
        le32(r + 12, ref(w, o + OBJ_MASK));
        le32(r + 16, win_u32(w, o + OBJ_DEPTH));
        le32(r + 20, flags);
        le32(r + 24, first);
        le32(r + 28, bindings - first);
        le32(r + 32, (u32)depths[i]);
    }
    if (bindings != 2075) { dw_fail("data.win has an unexpected number of object events"); goto done; }
    p = out->p;
    memcpy(p, "DWOB", 4);
    le16(p + 4, 3); le16(p + 6, 40);
    le32(p + 8, 3); le32(p + 12, count); le32(p + 16, bindings);
    memcpy(p + 20, SOURCE_PREFIX, 8);
    for (at = 28; at < 40; ++at) p[at] = 0;
    ok = !w->failed && !out->failed;
done:
    dw_free(depths);
    return ok;
}

/* ---- Rooms ------------------------------------------------------------ */

/* ROOM record: name, caption, width, height, speed, persistent, colour,
   draw colour, creation code, flags, backgrounds, views, game objects,
   tiles, ..., layers (+88). */
enum { ROOM_WIDTH = 8, ROOM_HEIGHT = 12, ROOM_SPEED = 16, ROOM_PERSISTENT = 20, ROOM_CODE = 32,
       ROOM_VIEWS = 44, ROOM_OBJECTS = 48, ROOM_LAYERS = 88 };
/* Room instance: x, y, object, id, creation code, scale x, y, image speed,
   image index, colour, rotation, pre-create code. */
enum { INST_X = 0, INST_Y = 4, INST_OBJECT = 8, INST_ID = 12, INST_CODE = 16, INST_SCALE_X = 20, INST_SCALE_Y = 24,
       INST_SPEED = 28, INST_INDEX = 32, INST_ROTATION = 40, INST_PRECREATE = 44 };
/* Layer: name, id, type, depth, x/y offset, h/v speed, visible, effect
   enabled, effect type, effect properties (simple list of 12-byte
   records), then the layer's data. */
enum { LAYER_ID = 4, LAYER_TYPE = 8, LAYER_DEPTH = 12, LAYER_MOTION = 16, LAYER_VISIBLE = 32, LAYER_EFFECTS = 44 };

/* GEN8: LastObj at +12, the room order (count, ids) at +128. */
enum { GEN8_LAST_OBJECT = 12, GEN8_ROOM_ORDER = 128 };

#define RUNTIME_ROOMS_COUNT 5
static const u32 RUNTIME_ROOMS[RUNTIME_ROOMS_COUNT] = {7, 8, 11, 16, 20};
#define RUNTIME_BUDGET (28 * 1024)

enum { HEADER_BYTES = 40, ROOM_BYTES = 40 + 8 * 56, INSTANCE_BYTES = 48, LAYER_BYTES = 48 };

static int admitted(u32 id)
{
    u32 i;
    for (i = 0; i < RUNTIME_ROOMS_COUNT; ++i)
        if (RUNTIME_ROOMS[i] == id) return 1;
    return 0;
}

static u32 layer_data(Win* w, u32 layer) { return layer + 48 + 12 * win_u32(w, layer + LAYER_EFFECTS); }

typedef struct { u32 gen8, order, rooms, objt, objects, nextId; } RoomSource;

static int room_source(Win* w, RoomSource* s)
{
    u32 last;
    TRY(win_chunk(w, "GEN8", &s->gen8, 0));
    TRY(win_chunk(w, "ROOM", &s->rooms, 0));
    TRY(win_chunk(w, "OBJT", &s->objt, 0));
    s->order = s->gen8 + GEN8_ROOM_ORDER;
    s->objects = win_list_count(w, s->objt);
    last = win_u32(w, s->gen8 + GEN8_LAST_OBJECT);
    REQUIRE(last > 0 && last < 0xffffffffu, "data.win's instance counter is out of range");
    s->nextId = last + 1;
    REQUIRE(win_u32(w, s->order) == win_list_count(w, s->rooms) && win_list_count(w, s->rooms) == 25,
            "data.win has an unexpected room order");
    return !w->failed;
}

/* The room at order position i. */
static int order_room(Win* w, const RoomSource* s, u32 i, u32* id, u32* room)
{
    *id = win_u32(w, s->order + 4 + 4 * i);
    REQUIRE(*id < win_list_count(w, s->rooms), "data.win's room order names an unknown room");
    *room = win_list_item(w, s->rooms, *id);
    return !w->failed;
}

/* generate-room-pack.py's build_pack (runtime 0), or the pack after
   runtime_projection (runtime 1): only the admitted rooms, original IDs
   and the global instance seed, version 5 with the catalogue's layer
   maximum after the header. */
static int room_pack(Context* c, int runtime, Buf* out)
{
    Win* w = &c->win;
    RoomSource s;
    u32 i, nrooms = 0, ninstances = 0, nlayers = 0, nmembers = 0, maximum = 0, header, base;
    u32 roomAt, instanceAt, layerAt, memberAt, found = 0, allInstances = 0, allMembers = 0;
    u8* p;
    TRY(room_source(w, &s));
    /* Counts first, so the pack is written in place. */
    for (i = 0; i < 25; ++i) {
        u32 id = 0, room = 0, layers, l;
        TRY(order_room(w, &s, i, &id, &room));
        layers = win_u32(w, room + ROOM_LAYERS);
        for (l = 0; l < win_list_count(w, layers); ++l) {
            u32 layer = win_list_item(w, layers, l), lid = win_u32(w, layer + LAYER_ID);
            if (lid > maximum) maximum = lid;
            if (!runtime || admitted(id)) {
                ++nlayers;
                if (win_u32(w, layer + LAYER_TYPE) == 2) nmembers += win_u32(w, layer_data(w, layer));
            }
        }
        if (!runtime || admitted(id)) {
            REQUIRE(!(found >> id & 1), "a runtime room is admitted twice");
            if (runtime) found |= 1u << id;
            ++nrooms;
            ninstances += win_list_count(w, win_u32(w, room + ROOM_OBJECTS));
        }
    }
    TRY(!w->failed);
    REQUIRE(maximum <= 0x7fffffffu, "data.win's layer IDs are out of range");
    header = runtime ? HEADER_BYTES + 4 : HEADER_BYTES;
    base = out->n;
    p = buf_grow(out, header + nrooms * ROOM_BYTES + ninstances * INSTANCE_BYTES + nlayers * LAYER_BYTES + 4 * nmembers);
    REQUIRE(p, "not enough memory");
    memcpy(p, "DWRM", 4);
    le16(p + 4, runtime ? 5 : 4); le16(p + 6, header);
    le32(p + 8, 3); le32(p + 12, nrooms); le32(p + 16, ninstances); le32(p + 20, nlayers); le32(p + 24, nmembers);
    memcpy(p + 28, SOURCE_PREFIX, 8);
    le32(p + 36, s.nextId);
    if (runtime) le32(p + 40, maximum);
    roomAt = header;
    instanceAt = roomAt + nrooms * ROOM_BYTES;
    layerAt = instanceAt + ninstances * INSTANCE_BYTES;
    memberAt = layerAt + nlayers * LAYER_BYTES;
    ninstances = nlayers = nmembers = 0;
    for (i = 0; i < 25; ++i) {
        u32 id = 0, room = 0, objects, layers, views, n, k, l, firstInstance = ninstances, firstLayer = nlayers;
        u8* r;
        TRY(order_room(w, &s, i, &id, &room));
        objects = win_u32(w, room + ROOM_OBJECTS);
        layers = win_u32(w, room + ROOM_LAYERS);
        views = win_u32(w, room + ROOM_VIEWS);
        allInstances += win_list_count(w, objects);
        if (runtime && !admitted(id)) {
            for (l = 0; l < win_list_count(w, layers); ++l) {
                u32 layer = win_list_item(w, layers, l);
                if (win_u32(w, layer + LAYER_TYPE) == 2) allMembers += win_u32(w, layer_data(w, layer));
            }
            continue;
        }
        n = win_list_count(w, objects);
        for (k = 0; k < n; ++k) {
            u32 o = win_list_item(w, objects, k), object = win_u32(w, o + INST_OBJECT), iid = win_u32(w, o + INST_ID), j;
            u8* q = out->p + base + instanceAt + INSTANCE_BYTES * ninstances++;
            REQUIRE(object < s.objects, "a room instance names an unknown object");
            REQUIRE(iid < s.nextId, "a room instance ID crosses the runtime allocator boundary");
            for (j = 0; j < k; ++j)
                REQUIRE(win_u32(w, win_list_item(w, objects, j) + INST_ID) != iid, "a room repeats an instance ID");
            le32(q, iid);
            le32(q + 4, object);
            le32(q + 8, win_u32(w, o + INST_X));
            le32(q + 12, win_u32(w, o + INST_Y));
            le32(q + 16, win_u32(w, o + INST_SCALE_X));
            le32(q + 20, win_u32(w, o + INST_SCALE_Y));
            le32(q + 24, win_u32(w, o + INST_ROTATION));
            le32(q + 28, win_u32(w, o + INST_SPEED));
            le32(q + 32, int_float(win_s32(w, o + INST_INDEX)));
            le32(q + 36, ref(w, o + INST_CODE));
            le32(q + 40, ref(w, o + INST_PRECREATE));
            le32(q + 44, flag(w, win_list_item(w, s.objt, object) + OBJ_PERSISTENT));
        }
        for (l = 0; l < win_list_count(w, layers); ++l) {
            u32 layer = win_list_item(w, layers, l), type = win_u32(w, layer + LAYER_TYPE), d = layer_data(w, layer);
            u32 firstMember = nmembers, members = 0, m;
            u8* q = out->p + base + layerAt + LAYER_BYTES * nlayers++;
            for (m = 0; m < 4; ++m)
                REQUIRE(!(win_u32(w, layer + LAYER_MOTION + 4 * m) & 0x7fffffffu), "nonzero layer motion requires a pack extension");
            REQUIRE(type == 1 || type == 2, "a room has a layer the pack does not hold");
            memset(q, 0, LAYER_BYTES);
            le32(q, win_u32(w, layer + LAYER_ID));
            le32(q + 4, win_u32(w, layer + LAYER_DEPTH));
            le32(q + 8, flag(w, layer + LAYER_VISIBLE));
            le32(q + 20, type);
            le32(q + 24, 0xffffffffu);
            if (type == 2) {
                members = win_u32(w, d);
                for (m = 0; m < members; ++m) {
                    u32 iid = win_u32(w, d + 4 + 4 * m), j;
                    for (j = 0; j < n && win_u32(w, win_list_item(w, objects, j) + INST_ID) != iid; ++j) continue;
                    REQUIRE(j < n, "a room layer references an unknown instance");
                    le32(out->p + base + memberAt + 4 * nmembers++, iid);
                }
                allMembers += members;
            } else {
                le32(q + 24, ref(w, d + 8));
                le32(q + 28, win_u32(w, d + 24));
                le32(q + 32, flag(w, d) | flag(w, d + 4) << 1 | flag(w, d + 12) << 2 | flag(w, d + 16) << 3 | flag(w, d + 20) << 4);
                le32(q + 36, win_u32(w, d + 28));
                le32(q + 40, win_u32(w, d + 32));
                le32(q + 44, win_u32(w, d + 36));
            }
            le32(q + 12, firstMember);
            le32(q + 16, members);
        }
        REQUIRE(win_list_count(w, views) == 8, "a room does not have eight views");
        r = out->p + base + roomAt;
        roomAt += ROOM_BYTES;
        le32(r, id);
        le32(r + 4, win_u32(w, room + ROOM_WIDTH));
        le32(r + 8, win_u32(w, room + ROOM_HEIGHT));
        le32(r + 12, win_u32(w, room + ROOM_SPEED));
        le32(r + 16, ref(w, room + ROOM_CODE));
        le32(r + 20, flag(w, room + ROOM_PERSISTENT));
        le32(r + 24, firstInstance);
        le32(r + 28, n);
        le32(r + 32, firstLayer);
        le32(r + 36, nlayers - firstLayer);
        for (k = 0; k < 8; ++k) {
            u32 v = win_list_item(w, views, k), f;
            u8* q = r + 40 + 56 * k;
            le32(q, flag(w, v));
            for (f = 1; f < 13; ++f) le32(q + 4 * f, win_u32(w, v + 4 * f));
            le32(q + 52, ref(w, v + 52));
        }
        if (w->failed) return 0;
    }
    REQUIRE(allInstances == 5186 && allMembers == allInstances, "data.win's rooms are not the expected ones");
    if (runtime) {
        for (i = 0; i < RUNTIME_ROOMS_COUNT; ++i) REQUIRE(found >> RUNTIME_ROOMS[i] & 1, "data.win lacks a runtime room");
        REQUIRE(out->n - base <= RUNTIME_BUDGET, "the runtime rooms exceed their budget");
    } else
        REQUIRE(nlayers == 132, "data.win has an unexpected number of room layers");
    return !w->failed;
}

/* ---- Room terrain plan ------------------------------------------------ */

/* generate-room-terrain-plan.py: room 11's ordered terrain writes, as the
   Command table of amiga-room-terrain-plan.inc ('IHHhhHHBB'). */
static int terrain_plan(Context* c, Buf* out)
{
    Win* w = &c->win;
    RoomSource s;
    static Section sec; /* 64 table buffers: kept off the stack */
    Buf cells; /* (cell, value) pairs, the occupied map */
    Buf* t;
    u32 room, objects, n, k;
    int ok = 0;
    TRY(room_source(w, &s));
    REQUIRE(win_list_count(w, s.rooms) > 11, "data.win lacks room 11");
    room = win_list_item(w, s.rooms, 11);
    objects = win_u32(w, room + ROOM_OBJECTS);
    n = win_list_count(w, objects);
    sec_init(&sec);
    buf_init(&cells);
    t = sec_table(&sec);
    for (k = 0; k < n; ++k) {
        u32 o = win_list_item(w, objects, k), obj = win_u32(w, o + INST_OBJECT);
        s32 x = win_s32(w, o + INST_X), y = win_s32(w, o + INST_Y);
        u32 cell = 0, bit = 0, before = 0, after = 0;
        if (obj != 60 && obj != 93 && obj != 99) continue;
        if (obj != 99) {
            u32 row, column, j, pairs;
            if ((x & 15) || (y & 15) || x < -512 || x >= 1536 || y < 0 || y >= 8192) { dw_fail("unsupported room terrain geometry"); goto done; }
            row = (u32)y / 16; column = (u32)(x + 512) / 16;
            cell = row * 128 + column; bit = 0x8000u >> (row % 16);
            pairs = cells.n / 8;
            for (j = 0; j < pairs && rd_le32(cells.p + 8 * j) != cell; ++j) continue;
            if (j < pairs) before = rd_le32(cells.p + 8 * j + 4);
            if (obj == 60 && (before & 192) == 192) { dw_fail("overflow-contact room requires scalar publication"); goto done; }
            after = obj == 60 ? (before + 64) | 27 : before | 34;
            if (j < pairs) le32(cells.p + 8 * j + 4, after);
            else { put_le32(&cells, cell); put_le32(&cells, after); }
        }
        if (x < -32768 || x > 32767 || y < -32768 || y > 32767) { dw_fail("unsupported room terrain geometry"); goto done; }
        buf_u32(t, win_u32(w, o + INST_ID));
        buf_u16(t, k); buf_u16(t, obj);
        buf_u16(t, (u32)x); buf_u16(t, (u32)y);
        buf_u16(t, cell); buf_u16(t, bit);
        buf_u8(t, before); buf_u8(t, after);
    }
    if (w->failed || cells.failed) goto done;
    ok = sec_finish(&sec, out);
done:
    buf_free(&cells);
    sec_free(&sec);
    return ok;
}

int rooms_full_pack(Context* c, Buf* out) { return room_pack(c, 0, out); }

int stage_rooms(Context* c)
{
    Buf* out = tables_add(&c->tables, "OBJT");
    TRY(out && object_pack(c, out));
    out = tables_add(&c->tables, "ROOM");
    TRY(out && room_pack(c, 1, out));
    out = tables_add(&c->tables, "RTPL");
    TRY(out && terrain_plan(c, out));
#ifdef ROOMS_FULL_TEST
    {
        Buf full;
        int ok;
        buf_init(&full);
        ok = rooms_full_pack(c, &full) && dw_write_file(c->dest, "full.dwrm", full.p, full.n);
        buf_free(&full);
        TRY(ok);
    }
#endif
    return 1;
}

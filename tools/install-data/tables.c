#include "tables.h"

void sec_init(Section* s) { int i; s->ntables = 0; for (i = 0; i < SECTION_TABLES; ++i) buf_init(&s->tables[i]); buf_init(&s->pointers); buf_init(&s->strings); }
void sec_free(Section* s) { int i; for (i = 0; i < SECTION_TABLES; ++i) buf_free(&s->tables[i]); buf_free(&s->pointers); buf_free(&s->strings); }
Buf* sec_table(Section* s) { if (s->ntables >= SECTION_TABLES) { dw_fail("too many tables in a section"); return 0; } return &s->tables[s->ntables++]; }

static void add(Section* s, u32 table, u32 at, u32 kind, u32 target, u32 len)
{
    SectionPointer p;
    p.table = table; p.at = at; p.kind = kind; p.target = target; p.len = len;
    buf_put(&s->pointers, &p, sizeof p);
}
void sec_ptr_string(Section* s, int table, u32 at, const void* bytes, u32 n)
{
    add(s, (u32)table, at, 0, s->strings.n, n);
    buf_put(&s->strings, bytes, n);
}
void sec_ptr_table(Section* s, int table, u32 at, int target) { add(s, (u32)table, at, 1, (u32)target, 0); }

int sec_finish(Section* s, Buf* out)
{
    SectionPointer* ptr = (SectionPointer*)s->pointers.p;
    u32 count = s->pointers.n / sizeof(SectionPointer), start, i, j, offsets[SECTION_TABLES];
    u32* placed;
    Buf body;
    REQUIRE(!s->pointers.failed && !s->strings.failed, "not enough memory");
    start = 8 + 4 * (u32)s->ntables + 4 * count;
    placed = dw_alloc(4 * count + 4);
    if (!placed) return 0;
    buf_init(&body);
    for (i = 0; i < (u32)s->ntables; ++i) {
        REQUIRE(!s->tables[i].failed, "not enough memory");
        buf_align(&body, start, 4);
        offsets[i] = start + body.n;
        buf_put(&body, s->tables[i].p, s->tables[i].n);
    }
    /* Each distinct string once, in the order first pointed at. */
    for (i = 0; i < count; ++i) {
        u32 target;
        if (ptr[i].kind == 1) target = offsets[ptr[i].target];
        else {
            for (j = 0; j < i; ++j)
                if (ptr[j].kind == 0 && ptr[j].len == ptr[i].len &&
                    !memcmp(s->strings.p + ptr[j].target, s->strings.p + ptr[i].target, ptr[i].len)) break;
            if (j < i) target = placed[j];
            else {
                target = start + body.n;
                buf_put(&body, s->strings.p + ptr[i].target, ptr[i].len);
                buf_u8(&body, 0);
            }
        }
        placed[i] = target;
        buf_set_u32(&body, offsets[ptr[i].table] + ptr[i].at - start, target);
    }
    buf_u16(out, (u32)s->ntables); buf_u16(out, 0); buf_u32(out, count);
    for (i = 0; i < (u32)s->ntables; ++i) buf_u32(out, offsets[i]);
    for (i = 0; i < count; ++i) buf_u32(out, offsets[ptr[i].table] + ptr[i].at);
    buf_put(out, body.p, body.n);
    dw_free(placed);
    i = !body.failed && !out->failed;
    buf_free(&body);
    REQUIRE(i, "not enough memory");
    return 1;
}

static u32 tag_value(const char* t) { return (u32)(u8)t[0] << 24 | (u32)(u8)t[1] << 16 | (u32)(u8)t[2] << 8 | (u8)t[3]; }

/* tools/pack-amiga-tables.py's SECTIONS order. */
static const char ORDER[] = "ROOMOBJTSCOLSTMPTPH6TINIAMFAAMDVHUDTGOALTFLDATTLMTXTSPCHPALSRISERTPLSNDDSNDGHARTDMGTCMOTSTRG";
#define ORDER_COUNT 23

void tables_init(TablesFile* t) { int i; t->n = 0; for (i = 0; i < TABLES_MAX; ++i) { t->s[i].tag = 0; buf_init(&t->s[i].data); } }

Buf* tables_add(TablesFile* t, const char* tag)
{
    int i;
    for (i = 0; i < ORDER_COUNT; ++i)
        if (!memcmp(ORDER + 4 * i, tag, 4)) {
            if (t->s[i].tag) { dw_fail("a Downwell.tables section was built twice"); return 0; }
            t->s[i].tag = tag_value(tag);
            ++t->n;
            return &t->s[i].data;
        }
    dw_fail("unknown Downwell.tables section");
    return 0;
}

int tables_pack(TablesFile* t, Buf* out)
{
    REQUIRE(t->n == ORDER_COUNT, "a Downwell.tables section is missing");
    u32 start = 8 + 12 * (u32)t->n, at = start, i;
    buf_put(out, "DWTB", 4); buf_u16(out, 1); buf_u16(out, (u32)t->n);
    for (i = 0; i < (u32)t->n; ++i) {
        at = (at + 3) & ~3u;
        buf_u32(out, t->s[i].tag); buf_u32(out, at); buf_u32(out, t->s[i].data.n);
        at += t->s[i].data.n;
    }
    for (i = 0; i < (u32)t->n; ++i) {
        buf_align(out, 0, 4);
        REQUIRE(!t->s[i].data.failed, "not enough memory");
        buf_put(out, t->s[i].data.p, t->s[i].data.n);
    }
    REQUIRE(!out->failed, "not enough memory");
    return 1;
}

/* Each built section as <tag>.bin, so tests can compare one at a time. */
int tables_partial(TablesFile* t, const char* dir)
{
    int i;
    for (i = 0; i < ORDER_COUNT; ++i)
        if (t->s[i].tag) {
            char name[12];
            memcpy(name, ORDER + 4 * i, 4); dw_strcpy(name + 4, ".bin");
            TRY(dw_write_file(dir, name, t->s[i].data.p, t->s[i].data.n));
        }
    return 1;
}

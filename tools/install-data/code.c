/* data.win CODE and SHDR: Downwell.patterns (level-builder patterns) and palettes (PALS).

   Both read GameMaker VM bytecode 17 straight from CODE, as UndertaleModTool
   decodes it: a code entry is {name, length, u16 locals, u16 arguments,
   s32 bytecode address relative to that field, u32 offset}; an instruction
   is a little-endian word {opcode:8, type2:4, type1:4, value:16}, followed
   by an operand for pushes (by type1), pops (unless type1 is Int16), calls
   and Int32 extended (break) instructions. push.s's operand indexes STRG.
   VARI's entries {name, instance, id, occurrences, first address} chain the
   instructions naming a variable: each operand's low 27 bits step to the
   next one. */
#include "code.h"

/* Opcodes and types (bytecode 15+). */
enum { OP_CONV = 0x07, OP_POP = 0x45, OP_PUSHI = 0x84, OP_PUSH = 0xC0, OP_PUSHLOC = 0xC1,
       OP_PUSHGLB = 0xC2, OP_PUSHBLTN = 0xC3, OP_CALL = 0xD9, OP_BREAK = 0xFF };
enum { T_DOUBLE = 0, T_FLOAT = 1, T_INT32 = 2, T_INT64 = 3, T_BOOL = 4, T_VAR = 5,
       T_STRING = 6, T_INT16 = 15 };

typedef struct { u32 start, end; } CodeRange;

static int code_find(Win* w, const char* name, CodeRange* r)
{
    u32 off, n, i;
    TRY(win_chunk(w, "CODE", &off, 0));
    n = win_list_count(w, off);
    for (i = 0; i < n; ++i) {
        u32 e = win_list_item(w, off, i);
        if (w->failed) return 0;
        if (win_streq(w, win_u32(w, e), name)) {
            r->start = e + 12 + (u32)win_s32(w, e + 12) + win_u32(w, e + 16);
            r->end = r->start + win_u32(w, e + 4);
            REQUIRE(r->end >= r->start && r->end <= w->size, "data.win is damaged (a code entry)");
            return !w->failed;
        }
    }
    return dw_fail("data.win lacks a code entry the converter needs");
}

/* An instruction's size in bytes from its first word (0: unknown). */
static u32 insn_size(u32 word)
{
    u32 op = word >> 24, t1 = (word >> 16) & 15;
    switch (op) {
    case OP_PUSH: case OP_PUSHLOC: case OP_PUSHGLB: case OP_PUSHBLTN: case OP_PUSHI:
        switch (t1) {
        case T_DOUBLE: case T_INT64: return 12;
        case T_FLOAT: case T_INT32: case T_BOOL: case T_VAR: case T_STRING: return 8;
        case T_INT16: return 4;
        default: return 0;
        }
    case OP_POP: return t1 == T_INT16 ? 4 : 8;
    case OP_CALL: return 8;
    case OP_BREAK: return t1 == T_INT32 ? 8 : 4;
    default: return 4;
    }
}

static int is_push_string(u32 word) { return word >> 24 == OP_PUSH && ((word >> 16) & 15) == T_STRING; }

/* The addresses in `r` of instructions naming variable `name` (any instance). */
#define OCC_MAX 64
typedef struct { u32 n, at[OCC_MAX]; } Occurrences;
static int var_occurrences(Win* w, const char* name, const CodeRange* r, Occurrences* o)
{
    u32 off, len, n, i;
    TRY(win_chunk(w, "VARI", &off, &len));
    REQUIRE(len >= 12, "data.win is damaged (VARI)");
    n = (len - 12) / 20;
    o->n = 0;
    for (i = 0; i < n; ++i) {
        u32 e = off + 12 + 20 * i, count, at, k;
        if (w->failed) return 0;
        if (!win_streq(w, win_u32(w, e), name)) continue;
        count = win_u32(w, e + 12);
        at = win_u32(w, e + 16);
        for (k = 0; k < count && !w->failed; ++k) {
            if (at >= r->start && at < r->end) {
                REQUIRE(o->n < OCC_MAX, "a variable is used more often than expected");
                o->at[o->n++] = at;
            }
            if (k + 1 < count) at += win_u32(w, at + 4) & 0x07ffffffu;
        }
    }
    return !w->failed;
}

static int occurs(const Occurrences* o, u32 at)
{
    u32 i;
    for (i = 0; i < o->n; ++i) if (o->at[i] == at) return 1;
    return 0;
}

/* STRG entry `id`: its offset (length, then the characters). */
static int strg_entry(Win* w, u32 id, u32* at, u32* n)
{
    u32 off;
    TRY(win_chunk(w, "STRG", &off, 0));
    REQUIRE(id < win_list_count(w, off), "data.win is damaged (a string index)");
    *at = win_list_item(w, off, id) + 4;
    *n = win_u32(w, *at - 4);
    REQUIRE(*at <= w->size && *n <= w->size - *at, "data.win is damaged (a string)");
    return !w->failed;
}

/* ---- Downwell.patterns (tools/generate-builder-pattern-pack.py) ---- */

enum { AREA_ONE_START = 1, AREA_ONE_MAIN = 2, AREA_ONE_RIGHT = 3, AREA_ONE_SHOP = 4, AREA_ONE_END = 5 };
enum { EXPECTED_PATTERNS = 62, EXPECTED_ROWS = 737, EXPECTED_CELLS = 8435, PATTERN_MAX = 41 };
typedef struct { const char* script; const char* array; int family, count, width, irregular; } Family;
static const Family FAMILIES[] = {
    {"gml_GlobalScript_scrStartPat1", "partStart", AREA_ONE_START, 5, 9, 0},
    {"gml_GlobalScript_scrLevelPat1", "partMain", AREA_ONE_MAIN, 41, 9, 0},
    {"gml_GlobalScript_scrRightPat1", "partRight", AREA_ONE_RIGHT, 10, 19, 0},
    {"gml_GlobalScript_scrShopPat1", "partShopR", AREA_ONE_SHOP, 2, 19, 1},
    {"gml_GlobalScript_scrEndPat1", "partEnd", AREA_ONE_END, 4, 9, 0},
};
#define FAMILY_COUNT (sizeof FAMILIES / sizeof FAMILIES[0])

static void le16(Buf* b, u32 v) { u8* d = buf_grow(b, 2); if (d) { d[0] = (u8)v; d[1] = (u8)(v >> 8); } }
static void le32(Buf* b, u32 v) { u8* d = buf_grow(b, 4); if (d) { d[0] = (u8)v; d[1] = (u8)(v >> 8); d[2] = (u8)(v >> 16); d[3] = (u8)(v >> 24); } }

/* The string ids pushed in order: each is stored into `array` (the next pop
   names it), and one more store sets array[100] = i. */
static int pattern_strings(Win* w, const Family* f, u32* ids)
{
    CodeRange r;
    Occurrences* o;
    u32 at, word, size, count = 0, pending = 0, pops = 0;
    int ok = 0;
    TRY(code_find(w, f->script, &r));
    o = dw_alloc(sizeof *o);
    if (!o) return 0;
    if (!var_occurrences(w, f->array, &r, o)) goto done;
    for (at = r.start; at < r.end; at += size) {
        word = win_u32(w, at);
        size = insn_size(word);
        if (!size || w->failed) { dw_fail("data.win has an unexpected instruction"); goto done; }
        if (is_push_string(word)) {
            if (pending || count >= (u32)f->count) { dw_fail("unexpected level-pattern script shape"); goto done; }
            ids[count++] = win_u32(w, at + 4);
            pending = 1;
        } else if (word >> 24 == OP_POP && occurs(o, at)) {
            ++pops;
            pending = 0;
        } else if (word >> 24 == OP_POP && pending) { dw_fail("unexpected level-pattern script shape"); goto done; }
    }
    if (at != r.end || pending || count != (u32)f->count || pops != count + 1 || o->n != pops) {
        dw_fail("unexpected level-pattern script shape");
        goto done;
    }
    ok = !w->failed;
done:
    dw_free(o);
    return ok;
}

/* Python's str.splitlines() and str.isspace() over ASCII. */
static int line_break(u8 ch) { return ch == '\n' || ch == '\r' || ch == 0x0b || ch == 0x0c || (ch >= 0x1c && ch <= 0x1e); }
static int space(u8 ch) { return ch == ' ' || (ch >= 0x09 && ch <= 0x0d) || (ch >= 0x1c && ch <= 0x1f); }
static int blank(const u8* p, u32 n) { while (n--) if (!space(*p++)) return 0; return 1; }

#define ROW_MAX 64
typedef struct { u32 at, n; } Row;

typedef struct {
    Buf* out;
    u8 creates[256][2];
    u32 ncreates, pending, draws, special;
} Prepared;

static int create(Prepared* p, int x, int platform, int border)
{
    int column = (x + 512) / 16;
    REQUIRE(column >= 0 && column < 128, "prepared terrain column outside grid");
    REQUIRE(p->pending < 128, "prepared draw span too long");
    REQUIRE(p->ncreates < 255, "prepared segment too large");
    p->creates[p->ncreates][0] = (u8)(column | (platform ? 128 : 0));
    p->creates[p->ncreates][1] = (u8)(p->pending | (border ? 128 : 0));
    ++p->ncreates;
    p->pending = 0;
    p->special += (u32)border;
    return 1;
}

static int flush(Prepared* p)
{
    u8 flags[128], walls[128];
    u32 i, words = 0, platforms = 0, most;
    int column;
    if (!p->draws && !p->ncreates) return 1;
    memset(flags, 0, sizeof flags); memset(walls, 0, sizeof walls);
    for (i = 0; i < p->ncreates; ++i) {
        u8 item = p->creates[i][0];
        column = item & 127;
        flags[column] |= item & 128 ? 34 : 19;
        if (!(item & 128)) ++walls[column]; else ++platforms;
    }
    for (column = 0; column < 128; column += 2) {
        REQUIRE(walls[column] <= 3 && walls[column + 1] <= 3, "prepared word contact count exceeds packed field");
        if (flags[column] || flags[column + 1]) ++words;
    }
    most = p->draws > p->special ? p->draws : p->special;
    if (p->ncreates > most) most = p->ncreates;
    if (words > most) most = words;
    REQUIRE(most < 255, "prepared segment too large");
    buf_u8(p->out, 0); buf_u8(p->out, p->draws); buf_u8(p->out, p->special);
    buf_u8(p->out, p->ncreates); buf_u8(p->out, words); buf_u8(p->out, platforms);
    buf_put(p->out, p->creates, 2 * p->ncreates);
    for (column = 0; column < 128; column += 2) {
        u32 a = flags[column] ? flags[column] | (u32)walls[column] << 6 : 0;
        u32 b = flags[column + 1] ? flags[column + 1] | (u32)walls[column + 1] << 6 : 0;
        if (!flags[column] && !flags[column + 1]) continue;
        buf_u8(p->out, (u32)column); buf_u8(p->out, (a ? 1u : 0u) | (b ? 2u : 0u));
        buf_u8(p->out, a); buf_u8(p->out, b);
    }
    p->ncreates = p->pending = p->draws = p->special = 0;
    return 1;
}

static int event_cell(u8 ch)
{
    static const char events[] = "abcdet%IJ^2B3vO_$\\=@";
    const char* e;
    for (e = events; *e; ++e) if ((u8)*e == ch) return 1;
    return 0;
}

static int prepared_row(Prepared* p, const u8* text, u32 n, int family, int reversed, int edge)
{
    u32 column;
    p->ncreates = p->pending = p->draws = p->special = 0;
    for (column = 0; column < n; ++column) {
        u8 symbol = text[column];
        int x = reversed ? 320 - (int)(column + 1) * 16 : 160 + (int)(column + 1) * 16;
        if (event_cell(symbol)) {
            TRY(flush(p));
            buf_u8(p->out, symbol); buf_u8(p->out, (u32)((x + 512) / 16));
            continue;
        }
        ++p->draws; ++p->pending;
        if (symbol == '1' || symbol == '|' || symbol == '-') TRY(create(p, x, symbol == '-', 0));
        else if (symbol == '{' || symbol == '}') TRY(create(p, reversed ? 16 : 464, 0, 1));
    }
    if (edge) {
        int side = family == AREA_ONE_RIGHT || family == AREA_ONE_SHOP;
        ++p->draws; ++p->pending;
        TRY(create(p, side ? (reversed ? 0 : 480) : (reversed ? 160 : 320), 0, 0));
        TRY(create(p, reversed ? 320 : 160, 0, 0));
    }
    TRY(flush(p));
    buf_u8(p->out, 255);
    return 1;
}

/* Lines of one pattern string, as the generator trims them. */
static int pattern_rows(const u8* s, u32 n, const Family* f, Row* rows, u32* nrows, u32* width)
{
    u32 i = 0, k, first, last, lo = 0xffffffffu, hi = 0;
    *nrows = 0;
    while (i < n) {
        u32 start = i;
        while (i < n && !line_break(s[i])) ++i;
        REQUIRE(*nrows < ROW_MAX, "a level pattern has too many rows");
        rows[*nrows].at = start; rows[*nrows].n = i - start; ++*nrows;
        if (i < n) { if (s[i] == '\r' && i + 1 < n && s[i + 1] == '\n') ++i; ++i; }
    }
    for (first = 0; first < *nrows && blank(s + rows[first].at, rows[first].n); ++first) continue;
    for (last = *nrows; last > first && blank(s + rows[last - 1].at, rows[last - 1].n); --last) continue;
    REQUIRE(last > first, "a level pattern is empty");
    for (k = first; k < last; ++k) {
        Row r = rows[k];
        if (r.n && s[r.at] == '\t') ++r.at, --r.n;
        rows[k - first] = r;
        if (r.n < lo) lo = r.n;
        if (r.n > hi) hi = r.n;
        for (i = 0; i < r.n; ++i) REQUIRE(s[r.at + i] >= 0x20 && s[r.at + i] <= 0x7e, "a level pattern contains a non-printable cell");
    }
    *nrows = last - first;
    if (f->irregular) REQUIRE(lo == (u32)f->width, "a level pattern has a short row");
    else REQUIRE(hi == (u32)f->width && lo == hi, "a level pattern has an unexpected width");
    *width = hi;
    return 1;
}

typedef struct {
    Buf records, rows, cells, programs, offsets;
    u32 ids[PATTERN_MAX];
    Row lines[ROW_MAX];
    Prepared prepared;
} Patterns;

static int patterns_build(Win* w, Patterns* t)
{
    u32 f, j, k, npatterns = 0, nrows = 0;
    Prepared* p = &t->prepared;
    p->out = &t->programs;
    for (f = 0; f < FAMILY_COUNT; ++f) {
        const Family* fam = &FAMILIES[f];
        TRY(pattern_strings(w, fam, t->ids));
        for (j = 0; j < (u32)fam->count; ++j) {
            u32 at, n, count = 0, width = 0, firstRow = nrows, firstCell = t->cells.n;
            u8* s;
            int ok;
            TRY(strg_entry(w, t->ids[j], &at, &n));
            s = dw_alloc(n + 1);
            if (!s) return 0;
            ok = win_read(w, at, s, n) && pattern_rows(s, n, fam, t->lines, &count, &width);
            for (k = 0; ok && k < count; ++k) {
                Row r = t->lines[k];
                le32(&t->rows, t->cells.n); le32(&t->rows, r.n);
                buf_put(&t->cells, s + r.at, r.n);
                ++nrows;
                le32(&t->offsets, t->programs.n);
                ok = prepared_row(p, s + r.at, r.n, fam->family, 0, k + 1 < count);
                le32(&t->offsets, t->programs.n);
                ok = ok && prepared_row(p, s + r.at, r.n, fam->family, 1, k + 1 < count);
            }
            dw_free(s);
            TRY(ok);
            le16(&t->records, (u32)fam->family); le16(&t->records, j); le16(&t->records, width); le16(&t->records, count);
            le32(&t->records, firstRow); le32(&t->records, firstCell); le32(&t->records, t->cells.n - firstCell);
            ++npatterns;
        }
    }
    REQUIRE(npatterns == EXPECTED_PATTERNS, "unexpected recovered pattern count");
    REQUIRE(nrows == EXPECTED_ROWS, "unexpected recovered row count");
    REQUIRE(t->cells.n == EXPECTED_CELLS, "unexpected recovered cell count");
    return 1;
}

/* The appended pillar pair: no RNG, the same wire format as rows. */
static void pillars(Buf* b, int reversed)
{
    static const u8 tail[] = {42, 1, 83, 0, 52, 1, 83, 0, 255};
    buf_u8(b, 0); buf_u8(b, 0); buf_u8(b, 0); buf_u8(b, 2); buf_u8(b, 2); buf_u8(b, 0);
    buf_u8(b, reversed ? 42 : 52); buf_u8(b, 0); buf_u8(b, reversed ? 52 : 42); buf_u8(b, 0);
    buf_put(b, tail, sizeof tail);
}

static int build_patterns(Context* c)
{
    /* data.win's SHA-256 (install.c verified it): the first 8 bytes. */
    static const u8 sourceHash[8] = {0xc0, 0x7d, 0xaf, 0x5f, 0xbe, 0x94, 0x0a, 0xe0};
    Patterns* t = dw_alloc(sizeof *t);
    u8* out = 0;
    u32 n = 0;
    int ok;
    if (!t) return 0;
    buf_init(&t->records); buf_init(&t->rows); buf_init(&t->cells); buf_init(&t->programs); buf_init(&t->offsets);
    ok = patterns_build(&c->win, t);
    if (ok) {
        le32(&t->offsets, t->programs.n); pillars(&t->programs, 0);
        le32(&t->offsets, t->programs.n); pillars(&t->programs, 1);
        le32(&t->offsets, t->programs.n);
        ok = !t->records.failed && !t->rows.failed && !t->cells.failed && !t->programs.failed && !t->offsets.failed;
        if (!ok) dw_fail("not enough memory");
    }
    if (ok) {
        /* The file in one exact allocation: header, patterns, rows, cells,
           "DWPR", program offsets, programs (little-endian). */
        Buf head;
        buf_init(&head);
        buf_put(&head, "DWBP", 4); le16(&head, 4); le16(&head, 32); le32(&head, 3);
        le32(&head, t->records.n / 20); le32(&head, t->cells.n);
        buf_put(&head, sourceHash, 8);
        le32(&head, t->rows.n / 8);
        n = head.n + t->records.n + t->rows.n + t->cells.n + 4 + t->offsets.n + t->programs.n;
        out = head.failed ? 0 : dw_alloc(n);
        ok = out != 0;
        if (ok) {
            u8* d = out;
            memcpy(d, head.p, head.n); d += head.n;
            memcpy(d, t->records.p, t->records.n); d += t->records.n;
            memcpy(d, t->rows.p, t->rows.n); d += t->rows.n;
            memcpy(d, t->cells.p, t->cells.n); d += t->cells.n;
            memcpy(d, "DWPR", 4); d += 4;
            memcpy(d, t->offsets.p, t->offsets.n); d += t->offsets.n;
            memcpy(d, t->programs.p, t->programs.n);
        }
        buf_free(&head);
    }
    buf_free(&t->records); buf_free(&t->rows); buf_free(&t->cells); buf_free(&t->programs); buf_free(&t->offsets);
    dw_free(t);
    ok = ok && dw_write_file(c->dest, "Downwell.patterns", out, n);
    dw_free(out);
    return ok;
}

/* ---- PALS (tools/generate-palettes.py) ---- */

#define PALETTE_MAX 64
typedef struct { s32 id; u32 name, nameLen; } Palette;

/* scrShaderListInit: shaderAr[i] = [shader id, name]; ids are the pushi.e
   followed by conv.i.v and pushi.e -5, names the pushed strings. */
static int palette_list(Win* w, Palette* list, u32* n)
{
    CodeRange r;
    Occurrences* o;
    u32 at, size, ids = 0, names = 0, prev[2] = {0, 0}, prevAt[2] = {0, 0}, i;
    int ok = 0, max = 0;
    TRY(code_find(w, "gml_GlobalScript_scrShaderListInit", &r));
    for (at = r.start; at < r.end; at += size) {
        u32 word = win_u32(w, at);
        size = insn_size(word);
        REQUIRE(size && !w->failed, "data.win has an unexpected instruction");
        /* The instruction two back, then conv.i.v, then pushi.e -5. */
        if (word == ((u32)OP_PUSHI << 24 | (u32)T_INT16 << 16 | 0xfffbu) && prevAt[1] &&
            prev[0] == ((u32)OP_CONV << 24 | (u32)T_VAR << 20 | (u32)T_INT32 << 16) &&
            (prev[1] & 0xffff0000u) == ((u32)OP_PUSHI << 24 | (u32)T_INT16 << 16)) {
            REQUIRE(ids < PALETTE_MAX, "too many palettes");
            list[ids++].id = (s16)(prev[1] & 0xffff);
        }
        if (is_push_string(word)) {
            u32 sat, sn;
            REQUIRE(names < PALETTE_MAX, "too many palettes");
            TRY(strg_entry(w, win_u32(w, at + 4), &sat, &sn));
            list[names].name = sat; list[names].nameLen = sn;
            ++names;
        }
        prev[1] = prev[0]; prevAt[1] = prevAt[0];
        prev[0] = word; prevAt[0] = at;
    }
    REQUIRE(at == r.end && ids == names && ids, "unexpected scrShaderListInit shape");
    o = dw_alloc(sizeof *o);
    if (!o) return 0;
    if (var_occurrences(w, "shaderArMax", &r, o)) {
        for (i = 0; i < o->n; ++i)
            if (win_u32(w, o->at[i]) == ((u32)OP_POP << 24 | (u32)T_VAR << 20 | (u32)T_VAR << 16 | 0xfffbu)) max = 1;
        ok = !w->failed;
    }
    dw_free(o);
    TRY(ok);
    REQUIRE(max, "missing shaderArMax");
    *n = ids;
    return 1;
}

static int find(const u8* s, u32 n, const char* what, u32 from)
{
    u32 k = (u32)strlen(what), i;
    for (i = from; i + k <= n; ++i) if (!memcmp(s + i, what, k)) return (int)i;
    return -1;
}

/* One component: digits[.digits], optionally "/256.0"; clamped to [0, 1],
   times 255, rounded half to even (Python's round of the exact double). */
static int component(const u8* s, u32 n, u32* eight)
{
    u32 num = 0, den = 1, i, digits = 0, dots = 0, frac = 0, t, q, rem;
    if (n > 6 && !memcmp(s + n - 6, "/256.0", 6)) { n -= 6; den = 256; }
    REQUIRE(n, "unsupported palette component");
    for (i = 0; i < n; ++i) {
        if (s[i] == '.') { ++dots; continue; }
        REQUIRE(s[i] >= '0' && s[i] <= '9', "unsupported palette component");
        if (!num && s[i] == '0' && !dots) continue;
        REQUIRE(++digits <= 7, "unsupported palette component");
        num = num * 10 + (u32)(s[i] - '0');
        if (dots) ++frac;
    }
    REQUIRE(dots <= 1 && n > dots, "unsupported palette component");
    for (i = 0; i < frac; ++i) den *= 10;
    REQUIRE(den <= 2560000u, "unsupported palette component");
    if (num >= den) { *eight = 255; return 1; }
    t = num * 255; q = t / den; rem = t % den;
    if (2 * rem > den || (2 * rem == den && (q & 1))) ++q;
    *eight = q;
    return 1;
}

/* A colour expression without whitespace: a named colour or vec3(r,g,b). */
static int colour(const u8* e, u32 n, u32* rgb)
{
    static const char* const named[][2] = {
        {"white", "vec3(1.0,1.0,1.0)"}, {"red", "vec3(1.0,0.0,0.0)"}, {"black", "vec3(0.0,0.0,0.0)"},
        {"green", "vec3(0.0,1.0,0.0)"}, {"blue", "vec3(0.0/256.0,126.0/256.0,256.0/256.0)"}};
    u32 i, k, start, part = 0, v[3];
    for (i = 0; i < sizeof named / sizeof named[0]; ++i)
        if (strlen(named[i][0]) == n && !memcmp(e, named[i][0], n)) { e = (const u8*)named[i][1]; n = (u32)strlen(named[i][1]); break; }
    REQUIRE(n > 6 && !memcmp(e, "vec3(", 5) && e[n - 1] == ')', "unsupported palette colour");
    for (start = k = 5; k <= n - 1; ++k)
        if (k == n - 1 || e[k] == ',') {
            REQUIRE(part < 3 && k > start, "unsupported palette colour");
            TRY(component(e + start, k - start, &v[part]));
            ++part; start = k + 1;
        }
    REQUIRE(part == 3, "unsupported palette colour");
    for (i = 0; i < 3; ++i) v[i] = (v[i] * 15 + 127) / 255;
    *rgb = v[0] << 8 | v[1] << 4 | v[2];
    return 1;
}

/* The last "^\s*color<key>\s*=\s*([^;]+);" in main (re.M), whitespace removed. */
static int colour_of(const u8* m, u32 n, char key, u32* rgb)
{
    u8 expr[128];
    u32 i, found = 0, len = 0;
    for (i = 0; i + 6 <= n; ++i) {
        u32 j, k, e;
        int line = 0;
        if (memcmp(m + i, "color", 5) || m[i + 5] != (u8)key) continue;
        for (j = i; j > 0 && space(m[j - 1]); --j) if (m[j - 1] == '\n') line = 1;
        if (!line && j != 0) continue;
        for (k = i + 6; k < n && space(m[k]); ++k) continue;
        if (k >= n || m[k] != '=') continue;
        for (++k; k < n && space(m[k]); ++k) continue;
        for (e = k; e < n && m[e] != ';'; ++e) continue;
        if (e >= n || e == k) continue;
        found = 1; len = 0;
        for (j = k; j < e; ++j)
            if (!space(m[j])) { REQUIRE(len < sizeof expr, "unsupported palette colour"); expr[len++] = m[j]; }
        i = e;
    }
    REQUIRE(found, "a palette shader lacks a colour");
    return colour(expr, len, rgb);
}

static int build_palettes(Context* c)
{
    Win* w = &c->win;
    Palette* list = dw_alloc(PALETTE_MAX * sizeof *list);
    Section s;
    Buf* table;
    u32 n = 0, i, shdr, shaders;
    int ok;
    if (!list) return 0;
    sec_init(&s);
    table = sec_table(&s);
    ok = palette_list(w, list, &n) && win_chunk(w, "SHDR", &shdr, 0);
    shaders = ok ? win_list_count(w, shdr) : 0;
    for (i = 0; ok && i < n; ++i) {
        u32 entry, ref, len, rgb[4], k;
        u8* frag;
        int at;
        char name[64];
        if (list[i].id < 0 || (u32)list[i].id >= shaders) { ok = dw_fail("a palette names an unknown shader"); break; }
        entry = win_list_item(w, shdr, (u32)list[i].id);
        ref = win_u32(w, entry + 12); /* GLSL ES fragment source */
        len = win_strlen(w, ref);
        if (len > 65536 || list[i].nameLen >= sizeof name) { ok = dw_fail("unexpected palette shader"); break; }
        frag = dw_alloc(len + 1);
        if (!frag) { ok = 0; break; }
        ok = win_read(w, ref, frag, len) && win_read(w, list[i].name, name, list[i].nameLen);
        at = ok ? find(frag, len, "void main", 0) : -1;
        if (ok && at < 0) ok = dw_fail("a palette shader lacks main");
        if (ok) {
            const u8* m = frag + at;
            u32 mn = len - (u32)at;
            for (k = 0; k < mn && ok; ++k) if (m[k] >= 0x80) ok = dw_fail("unexpected palette shader");
            if (ok && (find(m, mn, "whiteOrNot", 0) < 0 || find(m, mn, "redOrNot", 0) < 0 || find(m, mn, "blackOrNot", 0) < 0))
                ok = dw_fail("a palette shader is not a four-class palette shader");
            for (k = 0; k < 4 && ok; ++k) ok = colour_of(m, mn, "LMDS"[k], &rgb[k]);
        }
        dw_free(frag);
        for (k = 0; ok && k < list[i].nameLen; ++k)
            if ((u8)name[k] < 0x20 || (u8)name[k] > 0x7e || name[k] == '"' || name[k] == '\\') ok = dw_fail("unexpected palette name");
        if (!ok) break;
        sec_ptr_string(&s, 0, table->n, name, list[i].nameLen);
        buf_u32(table, 0); buf_u32(table, (u32)list[i].id);
        for (k = 0; k < 4; ++k) buf_u16(table, rgb[k]);
    }
    dw_free(list);
    if (ok) {
        Buf* out = tables_add(&c->tables, "PALS");
        ok = out && sec_finish(&s, out);
    }
    sec_free(&s);
    return ok;
}

int stage_code(Context* c)
{
    return build_patterns(c) && build_palettes(c);
}

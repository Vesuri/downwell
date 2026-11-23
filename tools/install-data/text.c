/* language.ini: area titles (ATTL), menu text (MTXT), shop speech (SPCH), rising texts (RISE).
   The generators read the file with Python's configparser (interpolation off,
   strict, '=' and ':' delimiters, '#' and ';' full-line comments, indented
   continuation lines, empty lines kept inside values); this parser follows
   those rules, including Unicode whitespace in str.strip() and \s. */
#include "text.h"

typedef struct { u32 section, key, keyLen, value, valueLen; } Option;
static Section sec; /* one section at a time; 64 table buffers are too many for the stack */
typedef struct { u32 name, nameLen, first, count; } IniSection;
typedef struct {
    Buf file;      /* language.ini without its BOM */
    Buf values;    /* each option's joined value */
    Buf options;   /* Option records, a section's contiguous */
    Buf sections;  /* IniSection records, in file order */
} Ini;

/* One UTF-8 character at p (< end): its code point and length. */
static u32 utf8(const u8* p, const u8* end, u32* len)
{
    u32 c = p[0], n = 1, i;
    if (c >= 0xf0 && c < 0xf8) { c &= 7; n = 4; }
    else if (c >= 0xe0) { c &= 15; n = 3; }
    else if (c >= 0xc0) { c &= 31; n = 2; }
    if (p + n > end) n = 1;
    for (i = 1; i < n; ++i) c = c << 6 | (p[i] & 63);
    *len = n;
    return c;
}

/* str.isspace(). */
static int space(u32 c)
{
    return (c >= 9 && c <= 13) || (c >= 0x1c && c <= 0x20) || c == 0x85 || c == 0xa0 || c == 0x1680 ||
           (c >= 0x2000 && c <= 0x200a) || c == 0x2028 || c == 0x2029 || c == 0x202f || c == 0x205f ||
           c == 0x3000;
}

/* str.strip() of [*s, *e); returns the leading whitespace in characters. */
static u32 strip(const u8* base, u32* s, u32* e)
{
    u32 at = *s, first = *e, last = *s, chars = 0, len;
    int seen = 0;
    while (at < *e) {
        u32 c = utf8(base + at, base + *e, &len);
        if (!space(c)) { if (!seen) first = at; seen = 1; last = at + len; }
        else if (!seen) ++chars;
        at += len;
    }
    if (!seen) { *s = *e; return chars; }
    *s = first; *e = last;
    return chars;
}

static u8 lower(u8 c) { return c >= 'A' && c <= 'Z' ? (u8)(c + 32) : c; }

static void ini_free(Ini* ini) { buf_free(&ini->file); buf_free(&ini->values); buf_free(&ini->options); buf_free(&ini->sections); }

static int ini_read(Context* c, Ini* ini)
{
    File f;
    u8* p;
    u32 at, n, indent = 0, optname = 0;
    IniSection* cur = 0;
    buf_init(&ini->file); buf_init(&ini->values); buf_init(&ini->options); buf_init(&ini->sections);
    TRY(ctx_open(c, "language.ini", &f));
    p = buf_grow(&ini->file, f.size);
    if (!p || (f.size && !io_read(&f, p, f.size))) { io_close(&f); return dw_fail("cannot read language.ini"); }
    io_close(&f);
    p = ini->file.p; n = ini->file.n;
    at = n >= 3 && p[0] == 0xef && p[1] == 0xbb && p[2] == 0xbf ? 3 : 0;
    while (at < n) {
        u32 end = at, s, e, lead;
        while (end < n && p[end] != '\n') ++end;
        s = at; e = end;
        at = end < n ? end + 1 : end;
        lead = strip(p, &s, &e);
        if (s < e && (p[s] == '#' || p[s] == ';')) continue; /* a comment line */
        if (s == e) { /* an empty line: kept in the value it may be inside */
            if (optname) buf_u8(&ini->values, '\n');
            continue;
        }
        if (cur && optname && lead > indent) { /* a continuation line */
            buf_u8(&ini->values, '\n');
            buf_put(&ini->values, p + s, e - s);
            continue;
        }
        indent = lead;
        {
            u32 close = e;
            while (close > s && p[close - 1] != ']') --close;
            if (p[s] == '[' && close >= s + 3) {
                IniSection* all = (IniSection*)ini->sections.p;
                u32 i, count = ini->sections.n / sizeof *all;
                for (i = 0; i < count; ++i)
                    REQUIRE(all[i].nameLen != close - 1 - (s + 1) || memcmp(p + all[i].name, p + s + 1, all[i].nameLen),
                            "language.ini repeats a section");
                REQUIRE(close - 1 - (s + 1) != 7 || memcmp(p + s + 1, "DEFAULT", 7), "language.ini has a DEFAULT section");
                cur = (IniSection*)buf_grow(&ini->sections, sizeof *cur);
                REQUIRE(cur, "not enough memory");
                cur->name = s + 1; cur->nameLen = close - 1 - (s + 1);
                cur->first = ini->options.n / sizeof(Option); cur->count = 0;
                optname = 0;
                continue;
            }
        }
        REQUIRE(cur, "language.ini lacks a section header");
        {
            u32 d = s, ks, ke, vs, ve, i;
            Option* o;
            while (d < e && p[d] != '=' && p[d] != ':') ++d;
            REQUIRE(d < e, "language.ini has a line that is not an option");
            ks = s; ke = d; strip(p, &ks, &ke);
            vs = d + 1; ve = e; strip(p, &vs, &ve);
            REQUIRE(ke > ks, "language.ini has an option without a name");
            o = (Option*)ini->options.p + cur->first;
            for (i = 0; i < cur->count; ++i) {
                u32 j;
                if (o[i].keyLen != ke - ks) continue;
                for (j = 0; j < o[i].keyLen && lower(p[o[i].key + j]) == lower(p[ks + j]); ++j) continue;
                REQUIRE(j < o[i].keyLen, "language.ini repeats an option");
            }
            o = (Option*)buf_grow(&ini->options, sizeof *o);
            REQUIRE(o, "not enough memory");
            cur = (IniSection*)ini->sections.p + ini->sections.n / sizeof *cur - 1;
            o->section = ini->sections.n / sizeof *cur - 1;
            o->key = ks; o->keyLen = ke - ks;
            o->value = ini->values.n; o->valueLen = 0;
            buf_put(&ini->values, p + vs, ve - vs);
            ++cur->count;
            optname = 1;
        }
    }
    REQUIRE(!ini->file.failed && !ini->values.failed && !ini->options.failed && !ini->sections.failed, "not enough memory");
    /* Each value: its lines joined by newlines, then rstrip(). */
    {
        Option* o = (Option*)ini->options.p;
        u32 i, count = ini->options.n / sizeof *o;
        for (i = 0; i < count; ++i) {
            u32 s = o[i].value, e = i + 1 < count ? o[i + 1].value : ini->values.n, at2 = s, last = s, len;
            while (at2 < e) {
                u32 ch = utf8(ini->values.p + at2, ini->values.p + e, &len);
                at2 += len;
                if (!space(ch)) last = at2;
            }
            o[i].valueLen = last - s;
        }
    }
    return 1;
}

static u32 section_count(const Ini* ini) { return ini->sections.n / sizeof(IniSection); }
static const IniSection* section_at(const Ini* ini, u32 i) { return (const IniSection*)ini->sections.p + i; }

static const IniSection* section_named(const Ini* ini, const char* name)
{
    u32 i, n = (u32)strlen(name);
    for (i = 0; i < section_count(ini); ++i) {
        const IniSection* s = section_at(ini, i);
        if (s->nameLen == n && !memcmp(ini->file.p + s->name, name, n)) return s;
    }
    dw_fail("language.ini lacks a language");
    return 0;
}

/* section.get(key, " "): the value's bytes (keys compare case-blind). */
static const u8* get(const Ini* ini, const IniSection* s, const char* key, u32* len)
{
    const Option* o = (const Option*)ini->options.p + s->first;
    u32 i, j, n = (u32)strlen(key);
    for (i = 0; i < s->count; ++i) {
        if (o[i].keyLen != n) continue;
        for (j = 0; j < n && lower(ini->file.p[o[i].key + j]) == lower((u8)key[j]); ++j) continue;
        if (j == n) { *len = o[i].valueLen; return ini->values.p + o[i].value; }
    }
    *len = 1;
    return (const u8*)" ";
}

/* A string pointer whose '*' (langString's newline) becomes '#'. */
static void ptr_hashed(Section* sec, int table, u32 at, const u8* v, u32 n, Buf* scratch)
{
    u32 i;
    scratch->n = 0;
    buf_put(scratch, v, n);
    for (i = 0; i < n && !scratch->failed; ++i)
        if (scratch->p[i] == '*') scratch->p[i] = '#';
    sec_ptr_string(sec, table, at, scratch->p, n);
}

static int finish(Section* sec, Context* c, const char* tag, Buf* scratch)
{
    Buf* out = tables_add(&c->tables, tag);
    int ok = out && !scratch->failed && sec_finish(sec, out);
    sec_free(sec);
    return ok;
}

/* ATTL (downwell-area-titles.h): text<i>, each title's code points (one 0
   when empty), then entries[] of AreaTitle {language, area, text, length}. */
static int area_titles(Context* c, const Ini* ini)
{
    Buf* entries;
    u32 l, area, i = 0;
    sec_init(&sec);
    for (l = 0; l < section_count(ini); ++l)
        for (area = 0; area < 6; ++area) {
            const IniSection* s = section_at(ini, l);
            char key[6] = {'a', 'r', 'e', 'a', (char)('0' + area), 0};
            u32 n, at = 0, len;
            const u8* v = get(ini, s, key, &n);
            Buf* t = sec_table(&sec);
            if (!t) { sec_free(&sec); return 0; }
            while (at < n) { buf_u32(t, utf8(v + at, v + n, &len)); at += len; }
            if (!n) buf_u32(t, 0);
        }
    entries = sec_table(&sec);
    if (!entries) { sec_free(&sec); return 0; }
    for (l = 0; l < section_count(ini); ++l)
        for (area = 0; area < 6; ++area, ++i) {
            const IniSection* s = section_at(ini, l);
            char key[6] = {'a', 'r', 'e', 'a', (char)('0' + area), 0};
            u32 n, at = 0, len, chars = 0;
            const u8* v = get(ini, s, key, &n);
            u32 base = entries->n;
            while (at < n) { utf8(v + at, v + n, &len); at += len; ++chars; }
            for (at = 0; at < s->nameLen; ++at)
                if (ini->file.p[s->name + at] < 32 || ini->file.p[s->name + at] > 126 || ini->file.p[s->name + at] == '"' ||
                    ini->file.p[s->name + at] == '\\') { sec_free(&sec); return dw_fail("language.ini names a language the converter does not expect"); }
            buf_u32(entries, 0); buf_u32(entries, area); buf_u32(entries, 0); buf_u32(entries, chars);
            sec_ptr_string(&sec, (int)(sec.ntables - 1), base, ini->file.p + s->name, s->nameLen);
            sec_ptr_table(&sec, (int)(sec.ntables - 1), base + 8, (int)i);
        }
    {
        Buf none;
        buf_init(&none);
        return finish(&sec, c, "ATTL", &none);
    }
}

static const char* const MENU_KEYS[] = {
    "menuPaused", "menuResume", "menuRetry", "menuStyleSelect", "menuPalette", "menuStats",
    "menuOption", "menuQuickStart", "menuQuit", "menuConfirmQuit", "menuConfirmReset",
    "menuYes", "menuNo", "menuBack", "menuBGM", "menuTimer", "menuVolume", "menuOn",
    "menuOff", "menuMaxed", "menuHardMode", "menuLocked", "statFurthest", "statGems",
    "statCombo", "statFastestTime",
    "area0", "area1", "area2", "area3", "area4", "area5",
    "styleName0", "styleName1", "styleName2", "styleName3", "styleName4",
    "styleDesc0", "styleDesc1", "styleDesc2", "styleDesc3", "styleDesc4"};
#define MENU_KEY_COUNT (sizeof MENU_KEYS / sizeof MENU_KEYS[0])

/* MTXT (downwell-menu-text.h): English ShopLanguageEntry {language, key, value}. */
static int menu_text(Context* c, const Ini* ini, Buf* scratch)
{
    const IniSection* s = section_named(ini, "english");
    Buf* t;
    u32 i, j;
    if (!s) return 0;
    sec_init(&sec);
    t = sec_table(&sec);
    for (i = 0; i < MENU_KEY_COUNT; ++i) {
        u32 n;
        const u8* v = get(ini, s, MENU_KEYS[i], &n);
        for (j = 0; j < n; ++j)
            if (v[j] < 32 || v[j] > 126) { sec_free(&sec); return dw_fail("language.ini's English menu text is outside the font"); }
        buf_u32(t, 0); buf_u32(t, 0); buf_u32(t, 0);
        sec_ptr_string(&sec, 0, 12 * i, "english", 7);
        sec_ptr_string(&sec, 0, 12 * i + 4, MENU_KEYS[i], (u32)strlen(MENU_KEYS[i]));
        ptr_hashed(&sec, 0, 12 * i + 8, v, n, scratch);
    }
    return finish(&sec, c, "MTXT", scratch);
}

static const char* const SPEECH_KEYS[] = {
    "shopSold", "shopGreet0", "shopGreet1", "shopGreet2", "shopBye0", "shopBye1",
    "shopBye2", "shopPoor", "shopWhat0", "shopWhat1"};
#define SPEECH_KEY_COUNT (sizeof SPEECH_KEYS / sizeof SPEECH_KEYS[0])

/* SPCH (downwell-shop-speech.h): every language's merchant speech. */
static int shop_speech(Context* c, const Ini* ini, Buf* scratch)
{
    Buf* t;
    u32 l, i, row = 0;
    sec_init(&sec);
    t = sec_table(&sec);
    for (l = 0; l < section_count(ini); ++l)
        for (i = 0; i < SPEECH_KEY_COUNT; ++i, ++row) {
            const IniSection* s = section_at(ini, l);
            u32 n;
            const u8* v = get(ini, s, SPEECH_KEYS[i], &n);
            buf_u32(t, 0); buf_u32(t, 0); buf_u32(t, 0);
            sec_ptr_string(&sec, 0, 12 * row, ini->file.p + s->name, s->nameLen);
            sec_ptr_string(&sec, 0, 12 * row + 4, SPEECH_KEYS[i], (u32)strlen(SPEECH_KEYS[i]));
            ptr_hashed(&sec, 0, 12 * row + 8, v, n, scratch);
        }
    return finish(&sec, c, "SPCH", scratch);
}

/* RISE (downwell-rising-texts.h): omnom[], English omnomText unquoted, upper case. */
static int rising_texts(Context* c, const Ini* ini, Buf* scratch)
{
    const IniSection* s = section_named(ini, "english");
    Buf* t;
    u32 n, b, e, i;
    const u8* v;
    if (!s) return 0;
    v = get(ini, s, "omnomText", &n);
    REQUIRE(v[0] != ' ' || n != 1, "language.ini lacks omnomText");
    b = 0; e = n;
    while (b < e && v[b] == '"') ++b;
    while (e > b && v[e - 1] == '"') --e;
    REQUIRE(e > b, "omnomText: unsupported rising text");
    sec_init(&sec);
    t = sec_table(&sec);
    for (i = b; i < e; ++i) {
        u8 ch = v[i] >= 'a' && v[i] <= 'z' ? (u8)(v[i] - 32) : v[i];
        if (!((ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == ' ' || ch == '!')) {
            sec_free(&sec);
            return dw_fail("omnomText: unsupported rising text");
        }
        buf_u8(t, ch);
    }
    buf_u8(t, 0);
    return finish(&sec, c, "RISE", scratch);
}

int stage_text(Context* c)
{
    Ini ini;
    Buf scratch;
    int ok;
    buf_init(&scratch);
    ok = ini_read(c, &ini) && area_titles(c, &ini) && menu_text(c, &ini, &scratch) &&
         shop_speech(c, &ini, &scratch) && rising_texts(c, &ini, &scratch);
    buf_free(&scratch);
    ini_free(&ini);
    return ok;
}

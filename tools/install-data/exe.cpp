/* Downwell.exe: the runner trig table and constants (STRG) and contact motion (CMOT).

   STRG (tools/export-smoke-trig.py, then install-table-headers.py's
   convert_program): the runner's SSE cos/sin paths at 0x74b638 and
   0x74b7f8 read a 64x4 binary64 table at 0x877630 and constants from
   .rdata; the section holds the table and the distinct constants in the
   order the two paths first read them.

   CMOT (tools/generate-contact-motion.cpp): the contact-motion decoder's
   cubic coefficients from 403x4 cosine samples, then every signed Q16
   atan result's exact speed against that decoder; the mismatches are the
   corrections. All in our integer binary64 (src/core), so the host and
   the 68020 compute the same bits. The generator's host doubles
   (-ffp-contract=off; long double is double on its arm64 host) round
   each operation once to nearest-even, and so do these. */
#ifndef INSTALL_AMIGA
#include <stdint.h>
#include <string.h>
#endif
extern "C" {
#include "exe.h"
}
#define DOWNWELL_INSTALLED_TABLES
#include "core/SmokeTrig.h"
#include "core/SmokeDirection.h"
#include "core/RunnerPointDirection.h"
#include "core/ContactMotionCodec.h"

using downwell::MotionBinary64;
using downwell::RunnerImageValue;

namespace {

/* .text and .rdata, from the PE section headers. */
struct Image {
    struct Mapped { u32 base, length, raw; } s[16];
    u32 n;
};

int image_read(File* f, const Image& image, u32 address, void* out, u32 bytes)
{
    for (u32 i = 0; i < image.n; ++i) {
        const Image::Mapped& m = image.s[i];
        if (m.base <= address && address - m.base <= m.length && bytes <= m.length - (address - m.base))
            return io_seek(f, m.raw + address - m.base) && io_read(f, out, bytes) ? 1 : dw_fail("cannot read Downwell.exe");
    }
    return dw_fail("Downwell.exe lacks the runner trig code");
}

int image_open(File* f, Image& image)
{
    u8 h[1024];
    REQUIRE(f->size >= sizeof h && io_seek(f, 0) && io_read(f, h, sizeof h) && h[0] == 'M' && h[1] == 'Z',
            "Downwell.exe is not a Windows program");
    const u32 pe = rd_le32(h + 60);
    REQUIRE(pe <= sizeof h - 24 - 96 && !memcmp(h + pe, "PE\0\0", 4), "Downwell.exe is not a Windows program");
    const u32 optional = pe + 24, count = rd_le16(h + pe + 6);
    const u32 sections = optional + rd_le16(h + pe + 20), base = rd_le32(h + optional + 28);
    REQUIRE(count <= 16 && sections + 40 * count <= sizeof h, "Downwell.exe has unexpected sections");
    image.n = count;
    for (u32 i = 0; i < count; ++i) {
        const u8* s = h + sections + 40 * i;
        image.s[i].base = base + rd_le32(s + 12);
        image.s[i].length = rd_le32(s + 16);
        image.s[i].raw = rd_le32(s + 20);
    }
    return 1;
}

/* Each SSE instruction of the two paths that reads a constant: its address,
   where its disp32 points and how many binary64 lanes it reads, in program
   order (cosine path, then sine). The disp32 sits 4 bytes in (prefix,
   0f, opcode, modrm) and is checked against the code. */
struct ConstantRead { u32 at, address, lanes; };
const ConstantRead READS[] = {
    {0x74b638, 0x871510, 1}, {0x74b644, 0x871518, 1}, {0x74b654, 0x871530, 1}, {0x74b660, 0x871520, 2},
    {0x74b67d, 0x871500, 2}, {0x74b698, 0x871538, 1}, {0x74b6bd, 0x8714e0, 2}, {0x74b702, 0x8714f0, 2},
    {0x74b70e, 0x8714d0, 2},
    {0x74b7f8, 0x8715c0, 1}, {0x74b804, 0x8715c8, 1}, {0x74b814, 0x8715e0, 1}, {0x74b820, 0x8715d0, 2},
    {0x74b83d, 0x8715b0, 2}, {0x74b858, 0x8715e8, 1}, {0x74b87d, 0x871590, 2}, {0x74b8c2, 0x8715a0, 2},
    {0x74b8ce, 0x871580, 2},
};
const u32 TABLE = 0x877630;
enum { CONSTANTS_MAX = 32 };

/* Native-order copies the trig program reads (SmokeTrig.h's installed form). */
struct Trig {
    std::uint32_t table[64][4][2];
    std::uint32_t constants[CONSTANTS_MAX][2];
    u32 count;
};

int read_trig(Context* c, Trig& t)
{
    File f;
    Image image;
    TRY(ctx_open(c, "Downwell.exe", &f));
    int ok = image_open(&f, image);
    u8 b[2048];
    if (ok) ok = image_read(&f, image, TABLE, b, sizeof b);
    for (u32 i = 0; ok && i < 256; ++i) {
        t.table[i / 4][i % 4][0] = rd_le32(b + 8 * i + 4);
        t.table[i / 4][i % 4][1] = rd_le32(b + 8 * i);
    }
    t.count = 0;
    for (u32 i = 0; ok && i < sizeof READS / sizeof READS[0]; ++i) {
        u8 code[8], v[16];
        ok = image_read(&f, image, READS[i].at, code, 8) && image_read(&f, image, READS[i].address, v, 16);
        if (ok && rd_le32(code + 4) != READS[i].address) ok = dw_fail("Downwell.exe's trig code is not the expected one");
        for (u32 lane = 0; ok && lane < READS[i].lanes; ++lane) {
            const std::uint32_t high = rd_le32(v + 8 * lane + 4), low = rd_le32(v + 8 * lane);
            u32 k = 0;
            while (k < t.count && (t.constants[k][0] != high || t.constants[k][1] != low)) ++k;
            if (k < t.count) continue;
            if (t.count == CONSTANTS_MAX) { ok = dw_fail("too many trig constants"); break; }
            t.constants[k][0] = high; t.constants[k][1] = low;
            ++t.count;
        }
    }
    io_close(&f);
    if (ok && t.count != downwell::smoke_trig_detail::smokeTrigConstants_count)
        ok = dw_fail("Downwell.exe's trig constants do not match the game");
    return ok;
}

int trig_section(Context* c, const Trig& t)
{
    Section s;
    sec_init(&s);
    Buf* table = sec_table(&s);
    Buf* constants = sec_table(&s);
    for (u32 i = 0; i < 256; ++i) { buf_u32(table, t.table[i / 4][i % 4][0]); buf_u32(table, t.table[i / 4][i % 4][1]); }
    for (u32 i = 0; i < t.count; ++i) { buf_u32(constants, t.constants[i][0]); buf_u32(constants, t.constants[i][1]); }
    Buf* out = tables_add(&c->tables, "STRG");
    const int ok = out && sec_finish(&s, out);
    sec_free(&s);
    return ok;
}

/* Binary64 arithmetic the coefficient fit needs, each rounded once to
   nearest-even like the generator's host doubles. */
MotionBinary64 negated(MotionBinary64 a) { a.high ^= 0x80000000u; return a; }
bool sub(MotionBinary64& a, MotionBinary64 b) { return a.add(negated(b)); }
bool times(MotionBinary64& a, std::int32_t k)
{
    MotionBinary64 out;
    if (!downwell::multiplyBinary64(a, MotionBinary64::fromInteger(k), out)) return false;
    a = out; return true;
}
/* Exact scaling by 2^k of a normal (or zero) value. */
bool scaled(MotionBinary64& a, int k)
{
    if (a.zero()) return true;
    const int e = int((a.high >> 20) & 2047u) + k;
    if (e <= 0 || e >= 2047) return false;
    a.high = (a.high & 0x800fffffu) | (unsigned(e) << 20);
    return true;
}
/* a / 3: the 53-bit significand shifted left 11, divided in 16-bit limbs. */
bool third(MotionBinary64& a)
{
    if (!a.valid()) return false;
    if (a.zero()) return true;
    const unsigned exponent = (a.high >> 20) & 2047u;
    const MotionBinary64::Words n = MotionBinary64::Words{(a.high & 0xfffffu) | 0x100000u, a.low}.left(11);
    const std::uint32_t limbs[4] = {n.high >> 16, n.high & 65535u, n.low >> 16, n.low & 65535u};
    std::uint32_t q[4], r = 0;
    for (unsigned i = 0; i < 4; ++i) { const std::uint32_t v = (r << 16) | limbs[i]; q[i] = v / 3u; r = v % 3u; }
    MotionBinary64::Words m{(q[0] << 16) | q[1], (q[2] << 16) | q[3]};
    const unsigned top = 63 - MotionBinary64::Words::leadingZeros32(m.high); /* bit 61 or 62 */
    const unsigned shift = top - 55;
    m = m.rightJam(shift);
    m.low |= unsigned(r != 0);
    const unsigned tail = m.low & 7;
    const bool up = tail > 4 || (tail == 4 && (m.low & 8));
    m = {m.high >> 3, (m.low >> 3) | (m.high << 29)};
    if (up) m.add({0, 1});
    int e = int(exponent) - 11 + int(shift) + 3;
    if (m.high & 0x200000u) { m = m.rightJam(1); ++e; }
    if (e <= 0 || e >= 2047) return false;
    a.high = (a.high & 0x80000000u) | (unsigned(e) << 20) | (m.high & 0xfffffu);
    a.low = m.low;
    return true;
}
/* llround(a * 2^46): to nearest, ties away from zero. */
bool fixed46(MotionBinary64 a, std::int64_t& out)
{
    if (!a.valid()) return false;
    if (a.zero()) { out = 0; return true; }
    const int e = int((a.high >> 20) & 2047u) - 1075 + 46;
    MotionBinary64::Words m{(a.high & 0xfffffu) | 0x100000u, a.low};
    if (e > 9) return false;
    if (e >= 0) m = m.left(unsigned(e));
    else if (-e >= 64) m = {0, 0};
    else {
        const unsigned k = unsigned(-e);
        m.add(MotionBinary64::Words{0, 1}.left(k - 1));
        if (k >= 32) m = {0, m.high >> (k - 32)};
        else m = {m.high >> k, (m.low >> k) | (k ? m.high << (32 - k) : 0)};
    }
    const std::int64_t v = std::int64_t((std::uint64_t(m.high) << 32) | m.low);
    out = (a.high & 0x80000000u) ? -v : v;
    return true;
}

int coefficient_fit(std::int64_t (*coefficients)[4])
{
    for (unsigned segment = 0; segment < 403; ++segment) {
        MotionBinary64 s[4];
        for (unsigned j = 0; j < 4; ++j) {
            /* (segment + j/4.0)/64, exactly */
            MotionBinary64 radians = MotionBinary64::fromInteger(std::int32_t(4 * segment + j));
            REQUIRE(scaled(radians, -8) && downwell::smokeCosine(radians, s[j]), "the contact motion fit failed");
        }
        MotionBinary64 d1 = s[1], d2 = s[2], d3 = s[3], t;
        bool ok = sub(d1, s[0]);
        t = s[1]; ok = ok && times(t, 2) && sub(d2, t) && d2.add(s[0]);
        t = s[2]; ok = ok && times(t, 3) && sub(d3, t);
        t = s[1]; ok = ok && times(t, 3) && d3.add(t) && sub(d3, s[0]);
        MotionBinary64 p[4] = {s[0], d1, d2, d3};
        t = d2; ok = ok && scaled(t, -1) && sub(p[1], t);
        t = d3; ok = ok && third(t) && p[1].add(t) && scaled(p[1], 2);
        ok = ok && sub(p[2], d3) && scaled(p[2], 3);
        ok = ok && scaled(p[3], 6) && third(p[3]) && scaled(p[3], -1);
        for (unsigned j = 0; ok && j < 4; ++j) ok = fixed46(p[j], coefficients[segment][j]);
        REQUIRE(ok, "the contact motion fit failed");
    }
    return 1;
}

typedef downwell::ContactMotionCodec::Correction Correction;

/* Sorted by radians; a repeated input keeps its last speed (std::map). */
int note(Buf* list, std::uint32_t radians, std::uint32_t speed)
{
    Correction* c = (Correction*)list->p;
    u32 count = list->n / sizeof(Correction), begin = 0, end = count;
    while (begin < end) { const u32 mid = begin + (end - begin) / 2; if (c[mid].radians < radians) begin = mid + 1; else end = mid; }
    if (begin < count && c[begin].radians == radians) { c[begin].speed = speed; return 1; }
    REQUIRE(buf_grow(list, sizeof(Correction)), "not enough memory");
    c = (Correction*)list->p;
    for (u32 i = count; i > begin; --i) c[i] = c[i - 1];
    c[begin].radians = radians; c[begin].speed = speed;
    return 1;
}

/* table_atan_detail::rounded: an unsigned Q30 angle to binary32. */
std::uint32_t q30_binary32(std::uint32_t value)
{
    if (!value) return 0;
    const unsigned top = 31 - MotionBinary64::Words::leadingZeros32(value);
    std::uint32_t mantissa;
    unsigned exponent = top + 97;
    if (top <= 23) mantissa = value << (23 - top);
    else {
        const unsigned shift = top - 23;
        mantissa = value >> shift;
        const unsigned tail = value & ((1u << shift) - 1), half = 1u << (shift - 1);
        if (tail > half || (tail == half && (mantissa & 1u))) ++mantissa;
        if (mantissa == 0x1000000u) { mantissa >>= 1; ++exponent; }
    }
    return (exponent << 23) | (mantissa & 0x7fffffu);
}

/* One signed Q16 atan result (Q16 angle q, sign): the speed the program
   gives and the speed the decoder predicts; a difference becomes a
   correction. An input that needs one must keep its speed through binary32
   (the generator's certification of the final decoder). */
int search_one(const std::int64_t (*coefficients)[4], std::uint32_t q, std::uint32_t sign, Buf* list)
{
    const MotionBinary64 source = RunnerImageValue{q30_binary32(q << 14) | (sign << 31)}.promoted();
    MotionBinary64 direction, cosine, expected, predicted;
    RunnerImageValue angle, radians, speed;
    const bool ok = downwell::RunnerPointDirection::evaluate({}, {}, MotionBinary64::fromInteger(1), MotionBinary64::fromInteger(1),
            direction, [&](MotionBinary64, MotionBinary64, MotionBinary64& result) { result = source; return true; }) &&
        angle.assign(direction) && downwell::SmokeDirection::radians(angle, radians) &&
        downwell::smokeCosine(radians.promoted(), cosine) &&
        downwell::SmokeDirection::component({0x40000000u}, cosine, false, expected) && speed.assign(expected) &&
        downwell::ContactMotionCodec::decode(radians, coefficients, nullptr, 0, predicted);
    REQUIRE(ok, "the contact motion search failed");
    if (expected.high == predicted.high && expected.low == predicted.low) return 1;
    const MotionBinary64 back = speed.promoted();
    REQUIRE(back.high == expected.high && back.low == expected.low, "a contact motion correction is not exact");
    return note(list, radians.bits, speed.bits);
}

/* Every signed Q16 atan result up to pi (TableAtan.h's domain). */
int corrections_search(const std::int64_t (*coefficients)[4], Buf* list)
{
    /* round(pi * 65536), generated/atan-q16.inc; the amiga-cmot-probe
       build stops at INSTALL_CMOT_PROBE instead. */
#ifdef INSTALL_CMOT_PROBE
    const std::uint32_t pi = INSTALL_CMOT_PROBE;
#else
    const std::uint32_t pi = 205887u;
#endif
    for (std::uint32_t q = 0; q <= pi; ++q) {
        if (!(q & 63) && io_cancelled()) return dw_fail("cancelled");
        TRY(search_one(coefficients, q, 0, list) && search_one(coefficients, q, 1, list));
    }
    return 1;
}

int motion_section(Context* c)
{
    const u32 bytes = 403 * 4 * sizeof(std::int64_t);
    std::int64_t (*coefficients)[4] = (std::int64_t (*)[4])dw_alloc(bytes);
    if (!coefficients) return 0;
    Buf list;
    buf_init(&list);
    int ok = coefficient_fit(coefficients) && corrections_search(coefficients, &list) && !list.failed;
    if (ok) {
        Section s;
        sec_init(&s);
        Buf* table = sec_table(&s);
        Buf* corrections = sec_table(&s);
        for (u32 i = 0; i < 403 * 4; ++i) {
            const std::uint64_t v = std::uint64_t(coefficients[i / 4][i % 4]);
            buf_u32(table, u32(v >> 32)); buf_u32(table, u32(v));
        }
        const Correction* e = (const Correction*)list.p;
        for (u32 i = 0; i < list.n / sizeof(Correction); ++i) { buf_u32(corrections, e[i].radians); buf_u32(corrections, e[i].speed); }
        Buf* out = tables_add(&c->tables, "CMOT");
        ok = out && sec_finish(&s, out);
        sec_free(&s);
    }
    buf_free(&list);
    dw_free(coefficients);
    return ok;
}

} // namespace

extern "C" int stage_exe(Context* c)
{
    Trig* t = (Trig*)dw_alloc(sizeof(Trig));
    if (!t) return 0;
    int ok = read_trig(c, *t) && trig_section(c, *t);
    if (ok) {
        downwell::smoke_trig_detail::smokeTrigTable = t->table;
        downwell::smoke_trig_detail::smokeTrigConstants = t->constants;
        ok = motion_section(c);
        downwell::smoke_trig_detail::smokeTrigTable = nullptr;
        downwell::smoke_trig_detail::smokeTrigConstants = nullptr;
    }
    dw_free(t);
    return ok;
}

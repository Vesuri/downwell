#include "context.h"

int sounds_read(Win* w, Sounds* out)
{
    u32 off, n, i;
    TRY(win_chunk(w, "SOND", &off, 0));
    n = win_list_count(w, off);
    REQUIRE(n <= SOUND_MAX, "data.win has more sounds than expected");
    out->count = (int)n;
    for (i = 0; i < n; ++i) {
        u32 e = win_list_item(w, off, i);
        SoundInfo* s = &out->s[i];
        TRY(win_string(w, win_u32(w, e), s->name, sizeof s->name));
        TRY(win_string(w, win_u32(w, e + 12), s->file, sizeof s->file));
        s->volumeBits = win_u32(w, e + 20);
        s->group = (int)win_s32(w, e + 28);
        s->audioId = (int)win_s32(w, e + 32);
    }
    return !w->failed;
}

static const char* const groupFile[7] = {
    "data.win", "audiogroup1.dat", "audiogroup2.dat", "audiogroup3.dat",
    "audiogroup4.dat", "audiogroup5.dat", "audiogroup6.dat"};

/* AUDO entry `index` of a FORM file: its payload offset and size. */
static int audo_entry(File* f, u32 index, u32* off, u32* size)
{
    u8 b[8];
    u32 pos = 8, count;
    REQUIRE(io_seek(f, 0) && io_read(f, b, 8) && !memcmp(b, "FORM", 4), "an audio group is not a GameMaker FORM");
    for (;;) {
        REQUIRE(pos + 8 <= f->size && io_seek(f, pos) && io_read(f, b, 8), "an audio group lacks its AUDO chunk");
        if (!memcmp(b, "AUDO", 4)) break;
        pos += 8 + rd_le32(b + 4);
    }
    REQUIRE(io_read(f, b, 4), "cannot read an audio group");
    count = rd_le32(b);
    REQUIRE(index < count && io_seek(f, pos + 12 + 4 * index) && io_read(f, b, 4), "an audio group lacks a sound");
    *off = rd_le32(b);
    REQUIRE(io_seek(f, *off) && io_read(f, b, 4), "cannot read an audio group");
    *size = rd_le32(b);
    *off += 4;
    REQUIRE(*size <= f->size && *off <= f->size - *size, "an audio group is damaged");
    return 1;
}

int sound_payload(Context* c, int id, SoundPayload* out)
{
    const SoundInfo* s = &c->sounds.s[id];
    File f;
    int ok;
    REQUIRE(id >= 0 && id < c->sounds.count, "no such sound");
    if (s->audioId < 0) {
        out->container = s->file;
        TRY(ctx_open(c, s->file, &f));
        out->off = 0; out->size = f.size;
        io_close(&f);
        return 1;
    }
    REQUIRE(s->group >= 0 && s->group < 7, "a sound names an unknown audio group");
    out->container = groupFile[s->group];
    TRY(ctx_open(c, out->container, &f));
    ok = audo_entry(&f, (u32)s->audioId, &out->off, &out->size);
    io_close(&f);
    return ok;
}

/* WAV: data bytes / block align. Ogg: the last page's granule position. */
int sound_length(Context* c, int id, u32* frames, u32* rate)
{
    SoundPayload p;
    File f;
    u8 b[64];
    TRY(sound_payload(c, id, &p));
    TRY(ctx_open(c, p.container, &f));
    if (!io_seek(&f, p.off) || !io_read(&f, b, 12)) { io_close(&f); return dw_fail("cannot read a sound"); }
    if (!memcmp(b, "RIFF", 4) && !memcmp(b + 8, "WAVE", 4)) {
        u32 pos = 12, align = 0;
        *rate = 0;
        while (pos + 8 <= p.size) {
            u32 n;
            if (!io_seek(&f, p.off + pos) || !io_read(&f, b, 8)) break;
            n = rd_le32(b + 4);
            if (!memcmp(b, "fmt ", 4) && n >= 16 && io_read(&f, b + 8, 16)) { *rate = rd_le32(b + 12); align = rd_le16(b + 20); }
            else if (!memcmp(b, "data", 4)) { io_close(&f); REQUIRE(align && *rate, "a WAV sound lacks its format"); *frames = n / align; return 1; }
            pos += 8 + n + (n & 1);
        }
        io_close(&f);
        return dw_fail("a WAV sound lacks its data");
    }
    if (!memcmp(b, "OggS", 4)) {
        /* The identification header on the first page gives the rate; the
           last page header in the file gives the granule position. */
        enum { TAIL = 65536 + 300 };
        u32 tail = p.size < TAIL ? p.size : TAIL, k;
        u8* d;
        if (!io_seek(&f, p.off + 28) || !io_read(&f, b, 30) || memcmp(b + 1, "vorbis", 6)) { io_close(&f); return dw_fail("an Ogg sound is not Vorbis"); }
        *rate = rd_le32(b + 12);
        d = dw_alloc(tail);
        if (!d) { io_close(&f); return 0; }
        if (!io_seek(&f, p.off + p.size - tail) || !io_read(&f, d, tail)) { dw_free(d); io_close(&f); return dw_fail("cannot read a sound"); }
        io_close(&f);
        for (k = tail - 27 + 1; k-- > 0;)
            if (!memcmp(d + k, "OggS", 4) && d[k + 4] == 0) {
                int ok = !d[k + 10] && !d[k + 11] && !d[k + 12] && !d[k + 13];
                *frames = rd_le32(d + k + 6);
                dw_free(d);
                REQUIRE(ok, "an Ogg sound is too long");
                return 1;
            }
        dw_free(d);
        return dw_fail("an Ogg sound lacks its last page");
    }
    io_close(&f);
    return dw_fail("a sound is neither WAV nor Ogg");
}

static void double_bits(u32 f, u32* hi, u32* lo)
{
    u32 sign = f & 0x80000000u, e = (f >> 23) & 0xff, m = f & 0x7fffff;
    if (e == 0 && m == 0) { *hi = sign; *lo = 0; return; }
    if (e == 0xff) { *hi = sign | 0x7ff00000u | (m >> 3); *lo = m << 29; return; }
    if (e == 0) { int shift = 0; while (!(m & 0x800000)) { m <<= 1; ++shift; } m &= 0x7fffff; e = 1 - (u32)shift; }
    e = e - 127 + 1023;
    *hi = sign | e << 20 | m >> 3;
    *lo = m << 29;
}

int sound_tables(Context* c)
{
    Section ticks, gains;
    Buf *t, *g, *out;
    int i;
    sec_init(&ticks); sec_init(&gains);
    t = sec_table(&ticks); g = sec_table(&gains);
    for (i = 0; i < c->sounds.count; ++i) {
        u32 frames, rate, q, r, n, hi, lo;
        if (!sound_length(c, i, &frames, &rate)) { sec_free(&ticks); sec_free(&gains); return 0; }
        q = frames / rate; r = frames % rate;
        n = q * 60 + (r * 60 + rate - 1) / rate;
        buf_u16(t, n > 65535 ? 65535 : n);
        double_bits(c->sounds.s[i].volumeBits, &hi, &lo);
        buf_u32(g, hi); buf_u32(g, lo);
    }
    out = tables_add(&c->tables, "SNDD");
    i = out && sec_finish(&ticks, out);
    out = tables_add(&c->tables, "SNDG");
    i = i && out && sec_finish(&gains, out);
    sec_free(&ticks); sec_free(&gains);
    return i;
}

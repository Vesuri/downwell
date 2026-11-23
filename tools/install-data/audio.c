/* data.win SOND and AUDO, sfxfalling.ogg: the sound banks (Downwell.sndrun,
   .sndfront, .sndresults, .sndclear).

   Each kept sound (audio_plan.h, from docs/sound-inventory.md and
   tools/sfx-bank-policy.json) is decoded to 16-bit mono (WAV, or Ogg Vorbis
   through Tremor), resampled to its Paula rate by our polyphase FIR, cut
   (one-shots cropped after their last loud window, long sounds to a head,
   a swell window or a crossfaded loop), peak-normalised to signed 8 bits and
   packed in tools/pack-amiga-sfx.py's DWSX format, one pack per scene bank.
   Integer arithmetic throughout: the host and the 68020 write the same bytes,
   and these samples are the reference (the user's choice, 2026-11-23). */
#include "audio.h"
#include "audio_plan.h"
#include "vorbis_heap.h"
#include "tremor/ivorbisfile.h"

typedef struct { s16* p; u32 n; } Pcm;

static void pcm_free(Pcm* x) { dw_free(x->p); x->p = 0; x->n = 0; }

/* ---- decoding ---------------------------------------------------------- */

typedef struct { const u8* p; long n, at; } Memory;

static size_t memory_read(void* d, size_t size, size_t count, void* source)
{
    Memory* m = source;
    long want = (long)(size * count), left = m->n - m->at;
    if (want > left) want = left;
    memcpy(d, m->p + m->at, (size_t)want);
    m->at += want;
    return (size_t)want / size;
}

static OggVorbis_File vorbis;

static int decode_ogg(const u8* data, u32 size, Pcm* out)
{
    enum { WORK = 256 * 1024, CHUNK = 4096 };
    ov_callbacks callbacks = {memory_read, 0, 0, 0};
    Memory m;
    Buf pcm;
    void* work = dw_alloc(WORK);
    int section, ok = 1;
    long n;
    vorbis_info* info;
    if (!work) return 0;
    vh_init(work, WORK);
    m.p = data; m.n = (long)size; m.at = 0;
    if (ov_open_callbacks(&m, &vorbis, 0, 0, callbacks)) { dw_free(work); return dw_fail("an Ogg sound does not decode"); }
    info = ov_info(&vorbis, -1);
    if (!info || info->channels != 1 || info->rate != (long)SFX_SOURCE_RATE) { ov_clear(&vorbis); dw_free(work); return dw_fail("an Ogg sound is not 44.1 kHz mono"); }
    buf_init(&pcm);
    for (;;) {
        u8* d = buf_grow(&pcm, CHUNK);
        if (!d) { ok = 0; break; }
        n = ov_read(&vorbis, (char*)d, CHUNK, &section);
        pcm.n -= CHUNK - (n > 0 ? (u32)n : 0);
        if (n <= 0) { if (n < 0) ok = dw_fail("an Ogg sound is damaged"); break; }
    }
    ov_clear(&vorbis);
    dw_free(work);
    if (!ok) { buf_free(&pcm); return 0; }
    out->p = (s16*)pcm.p;
    out->n = pcm.n / 2;
    return 1;
}

static int decode_wav(const u8* d, u32 size, Pcm* out)
{
    u32 pos = 12, rate = 0, channels = 0, bits = 0, format = 0, i;
    while (pos + 8 <= size) {
        u32 n = rd_le32(d + pos + 4);
        if (n > size - pos - 8) break;
        if (!memcmp(d + pos, "fmt ", 4) && n >= 16) {
            format = rd_le16(d + pos + 8); channels = rd_le16(d + pos + 10);
            rate = rd_le32(d + pos + 12); bits = rd_le16(d + pos + 22);
        } else if (!memcmp(d + pos, "data", 4)) {
            REQUIRE(format == 1 && channels == 1 && bits == 16 && rate == SFX_SOURCE_RATE, "a WAV sound is not 44.1 kHz 16-bit mono");
            out->n = n / 2;
            out->p = dw_alloc(out->n * 2 + 2);
            if (!out->p) return 0;
            for (i = 0; i < out->n; ++i) out->p[i] = (s16)rd_le16(d + pos + 8 + 2 * i);
            return 1;
        }
        pos += 8 + n + (n & 1);
    }
    return dw_fail("a WAV sound lacks its data");
}

static int decode(Context* c, int id, Pcm* out)
{
    SoundPayload p;
    File f;
    u8* data;
    int ok;
    TRY(sound_payload(c, id, &p));
    data = dw_alloc(p.size);
    if (!data) return 0;
    if (!ctx_open(c, p.container, &f)) { dw_free(data); return 0; }
    ok = io_seek(&f, p.off) && io_read(&f, data, p.size);
    io_close(&f);
    if (!ok) { dw_free(data); return dw_fail("cannot read a sound"); }
    if (p.size >= 12 && !memcmp(data, "RIFF", 4) && !memcmp(data + 8, "WAVE", 4)) ok = decode_wav(data, p.size, out);
    else if (p.size >= 4 && !memcmp(data, "OggS", 4)) ok = decode_ogg(data, p.size, out);
    else ok = dw_fail("a sound is neither WAV nor Ogg");
    dw_free(data);
    return ok;
}

/* ---- resampling -------------------------------------------------------- */

/* Output sample k sits at input position k * 44100 * 424 / 3546895, kept as
   a whole part and an exact remainder. Its phase is the remainder in 256ths;
   the taps cover input samples whole - CENTRE .. whole - CENTRE + TAPS - 1,
   zero outside the source. There is an output sample for every input
   position before the end. The output is at half scale (16384 is full
   scale): a full-scale source rings past it. */
static int resample(const Pcm* in, Pcm* out)
{
    u32 whole = 0, part = 0, k = 0, count;
    /* count = ceil(n * denom / (rate * 424)), stepped exactly. */
    for (count = 0; whole < in->n; ++count) {
        whole += SFX_STEP_WHOLE; part += SFX_STEP_PART;
        if (part >= SFX_STEP_DENOM) { part -= SFX_STEP_DENOM; ++whole; }
    }
    out->n = count;
    out->p = dw_alloc(count * 2 + 2);
    if (!out->p) return 0;
    whole = part = 0;
    for (k = 0; k < count; ++k) {
        const short* h = SFX_FIR[(part * SFX_PHASES) / SFX_STEP_DENOM];
        s32 first = (s32)whole - (s32)SFX_CENTRE, acc = 0, j, from = 0, to = (s32)SFX_TAPS;
        if (first < 0) from = -first;
        if (first + to > (s32)in->n) to = (s32)in->n - first;
        for (j = from; j < to; ++j) acc += (s32)in->p[first + j] * h[j];
        acc = (acc + 32768) >> 16;
        out->p[k] = (s16)(acc > 32767 ? 32767 : acc < -32768 ? -32768 : acc);
        whole += SFX_STEP_WHOLE; part += SFX_STEP_PART;
        if (part >= SFX_STEP_DENOM) { part -= SFX_STEP_DENOM; ++whole; }
    }
    return 1;
}

/* ---- cutting ----------------------------------------------------------- */

/* The quarter sine at k / (n - 1) of the way from 0 to pi/2 (Q15), as numpy's
   linspace(0, pi/2, n) samples it: the table entry nearest. */
static u32 quarter(u32 k, u32 n, int cosine)
{
    u32 at = n > 1 ? (k * 1024 * 2 + (n - 1)) / (2 * (n - 1)) : 0;
    return SFX_SINE[cosine ? 1024 - at : at];
}

static s16 scale(s32 x, u32 q15) { return (s16)((x * (s32)q15) / 32768); }

static void crop(Pcm* x, u32 threshold, u32 window)
{
    u32 i, last = 0, end, n, found = 0;
    for (i = 0; i < x->n; ++i)
        if (2 * (u32)(x->p[i] < 0 ? -x->p[i] : x->p[i]) >= threshold) { last = i; found = 1; }
    if (!found) { x->n = 0; return; }
    end = (last / window + 1) * window;
    if (end > x->n) end = x->n;
    x->n = end;
    n = window < end ? window : end;
    /* The last window fades: sample k of it keeps (n - 1 - k) / n. */
    for (i = 0; i < n; ++i) {
        s32 v = x->p[end - n + i];
        x->p[end - n + i] = (s16)(v * (s32)(n - 1 - i) / (s32)n);
    }
}

static void head(Pcm* x, u32 n, u32 fade)
{
    u32 i;
    if (n > x->n) n = x->n;
    if (n < x->n && fade) {
        if (fade > n) fade = n;
        for (i = 0; i < fade; ++i) {
            u32 cq = quarter(i, fade, 1);
            x->p[n - fade + i] = scale(scale(x->p[n - fade + i], cq), cq);
        }
    }
    x->n = n;
}

/* A window around the loudest 50 ms: the sum of squares (samples / 8) over
   numpy's 'same' convolution window, first maximum; it starts 45 % of the
   window before that point and fades in over its first 40 % and out over
   its last 50 % (sine and cosine squared). */
static u32 energy(const Pcm* x, s32 i) { s32 s; if (i < 0 || i >= (s32)x->n) return 0; s = x->p[i] / 8; return (u32)(s * s); }

static int swell(Pcm* x, u32 n, u32 hop)
{
    /* Output i of numpy's convolve(e, ones(hop), 'same') sums e over
       i + lead - hop + 1 .. i + lead, lead = (hop - 1) / 2. */
    s32 lead = (s32)(hop - 1) / 2, i, j;
    u32 sum = 0, best = 0, at = 0, start, in, outf, k;
    s16* y;
    if (n > x->n) n = x->n;
    for (j = lead - (s32)hop + 1; j <= lead; ++j) sum += energy(x, j);
    best = sum;
    for (i = 1; i < (s32)x->n; ++i) {
        sum += energy(x, i + lead);
        sum -= energy(x, i + lead - (s32)hop);
        if (sum > best) { best = sum; at = (u32)i; }
    }
    start = 20 * at > 9 * n ? (20 * at - 9 * n) / 20 : 0;
    if (start > x->n - n) start = x->n - n;
    y = dw_alloc(n * 2 + 2);
    if (!y) return 0;
    memcpy(y, x->p + start, n * 2);
    in = 2 * n / 5;
    outf = n / 2;
    for (k = 0; k < in; ++k) { u32 s = quarter(k, in, 0); y[k] = scale(scale(y[k], s), s); }
    for (k = 0; k < outf; ++k) { u32 cq = quarter(k, outf, 1); y[n - outf + k] = scale(scale(y[n - outf + k], cq), cq); }
    dw_free(x->p);
    x->p = y;
    x->n = n;
    return 1;
}

/* A loop of x[a, a + n) whose head fades in (sine) from the audio after the
   segment (cosine), so the wrap continues as the source did. */
static int loop(Pcm* x, u32 a, u32 n, u32 f)
{
    u32 i;
    s16* y;
    REQUIRE(a + n + f <= x->n, "a sound loop runs past its source");
    y = dw_alloc(n * 2 + 2);
    if (!y) return 0;
    memcpy(y, x->p + a, n * 2);
    for (i = 0; i < f; ++i) {
        s32 v = ((s32)x->p[a + i] * (s32)quarter(i, f, 0) + (s32)x->p[a + n + i] * (s32)quarter(i, f, 1)) / 32768;
        y[i] = (s16)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
    }
    dw_free(x->p);
    x->p = y;
    x->n = n;
    return 1;
}

/* ---- the sounds and their banks ---------------------------------------- */

typedef struct { s8* data; u32 bytes; u8 volume, loop; u16 gain; } Converted;

/* round(volume * 32768) of the stored float, half to even (pack-amiga-sfx.py). */
static u32 gain_q15(u32 bits)
{
    s32 e = (s32)((bits >> 23) & 0xff) - 127 - 23 + 15;
    u32 m = (bits & 0x7fffff) | 0x800000, whole, rest, half;
    if ((bits >> 23 & 0xff) == 0 || (bits >> 31)) return 0;
    if (e >= 0) return e > 8 ? 0x10000 : m << e;
    if (e < -24) return 0;
    whole = m >> -e; rest = m & ((1u << -e) - 1); half = 1u << (-e - 1);
    return whole + (rest > half || (rest == half && (whole & 1)));
}

static int convert(Context* c, const SfxPlan* plan, Converted* out)
{
    Pcm source, x;
    u32 i, peak = 0, gain;
    int ok = 1;
    source.p = x.p = 0;
    source.n = x.n = 0;
    out->data = 0; out->bytes = 0; out->loop = plan->kind == SFX_LOOP;
    gain = gain_q15(c->sounds.s[plan->id].volumeBits);
    REQUIRE(gain > 0 && gain <= 65535, "a sound's volume is outside (0, 2)");
    out->gain = (u16)gain;
    if (plan->kind == SFX_DROP) return dw_fail("a dropped sound is in the plan");
    TRY(decode(c, plan->id, &source));
    ok = resample(&source, &x);
    pcm_free(&source);
    if (!ok) return 0;
    switch (plan->kind) {
    case SFX_CROP: crop(&x, plan->a, plan->b); break;
    case SFX_HEAD: head(&x, plan->a, plan->b); break;
    case SFX_SWELL: ok = swell(&x, plan->a, plan->b); break;
    default: ok = loop(&x, plan->a, plan->b, plan->c); break;
    }
    if (!ok) { pcm_free(&x); return 0; }
    for (i = 0; i < x.n; ++i) { u32 v = (u32)(x.p[i] < 0 ? -x.p[i] : x.p[i]); if (v > peak) peak = v; }
    if (!peak) { pcm_free(&x); return dw_fail("a kept sound is silent"); }
    /* Peak to 127, rounding half away from zero; odd lengths get a zero. */
    out->bytes = (x.n + 1) & ~1u;
    out->data = dw_alloc(out->bytes);
    if (!out->data) { pcm_free(&x); return 0; }
    for (i = 0; i < x.n; ++i) {
        s32 v = x.p[i], q = v >= 0 ? (254 * v + (s32)peak) / (2 * (s32)peak) : -((254 * -v + (s32)peak) / (2 * (s32)peak));
        out->data[i] = (s8)(q > 127 ? 127 : q < -128 ? -128 : q);
    }
    if (x.n & 1) out->data[x.n] = 0;
    pcm_free(&x);
    /* The asset volume and the normalisation move into the Paula volume:
       round(64 * volume * peak / 16384). */
    i = (gain * peak + (1u << 22)) >> 23;
    out->volume = (u8)(i < 1 ? 1 : i > 64 ? 64 : i);
    REQUIRE(out->bytes >= 2 && out->bytes <= 131070, "a converted sound does not fit a Paula sample");
    REQUIRE(out->bytes >= plan->least, "a converted one-shot is shorter than a PAL field at its top pitch");
    return 1;
}

/* tools/pack-amiga-sfx.py's DWSX pack of the plan's sounds in `bank`. */
static int pack(const Converted* sounds, unsigned bank, Buf* out)
{
    u32 i, count = 0, index = 0, header, data = 0, total, n;
    for (i = 0; i < SFX_PLAN_COUNT; ++i)
        if (SFX_PLAN[i].banks & bank) { ++count; data += sounds[i].bytes; index = SFX_PLAN[i].id + 1u; }
    REQUIRE(count >= 1 && count <= 255, "a sound bank holds 1 to 255 sounds");
    header = 32 + 16 * count + index + (index & 1);
    total = header + data;
    buf_put(out, "DWSX", 4); buf_u16(out, 2); buf_u16(out, count);
    buf_u32(out, total); buf_u32(out, header); buf_u16(out, index);
    for (i = 0; i < 14; ++i) buf_u8(out, 0);
    for (i = 0, data = 0; i < SFX_PLAN_COUNT; ++i)
        if (SFX_PLAN[i].banks & bank) {
            buf_u16(out, SFX_PLAN[i].id); buf_u16(out, SFX_PLAN[i].period);
            buf_u8(out, sounds[i].volume); buf_u8(out, sounds[i].loop);
            buf_u16(out, sounds[i].gain); buf_u32(out, data); buf_u32(out, sounds[i].bytes);
            data += sounds[i].bytes;
        }
    for (n = 0; n < index + (index & 1); ++n) {
        u32 entry = 0, k;
        for (i = 0, k = 0; i < SFX_PLAN_COUNT; ++i)
            if (SFX_PLAN[i].banks & bank) { ++k; if (SFX_PLAN[i].id == n) entry = k; }
        buf_u8(out, entry);
    }
    for (i = 0; i < SFX_PLAN_COUNT; ++i)
        if (SFX_PLAN[i].banks & bank) buf_put(out, sounds[i].data, sounds[i].bytes);
    REQUIRE(!out->failed && out->n == total, "not enough memory for a sound bank");
    return 1;
}

int stage_audio(Context* c)
{
    static const struct { unsigned bank; const char* name; } BANKS[] = {
        {SFX_RUN, "Downwell.sndrun"}, {SFX_FRONT, "Downwell.sndfront"},
        {SFX_RESULTS, "Downwell.sndresults"}, {SFX_CLEAR, "Downwell.sndclear"}};
    Converted* sounds = dw_zalloc(sizeof(Converted) * SFX_PLAN_COUNT);
    u32 i;
    int ok = sounds != 0;
    for (i = 0; ok && i < SFX_PLAN_COUNT; ++i) {
        REQUIRE((int)SFX_PLAN[i].id < c->sounds.count, "the sound plan names a sound data.win lacks");
        ok = convert(c, &SFX_PLAN[i], &sounds[i]);
        if (io_cancelled()) ok = dw_fail("cancelled");
    }
    for (i = 0; ok && i < sizeof BANKS / sizeof BANKS[0]; ++i) {
        Buf out;
        buf_init(&out);
        ok = pack(sounds, BANKS[i].bank, &out) && dw_write_file(c->dest, BANKS[i].name, out.p, out.n);
        buf_free(&out);
    }
    if (sounds) for (i = 0; i < SFX_PLAN_COUNT; ++i) dw_free(sounds[i].data);
    dw_free(sounds);
    return ok;
}

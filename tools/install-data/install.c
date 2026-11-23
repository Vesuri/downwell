/* DownwellInstallData: builds the installed data files from the player's
   Windows Steam copy of Downwell (app 360740, version 1.0.5). Every input is
   checked by SHA-256 first; any other version is refused. */
#include "install_config.h"
#include "context.h"
#include "sha256.h"
#include "text.h"
#include "rooms.h"
#include "masks.h"
#include "code.h"
#include "exe.h"
#include "art.h"
#include "derived.h"
#include "audio.h"

/* In order: a stage may read what an earlier one wrote (the scene banks). */
typedef struct { const char* name; const char* message; int (*run)(Context*); } Stage;
static const Stage STAGES[] = {
    {"text", "Texts...", stage_text},
    {"rooms", "Objects and rooms...", stage_rooms},
    {"masks", "Collision masks...", stage_masks},
    {"code", "Level patterns and palettes...", stage_code},
    {"exe", "Runner tables...", stage_exe},
    {"art", "Scene art...", stage_art},
    {"derived", "Prepared art...", stage_derived},
    {"audio", "Sound banks...", stage_audio},
    {"soundtables", "Sound tables...", sound_tables},
};
#define STAGE_COUNT (sizeof STAGES / sizeof STAGES[0])

typedef struct { const char* name; const char* sha256; } Input;
/* docs/source-inventory.md */
static const Input INPUTS[] = {
    {"data.win", "c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8"},
    {"Downwell.exe", "340b43dae1a18d2861cba9f6a9cdbb92f3754699854e0dc1a1ca367a8cd11085"},
    {"language.ini", "b56ff8dc2f9ff012fcab1af8757d2e765a3a2c1f972c1ba41ae405e8bd4e4c68"},
    {"sfxfalling.ogg", "7b82aa294300209ba772acd3a21af1bb6695d0421491b22c400a1d5e4f2e4209"},
    {"amb1.ogg", "bb1a6440193ecaf4160f5b1e5a773f054c1289554dd6adf43cfbbe42d0b37b7f"},
    {"sfxbossfirsthit.ogg", "07a3a0444fcfb2d57fc369ea93a360802e5e6a9afa923018aec048455772494e"},
    {"audiogroup1.dat", "c0d53595a4fcf7b8ca5d08d8deb4db4cb9a893c96b17954518c938a200cfce81"},
    {"audiogroup2.dat", "168eab2e92bf47f9b1354b2d44b22094c69ed039c891f172a63215a48d590d88"},
    {"audiogroup3.dat", "042188e65c982d15a6183e1fc3a7f70cfe811ed63fe30601e5cea6af90ccd6ea"},
    {"audiogroup4.dat", "dc972711b0e4e5747d196eff5692fc5502f9ec735fb0a9bae98002d1dc205083"},
    {"audiogroup5.dat", "3df0bc4ea1f8625be1363b6a71d4fb6f802da179f36d2d96a23c72dbb922aac2"},
    {"audiogroup6.dat", "6fe5a5ae2c328f348d1a3870cdb3a7002d89d98606b44d2e7f4764e0123a5893"},
};
#define INPUT_COUNT (sizeof INPUTS / sizeof INPUTS[0])

/* Messages naming a file; dw_error keeps the first, so each has its own buffer. */
static char messages[4][128];
static int nextMessage;
static const char* say(const char* a, const char* b)
{
    char* message = messages[nextMessage++ & 3];
    u32 n = 0;
    while (*a && n < sizeof messages[0] - 1) message[n++] = *a++;
    while (b && *b && n < sizeof messages[0] - 1) message[n++] = *b++;
    message[n] = 0;
    return message;
}

int ctx_open(Context* c, const char* name, File* f)
{
    char path[512];
    dw_path(path, sizeof path, c->source, name);
    if (!io_open(f, path, 0)) return dw_fail(say("cannot open ", name));
    return 1;
}

int ctx_read_output(Context* c, const char* name, Buf* out)
{
    char path[512];
    File f;
    u8* p;
    dw_path(path, sizeof path, c->dest, name);
    if (!io_open(&f, path, 0)) return dw_fail(say("cannot read back ", name));
    buf_init(out);
    p = buf_grow(out, f.size);
    if (!p || (f.size && !io_read(&f, p, f.size))) { io_close(&f); buf_free(out); return dw_fail(say("cannot read back ", name)); }
    io_close(&f);
    return 1;
}

static int verify(Context* c, const Input* in, u8* buffer, u32 size)
{
    static const char hex[] = "0123456789abcdef";
    File f;
    SHA256 s;
    u8 digest[32];
    u32 left, i;
    TRY(ctx_open(c, in->name, &f));
    sha_init(&s);
    for (left = f.size; left;) {
        u32 n = left < size ? left : size;
        if (io_cancelled()) { io_close(&f); return dw_fail("cancelled"); }
        if (!io_read(&f, buffer, n)) { io_close(&f); return dw_fail(say("cannot read ", in->name)); }
        sha_update(&s, buffer, n);
        left -= n;
    }
    io_close(&f);
    sha_final(&s, digest);
    for (i = 0; i < 32; ++i)
        if (hex[digest[i] >> 4] != in->sha256[2 * i] || hex[digest[i] & 15] != in->sha256[2 * i + 1])
            return dw_fail(say(in->name, " is not the Steam release of Downwell 1.0.5"));
    return 1;
}

static int verify_inputs(Context* c)
{
    enum { CHUNK = 64 * 1024 };
    u8* buffer = dw_alloc(CHUNK);
    u32 i;
    int ok = 1;
    if (!buffer) return 0;
    for (i = 0; i < INPUT_COUNT && ok; ++i) ok = verify(c, &INPUTS[i], buffer, CHUNK);
    dw_free(buffer);
    return ok;
}

static Context context;

#ifdef INSTALL_TIMING
/* Timing build only (make amiga-timing): 50 Hz ticks before each stage and
   at the end, and the peak allocation, read by tools/install-data/time_amiga.py. */
volatile long install_ticks[16];
volatile unsigned long install_peak;
long install_now(void); /* timing_amiga.c */
void install_cache_probe(void);
#define MARK(i) (install_ticks[i] = install_now())
#else
#define MARK(i) ((void)0)
#endif

#ifndef INSTALL_AMIGA
#include <stdlib.h>
/* Host builds only: DOWNWELL_INSTALL_STAGES=audio,... runs just those stages
   (the repo Makefile builds generated/amiga-sfx-*.bin this way). */
static int selected(const char* name)
{
    const char* list = getenv("DOWNWELL_INSTALL_STAGES");
    u32 n = (u32)strlen(name);
    if (!list) return 1;
    while (*list) {
        if (!strncmp(list, name, n) && (list[n] == ',' || !list[n])) return 1;
        while (*list && *list != ',') ++list;
        if (*list) ++list;
    }
    return 0;
}
#elif defined(INSTALL_CMOT_PROBE)
/* The amiga-cmot-probe build: the exe stage only, inputs unchecked. */
static int selected(const char* name) { return !dw_strcmp(name, "exe"); }
#else
static int selected(const char* name) { (void)name; return 1; }
#endif

#ifdef INSTALL_AMIGA
void install_cpu_init(void); /* binary64_amiga.cpp */
#ifdef INSTALL_MULTIPLY_TEST
void install_multiply_test(unsigned long randoms);
#endif
#endif

int install_data(const char* source, const char* dest, const char* temp)
{
    Context* c = &context;
    char path[512];
    int ok;
    (void)temp;
#ifdef INSTALL_AMIGA
    install_cpu_init();
#ifdef INSTALL_TIMING
    install_cache_probe();
#endif
#ifdef INSTALL_MULTIPLY_TEST
    install_multiply_test(INSTALL_MULTIPLY_TEST);
    return 0;
#endif
#endif
    memset(c, 0, sizeof *c);
    c->source = source; c->dest = dest;
    tables_init(&c->tables);
    MARK(0);
#ifdef INSTALL_CMOT_PROBE
    ok = 1;
#else
    ok = verify_inputs(c);
#endif
    MARK(1);
    if (ok) {
        io_message("Reading data.win...");
        dw_path(path, sizeof path, source, "data.win");
        ok = win_open(&c->win, path, 512 * 1024) && sounds_read(&c->win, &c->sounds);
    }
    {
        u32 i;
        for (i = 0; i < STAGE_COUNT && ok; ++i) {
            MARK(2 + i);
            if (!selected(STAGES[i].name)) continue;
            io_message(STAGES[i].message);
            ok = !io_cancelled() && STAGES[i].run(c);
            if (ok && io_cancelled()) ok = dw_fail("cancelled");
        }
    }
    if (ok) {
        Buf out;
        buf_init(&out);
        if (c->tables.n == 23) ok = tables_pack(&c->tables, &out) && dw_write_file(dest, "Downwell.tables", out.p, out.n);
        else ok = tables_partial(&c->tables, dest);
        buf_free(&out);
    }
    MARK(2 + STAGE_COUNT);
#ifdef INSTALL_TIMING
    install_peak = dw_alloc_peak();
#endif
    win_close(&c->win);
    if (!ok) { io_message("DownwellInstallData failed:"); io_message(dw_error ? dw_error : "unknown error"); return 20; }
#ifndef INSTALL_AMIGA
    if (getenv("DOWNWELL_INSTALL_PEAK")) {
        char line[64];
        u32 v = dw_alloc_peak(), n = 0, d = 1000000000u;
        dw_strcpy(line, "peak allocation ");
        n = (u32)strlen(line);
        while (d > 1 && v / d == 0) d /= 10;
        for (; d; d /= 10) line[n++] = (char)('0' + v / d % 10);
        dw_strcpy(line + n, " bytes");
        io_message(line);
    }
#endif
    io_message("Done.");
    return 0;
}

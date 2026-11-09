// Sweep every gem streak block view through the real game-bank art: each
// composed piece must stay inside the staged region (a refusal ends the
// native game with RETURN_FAIL) and moves must clear the previous rows.
// compose() shares the same placement path.
#include "platform/amiga/AmigaGemStreakHud.h"
#include "platform/amiga/AmigaLevelTitleHud.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

// The HUD stage never blits; satisfy the renderer's blitter references.
int amigaBlitterDrawPlanarMasked(const UBYTE*, const UBYTE*, UWORD, UWORD, UWORD, UBYTE*, UWORD, UWORD,
                                 UWORD, UWORD, UWORD, UWORD, UWORD, int) { return 0; }
int amigaBlitterClearPlanar(UBYTE*, UWORD, UWORD) { return 0; }
int amigaBlitterSolidRect(UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD) { return 0; }

static std::vector<UBYTE> load(const char* path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}

int main(int argc, char** argv) {
    unsigned views = 0;
    for (int arg = 1; arg < argc; ++arg) {
        const auto pack = load(argv[arg]);
        assert(!pack.empty());
        std::vector<UBYTE> game(downwellLayerBytes), hud(downwellLayerBytes);
        AmigaRenderer renderer{};
        amigaRendererInit(&renderer, game.data(), hud.data());
        AmigaGemStreakHud owner;
        assert(owner.bind(pack.data(), pack.data() + pack.size()));
        auto check = [&](const AmigaGemStreakHud::View& v) {
            ++views;
            if (!owner.draw(&renderer, v)) {
                std::fprintf(stderr, "refused: high=%d sick=%d streak=%u line=%ld cap=%d motion=%d y=%d\n",
                             v.high, v.sick, v.streak, v.line, v.cap, v.motion, v.drawY);
                assert(false);
            }
        };
        for (int y = 12; y <= 60; y += 4) {
            AmigaGemStreakHud::View v;
            v.visible = true; v.drawY = y;
            for (unsigned streak = 1; streak < 100; ++streak)
                for (long line = 0; line <= (54L << 16); line += 1L << 14) { v.streak = streak; v.line = line; check(v); }
            v.high = true;
            for (unsigned streak : {100u, 256u, 999u, 4294967295u})
                for (int sick = 0; sick < 2; ++sick)
                    // round(58 * timer / 300): 420 after playerCreated, 480 gem-sick.
                    for (long line = 0; line <= (93L << 16); line += 1L << 14)
                        for (int cap = 0; cap < 3; ++cap)
                            for (int motion = 0; motion < 14; ++motion) {
                                v.streak = streak; v.sick = sick; v.line = line; v.cap = cap; v.motion = motion;
                                check(v);
                            }
        }
        // draw() erases exactly what it published: hiding leaves nothing, and
        // other writers' pixels survive wherever the block never drew.
        // Pass 0 on an empty layer records every pixel the block ever drew;
        // pass 1 repeats the sequence over a patterned layer.
        std::vector<UBYTE> drawn(downwellPlaneBytes);
        for (int pass = 0; pass < 2; ++pass) {
            const UBYTE background = pass ? 0xa5 : 0;
            for (unsigned i = 0; i < downwellLayerBytes; ++i) hud[i] = background;
            owner.invalidate();
            auto track = [&](const AmigaGemStreakHud::View& v) {
                check(v);
                if (!pass)
                    for (unsigned i = 0; i < downwellPlaneBytes; ++i)
                        drawn[i] |= UBYTE(hud[i] | hud[i + downwellPlaneBytes]);
            };
            for (int y = 12; y <= 60; y += 3) {
                AmigaGemStreakHud::View v;
                v.visible = true; v.drawY = y; v.streak = unsigned(y); v.line = long(y) << 16;
                track(v);
                v.high = true; v.line = long(y + 30) << 16; v.cap = y % 3; v.motion = y % 14;
                track(v);
                track(v); // unchanged position: merged erase
            }
            assert(owner.draw(&renderer, AmigaGemStreakHud::View{}));
            for (unsigned i = 0; i < downwellLayerBytes; ++i) {
                // Per pixel (both planes): a pixel the block drew and erased
                // reads pen 0; every other pixel keeps the background.
                const UBYTE pixels = drawn[i % downwellPlaneBytes];
                assert((hud[i] & ~pixels) == (background & ~pixels));
                assert(!(hud[i] & pixels));
            }
        }
    }
    std::printf("Gem streak HUD: %u views within the staged region\n", views);
    // levelNumber: whole lifecycles (reveal, hold, fall) of CAVERN-1..99 and
    // a full-capacity title, published on change only; nothing remains.
    unsigned titleFrames = 0, titlePublishes = 0;
    for (int arg = 1; arg < argc; ++arg) {
        const auto pack = load(argv[arg]);
        std::vector<UBYTE> game(downwellLayerBytes), hud(downwellLayerBytes);
        AmigaRenderer renderer{};
        amigaRendererInit(&renderer, game.data(), hud.data());
        AmigaLevelTitleHud owner;
        assert(owner.bind(pack.data(), pack.data() + pack.size()));
        // Reference: the same view drawn from scratch on an empty layer.
        std::vector<UBYTE> refGame(downwellLayerBytes), refHud(downwellLayerBytes);
        AmigaRenderer reference{};
        amigaRendererInit(&reference, refGame.data(), refHud.data());
        static const unsigned cavern[] = {'C', 'A', 'V', 'E', 'R', 'N'};
        static const unsigned longest[] = {'C', 'A', 'V', 'E', 'R', 'N', 'C', 'A', 'V', 'E', 'R', 'N'};
        for (int level = 1; level <= 100; ++level) {
            downwell::CompactLevelTitle title;
            const bool full = level == 100;
            assert(title.create(full ? longest : cavern, full ? 12 : 6, full ? 9 : level));
            AmigaLevelTitleHud::View shown;
            while (title.live) {
                const auto next = AmigaLevelTitleHud::view(title);
                if (next != shown) {
                    shown = next; ++titlePublishes;
                    if (!owner.draw(&renderer, shown)) {
                        std::fprintf(stderr, "title refused: level=%d length=%d\n", level, title.length);
                        assert(false);
                    }
                    AmigaLevelTitleHud fresh;
                    for (auto& byte : refHud) byte = 0;
                    assert(fresh.bind(pack.data(), pack.data() + pack.size()) && fresh.draw(&reference, shown));
                    if (hud != refHud) {
                        std::fprintf(stderr, "title mismatch: level=%d update=%u\n", level, titleFrames);
                        assert(false);
                    }
                }
                ++titleFrames;
                title.advanceDraw();
                title.advanceAlarms();
            }
            assert(owner.draw(&renderer, AmigaLevelTitleHud::view(title)));
            for (unsigned i = 0; i < downwellLayerBytes; ++i) assert(!hud[i]);
        }
    }
    std::printf("Level title HUD: %u updates, %u publishes, nothing left behind\n", titleFrames, titlePublishes);
}

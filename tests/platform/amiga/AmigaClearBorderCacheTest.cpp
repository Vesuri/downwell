#include "platform/amiga/AmigaClearBorderCache.h"
#include "platform/amiga/AmigaClearBorderRenderer.h"
#include "platform/amiga/AmigaRenderer.h"
#include "core/LevelClearMotion.h"
#include "core/LevelUpgradeArt.h"
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <cstring>
int amigaBlitterDrawPlanarMasked(const UBYTE*, const UBYTE*, UWORD, UWORD, UWORD,
    UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD, int) { return 0; }
int amigaBlitterClearPlanar(UBYTE*, UWORD, UWORD) { return 0; }
int amigaBlitterSolidRect(UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD) { return 0; }
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main(int argc, char** argv) {
    require(argc == 2, "clear pack required");
    std::ifstream input(argv[1], std::ios::binary);
    std::vector<UBYTE> pack{std::istreambuf_iterator<char>(input), {}};
    require(pack.size() > 20, "clear pack loaded");
    unsigned comparisons = 0;
    for (int area = 1; area <= 4; ++area) for (int level = 1; level <= 3; ++level) {
        std::vector<UBYTE> retained(downwellGameRingBytes), fresh(downwellGameRingBytes), hud(downwellLayerBytes);
        AmigaRenderer cached, reference;
        amigaRendererInit(&cached, retained.data(), hud.data());
        amigaRendererInit(&reference, fresh.data(), hud.data());
        AmigaClearBorderCache cache;
        downwell::LevelClearPresentation presentation;
        downwell::LevelClearMotion motion;
        unsigned painted = 0;
        for (unsigned tick = 0; tick < 320; ++tick) {
            if (tick == 60) motion.showChoices();
            motion.step(false);
            // Include backward/large jumps and returning from nonuniform
            // finite ends after the normal reveal/selection tape.
            const int viewY = tick < 240 ? motion.viewY() :
                tick < 260 ? 100 - int(tick - 240) * 3 :
                tick < 280 ? 480 : tick < 300 ? -32 : 150;
            const auto frame = presentation.advance(area, level, tick < 60 ? 0 : 4, tick >= 60);
            unsigned changed = 0;
            require(amigaRenderClearBorder(cached, cache, pack.data(), pack.data() + pack.size(), frame, viewY, changed),
                "production border renderer accepts recovered assets");
            painted += changed;
            if (level < 3 && tick > 120 && tick < 240)
                require(cache.examinedRows <= 6,
                    "settled repeating strip checks only exposed rows, not the viewport");
            require(tick == 0 || tick >= 240 || changed < 40, "scroll update never repaints the visible border playfield");
            require(amigaRenderClearBorder(cached, cache, pack.data(), pack.data() + pack.size(), frame, viewY, changed) &&
                changed == 0, "repeated presentation does not redraw or scroll");
            if (level < 3 && tick > 120 && tick < 240)
                require(cache.examinedRows == 0,
                    "unchanged repeating strip performs no row checks");
            std::memset(fresh.data(), 0, fresh.size());
            amigaRendererSetClipRect(&reference, 110, 0, 160, 284);
            frame.borders([&](int selectedArea, int subimage, int x, int y) {
                amigaRendererDrawAsset(&reference, pack.data(), pack.data() + pack.size(),
                    downwell::levelBorderSprite(selectedArea), subimage, x + 110, y - viewY, 0);
            });
            for (unsigned plane = 0; plane < 2; ++plane) for (unsigned y = 0; y < 284; ++y) {
                const unsigned physical = (cache.origin + y) % 320;
                require(std::memcmp(retained.data() + plane * downwellGameRingPlaneBytes + physical * 48,
                    fresh.data() + plane * downwellGameRingPlaneBytes + y * 48, 48) == 0,
                    "circular border bytes match fresh source-order rendering");
            }
            ++comparisons;
        }
        require(painted < 320 * 284 / 10, "border cache avoids over ninety percent of full-frame row repaints");
        const auto pixelsBefore = retained;
        const auto cacheBefore = cache;
        unsigned changed = 99;
        const auto invalidFrame = presentation.advance(area, level, 4, true);
        require(!amigaRenderClearBorder(cached, cache, pack.data(), pack.data() + 20,
            invalidFrame, motion.viewY(), changed) && changed == 0 && pixelsBefore == retained &&
            cache.origin == cacheBefore.origin && cache.previousTileY == cacheBefore.previousTileY &&
            std::memcmp(cache.rows, cacheBefore.rows, sizeof cache.rows) == 0,
            "missing border assets fail before touching the retained buffer or cache");
    }
    std::printf("%u circular clear-border frames match full rendering\n", comparisons);
}

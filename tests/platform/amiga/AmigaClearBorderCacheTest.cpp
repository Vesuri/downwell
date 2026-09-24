#include "platform/amiga/AmigaClearBorderCache.h"
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
    UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD) { return 0; }
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
        for (unsigned tick = 0; tick < 240; ++tick) {
            if (tick == 60) motion.showChoices();
            motion.step(false);
            const int viewY = motion.viewY();
            const auto frame = presentation.advance(area, level, tick < 60 ? 0 : 4, tick >= 60);
            auto paint = [&](int y, int anchorY, unsigned current, unsigned incoming) {
                amigaRendererSetGameOrigin(&cached, cache.origin);
                amigaRendererClearGameRows(&cached, y, 1, 0);
                amigaRendererSetClipRect(&cached, 110, y, 160, 1);
                for (unsigned pass = 0; pass < 2; ++pass) {
                    const unsigned id = downwell::levelBorderSprite(area + pass);
                    for (unsigned slot = 0; slot < 2; ++slot) {
                        if (!((pass ? incoming : current) & (1u << slot))) continue;
                        amigaRendererDrawAsset(&cached, pack.data(), pack.data() + pack.size(), id, 13,
                            pass ? 118 : 110, anchorY + 16 * slot, 0);
                        amigaRendererDrawAsset(&cached, pack.data(), pack.data() + pack.size(), id, 7,
                            pass ? 262 : 270, anchorY + 16 * slot, 0);
                    }
                }
            };
            const unsigned changed = cache.update(frame, viewY, paint);
            painted += changed;
            require(tick == 0 || changed < 40, "scroll update never repaints the visible border playfield");
            require(cache.update(frame, viewY, paint) == 0, "repeated presentation does not redraw or scroll");
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
        require(painted < 240 * 284 / 10, "border cache avoids over ninety percent of full-frame row repaints");
    }
    std::printf("%u circular clear-border frames match full rendering\n", comparisons);
}

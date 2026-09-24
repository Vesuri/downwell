#include "platform/amiga/AmigaClearOverlay.h"
#include "core/LevelClearLayout.h"
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
int amigaBlitterDrawPlanarMasked(const UBYTE*, const UBYTE*, UWORD, UWORD, UWORD,
    UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD) { return 0; }
int amigaBlitterClearPlanar(UBYTE*, UWORD, UWORD) { return 0; }
int amigaBlitterSolidRect(UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD) { return 0; }
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
static downwell::HudReal sample(double value) {
    if (!value) return {};
    std::uint64_t bits;
    std::memcpy(&bits, &value, sizeof bits);
    return {(bits & 0xfffffffffffffULL) | (1ULL << 52), int((bits >> 52) & 2047) - 1023};
}
int main(int argc, char** argv) {
    require(argc == 2, "clear pack required");
    std::ifstream input(argv[1], std::ios::binary);
    std::vector<UBYTE> pack{std::istreambuf_iterator<char>(input), {}};
    require(pack.size() > 20, "clear pack loaded");
    std::vector<UBYTE> game(downwellGameRingBytes), hud[2], reference(downwellLayerBytes);
    AmigaClearOverlay caches[2];
    for (auto& pixels : hud) pixels.resize(downwellLayerBytes);
    downwell::LevelClearController room;
    downwell::UpgradeInventory inventory;
    room.offers.ids[0] = 0; room.offers.ids[1] = 6; room.offers.ids[2] = 13;
    room.offers.ids[3] = 100;
    room.selection.lastIndex = 3;
    unsigned totalPainted = 0;
    for (unsigned tick = 0; tick < 360; ++tick) {
        if (tick == 60) room.motion.showChoices();
        room.motion.step(tick >= 300);
        room.selection.cursor = (tick / 19) % 4;
        room.carousel.step(room.selection.cursor);
        const auto frame = room.presentation.advance(1, 2, 4, tick >= 60 && tick < 300);
        struct Sink {
            AmigaClearOverlay::Command commands[AmigaClearOverlay::capacity];
            unsigned count = 0;
            int viewY;
            int coordinate(downwell::LevelClearCoordinate value) {
                const double n = std::ldexp(double(value.magnitude.mantissa), value.magnitude.exponent - 52);
                return int(std::floor(value.negative ? -n : n));
            }
            void sprite(unsigned id, unsigned image, downwell::LevelClearCoordinate x, downwell::LevelClearCoordinate y) {
                if (image) return; // borders belong to their hardware-scrolled playfield
                commands[count++] = {id, image, 110 + coordinate(x), coordinate(y) - viewY, -1};
            }
            void player(downwell::LevelClearCoordinate, downwell::LevelClearCoordinate) {}
            void label(unsigned id, downwell::LevelClearCoordinate x, downwell::LevelClearCoordinate y, unsigned pen) {
                commands[count++] = {id, 0, 110 + coordinate(x), coordinate(y) - viewY, int(pen)};
            }
            void description(unsigned id, downwell::LevelClearCoordinate x, downwell::LevelClearCoordinate y, unsigned suffix) {
                require(!suffix, "fixture uses unowned offers");
                const int px = 110 + coordinate(x), py = coordinate(y) - viewY;
                commands[count++] = {id, 0, px + 1, py, 3};
                commands[count++] = {id, 0, px - 1, py, 3};
                commands[count++] = {id, 0, px, py + 1, 3};
                commands[count++] = {id, 0, px, py - 1, 3};
                commands[count++] = {id, 0, px, py, 1};
            }
        } sink{{}, 0, room.motion.viewY()};
        downwell::drawLevelClear(sink, room, frame, inventory,
            sample(std::fabs(std::cos(frame.circle * (3.14159265358979323846 / 180.0)))),
            sample(std::fabs(std::sin(frame.time / 20.0))));
        const unsigned back = tick % 2;
        AmigaRenderer renderer;
        amigaRendererInit(&renderer, game.data(), hud[back].data());
        const auto before = renderer;
        unsigned painted = 0;
        require(caches[back].render(renderer, pack.data(), pack.data() + pack.size(), sink.commands, sink.count, painted),
            "retained overlay accepts source layout commands");
        totalPainted += painted;
        require(renderer.target == before.target && renderer.clipX == before.clipX && renderer.clipWidth == before.clipWidth,
            "overlay restores caller renderer state");
        require(caches[back].render(renderer, pack.data(), pack.data() + pack.size(), sink.commands, sink.count, painted) && !painted,
            "unchanged overlay performs no repaint");
        std::fill(reference.begin(), reference.end(), 0);
        AmigaRenderer full;
        amigaRendererInit(&full, game.data(), reference.data());
        full.target = full.hud; full.clipX = 110; full.clipY = 0; full.clipWidth = 160; full.clipHeight = 284;
        for (unsigned i = 0; i < sink.count; ++i) {
            const auto& c = sink.commands[i];
            if (c.pen < 0) amigaRendererDrawAsset(&full, pack.data(), pack.data() + pack.size(), c.id, c.frame, c.x, c.y, 0);
            else amigaRendererDrawAssetPen(&full, pack.data(), pack.data() + pack.size(), c.id, c.frame, c.x, c.y, 0, c.pen);
        }
        require(hud[back] == reference, "retained double-buffer overlay matches full source-order rasterization");
        const auto savedPixels = hud[back];
        require(!caches[back].render(renderer, pack.data(), pack.data() + 20, sink.commands, sink.count, painted) &&
            !painted && hud[back] == savedPixels, "missing artwork fails without touching pixels");
    }
    require(totalPainted < 360 * 160 * 284 / 3, "dirty regions avoid at least two thirds of full-overlay clears");
    for (unsigned back = 0; back < 2; ++back) {
        AmigaRenderer renderer;
        amigaRendererInit(&renderer, game.data(), hud[back].data());
        unsigned painted = 0;
        const auto before = hud[back];
        require(!caches[back].render(renderer, pack.data(), pack.data() + pack.size(), nullptr, 1, painted) &&
            before == hud[back], "invalid command list leaves existing overlay intact");
        require(caches[back].render(renderer, pack.data(), pack.data() + pack.size(), nullptr, 0, painted),
            "empty frame retires all previous commands");
        require(hud[back] == std::vector<UBYTE>(downwellLayerBytes), "disappearing content leaves no stale pixels");
        caches[back].invalidate();
        require(caches[back].render(renderer, pack.data(), pack.data() + pack.size(), nullptr, 0, painted) &&
            painted == 160 * 284, "explicit invalidation resets the whole owned viewport");
    }
    std::printf("360 alternating clear overlays match full rendering; %u pixels cleared\n", totalPainted);
}

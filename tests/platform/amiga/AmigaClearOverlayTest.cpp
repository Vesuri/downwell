#include "platform/amiga/AmigaClearOverlay.h"
#include "platform/amiga/AmigaClearDrawList.h"
#include "platform/amiga/AmigaClearPresenter.h"
#include "core/LevelClearLayout.h"
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
int amigaBlitterDrawPlanarMasked(const UBYTE*, const UBYTE*, UWORD, UWORD, UWORD,
    UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD, int) { return 0; }
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
    AmigaClearPresenter presenter;
    std::vector<UBYTE> presentedGame[2], presentedHud[2], fullBorder(downwellGameRingBytes);
    for (unsigned i = 0; i < 2; ++i) {
        presentedGame[i].resize(downwellGameRingBytes, 0xa5);
        presentedHud[i].resize(downwellLayerBytes, 0x5a);
    }
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
        for (unsigned id = 0; id < 23; ++id) inventory.levels[id] = tick % 2;
        room.carousel.step(room.selection.cursor);
        const auto frame = room.presentation.advance(1, 2, 4, tick >= 60 && tick < 300);
        struct Sink {
            AmigaClearOverlay::Command commands[AmigaClearOverlay::capacity];
            unsigned count = 0;
            int viewY;
            int coordinate(downwell::LevelClearCoordinate value) {
                const double n = std::ldexp(double(value.magnitude.mantissa), value.magnitude.exponent - 52);
                return int(std::floor(float(value.negative ? -n : n)));
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
                if (suffix) {
                    require(suffix == 2 && id >= 64100 && id <= 64122, "source owned-name path");
                    id += 500;
                }
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
        AmigaClearDrawList native;
        require(native.build(room, frame, inventory),
            "production draw-list adapter accepts shared source layout");
        require(native.count == sink.count, "native layout retains every overlay command");
        require(native.playerX == 190 && native.playerY == sink.coordinate({room.motion.playerY, false}) - room.motion.viewY(),
            "player sprite position remains separate from planar overlay commands");
        for (unsigned i = 0; i < sink.count; ++i)
            require(native.commands[i] == sink.commands[i], "native draw commands match independent source-order sink");
        const unsigned back = tick % 2;
        AmigaRenderer renderer;
        amigaRendererInit(&renderer, game.data(), hud[back].data());
        const auto before = renderer;
        unsigned painted = 0;
        require(caches[back].render(renderer, pack.data(), pack.data() + pack.size(), native.commands, native.count, painted),
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
        if (tick == 220) {
            std::vector<UBYTE> replacementGame(downwellGameRingBytes, 0xa5);
            std::vector<UBYTE> replacementHud(downwellLayerBytes, 0x5a);
            presentedGame[back].swap(replacementGame);
            presentedHud[back].swap(replacementHud);
        }
        if (tick == 240) presenter.reset();
        AmigaRenderer integrated;
        amigaRendererInit(&integrated, presentedGame[back].data(), presentedHud[back].data());
        AmigaClearPresenter::Frame output;
        if (tick == 180) presenter.artworkChanged();
        require(presenter.render(back, integrated, pack.data(), pack.data() + pack.size(), room, frame, inventory, output) &&
            output.ready && output.playerX == native.playerX && output.playerY == native.playerY,
            "combined presenter returns publication parameters and separate player position");
        require(presentedHud[back] == reference, "combined overlay matches independent full rasterization");
        std::fill(fullBorder.begin(), fullBorder.end(), 0);
        AmigaRenderer borderOracle;
        amigaRendererInit(&borderOracle, fullBorder.data(), reference.data());
        amigaRendererSetClipRect(&borderOracle, 110, 0, 160, 284);
        frame.borders([&](int area, int image, int x, int y) {
            amigaRendererDrawAsset(&borderOracle, pack.data(), pack.data() + pack.size(),
                downwell::levelBorderSprite(area), image, 110 + x, y - room.motion.viewY(), 0);
        });
        for (unsigned plane = 0; plane < 2; ++plane) for (unsigned y = 0; y < 284; ++y)
            require(std::memcmp(presentedGame[back].data() + plane * downwellGameRingPlaneBytes +
                ((output.origin + y) % 320) * 48, fullBorder.data() + plane * downwellGameRingPlaneBytes + y * 48, 48) == 0,
                "combined circular border matches full source-order rasterization including margins");
        require(presenter.render(back, integrated, pack.data(), pack.data() + pack.size(), room, frame, inventory, output) &&
            output.ready && output.borderRows == 0 && output.overlayPixels == 0,
            "repeated complete presentation draws nothing and does not advance clocks");
        if (tick == 200) {
            require(!presenter.render(back, integrated, pack.data(), pack.data() + 20, room, frame, inventory, output) && !output.ready,
                "failed frame cannot be published");
            require(presenter.render(back, integrated, pack.data(), pack.data() + pack.size(), room, frame, inventory, output) &&
                output.ready && presentedHud[back] == reference, "valid retry rebuilds an unpublished failed back frame");
        }
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

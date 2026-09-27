#include "platform/amiga/AmigaRenderer.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
static bool emulateBlitter;
static unsigned calls;
static unsigned attempts;
static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::abort(); }
}
// Tests the renderer/blitter boundary independently of the span fallback.
// Hardware Bitmap mask shifting itself remains an emulator verification task.
int amigaBlitterDrawPlanarMasked(const UBYTE*, const UBYTE* mask, UWORD, UWORD,
    UWORD stride, UBYTE* target, UWORD targetHeight, UWORD dx, UWORD dy,
    UWORD sx, UWORD sy, UWORD width, UWORD height, int pen) {
    ++attempts;
    if (!emulateBlitter) return 0;
    require(pen >= 0 && pen < 4 && dy + height <= targetHeight, "valid recolor blit and ring split");
    ++calls;
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < width; ++x) {
        if (!(mask[(sy + y) * (stride / 8) + (sx + x) / 8] & (0x80 >> ((sx + x) % 8)))) continue;
        for (unsigned plane = 0; plane < 2; ++plane) {
            auto& byte = target[plane * targetHeight * 48 + (dy + y) * 48 + (dx + x) / 8];
            const unsigned bit = 0x80 >> ((dx + x) % 8);
            byte = (byte & ~bit) | ((pen & (1 << plane)) ? bit : 0);
        }
    }
    return 1;
}
int amigaBlitterClearPlanar(UBYTE*, UWORD, UWORD) { return 0; }
int amigaBlitterSolidRect(UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD) { return 0; }
int main() {
    unsigned cases = 0;
    for (unsigned height : {9u, 24u, 25u, 33u}) {
    std::vector<UBYTE> pack(44 + 4 * height * 3);
    const auto word = [&](unsigned at, unsigned value) { pack[at] = value >> 8; pack[at + 1] = value; };
    const auto longword = [&](unsigned at, unsigned value) { word(at, value >> 16); word(at + 2, value); };
    longword(0, 0x44574150); word(4, 2); word(6, 20); longword(8, 1);
    word(20, 7); word(24, 8); word(26, 3); word(28, 29); word(30, height); word(32, 32); word(34, 3);
    longword(36, 44); longword(40, 12 * height);
    for (unsigned y = 0; y < height; ++y) for (unsigned x = 0; x < 29; ++x) {
        unsigned pen = (x + 3 * y) % 4;
        for (unsigned p = 0; p < 3; ++p)
            if (p == 2 ? pen != 0 : (pen & (1 << p)) != 0)
                pack[44 + p * 4 * height + y * 4 + x / 8] |= 0x80 >> (x % 8);
    }
    // Disabled, accepted, and declined hardware dispatch all retain the same
    // independent pixel oracle. Heights straddle the 24-row CPU threshold.
    for (unsigned blit = 0; blit < 3; ++blit) for (unsigned gameTarget = 0; gameTarget < 2; ++gameTarget)
    for (unsigned pen = 0; pen < 4; ++pen) for (int dx = -5; dx < 35; ++dx)
    for (int dy : {-2, 5, 281}) for (unsigned origin : {0u, 317u}) {
        emulateBlitter = blit == 1;
        const unsigned callsBefore = calls, attemptsBefore = attempts;
        std::vector<UBYTE> game(downwellGameRingBytes, 0xa5), hud(downwellLayerBytes, 0x5a);
        auto expected = gameTarget ? game : hud;
        AmigaRenderer renderer;
        amigaRendererInit(&renderer, game.data(), hud.data());
        renderer.target = gameTarget ? renderer.game : renderer.hud;
        renderer.clipX = 113; renderer.clipY = 1; renderer.clipWidth = 21; renderer.clipHeight = 281;
        renderer.gameOrigin = origin;
        renderer.blitterEnabled = blit != 0;
        const int anchorX = 110 + dx;
        amigaRendererDrawAssetPen(&renderer, pack.data(), pack.data() + pack.size(), 7, 0, anchorX, dy, 0, pen);
        if (height <= 24 || blit == 0)
            require(attempts == attemptsBefore, "small images and disabled hardware use CPU path");
        if (height > 24 && blit != 0 && dy == 5 && dx == 15) {
            require(attempts > attemptsBefore, "large visible image attempts hardware dispatch");
            if (blit == 1)
                require(calls - callsBefore == (gameTarget && origin == 317 ? 2u : 1u),
                        "accepted blit splits exactly at circular buffer boundary");
            else require(calls == callsBefore, "declined blit falls back to CPU");
        }
        for (int y = 0; y < static_cast<int>(height); ++y) for (int x = 0; x < 29; ++x) {
            if (!(unsigned(x + 3 * y) % 4)) continue;
            const int px = anchorX - 8 + x, py = dy - 3 + y;
            if (px < 113 || px >= 134 || py < 1 || py >= 282) continue;
            const unsigned physicalY = gameTarget ? (origin + py) % 320 : py;
            for (unsigned plane = 0; plane < 2; ++plane) {
                auto& byte = expected[plane * (gameTarget ? 320 : 284) * 48 + physicalY * 48 + px / 8];
                const unsigned bit = 0x80 >> (px % 8);
                byte = (byte & ~bit) | ((pen & (1 << plane)) ? bit : 0);
            }
        }
        require(expected == (gameTarget ? game : hud), "recolor matches independent pixel oracle including clipping and ring wrap");
        ++cases;
    }
    }
    require(calls > 0, "blitter dispatch was exercised");
    std::printf("%u recolor cases match pixel oracle; %u mask blits dispatched\n", cases, calls);
}

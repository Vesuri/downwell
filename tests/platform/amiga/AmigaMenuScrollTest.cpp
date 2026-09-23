#include "platform/amiga/AmigaMenuScroll.h"
#include <cstdlib>
#include <iostream>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}
}

int main() {
    require(amigaMenuCacheWidth > amigaMenuDisplayWidth,
            "intro layers are wider static canvases than the display");
    require(amigaMenuDisplayHStop - amigaMenuDisplayHStart == 160,
            "intro display window is centered and 160 pixels wide");
    require(amigaMenuFetchStart == 0x58 &&
                amigaMenuFetchStop == 0xa8 &&
                amigaMenuFetchStop - amigaMenuFetchStart == 80,
            "extra scrolling word is fetched left of the 160-pixel window");
    require(amigaMenuModulo == 42,
            "intro modulo skips the unused cached row bytes");
    require(amigaMenuEntryWidth == 192 &&
                amigaMenuEntryBytesPerRow == 24 &&
                amigaMenuEntryPlaneBytes == 27648,
            "entry cache retains horizontal alignment slack");

    AmigaMenuScrollState state = amigaMenuScrollState(0, 284);
    require(state.foregroundOffset == 0 &&
                state.backgroundOffset == 25 * 64 &&
                state.scrollControl == 0xff,
            "aligned intro origin uses the full extra-word delay");

    state = amigaMenuScrollState(95, 300);
    require(state.foregroundOffset != state.backgroundOffset,
            "foreground and background use independent plane pointers");
    require(state.foregroundOffset == 16 * 64 + 10,
            "foreground pointer advances in sixteen-pixel words");
    require(state.backgroundOffset == 27 * 64,
            "background pointer applies recovered slow vertical parallax");
    require(state.scrollControl == 0,
            "fifteen-pixel offsets consume the extra-word delay");

    state = amigaMenuScrollState(192, 384);
    require(state.foregroundOffset == 100 * 64 + 24 &&
                state.backgroundOffset == 34 * 64 + 4,
            "cached planes preserve independent parallax pointers");
    require(state.scrollControl == 0xff,
            "word-aligned parallax offsets use the full delay");

    require(amigaMenuEntryOffset(306) == 306 * 24 &&
                amigaMenuEntryOffset(858) == 858 * 24 &&
                amigaMenuEntryOffset(2000) == (1152 - 284) * 24,
            "entry pointer follows and clamps the recovered vertical camera");

    for (int x = 240; x <= 256; ++x) {
        const auto garden = amigaMenuScrollState(x, 306);
        const auto entry = amigaMenuEntryScrollState(x, 306);
        require(garden.scrollControl == entry.scrollControl &&
                    garden.backgroundOffset == entry.backgroundOffset,
                "entry retains fine scroll and background without a jump");
        const int sourceX = amigaMenuEntryWorldX +
            (entry.foregroundOffset % 24) * 8 + (15 - (entry.scrollControl & 15));
        require(sourceX == x, "entry cache maps to the same world x");
    }
    require(amigaMenuEntryScrollState(248, 858).backgroundOffset == 78 * 64 + 4,
            "background keeps scrolling during deep entry");
    require(amigaMenuEntryScrollState(248, 306).backgroundVisibleLines == 270 &&
                amigaMenuEntryScrollState(248, 575).backgroundVisibleLines == 1 &&
                amigaMenuEntryScrollState(248, 576).backgroundVisibleLines == 0 &&
                amigaMenuEntryScrollState(248, 858).backgroundVisibleLines == 0,
            "opaque well shaft covers sky before the next scene");
    std::cout << "Amiga menu hardware-scroll tests passed\n";
    return 0;
}

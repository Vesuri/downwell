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
    require(amigaMenuFetchStop - amigaMenuFetchStart == 80,
            "eleven-word intro fetch uses ten DDF intervals");
    require(amigaMenuModulo == 42,
            "intro modulo skips the unused cached row bytes");
    require(amigaMenuEntryWidth == 176 &&
                amigaMenuEntryBytesPerRow == 22 &&
                amigaMenuEntryPlaneBytes == 25344,
            "entry cache is one exact eleven-word fetch wide");

    AmigaMenuScrollState state = amigaMenuScrollState(0, 284);
    require(state.foregroundOffset == 0 &&
                state.backgroundOffset == 25 * 64 &&
                state.scrollControl == 0,
            "aligned intro origin preserves background vertical parallax");

    state = amigaMenuScrollState(95, 300);
    require(state.foregroundOffset != state.backgroundOffset,
            "foreground and background use independent plane pointers");
    require(state.foregroundOffset == 16 * 64 + 10,
            "foreground pointer advances in sixteen-pixel words");
    require(state.backgroundOffset == 27 * 64,
            "background pointer applies recovered slow vertical parallax");
    require(state.scrollControl == 0xff,
            "independent fine-scroll nibbles preserve parallax remainder");

    state = amigaMenuScrollState(192, 384);
    require(state.foregroundOffset == 100 * 64 + 24 &&
                state.backgroundOffset == 34 * 64 + 4,
            "cached planes preserve independent parallax pointers");
    require(state.scrollControl == 0,
            "word-aligned parallax offsets need no fine scroll");

    require(amigaMenuEntryOffset(306) == 306 * 22 &&
                amigaMenuEntryOffset(858) == 858 * 22 &&
                amigaMenuEntryOffset(2000) == (1152 - 284) * 22,
            "entry pointer follows and clamps the recovered vertical camera");

    std::cout << "Amiga menu hardware-scroll tests passed\n";
    return 0;
}

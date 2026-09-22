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
    require(amigaMenuDisplayHStop - amigaMenuDisplayHStart == 160,
            "intro display window is centered and 160 pixels wide");
    require(amigaMenuFetchStop - amigaMenuFetchStart == 80,
            "eleven-word intro fetch uses ten DDF intervals");
    require(amigaMenuModulo == 42,
            "intro modulo skips the unused cached row bytes");

    AmigaMenuScrollState state = amigaMenuScrollState(0, 284);
    require(state.foregroundOffset == 0 && state.backgroundOffset == 0 &&
                state.bplcon1 == 0,
            "aligned intro origin needs no pointer or fine scroll");

    state = amigaMenuScrollState(95, 300);
    require(state.foregroundOffset == 16 * 64 + 10,
            "foreground pointer advances in sixteen-pixel words");
    require(state.backgroundOffset == 16 * 64,
            "background pointer retains its slower coarse position");
    require(state.bplcon1 == 0xff,
            "independent fine-scroll nibbles preserve parallax remainder");

    state = amigaMenuScrollState(192, 384);
    require(state.foregroundOffset == 100 * 64 + 24 &&
                state.backgroundOffset == 100 * 64 + 4,
            "both cached planes share vertical pointer scrolling");
    require(state.bplcon1 == 0,
            "word-aligned parallax offsets need no fine scroll");

    std::cout << "Amiga menu hardware-scroll tests passed\n";
    return 0;
}

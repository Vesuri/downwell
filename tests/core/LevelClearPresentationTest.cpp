#include "core/LevelClearPresentation.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
    using namespace downwell;
    for (int area = 1; area <= 4; ++area) {
        for (int level = 1; level <= 3; ++level) {
            LevelClearPresentation state;
            int tileY = 0, border = 36;
            unsigned circle = 0, time = 0;
            for (unsigned tick = 0; tick < 500; ++tick) {
                const int next = tick < 60 ? 0 : 4;
                const bool choices = tick >= 60 && tick < 100;
                if (tick == 60) { state.circle = 0; circle = 0; }
                if (tick == 75) { state.time = 0; time = 0; }
                const auto frame = state.advance(area, level, next, choices);
                for (unsigned render = 0; render < 2; ++render) {
                    unsigned count = 0;
                    frame.borders([&](int emittedArea, int spriteFrame, int x, int y) {
                        const bool incoming = count >= unsigned(2 * (border + 1));
                        const unsigned local = incoming ? count - 2 * (border + 1) : count;
                        const int row = int(local / 2) + (incoming ? border : 0);
                        require(emittedArea == area + int(incoming) &&
                            spriteFrame == (local % 2 ? 7 : 13) &&
                            x == (incoming ? (local % 2 ? 152 : 8) : (local % 2 ? 160 : 0)) &&
                            y == tileY + row * 16, "source border command ordering and coordinates");
                        ++count;
                    });
                    require(count == 76, "inclusive boundary row is emitted for both areas");
                }
                tileY -= area == 3 ? 3 : 6;
                if (tileY <= -16) {
                    tileY += 16;
                    if (next > 2 && level == 3 && border > 0) --border;
                }
                circle += 3;
                if (choices) ++time;
                require(state.tileY == tileY && state.borderRow == border && frame.circle == circle &&
                    frame.time == time && frame.choices == choices,
                    "source scroll, boundary replacement and animation clocks");
            }
        }
    }
    std::puts("Result border snapshots and presentation clocks passed across all four areas");
}

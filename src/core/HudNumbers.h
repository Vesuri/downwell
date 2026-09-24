#ifndef DOWNWELL_CORE_HUD_NUMBERS_H
#define DOWNWELL_CORE_HUD_NUMBERS_H

#include "Renderer.h"

namespace downwell {

// drawSpriteAmmoNumber: all outlines precede the foreground digits.
template<class Draw>
void drawAmmoNumberLayers(int x, int y, unsigned value, Draw draw) {
    unsigned char digits[10];
    unsigned count = 0;
    do { digits[count++] = value % 10; value /= 10; } while (value);
    const int firstX = x - (static_cast<int>(count) - 1) * 4;
    const int dx[] = {-1,0,1,0,-1,0,1,0,0,1,2,1,0,1};
    const int dy[] = {0,-1,0,1,1,0,1,2,0,-1,0,1,1,0};
    for (unsigned i = 0; i < count; ++i)
        for (unsigned layer = 0; layer < 14; ++layer)
            draw(digits[count - 1 - i] + (layer < 12 ? 20 : 10),
                 firstX + static_cast<int>(i) * 8 + dx[layer],
                 y + static_cast<int>(i) + dy[layer]);
    for (unsigned i = 0; i < count; ++i)
        draw(digits[count - 1 - i], firstX + static_cast<int>(i) * 8,
             y + static_cast<int>(i));
}

// scrGemmyText: source order matters where the four offset glyphs overlap.
template<class Draw>
void drawGemmyTextLayers(int x, int y, Draw draw) {
    draw(x - 1, y, Color{255, 0, 0, 255});
    draw(x, y - 1, Color{255, 0, 0, 255});
    draw(x + 1, y, Color{0, 0, 0, 255});
    draw(x, y + 1, Color{0, 0, 0, 255});
    draw(x, y, Color{255, 255, 255, 255});
}

void drawHudHealthNumber(Renderer& renderer, int x, int y,
                         int health, int maximum);
void drawHudCurrencyNumber(Renderer& renderer, int x, int y, int value);
void drawHudAmmoNumber(Renderer& renderer, int x, int y, int value);

} // namespace downwell

#endif

#ifndef DOWNWELL_CORE_HUD_NUMBERS_H
#define DOWNWELL_CORE_HUD_NUMBERS_H

#include "Renderer.h"

namespace downwell {

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

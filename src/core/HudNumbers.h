#ifndef DOWNWELL_CORE_HUD_NUMBERS_H
#define DOWNWELL_CORE_HUD_NUMBERS_H

#include "Renderer.h"

namespace downwell {

void drawHudHealthNumber(Renderer& renderer, int x, int y,
                         int health, int maximum);
void drawHudCurrencyNumber(Renderer& renderer, int x, int y, int value);
void drawHudAmmoNumber(Renderer& renderer, int x, int y, int value);

} // namespace downwell

#endif

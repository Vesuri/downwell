#ifndef DOWNWELL_AMIGA_GAMEPLAY_EFFECTS_H
#define DOWNWELL_AMIGA_GAMEPLAY_EFFECTS_H
#include "AmigaRenderer.h"
#include "core/BinaryEffectSurface.h"
#include "core/PaletteEffectSurface.h"
class Bitmap;
class AmigaGameplayRows;

// Publish at the controller's painter-order boundary, after terrain restoration.
// Does not own scene timing, surface construction, or a second terrain cache.
class AmigaGameplayEffects {
public:
    static void draw(AmigaRenderer renderer, Bitmap& display, unsigned buffer,
                     unsigned origin, const downwell::BinaryEffectSurface<>& surface,
                     AmigaGameplayRows& rows);
    static void draw(AmigaRenderer renderer, Bitmap& display, unsigned buffer,
                     unsigned origin, const downwell::PaletteEffectSurface<>& surface,
                     AmigaGameplayRows& rows);
};
#endif

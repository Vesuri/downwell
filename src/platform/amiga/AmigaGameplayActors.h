#ifndef DOWNWELL_AMIGA_GAMEPLAY_ACTORS_H
#define DOWNWELL_AMIGA_GAMEPLAY_ACTORS_H
#include "AmigaRenderer.h"
#include "core/LaserSightLine.h"
class Bitmap;
class AmigaGameplayRows;
class AmigaLaserSight;
// Marker in painter order; never looked up as a packed sprite.
static constexpr UWORD gameplayRayMarker = 65535;
struct GameplayBob { UWORD sprite, frame; int x, y, flip; };
class AmigaGameplayActors {
public:
    // Draw only into the retired display ring, not the terrain backing.
    static void draw(AmigaRenderer renderer, Bitmap& display,
        unsigned buffer, unsigned origin, const GameplayBob* bobs, unsigned count,
        const UBYTE* pack, const UBYTE* packEnd, AmigaGameplayRows& rows,
        AmigaLaserSight& laser, const downwell::LaserSightLine* ray = nullptr);
};
#endif

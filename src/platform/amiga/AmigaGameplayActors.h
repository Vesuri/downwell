#ifndef DOWNWELL_AMIGA_GAMEPLAY_ACTORS_H
#define DOWNWELL_AMIGA_GAMEPLAY_ACTORS_H
#include "AmigaRenderer.h"
#include "core/LaserSightLine.h"
#include "core/PaletteEffectSurface.h"
class Bitmap;
class Sprite;
class AmigaGameplayRows;
class AmigaLaserSight;
// Marker in painter order; never looked up as a packed sprite.
static constexpr UWORD gameplayRayMarker = 65535;
// x/y are text anchor, flip stores the signed integer damage, frame is unused.
static constexpr UWORD gameplayDamageTextMarker = 65534;
struct GameplayBob { UWORD sprite, frame; int x, y, flip; };
class AmigaGameplayActors {
public:
    static constexpr unsigned regularChannels = 6;
    struct Assignment {
        unsigned bobCount;
        bool ready;
        UWORD failedSprite, failedFrame;
    };
    // Channels are player, six regular slots, overflow. Select the frontmost
    // suffix only: a failed fit or planar ray forbids backfilling behind it.
    // Final controller alpha overlapping an actor canvas is another planar
    // boundary. Actors ahead of it remain eligible; holes permit DMA sprites.
    static Assignment assign(const GameplayBob* actors, unsigned count,
        const UBYTE* pack, const UBYTE* packEnd, Sprite& player,
        Sprite* regular, Sprite& overflow, unsigned height, unsigned limit = 8,
        const downwell::PaletteEffectSurface<>* controller = nullptr);
    // Draw only into the retired display ring, not the terrain backing.
    static void draw(AmigaRenderer renderer, Bitmap& display,
        unsigned buffer, unsigned origin, const GameplayBob* bobs, unsigned count,
        const UBYTE* pack, const UBYTE* packEnd, AmigaGameplayRows& rows,
        AmigaLaserSight& laser, const downwell::LaserSightLine* ray = nullptr);
};
#endif

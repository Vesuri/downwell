#ifndef DOWNWELL_AMIGA_GAMEPLAY_ACTORS_H
#define DOWNWELL_AMIGA_GAMEPLAY_ACTORS_H
#include "AmigaRenderer.h"
#include "AmigaActorFrames.h"
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
struct GameplayBob {
    UWORD sprite, frame; int x, y, flip;
    const AmigaActorFrames::Frame* prepared=nullptr;
};
class AmigaGameplayActors {
public:
    static constexpr unsigned regularChannels = 6;
    static bool prepareDamageText(const UBYTE* pack, const UBYTE* packEnd);
    struct Assignment {
        unsigned bobCount;
        bool ready;
        UWORD failedSprite, failedFrame;
    };
    // Channels are player, six regular slots, overflow, in front-to-back
    // priority. Disjoint planar occluders may be passed; overlapping ones
    // remain boundaries. Compact unassigned actors in stable painter order.
    static Assignment assign(GameplayBob* actors, unsigned count,
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

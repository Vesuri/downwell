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
// Clipped opaque-black rectangle: frame=width, flip=height. Never a sprite.
static constexpr UWORD gameplayBlackRectMarker = 65533;
// scrDrawBorderTextBlack glyph: frame=offline glyph ID (origin (4,0)), x/y
// its anchor, flip the fill pen. Four black neighbours, then the fill.
static constexpr UWORD gameplayGlyphMarker = 65532;
// Clipped opaque-white (pen 1) rectangle, laid out as the black one: the
// Heart Balloon's string in one-pixel runs.
static constexpr UWORD gameplayWhiteRectMarker = 65531;
// Offline font-2 border glyph IDs (origin (4,0)).
inline unsigned amigaBorderGlyph(char c){
    if(c>='0'&&c<='9')return 65100u+unsigned(c-'0');
    if(c=='+')return 65111u;
    if(c=='!')return 65112u;
    if(c=='?')return 65113u;
    if(c=='-')return 65110u;
    return 65120u+unsigned(c-'A');
}
static constexpr UWORD gameplayDitherEdge = 65006;
struct GameplayBob {
    UWORD sprite, frame; int x, y, flip;
    const AmigaActorFrames::Frame* prepared=nullptr;
};
class AmigaGameplayActors {
public:
    static constexpr unsigned regularChannels = 6;
    static bool prepareDamageText(const UBYTE* pack, const UBYTE* packEnd);
    static void releaseDamageText();
    struct Assignment {
        unsigned bobCount;
        bool ready;
        UWORD failedSprite, failedFrame;
    };
    // Channels are player, six regular slots, overflow, in front-to-back
    // priority. Disjoint planar occluders may be passed; overlapping ones
    // remain boundaries. Compact unassigned actors in stable painter order.
    // Actor x is in the steady view; spriteShiftX moves hardware sprites
    // with the display's screen shake (the BOBs move with the playfield).
    static Assignment assign(GameplayBob* actors, unsigned count,
        const UBYTE* pack, const UBYTE* packEnd, Sprite& player,
        Sprite* regular, Sprite& overflow, unsigned height, unsigned limit = 8,
        const downwell::PaletteEffectSurface<>* controller = nullptr,
        unsigned controllerSuffix = 0, int spriteShiftX = 0);
    // Draw only into the retired display ring, not the terrain backing.
    static void draw(AmigaRenderer renderer, Bitmap& display,
        unsigned buffer, unsigned origin, const GameplayBob* bobs, unsigned count,
        const UBYTE* pack, const UBYTE* packEnd, AmigaGameplayRows& rows,
        AmigaLaserSight& laser, const downwell::LaserSightLine* ray = nullptr);
};
#endif

#include "AmigaGameplayActors.h"
#include "AmigaGameplayRows.h"
#include "AmigaLaserSight.h"
#include "AmigaProfile.h"
#include "core/DamageNumberText.h"
#include "framework/Bitmap.h"
#include "framework/AmigaHardware.h"
#ifdef DOWNWELL_SCENE_VERIFY
extern "C" { extern volatile ULONG gDownwellGameplayBobChecks, gDownwellGameplayBobFailures; }
#ifdef DOWNWELL_BOB_VERIFY
extern "C" void downwellGameplayBobsReady();
#endif
#endif

// Overflow actors are ordinary masked planar blits into the inactive display,
// never into the pristine terrain backing. Only their touched rows are restored
// before this display buffer is reused.
void AmigaGameplayActors::draw(AmigaRenderer renderer, Bitmap& display,
                             unsigned buffer, unsigned origin, const GameplayBob* bobs,
                             unsigned count, const UBYTE* pack, const UBYTE* packEnd,
                             AmigaGameplayRows& rows, AmigaLaserSight& laser,
                             const downwell::LaserSightLine* ray) {
#ifndef DOWNWELL_SCENE_VERIFY
    (void)ray; // Only the independent scalar oracle needs the source line.
#endif
    renderer.game = (UBYTE*)display.data;
    renderer.target = renderer.game;
    amigaRendererSetGameOrigin(&renderer, origin);
    amigaRendererSetClipRect(&renderer, downwellPlayfieldX, 0,
                             downwellPlayfieldWidth, downwellDisplayHeight);
#ifdef DOWNWELL_SCENE_VERIFY
    alignas(2) static UBYTE expected[downwellGameRingBytes];
    for (unsigned byte = 0; byte < downwellGameRingBytes; ++byte)
        expected[byte] = renderer.game[byte];
    AmigaRenderer oracle = renderer;
    oracle.game = oracle.target = expected;
    oracle.blitterEnabled = 0;
#endif
    for (unsigned i = 0; i < count; ++i) {
        const GameplayBob& bob = bobs[i];
        if(bob.sprite==gameplayDamageTextMarker){
            AmigaHardware::blitterDrain();
            AmigaRenderer textRenderer=renderer;textRenderer.blitterEnabled=0;
            downwell::DamageNumberText label;label.formatInteger(bob.flip);
            label.draw(bob.x,bob.y,[&](unsigned glyph,int x,int y,unsigned pen){
                amigaRendererDrawAssetPen(&textRenderer,pack,packEnd,glyph,0,x,y,0,pen);
#ifdef DOWNWELL_SCENE_VERIFY
                amigaRendererDrawAssetPen(&oracle,pack,packEnd,glyph,0,x,y,0,pen);
#endif
            });
            int top=bob.y-1,bottom=bob.y+11;
            if(top<0)top=0;
            if(bottom>downwellDisplayHeight)bottom=downwellDisplayHeight;
            if(bottom>top)rows.damage(origin,top,bottom-top,(UBYTE)(1u<<buffer));
            continue;
        }
        if (bob.sprite == gameplayRayMarker) {
            DW_PROFILE_SCOPE(profileLaserDraw);
            { DW_PROFILE_SCOPE(profileLaserDrawDrain); AmigaHardware::blitterDrain(); }
            laser.draw();
#ifdef DOWNWELL_SCENE_VERIFY
            if (ray) ray->draw([&](int x, int y) { amigaRendererFillRect(&oracle, x, y, 1, 1, 2); });
#endif
            continue;
        }
        DW_PROFILE_SCOPE(profileGameplayBobs);
        int x, y, width, height;
        if (!amigaRendererAssetBounds(pack, packEnd,
                bob.sprite, bob.frame, bob.x, bob.y, bob.flip,
                &x, &y, &width, &height)) continue;
        if (x >= downwellPlayfieldX + downwellPlayfieldWidth ||
            x + width <= downwellPlayfieldX || y >= downwellDisplayHeight ||
            y + height <= 0) continue;
        amigaRendererDrawAsset(&renderer, pack, packEnd,
                               bob.sprite, bob.frame, bob.x, bob.y, bob.flip);
#ifdef DOWNWELL_SCENE_VERIFY
        amigaRendererDrawAsset(&oracle, pack, packEnd,
                               bob.sprite, bob.frame, bob.x, bob.y, bob.flip);
        ++gDownwellGameplayBobChecks;
#endif
        int bottom = y + height;
        if (y < 0) y = 0;
        if (bottom > downwellDisplayHeight) bottom = downwellDisplayHeight;
        // Only this display contains the BOB. Preserve pending terrain damage
        // in either buffer, but do not invalidate the untouched other display.
        rows.damage(origin, y, bottom - y, (UBYTE)(1u << buffer));
    }
    AmigaHardware::blitterDrain();
#ifdef DOWNWELL_SCENE_VERIFY
    for (unsigned byte = 0; byte < downwellGameRingBytes; ++byte)
        if (expected[byte] != renderer.game[byte]) ++gDownwellGameplayBobFailures;
#ifdef DOWNWELL_BOB_VERIFY
    if (gDownwellGameplayBobChecks >= 100) downwellGameplayBobsReady();
#endif
#endif
}

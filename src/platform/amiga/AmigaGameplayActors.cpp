#include "AmigaGameplayActors.h"
#include "AmigaGameplayRows.h"
#include "AmigaLaserSight.h"
#include "AmigaProfile.h"
#include "AmigaDamageText.h"
#include "core/DamageNumberText.h"
#include "framework/Bitmap.h"
#include "framework/AmigaHardware.h"
#ifdef DOWNWELL_SCENE_VERIFY
extern "C" { extern volatile ULONG gDownwellGameplayBobChecks, gDownwellGameplayBobFailures; }
#ifdef DOWNWELL_BOB_VERIFY
extern "C" void downwellGameplayBobsReady();
extern "C" { volatile unsigned gDownwellDamageTextChecks; }
#endif
#endif

namespace { AmigaDamageGlyphs damageGlyphs; bool damageGlyphsReady; AmigaDamageTextCache damageText; }
bool AmigaGameplayActors::prepareDamageText(const UBYTE* pack, const UBYTE* end) {
    damageText.invalidate();
    damageGlyphsReady=amigaRendererPrepareDamageGlyphs(&damageGlyphs,pack,end)!=0;
    if(damageGlyphsReady)damageText.bindGlyphs(damageGlyphs);
    return damageGlyphsReady;
}

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
            DW_PROFILE_SCOPE(profileDamageNumberDraw);
            AmigaHardware::blitterDrain();
            AmigaRenderer textRenderer=renderer;textRenderer.blitterEnabled=0;
            downwell::DamageNumberText label;
#if defined(DOWNWELL_REFERENCE_DAMAGE_TEXT) || defined(DOWNWELL_SCENE_VERIFY)
            label.formatInteger(bob.flip);
#else
            if(!damageGlyphsReady)label.formatInteger(bob.flip);
#endif
            if (damageGlyphsReady) {
#ifdef DOWNWELL_REFERENCE_DAMAGE_TEXT
                AmigaDamageText::draw(textRenderer,damageGlyphs,label,bob.x,bob.y);
#else
                damageText.draw(textRenderer,damageGlyphs,bob.flip,bob.x,bob.y);
#endif
            }
            if (!damageGlyphsReady) label.draw(bob.x,bob.y,[&](unsigned glyph,int x,int y,unsigned pen){
                amigaRendererDrawAssetPen(&textRenderer,pack,packEnd,glyph,0,x,y,0,pen);
            });
#ifdef DOWNWELL_SCENE_VERIFY
            label.draw(bob.x,bob.y,[&](unsigned glyph,int x,int y,unsigned pen){
                amigaRendererDrawAssetPen(&oracle,pack,packEnd,glyph,0,x,y,0,pen);
            });
#endif
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
#ifdef DOWNWELL_BOB_VERIFY
    // The short overflow tape need not cause combat damage. Exercise the
    // prepared text path explicitly without changing simulation or RNG.
    if (damageGlyphsReady && gDownwellDamageTextChecks<32) {
        const unsigned sample=gDownwellDamageTextChecks;
        const int values[]={0,1,8,12,99,123456789,-17,(-2147483647-1)};
        downwell::DamageNumberText label;label.formatInteger(values[sample&7]);
        const int x=(sample<16?104:258)+int(sample&15), y=(sample&1)?278:0;
        const unsigned testOrigin=(sample&2)?319:311;
        AmigaRenderer actualText=renderer,referenceText=oracle;
        amigaRendererSetGameOrigin(&actualText,testOrigin);
        amigaRendererSetGameOrigin(&referenceText,testOrigin);
        damageText.draw(actualText,damageGlyphs,values[sample&7],x,y);
        label.draw(x,y,[&](unsigned glyph,int px,int py,unsigned pen){
            amigaRendererDrawAssetPen(&referenceText,pack,packEnd,glyph,0,px,py,0,pen);
        });
        rows.damage(testOrigin,y?y-1:0,y?7:11,(UBYTE)(1u<<buffer));
        ++gDownwellDamageTextChecks;
    }
#endif
    for (unsigned byte = 0; byte < downwellGameRingBytes; ++byte)
        if (expected[byte] != renderer.game[byte]) ++gDownwellGameplayBobFailures;
#ifdef DOWNWELL_BOB_VERIFY
    if (gDownwellGameplayBobChecks >= 100 && gDownwellDamageTextChecks>=32) downwellGameplayBobsReady();
#endif
#endif
}

#include "AmigaGameplayActors.h"
#include "AmigaActorChannels.h"
#include "AmigaProfile.h"
#include "framework/Sprite.h"
#include "core/DamageNumberText.h"

AmigaGameplayActors::Assignment AmigaGameplayActors::assign(GameplayBob* actors,
    unsigned count, const UBYTE* pack, const UBYTE* packEnd, Sprite& player,
    Sprite* regular, Sprite& overflow, unsigned height, unsigned limit,
    const downwell::PaletteEffectSurface<>* controller,unsigned controllerSuffix) {
    Assignment result{count, true, 0, 0};
    if(controllerSuffix>count){result.ready=false;return result;}
    AmigaActorChannels channels(limit);
    auto slot = [&](unsigned channel) -> Sprite& {
        return channel == 0 ? player : channel <= regularChannels ? regular[channel - 1] : overflow;
    };
#ifndef DOWNWELL_REFERENCE_ACTOR_PREFIX
    // Bounded conservative occluders. Overflow and rays retain the original
    // global boundary, rather than adding an unbounded room-size scan.
    struct Rect {int left,top,right,bottom;};
    Rect planar[16];unsigned planarCount=0;
    unsigned selected[8],selectedCount=0;
    const auto overlaps=[](const Rect& a,const Rect& b){
        return a.left<b.right&&b.left<a.right&&a.top<b.bottom&&b.top<a.bottom;
    };
    const Rect view{downwellPlayfieldX,0,downwellPlayfieldX+downwellPlayfieldWidth,downwellDisplayHeight};
    const auto retain=[&](Rect r){
        if(!overlaps(r,view))return true;
        if(planarCount==16)return false;
        planar[planarCount++]=r;return true;
    };
    for(unsigned index=count;index;){
        --index;
        const auto& actor=actors[index];
        if(actor.sprite==gameplayRayMarker)break;
        if(actor.sprite==gameplayBlackRectMarker){
            if(!retain({actor.x,actor.y,actor.x+actor.frame,actor.y+actor.flip}))break;
            continue;
        }
        if(actor.sprite==gameplayDitherEdge){
            if(!retain({actor.x-160,actor.y,actor.x+160,actor.y+80}))break;
            continue;
        }
        if(actor.sprite==gameplayDamageTextMarker){
            downwell::DamageNumberText text;text.formatInteger(actor.flip);
            const int radius=int(text.length)*4+1;
            if(!retain({actor.x-radius,actor.y-1,actor.x+radius,actor.y+11}))break;
            continue;
        }
        if(controller||planarCount){
            int x,y,width,height;
            if(actor.prepared)actor.prepared->bounds(actor.x,actor.y,actor.flip,x,y,width,height);
            else if(!amigaRendererAssetBounds(pack,packEnd,actor.sprite,actor.frame,
                actor.x,actor.y,actor.flip,&x,&y,&width,&height)){
                result.ready=false;result.failedSprite=actor.sprite;result.failedFrame=actor.frame;break;
            }
            const Rect bounds{x,y,x+width,y+height};
            bool hidden=false;
            for(unsigned i=0;i<planarCount;++i)if(overlaps(bounds,planar[i])){hidden=true;break;}
            if(!hidden&&controller&&index<count-controllerSuffix)hidden=controller->intersects(x-downwellPlayfieldX,y,
                x-downwellPlayfieldX+width,y+height);
            if(hidden){if(!retain(bounds))break;continue;}
        }
#else
    while (result.bobCount > 0) {
        const auto& actor = actors[result.bobCount - 1];
        if (actor.sprite == gameplayRayMarker || actor.sprite == gameplayDamageTextMarker ||
            actor.sprite == gameplayBlackRectMarker || actor.sprite == gameplayDitherEdge) break;
        if (controller && result.bobCount<=count-controllerSuffix) {
            int x,y,width,height;
            if (!amigaRendererAssetBounds(pack,packEnd,actor.sprite,actor.frame,
                    actor.x,actor.y,actor.flip,&x,&y,&width,&height)) {
                result.ready=false;result.failedSprite=actor.sprite;result.failedFrame=actor.frame;
                break;
            }
            // The canvas is conservative (transparent actor pixels included).
            // Stop at the first overlap: backfilling behind that planar actor
            // would reverse world painter order even outside the effect itself.
            if (controller->intersects(x-downwellPlayfieldX,y,x-downwellPlayfieldX+width,y+height)) break;
        }
#endif
        AmigaResolvedSpritePart prepared;
        const auto resolve=[&](unsigned part){
            return actor.prepared ? actor.prepared->part(part,actor.flip,prepared) :
                amigaRendererResolveSpritePart(&prepared,pack,packEnd,actor.sprite,actor.frame,part,actor.flip)!=0;
        };
        if (!resolve(0)) {
            result.ready = false;
            result.failedSprite = actor.sprite; result.failedFrame = actor.frame;
            break;
        }
        const unsigned parts = prepared.partCount;
        const int first = channels.take(parts);
        if (first < 0) break;
        for (unsigned part = 0; part < parts; ++part) {
            DW_PROFILE_SCOPE(profileHardwareSpriteUpdate);
            Sprite& sprite = slot(first + part);
            if ((part && !resolve(part)) ||
                !amigaRendererDrawResolvedSpritePart(&prepared, actor.x, actor.y,
                    sprite.data(), height)) {
                result.ready = false;
                result.failedSprite = actor.sprite; result.failedFrame = actor.frame;
            }
        }
#ifndef DOWNWELL_REFERENCE_ACTOR_PREFIX
        selected[selectedCount++]=index;
#else
        --result.bobCount;
#endif
    }
#ifndef DOWNWELL_REFERENCE_ACTOR_PREFIX
    // Selected indices are descending, so this is one stable linear pass.
    // Copy only the suffix whose positions changed, not every actor record.
    if(selectedCount){
        unsigned remaining=selectedCount,out=0;
        for(unsigned i=0;i<count;++i){
            if(remaining&&selected[remaining-1]==i){--remaining;continue;}
            if(out!=i)actors[out]=actors[i];
            ++out;
        }
        result.bobCount=out;
    }
#endif
    for (unsigned channel = channels.used(); channel < 8; ++channel) slot(channel).hide();
    return result;
}

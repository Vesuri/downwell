#include "AmigaGameplayActors.h"
#include "AmigaActorChannels.h"
#include "AmigaProfile.h"
#include "framework/Sprite.h"

AmigaGameplayActors::Assignment AmigaGameplayActors::assign(const GameplayBob* actors,
    unsigned count, const UBYTE* pack, const UBYTE* packEnd, Sprite& player,
    Sprite* regular, Sprite& overflow, unsigned height, unsigned limit,
    const downwell::PaletteEffectSurface<>* controller) {
    Assignment result{count, true, 0, 0};
    AmigaActorChannels channels(limit);
    auto slot = [&](unsigned channel) -> Sprite& {
        return channel == 0 ? player : channel <= regularChannels ? regular[channel - 1] : overflow;
    };
    while (result.bobCount > 0) {
        const auto& actor = actors[result.bobCount - 1];
        if (actor.sprite == gameplayRayMarker || actor.sprite == gameplayDamageTextMarker) break;
        if (controller) {
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
        AmigaResolvedSpritePart prepared;
        if (!amigaRendererResolveSpritePart(&prepared, pack, packEnd,
                actor.sprite, actor.frame, 0, actor.flip)) {
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
            if ((part && !amigaRendererResolveSpritePart(&prepared, pack, packEnd,
                    actor.sprite, actor.frame, part, actor.flip)) ||
                !amigaRendererDrawResolvedSpritePart(&prepared, actor.x, actor.y,
                    sprite.data(), height)) {
                result.ready = false;
                result.failedSprite = actor.sprite; result.failedFrame = actor.frame;
            }
        }
        --result.bobCount;
    }
    for (unsigned channel = channels.used(); channel < 8; ++channel) slot(channel).hide();
    return result;
}

#include "AmigaControlsScene.h"
#include "AmigaSplashCache.h"
bool AmigaControlsScene::prepare(const UBYTE* cache,unsigned bytes,const Sprite& blank,const Palette& palette) {
    cache_ = nullptr;
    if (!cache || bytes < amigaSplashCacheBytes ||
        !hold_.buildNarrow(amigaSplashFrame(cache,4),blank,palette)) return false;
    cache_ = cache;
    return true;
}
const UBYTE* AmigaControlsScene::pixels() const {
    return cache_ ? amigaSplashFrame(cache_,4) : nullptr;
}
const CopperList* AmigaControlsScene::presentation(int stage,unsigned dither,unsigned slot,
    const Sprite& blank,const Palette& palette,bool& alternating) {
    alternating = false;
    if (!cache_) return nullptr;
    if (stage==4) return &hold_;
    if ((stage!=5 && stage!=6) || slot>=2) return nullptr;
    if (!fades_[slot].buildNarrow(amigaSplashFrame(cache_,stage),blank,palette,
        amigaSplashDitherTile(cache_,dither))) return nullptr;
    alternating = true;
    return &fades_[slot];
}

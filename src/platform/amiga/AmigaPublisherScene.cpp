#include "AmigaPublisherScene.h"
#include "AmigaSplashCache.h"
bool AmigaPublisherScene::prepare(const UBYTE* cache,unsigned bytes,
    const Sprite& blank,const Palette& palette) {
    frame_ = nullptr;
    if (!cache || bytes < amigaSplashCacheBytes) return false;
    const auto* frame = amigaSplashFrame(cache,2);
    if (!list_.buildNarrow(frame,blank,palette)) return false;
    frame_ = frame;
    return true;
}

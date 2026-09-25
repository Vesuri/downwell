#include "AmigaStyleScene.h"
#include "AmigaSceneRuntime.h"

AmigaStyleScene::AmigaStyleScene(UBYTE* cache, unsigned cacheBytes,
    UWORD* firstList, UWORD* secondList, unsigned listWords)
    : cache_(cache), cacheBytes_(cacheBytes),
      lists_{DownwellStyleCopperList(firstList,listWords),
             DownwellStyleCopperList(secondList,listWords)} {}

bool AmigaStyleScene::prepare(const UBYTE* pack, const UBYTE* end) {
    prepared_ = false;
    frames_[0] = {}; frames_[1] = {};
    built_[0] = built_[1] = false;
    if (!cache_ || cacheBytes_ < amigaStyleCacheBytes || !pack || !end ||
        !amigaBuildStyleBorder(pack,end,cache_ + amigaStyleFrameBytes*2)) return false;
    pack_ = pack; end_ = end; prepared_ = true;
    return true;
}

bool AmigaStyleScene::render(unsigned slot, const AmigaSceneRuntime& scenes,
    Frame& frame) {
    frame = {};
    if (!prepared_ || slot >= 2) return false;
    auto* content = cache_ + slot*amigaStyleFrameBytes;
    const auto* border = cache_ + amigaStyleFrameBytes*2;
    if (!amigaRenderStyleFrame(pack_,end_,border,scenes,content,frames_[slot])) return false;
    frame.content = content;
    frame.border = border - scenes.styleTileY()*amigaStyleBytesPerRow;
    frame.mask = scenes.styleDitherY() < 684 || scenes.styleSelected()
        ? amigaStyleMask(cache_,scenes) : nullptr;
    return true;
}

bool AmigaStyleScene::updateCopper(unsigned slot, const Frame& frame,
    const Sprite& nullSprite, const Palette& palette) {
    if (!prepared_ || slot >= 2 || !frame.content || !frame.border) return false;
    if (!(built_[slot] ? lists_[slot].update(frame.content,frame.mask,frame.border)
          : lists_[slot].build(frame.content,frame.mask,nullSprite,palette,frame.border))) return false;
    built_[slot] = true;
    return true;
}

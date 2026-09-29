#include "AmigaStyleScene.h"
#include "AmigaSceneRuntime.h"
#include "AmigaProfile.h"
#include "framework/AmigaHardware.h"

AmigaStyleScene::AmigaStyleScene(UBYTE* cache, unsigned cacheBytes,
    UWORD* firstList, UWORD* secondList, unsigned listWords,
    UBYTE* firstFrame, UBYTE* secondFrame, unsigned frameBytes)
    : cache_(cache), cacheBytes_(cacheBytes),
      targets_{firstFrame,secondFrame},frameBytes_(frameBytes),
      lists_{DownwellStyleCopperList(firstList,listWords,AmigaSelectionLayer::planeBytes),
             DownwellStyleCopperList(secondList,listWords,AmigaSelectionLayer::planeBytes)} {}

bool AmigaStyleScene::prepare(const UBYTE* pack, const UBYTE* end, bool borderPreserved) {
    const bool reuse = borderPreserved && prepared_ && pack_ == pack && end_ == end;
    prepared_ = false;
    layers_[0].invalidate(); layers_[1].invalidate();
    built_[0] = built_[1] = false;
    if (!cache_ || cacheBytes_ < amigaStyleCacheBytes || !pack || !end ||
        !targets_[0] || !targets_[1] || targets_[0]==targets_[1] || frameBytes_<AmigaSelectionLayer::bytes ||
        (!reuse && !amigaBuildStyleBorder(pack,end,cache_ + amigaStyleFrameBytes*2))) return false;
    pack_ = pack; end_ = end; prepared_ = true;
    return true;
}

bool AmigaStyleScene::render(unsigned slot, const AmigaSceneRuntime& scenes,
    Frame& frame) {
    frame = {};
    if (!prepared_ || slot >= 2) return false;
    const int top=AmigaStyleDrawList::baseView-scenes.styleYOffset();
    if(top<0 || top>int(AmigaSelectionLayer::height-284) || !drawList_.build(scenes)) return false;
    const auto* border = cache_ + amigaStyleFrameBytes*2;
    DW_PROFILE_PHASE(profileStyleActors);
    unsigned painted=0;
    if (!layers_[slot].render(targets_[slot],pack_,end_,drawList_.commands,drawList_.count,painted)) return false;
    DW_PROFILE_PHASE(profileStyleDrain);
    AmigaHardware::blitterDrain();
    frame.content = layers_[slot].view(unsigned(top));
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

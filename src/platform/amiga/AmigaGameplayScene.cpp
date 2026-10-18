#include "AmigaGameplayScene.h"
#include "AmigaEntryMask.h"

void AmigaGameplayScene::invalidateBackingOutside(int first, int end, int viewY, int height) {
    if (first >= end) return;
    const int below = first > viewY + height ? first : viewY + height;
    if (end > below && cachedEnd_ > below) cachedEnd_ = below;
    const int above = end < viewY ? end : viewY;
    if (first < above && cachedFirst_ < above) cachedFirst_ = above;
}

AmigaGameplayScene::StageSpan AmigaGameplayScene::exposeBacking(int viewY, int height, int ringHeight) {
    int first = 0, count = 0;
    if (viewY < cachedFirst_) {
        first = viewY - 8;
        count = cachedFirst_ - first;
        cachedFirst_ = first;
        if (cachedEnd_ > cachedFirst_ + ringHeight) cachedEnd_ = cachedFirst_ + ringHeight;
    } else if (viewY + height > cachedEnd_) {
        first = cachedEnd_;
        count = viewY + height + 8 - cachedEnd_;
        cachedEnd_ += count;
        if (cachedFirst_ < cachedEnd_ - ringHeight) cachedFirst_ = cachedEnd_ - ringHeight;
    }
    // A discontinuous camera snap can skip more than an entire ring. None of
    // those skipped rows will be displayed: replace only the destination view
    // and its guards, never write the ring repeatedly for intermediate rows.
    if (count > ringHeight) {
        cachedFirst_ = viewY - 8;
        cachedEnd_ = viewY + height + 8;
        first = cachedFirst_;
        count = cachedEnd_ - cachedFirst_;
    }
    return {first, count};
}

AmigaGameplayScene::StageSpan AmigaGameplayScene::stageSpan(int x, int y, int visibleHeight, int prependBudget) {
    if (x != stagedX_) {
        stagedX_ = x;
        stagedFirst_ = stagedEnd_ = y;
    }
    int first = stagedEnd_, count = y + visibleHeight + 16 - stagedEnd_;
    if (y < stagedFirst_) {
        count = stagedFirst_ - y;
        if (count > 48) count = 48;
        if (count > prependBudget) count = prependBudget > 0 ? prependBudget : 0;
        first = stagedFirst_ - count;
    } else if (count > 48) count = 48;
    return {first, count > 0 ? count : 0};
}

AmigaGameplayScene::StageSpan AmigaGameplayScene::prepareBacking(int viewY,int height,int ringHeight,int budget)const{
    if(budget<16||height+24>ringHeight||cachedFirst_>viewY||cachedEnd_<viewY+height)return {0,0};
    const int target=viewY+height+24;
    int count=target-cachedEnd_;
    if(count<16)return {0,0};
    if(count>budget)count=budget;
    return {cachedEnd_,count>0?count:0};
}

bool AmigaGameplayScene::preparedBacking(StageSpan span,int ringHeight){
    if(span.count<=0||span.count>ringHeight||span.first!=cachedEnd_)return false;
    cachedEnd_+=span.count;
    if(cachedFirst_<cachedEnd_-ringHeight)cachedFirst_=cachedEnd_-ringHeight;
    return true;
}

void AmigaGameplayScene::staged(StageSpan span, int ringHeight) {
    if (span.count <= 0) return;
    if (stagedFirst_ < stagedEnd_ &&
        (span.first > stagedEnd_ || span.first + span.count < stagedFirst_)) {
        invalidateStage(); // Never claim the unprepared gap between intervals.
        return;
    }
    if (span.first < stagedFirst_) stagedFirst_ = span.first;
    if (span.first + span.count > stagedEnd_) stagedEnd_ = span.first + span.count;
    if (stagedEnd_ - stagedFirst_ > ringHeight) {
        if (span.first == stagedFirst_) stagedEnd_ = stagedFirst_ + ringHeight;
        else stagedFirst_ = stagedEnd_ - ringHeight;
    }
}

AmigaGameplayScene::AmigaGameplayScene(UWORD* first, UWORD* second, unsigned capacity)
    : lists_{DownwellGameplayCopperList(first, capacity),
             DownwellGameplayCopperList(second, capacity)} {}

bool AmigaGameplayScene::build(unsigned slot, const Bitmap& game, const Bitmap& hud,
    const Sprite& player, const Sprite* actors, unsigned count,
    const Sprite& overflow, const Palette& palette, unsigned origin) {
    if (slot >= 2) return false;
    ready_[slot] = lists_[slot].build(game, hud, player, actors, count, overflow, palette, origin);
    if(ready_[slot])coverReferenced_[slot]=false;
    return ready_[slot];
}

bool AmigaGameplayScene::update(unsigned slot, const Bitmap& game, const Bitmap& hud,
    const Sprite& player, const Sprite* actors, unsigned count,
    const Sprite& overflow, const Palette& palette, unsigned origin) {
    if (slot >= 2) return false;
    if (!ready_[slot]) return build(slot, game, hud, player, actors, count, overflow, palette, origin);
    ready_[slot] = lists_[slot].update(game, origin);
    return ready_[slot];
}

bool AmigaGameplayScene::mask(unsigned slot, const UBYTE* mask) {
    if (slot >= 2 || !ready_[slot]) return false;
    ready_[slot] = lists_[slot].updateExitMask(mask);
    if(ready_[slot])coverReferenced_[slot]=mask!=nullptr;
    return ready_[slot];
}

bool AmigaGameplayScene::entryMask(unsigned slot,const UBYTE* storage,unsigned bytes,int screenY){
    unsigned offset;
    if(!storage || bytes!=AmigaEntryMask::bytes ||
       (reinterpret_cast<unsigned long>(storage)&1u) ||
       !AmigaEntryMask::offset(screenY,offset))return false;
    return mask(slot,storage+offset);
}

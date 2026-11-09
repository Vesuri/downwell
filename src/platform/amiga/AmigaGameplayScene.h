#ifndef DOWNWELL_AMIGA_GAMEPLAY_SCENE_H
#define DOWNWELL_AMIGA_GAMEPLAY_SCENE_H
#include "DownwellCopperLists.h"

// Owns gameplay's alternating Copper programs and backing/staging validity.
// Storage is borrowed from other mutually exclusive scenes. The application
// retires DMA before invalidate/reuse and publishes only successful frames.
class AmigaGameplayScene {
    DownwellGameplayCopperList lists_[2];
    bool ready_[2] = {false, false};
    bool coverReferenced_[2] = {false, false};
    int stagedX_ = -10000, stagedFirst_ = 0, stagedEnd_ = 0;
    int cachedFirst_ = 0, cachedEnd_ = 0;
public:
    struct StageSpan { int first, count; };
    static constexpr int noStageColumn = 10000;
    // Rendering prediction only, never a camera rule. Retain completed work
    // while stopped/turning; the normal switch path handles a late prediction.
    static int stageColumn(int playerX, long velocityX, int cameraX, bool dead) {
        if (dead) return noStageColumn;
        if (!cameraX) {
            if (playerX < -16 && velocityX < 0) return -160;
            if (playerX > 16 && velocityX > 0) return 160;
        } else if ((cameraX < 0 && playerX > -144 && velocityX > 0) ||
                   (cameraX > 0 && playerX < 144 && velocityX < 0)) return 0;
        return noStageColumn;
    }
    void setBackingRange(int first, int end) { cachedFirst_ = first; cachedEnd_ = end; }
    // Visible damage is repainted by the caller. Retained offscreen damage
    // must leave the valid interval before the camera can expose it.
    void invalidateBackingOutside(int first, int end, int viewY, int height);
    // Requires height + 16 <= ringHeight (284 visible rows, 320 allocated).
    // Caller must paint the returned span before publishing the frame.
    StageSpan exposeBacking(int viewY, int height, int ringHeight);
    // Predict pixels only. The range remains uncommitted until painting
    // succeeds; never overwrite rows that still belong to the visible view.
    StageSpan prepareBacking(int viewY,int height,int ringHeight,int budget)const;
    bool preparedBacking(StageSpan span,int ringHeight);
    int backingFirst() const { return cachedFirst_; }
    int backingEnd() const { return cachedEnd_; }
    // Staging belongs to the scene, independently of the Copper slot lifetime.
    // A span is committed only after its rows have been copied or freshly
    // composed. A preparation budget may complete only a prefix of a request.
    void invalidateStage() { stagedX_ = -10000; }
    void resetStage() { invalidateStage(); stagedFirst_ = stagedEnd_ = 0; }
    // Prepends must fit the cold-paint budget: committing only their prefix
    // would leave an uninitialized gap before the previous valid interval.
    StageSpan stageSpan(int x, int y, int visibleHeight, int prependBudget = 48);
    void staged(StageSpan span, int ringHeight);
    bool stageCovers(int x, int y, int height) const {
        return stagedX_ == x && stagedFirst_ <= y && stagedEnd_ >= y + height;
    }
    int stageFirst() const { return stagedFirst_; }
    int stageEnd() const { return stagedEnd_; }
    AmigaGameplayScene(UWORD* first, UWORD* second, unsigned capacity);
    AmigaGameplayScene(const AmigaGameplayScene&) = delete;
    AmigaGameplayScene& operator=(const AmigaGameplayScene&) = delete;
    void invalidate() { ready_[0] = ready_[1] = false; }
    void invalidate(unsigned slot) { if (slot < 2) ready_[slot] = false; }
    bool build(unsigned slot, const Bitmap& game, const Bitmap& hud,
        const Sprite& player, const Sprite* actors, unsigned count,
        const Sprite& overflow, const Palette& palette, unsigned origin);
    bool update(unsigned slot, const Bitmap& game, const Bitmap& hud,
        const Sprite& player, const Sprite* actors, unsigned count,
        const Sprite& overflow, const Palette& palette, unsigned origin);
    bool mask(unsigned slot, const UBYTE* mask);
    // Borrow immutable Chip RAM until both displayed Copper slots retire it.
    // Validation failure leaves the slot unchanged. No conversion or repaint.
    bool entryMask(unsigned slot, const UBYTE* storage, unsigned bytes, int screenY);
    // Both lists must be valid and detached. The caller must additionally
    // cross VBlank after publication before overwriting the old DMA bank.
    bool coverDetached()const{
        return ready_[0]&&ready_[1]&&!coverReferenced_[0]&&!coverReferenced_[1];
    }
    const DownwellGameplayCopperList& copper(unsigned slot) const { return lists_[slot]; }
    // W1.10 PALETTE: rewrite both programs' colour moves in place (the
    // paused world publishes no new list).
    void repaint(const Palette& palette) { lists_[0].repaintColours(palette); lists_[1].repaintColours(palette); }
};
#endif

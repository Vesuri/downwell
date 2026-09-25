#ifndef DOWNWELL_AMIGA_GAMEPLAY_SCENE_H
#define DOWNWELL_AMIGA_GAMEPLAY_SCENE_H
#include "DownwellCopperLists.h"

// Owns gameplay's alternating Copper programs and side-view staging validity.
// Storage is borrowed from other mutually exclusive scenes. The application
// retires DMA before invalidate/reuse and publishes only successful frames.
class AmigaGameplayScene {
    DownwellGameplayCopperList lists_[2];
    bool ready_[2] = {false, false};
    int stagedX_ = -10000, stagedFirst_ = 0, stagedEnd_ = 0;
public:
    struct StageSpan { int first, count; };
    // Staging belongs to the scene, independently of the Copper slot lifetime.
    // A span is committed only after its clean source rows have been copied.
    void invalidateStage() { stagedX_ = -10000; }
    void resetStage() { invalidateStage(); stagedFirst_ = stagedEnd_ = 0; }
    StageSpan stageSpan(int x, int y, int visibleHeight);
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
    const DownwellGameplayCopperList& copper(unsigned slot) const { return lists_[slot]; }
};
#endif

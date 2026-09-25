#ifndef DOWNWELL_AMIGA_GAMEPLAY_SCENE_H
#define DOWNWELL_AMIGA_GAMEPLAY_SCENE_H
#include "DownwellCopperLists.h"

// Owns gameplay's alternating Copper programs and their build/patch lifecycle.
// Storage is borrowed from other mutually exclusive scenes. The application
// retires DMA before invalidate/reuse and publishes only successful frames.
class AmigaGameplayScene {
    DownwellGameplayCopperList lists_[2];
    bool ready_[2] = {false, false};
public:
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

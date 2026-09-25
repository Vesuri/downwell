#include "AmigaGameplayScene.h"

AmigaGameplayScene::AmigaGameplayScene(UWORD* first, UWORD* second, unsigned capacity)
    : lists_{DownwellGameplayCopperList(first, capacity),
             DownwellGameplayCopperList(second, capacity)} {}

bool AmigaGameplayScene::build(unsigned slot, const Bitmap& game, const Bitmap& hud,
    const Sprite& player, const Sprite* actors, unsigned count,
    const Sprite& overflow, const Palette& palette, unsigned origin) {
    if (slot >= 2) return false;
    ready_[slot] = lists_[slot].build(game, hud, player, actors, count, overflow, palette, origin);
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
    return ready_[slot];
}

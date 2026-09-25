#include "AmigaUpgradeScene.h"
#include "core/GameplayWorld.h"

bool AmigaUpgradeScene::enter(GameplayWorld& world, unsigned style,
    const downwell::Input& input, UBYTE* storage, unsigned storageBytes,
    unsigned hudBytes, const UBYTE* art, unsigned artBytes,
    const UBYTE* mask, unsigned maskBytes) {
    if (!world.enterLevelClear(style, heartGoal_) ||
        !assets.prepare(storage, storageBytes, hudBytes, art, artBytes,
            mask, maskBytes, nullptr, 0, world.runStage().area, world.runStage().level))
        return false;
    presenter.reset(); offerCount_ = 0;
    left_ = input.left; right_ = input.right;
    for (unsigned i = 0; i < 23; ++i) owned_[i] = world.upgrades().levels[i];
    presented = true;
    return true;
}

bool AmigaUpgradeScene::step(GameplayWorld& world, const downwell::Input& input) {
    bool confirm = input.fire;
    if (!world.stepLevelClear(confirm, input.left && !left_, input.right && !right_,
        heartGoal_, [](downwell::LevelClearSelection::Effect, unsigned) {})) return false;
    left_ = input.left; right_ = input.right;
    return true;
}

bool AmigaUpgradeScene::needsArtwork(const GameplayWorld& world) const {
    const auto& room = world.levelClear();
    const unsigned count = world.levelClearFrame().choices ? room.selection.lastIndex + 1 : 0;
    if (count != offerCount_) return true;
    for (unsigned i = 0; i < count; ++i) if (offers_[i] != room.offers.ids[i]) return true;
    for (unsigned i = 0; i < 23; ++i) if (owned_[i] != world.upgrades().levels[i]) return true;
    return false;
}

bool AmigaUpgradeScene::refreshArtwork(const GameplayWorld& world, UBYTE* storage,
    const UBYTE* art, unsigned artBytes) {
    const auto& room = world.levelClear();
    const unsigned count = world.levelClearFrame().choices ? room.selection.lastIndex + 1 : 0;
    if (!assets.changeOffers(storage, art, artBytes, room.offers.ids, count,
        world.runStage().area, world.runStage().level, world.upgrades().levels)) return false;
    offerCount_ = count;
    for (unsigned i = 0; i < count; ++i) offers_[i] = room.offers.ids[i];
    for (unsigned i = 0; i < 23; ++i) owned_[i] = world.upgrades().levels[i];
    presenter.artworkChanged();
    return true;
}

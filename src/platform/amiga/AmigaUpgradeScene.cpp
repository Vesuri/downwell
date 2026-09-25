#include "AmigaUpgradeScene.h"
#include "core/GameplayWorld.h"
#include "AmigaMenuScroll.h"
#include "core/Profile.h"

bool AmigaUpgradeScene::enter(GameplayWorld& world, unsigned style,
    const downwell::Input& input, UBYTE* storage, unsigned storageBytes,
    unsigned hudBytes, const UBYTE* art, unsigned artBytes,
    const UBYTE* mask, unsigned maskBytes) {
    if (!world.enterLevelClear(style, heartGoal_) ||
        !assets.prepare(storage, storageBytes, hudBytes, art, artBytes,
            mask, maskBytes, nullptr, 0, world.runStage().area, world.runStage().level))
        return false;
    offerCount_ = 0;
    sourceArt_ = art; sourceArtBytes_ = artBytes;
    area_ = world.runStage().area; level_ = world.runStage().level;
    for (unsigned i=0;i<2;++i) { borders_[i].invalidate(); layers_[i].invalidate(); }
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
    // Offer changes only reselect records from this immutable source. A new
    // stage/source must enter a new scene instead of preserving old pixels.
    if (art != sourceArt_ || artBytes != sourceArtBytes_ ||
        world.runStage().area != area_ || world.runStage().level != level_) return false;
    const auto& room = world.levelClear();
    const unsigned count = world.levelClearFrame().choices ? room.selection.lastIndex + 1 : 0;
    if (!assets.changeOffers(storage, art, artBytes, room.offers.ids, count,
        world.runStage().area, world.runStage().level, world.upgrades().levels)) return false;
    offerCount_ = count;
    for (unsigned i = 0; i < count; ++i) offers_[i] = room.offers.ids[i];
    for (unsigned i = 0; i < 23; ++i) owned_[i] = world.upgrades().levels[i];
    const UBYTE* bank = storage + assets.layout.resultsOffset;
    for (unsigned i = 0; i < 2; ++i) {
        // AmigaClearBank always copies the same four border resources for
        // this area, independent of offers and inventory. Keep their pixels.
        borders_[i].rebindEquivalentArtwork(bank, bank + assets.bankBytes);
        layers_[i].invalidate();
    }
    return true;
}

bool AmigaUpgradeScene::render(unsigned back, UBYTE* border, UBYTE* layer,
    const UBYTE* pack, const UBYTE* end, const GameplayWorld& world, Frame& frame) {
    if (back >= 2) return false;
    const auto& room=world.levelClear(); const auto& snapshot=world.levelClearFrame();
    const int top=room.motion.viewY(), borderTop=top-snapshot.tileY;
    if (top<0 || borderTop<0 || top>324 || borderTop>324) return false;
    { DW_PROFILE_SCOPE(profileClearDrawList);
      if (!drawList_.build(room,snapshot,world.upgrades(),true,true)) return false;
      for(unsigned i=0;i<drawList_.count;++i) {
          drawList_.commands[i].x-=110;
          drawList_.commands[i].y+=top;
      }
    }
    unsigned pixels=0;
    { DW_PROFILE_SCOPE(profileClearBorder);
      if (!borders_[back].render(border,pack,end,snapshot,pixels)) return false; }
    { DW_PROFILE_SCOPE(profileClearOverlay);
      if (!layers_[back].render(layer,pack,end,drawList_.commands,drawList_.count,pixels)) return false; }
    // Sprite API uses the landscape logical origin, while this scene uses
    // the same centered narrow window as garden/style selection.
    constexpr int spriteOffset = amigaMenuDisplayHStart - downwellDisplayHStart - 110;
    frame={borders_[back].view(unsigned(borderTop)),layers_[back].view(unsigned(top)),
           drawList_.playerX+spriteOffset,drawList_.playerY};
    return frame.border && frame.layer;
}

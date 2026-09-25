#ifndef DOWNWELL_AMIGA_UPGRADE_SCENE_H
#define DOWNWELL_AMIGA_UPGRADE_SCENE_H
#include "AmigaClearPresenter.h"
#include "AmigaClearSceneAssets.h"
#include "core/Input.h"
class GameplayWorld;

// Upgrade-scene lifetime and retained artwork. The application must retire
// DMA before enter(), and drain outstanding blits before refreshArtwork().
// Gameplay rules remain in the shared world/controller.
class AmigaUpgradeScene {
    unsigned offerCount_ = 0, offers_[4] = {};
    unsigned char owned_[23] = {};
    bool left_ = false, right_ = false, heartGoal_ = false;
public:
    AmigaClearPresenter presenter;
    AmigaClearSceneAssets assets;
    bool presented = false;
    bool enter(GameplayWorld& world, unsigned style, const downwell::Input& input,
        UBYTE* storage, unsigned storageBytes, unsigned hudBytes,
        const UBYTE* art, unsigned artBytes, const UBYTE* mask, unsigned maskBytes);
    bool step(GameplayWorld& world, const downwell::Input& input);
    bool needsArtwork(const GameplayWorld& world) const;
    bool refreshArtwork(const GameplayWorld& world, UBYTE* storage,
        const UBYTE* art, unsigned artBytes);
};
#endif

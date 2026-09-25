#ifndef DOWNWELL_AMIGA_UPGRADE_SCENE_H
#define DOWNWELL_AMIGA_UPGRADE_SCENE_H
#include "AmigaClearDrawList.h"
#include "AmigaClearSceneAssets.h"
#include "core/Input.h"
#include "AmigaSelectionLayer.h"
class GameplayWorld;

// Upgrade-scene lifetime and retained artwork. The application must retire
// DMA before enter(), and drain outstanding blits before refreshArtwork().
// Gameplay rules remain in the shared world/controller.
class AmigaUpgradeScene {
    unsigned offerCount_ = 0, offers_[4] = {};
    unsigned char owned_[23] = {};
    bool left_ = false, right_ = false, heartGoal_ = false;
    AmigaSelectionLayer borders_[2], layers_[2];
    AmigaSelectionLayer::Command borderCommands_[AmigaSelectionLayer::capacity];
    AmigaClearDrawList drawList_;
public:
    AmigaClearSceneAssets assets;
    bool presented = false;
    bool enter(GameplayWorld& world, unsigned style, const downwell::Input& input,
        UBYTE* storage, unsigned storageBytes, unsigned hudBytes,
        const UBYTE* art, unsigned artBytes, const UBYTE* mask, unsigned maskBytes);
    bool step(GameplayWorld& world, const downwell::Input& input);
    bool needsArtwork(const GameplayWorld& world) const;
    bool refreshArtwork(const GameplayWorld& world, UBYTE* storage,
        const UBYTE* art, unsigned artBytes);
    struct Frame { const UBYTE* border = nullptr; const UBYTE* layer = nullptr; int playerX = 0, playerY = 0; };
    bool render(unsigned back, UBYTE* border, UBYTE* layer, const UBYTE* pack,
        const UBYTE* end, const GameplayWorld& world, Frame& frame);
};
#endif

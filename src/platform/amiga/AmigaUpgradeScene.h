#ifndef DOWNWELL_AMIGA_UPGRADE_SCENE_H
#define DOWNWELL_AMIGA_UPGRADE_SCENE_H
#include "AmigaClearDrawList.h"
#include "AmigaClearSceneAssets.h"
#include "core/Input.h"
#include "AmigaSelectionLayer.h"
#include "AmigaSelectionBorder.h"
#include "DownwellCopperLists.h"
class GameplayWorld;

// Upgrade-scene lifetime and retained artwork. The application must retire
// DMA before enter(), and drain outstanding blits before refreshArtwork().
// Gameplay rules remain in the shared world/controller.
class AmigaUpgradeScene {
    unsigned offerCount_ = 0, offers_[4] = {};
    unsigned char owned_[23] = {};
    bool left_ = false, right_ = false, heartGoal_ = false;
    const UBYTE* sourceArt_ = nullptr;
    unsigned sourceArtBytes_ = 0, area_ = 0, level_ = 0;
    AmigaSelectionBorder borders_[2];
    AmigaSelectionLayer layers_[2];
    AmigaClearDrawList drawList_;
    DownwellSelectionCopperList lists_[2] = {{nullptr, 0}, {nullptr, 0}};
    bool displayReady_[2] = {};
public:
    AmigaUpgradeScene() = default;
    AmigaUpgradeScene(const AmigaUpgradeScene&) = delete;
    AmigaUpgradeScene& operator=(const AmigaUpgradeScene&) = delete;
    // Borrow the alternating Chip programs only after previous DMA users
    // have retired. Binding does not write their contents or publish a list.
    void bindDisplay(UWORD* first, UWORD* second, unsigned capacity) {
        lists_[0] = DownwellSelectionCopperList(first, capacity);
        lists_[1] = DownwellSelectionCopperList(second, capacity);
        invalidateDisplay();
    }
    void invalidateDisplay() { displayReady_[0] = displayReady_[1] = false; }
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
    bool updateDisplay(unsigned back, const Frame& frame, const UBYTE* mask,
        const Sprite& player, const Sprite* actors, unsigned count,
        const Sprite& overflow, const Palette& palette) {
        if (back >= 2 || !lists_[back].data()) return false;
        displayReady_[back] = displayReady_[back]
            ? lists_[back].update(frame.border, frame.layer, mask)
            : lists_[back].build(frame.border, frame.layer, mask,
                player, actors, count, overflow, palette);
        return displayReady_[back];
    }
    const DownwellSelectionCopperList& copper(unsigned back) const { return lists_[back]; }
    bool render(unsigned back, UBYTE* border, UBYTE* layer, const UBYTE* pack,
        const UBYTE* end, const GameplayWorld& world, Frame& frame);
};
#endif

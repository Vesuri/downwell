#ifndef DOWNWELL_AMIGA_CLEAR_PRESENTER_H
#define DOWNWELL_AMIGA_CLEAR_PRESENTER_H
#include "AmigaClearBorderRenderer.h"
#include "AmigaClearDrawList.h"
#include "core/Profile.h"

// Persistent Fast-RAM scene state. Caller supplies one RETIRED border/overlay
// pair, drains the blitter after success, patches its copper list with origin
// and player position, then publishes at the platform boundary. Never advances
// game clocks, allocates Chip RAM, or rebuilds a copper list or sprite asset.
struct AmigaClearPresenter {
    struct Frame {
        unsigned origin = 0, borderRows = 0, overlayPixels = 0;
        int playerX = 0, playerY = 0;
        bool ready = false;
    };
    AmigaClearBorderCache borders[2];
    AmigaClearOverlay overlays[2];
    AmigaClearDrawList drawList;
    const UBYTE* borderMemory[2] = {};
    const UBYTE* overlayMemory[2] = {};
    bool initialized[2] = {};

    void reset() {
        for (unsigned i = 0; i < 2; ++i) initialized[i] = false;
    }
    void artworkChanged() {
        // The immutable bank was replaced after draining blits. Draw commands
        // may retain their IDs but refer to new directory positions/payloads.
        for (auto& overlay : overlays) overlay.invalidate();
    }
    bool render(unsigned back, AmigaRenderer renderer, const UBYTE* pack, const UBYTE* end,
                const downwell::LevelClearController& room,
                const downwell::LevelClearPresentation::Frame& snapshot,
                const downwell::UpgradeInventory& inventory, Frame& result) {
        result = {};
        DW_PROFILE_SCOPE(profileClearDrawList);
        if (back >= 2 || !renderer.game || !renderer.hud || !pack || !end ||
            !drawList.build(room, snapshot, inventory)) return false;
        auto& border = borders[back];
        auto& overlay = overlays[back];
        if (!initialized[back] || borderMemory[back] != renderer.game || overlayMemory[back] != renderer.hud) {
            // Scene entry/pointer rebinding retires old terrain, BOB and HUD
            // pixels, including the landscape margins outside the shaft.
            initialized[back] = false;
            border.initialized = false; border.origin = 0;
            for (auto& row : border.rows) row = 0;
            overlay.invalidate();
            amigaRendererClear(&renderer, 0);
        }
        unsigned rows = 0, pixels = 0;
        bool success;
        { DW_PROFILE_SCOPE(profileClearBorder);
          success = amigaRenderClearBorder(renderer, border, pack, end, snapshot, room.motion.viewY(), rows); }
        if (success) { DW_PROFILE_SCOPE(profileClearOverlay);
            success = overlay.render(renderer, pack, end, drawList.commands, drawList.count, pixels); }
        if (!success) {
            // A failed back frame must never be published; retry starts clean.
            initialized[back] = false;
            return false;
        }
        initialized[back] = true;
        borderMemory[back] = renderer.game; overlayMemory[back] = renderer.hud;
        result = {border.origin, rows, pixels, drawList.playerX, drawList.playerY, true};
        return true;
    }
};
#endif

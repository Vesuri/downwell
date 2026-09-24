#ifndef DOWNWELL_AMIGA_CLEAR_DRAW_LIST_H
#define DOWNWELL_AMIGA_CLEAR_DRAW_LIST_H
#include "AmigaClearOverlay.h"
#include "core/LevelClearLayout.h"
#include "core/LevelClearTrig.h"

// Convert the shared source-order layout to offline planar draw commands.
// Border DMA and the player hardware sprite are separate presenter outputs.
// Rasterization and trig sampling remain explicit inputs, not alternate game
// logic hidden in this backend. No allocation or glyph composition occurs.
struct AmigaClearDrawList {
    AmigaClearOverlay::Command commands[AmigaClearOverlay::capacity];
    unsigned count = 0;
    int playerX = 0, playerY = 0;
    bool valid = false;
    template<class Rasterize>
    bool build(const downwell::LevelClearController& room,
               const downwell::LevelClearPresentation::Frame& frame,
               const downwell::UpgradeInventory& inventory, Rasterize pixel) {
        return build(room, frame, inventory,
            downwell::LevelClearTrig::absoluteCosine(frame.circle),
            downwell::LevelClearTrig::absoluteSine(frame.time), pixel);
    }
    template<class Rasterize>
    bool build(const downwell::LevelClearController& room,
               const downwell::LevelClearPresentation::Frame& frame,
               const downwell::UpgradeInventory& inventory,
               downwell::HudReal absoluteCosine, downwell::HudReal absoluteSine,
               Rasterize pixel) {
        count = 0; valid = false;
        if (!downwell::levelClearStageLabel(frame.area, frame.level) ||
            room.selection.lastIndex > 3 || room.selection.cursor > room.selection.lastIndex) return false;
        valid = true;
        using C = downwell::LevelClearCoordinate;
        struct Sink {
            AmigaClearDrawList& list;
            Rasterize& pixel;
            int viewY;
            void add(unsigned id, unsigned frame, int x, int y, int pen) {
                if (!id || list.count == AmigaClearOverlay::capacity) { list.valid = false; return; }
                list.commands[list.count++] = {id, frame, x, y, pen};
            }
            void sprite(unsigned id, unsigned frame, C x, C y) {
                if (frame == 7 || frame == 13) return; // shared border commands
                add(id, frame, 110 + pixel(x), pixel(y) - viewY, -1);
            }
            void player(C x, C y) {
                list.playerX = 110 + pixel(x); list.playerY = pixel(y) - viewY;
            }
            void label(unsigned id, C x, C y, unsigned pen) {
                add(id, 0, 110 + pixel(x), pixel(y) - viewY, pen);
            }
            void description(unsigned id, C x, C y, unsigned suffix) {
                if (suffix) {
                    if (id < 64100 || id > 64122 || suffix != 2) { list.valid = false; return; }
                    id = downwell::levelUpgradeNameLabel(id - 64100, suffix - 1);
                }
                const int px = 110 + pixel(x), py = pixel(y) - viewY;
                // Pen 3 is opaque black in PF2, not transparent pen 0.
                add(id, 0, px + 1, py, 3); add(id, 0, px - 1, py, 3);
                add(id, 0, px, py + 1, 3); add(id, 0, px, py - 1, 3);
                add(id, 0, px, py, 1);
            }
        } sink{*this, pixel, room.motion.viewY()};
        downwell::drawLevelClear(sink, room, frame, inventory, absoluteCosine, absoluteSine);
        return valid;
    }
};
#endif

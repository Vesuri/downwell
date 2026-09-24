#ifndef DOWNWELL_AMIGA_SHOP_MENU_H
#define DOWNWELL_AMIGA_SHOP_MENU_H
#include "AmigaRenderer.h"
#include "core/ShopMenuText.h"

// Retained overlay commands: redraw only the union of changed asset bounds.
// The pristine world/ring is never touched. This object belongs in static/Fast
// storage rather than the small Amiga task stack.
struct AmigaShopMenu {
    struct Command {
        int id = 0, frame = 0, x = 0, y = 0, pen = 0;
        bool operator==(const Command& b) const {
            return id == b.id && frame == b.frame && x == b.x && y == b.y && pen == b.pen;
        }
    };
    enum { capacity = 128 };
    Command previous[capacity], current[capacity];
    unsigned previousCount = 0, count = 0;
    bool valid = true;
    void reset() { previousCount = count = 0; valid = true; }
    void append(Command command) {
        if (count == capacity) { valid = false; return; }
        current[count++] = command;
    }
    void asset(int id, unsigned frame, int x, int y) { append({id,int(frame),x,y,0}); }
    void mask(int id, int x, int y, unsigned pen) { append({id,0,x,y,int(pen)}); }
    void arrow(int phase, int x, int y, int angle) {
        // Negative draw_sprite subimages use the counter's image_index (zero).
        asset(484, (phase < 0 ? 0 : phase / 4) + (angle == 270 ? 768 : 0), x, y);
    }
    void text(downwell::ShopMenuText kind, unsigned value, int x, int y, bool red) {
        downwell::drawShopMenuText(*this, kind, value, x, y, red);
    }
    bool draw(AmigaRenderer* renderer, const UBYTE* pack, const UBYTE* end,
              const downwell::ShopWorld::Instance* shop, int viewX, int viewY,
              unsigned currency) {
        count = 0; valid = true;
        if (shop && !downwell::drawShopMenu(*shop, viewX, viewY, 284, currency, *this)) return false;
        if (!valid) return false;
        int left = 160, top = 284, right = 0, bottom = 0;
        auto include = [&](const Command& command) {
            int x, y, w, h;
            if (!amigaRendererAssetBounds(pack, end, command.id, command.frame,
                    command.x, command.y, 0, &x, &y, &w, &h)) { valid = false; return; }
            if (x < left) left = x;
            if (y < top) top = y;
            if (x + w > right) right = x + w;
            if (y + h > bottom) bottom = y + h;
        };
        const unsigned maximum = count > previousCount ? count : previousCount;
        for (unsigned i = 0; i < maximum; ++i) {
            if (i < count && i < previousCount && current[i] == previous[i]) continue;
            if (i < count) include(current[i]);
            if (i < previousCount) include(previous[i]);
        }
        if (!valid) return false;
        if (left < 0) left = 0;
        if (top < 0) top = 0;
        if (right > 160) right = 160;
        if (bottom > 284) bottom = 284;
        if (left < right && top < bottom) {
            UBYTE* saved = renderer->target;
            amigaRendererSetClipRect(renderer, downwellPlayfieldX + left, top, right-left, bottom-top);
            renderer->target = renderer->hud;
            amigaRendererFillRect(renderer, downwellPlayfieldX + left, top, right-left, bottom-top, 0);
            for (unsigned i = 0; i < count; ++i) {
                const auto& c = current[i];
                if (c.pen) amigaRendererDrawAssetPen(renderer, pack, end, c.id, c.frame,
                    downwellPlayfieldX+c.x, c.y, 0, c.pen);
                else amigaRendererDrawAsset(renderer, pack, end, c.id, c.frame,
                    downwellPlayfieldX+c.x, c.y, 0);
            }
            amigaRendererClearClipRect(renderer);
            renderer->target = saved;
        }
        previousCount = count;
        for (unsigned i = 0; i < count; ++i) previous[i] = current[i];
        return true;
    }
};
#endif

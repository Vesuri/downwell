#ifndef DOWNWELL_CORE_SHOP_MENU_LAYOUT_H
#define DOWNWELL_CORE_SHOP_MENU_LAYOUT_H
#include "ShopFixtures.h"
namespace downwell {
// Semantic text IDs keep localization/font rasterization in the backends.
// Text positions are center/middle anchors, not glyph bitmap top-lefts.
enum class ShopMenuText { exit, buy, itemName, itemDescription, price, purchased, back };

// Sink: asset(id,frame,x,y), arrow(phaseQuarters,x,y,rotation),
// text(kind,value,x,y,red). Coordinates are viewport-relative. Keep signed
// cursor phase intact until the backend resolves GameMaker subimage semantics.
// Returns false for the not-yet-ported selling branch rather than inventing UI.
template<class Sink>
bool drawShopMenu(const ShopWorld::Instance& shop, int viewX, int viewY,
                  int cameraHeight, unsigned currency, Sink& sink) {
    const auto& menu = shop.menu;
    if (!shop.present || !menu.active) return true;
    if (menu.menuState > 1) return false;
    const int boxY = cameraHeight / 2 - 85;
    const int wobble = menu.cursorWobbleX;
    sink.asset(489, 0, 0, boxY);
    if (menu.menuState == 0) {
        sink.text(ShopMenuText::exit, 0, 32, boxY + 48, false);
        sink.text(ShopMenuText::buy, 0, 80, boxY + 48, false);
        sink.arrow(menu.cursorPhaseQuarters, 32 + 48 * int(menu.menuSelect) - wobble,
                   boxY + 30, 0);
        return true;
    }
    const unsigned cursor = menu.cursor;
    const int arrowX = shop.x + 26 + 19 * int(cursor) - wobble - viewX;
    const int arrowY = shop.y - 28 - viewY;
    sink.arrow(menu.cursorPhaseQuarters, arrowX, arrowY, cursor ? 0 : 270);
    if (cursor) sink.asset(498, cursor, arrowX, arrowY);
    const int textX = 80 + wobble;
    if (!cursor) {
        sink.text(ShopMenuText::back, 0, textX, boxY + 36, false);
        return true;
    }
    const auto& item = shop.stock.items[cursor - 1];
    if (!item.remaining) {
        sink.text(ShopMenuText::purchased, 0, textX, boxY + 36, false);
        return true;
    }
    sink.asset(shopItemSprite(item.upgrade), 0, textX, boxY + 36);
    sink.text(ShopMenuText::itemName, item.upgrade, textX, boxY + 12, false);
    sink.text(ShopMenuText::price, item.price, textX, boxY + 54, currency < item.price);
    sink.text(ShopMenuText::itemDescription, item.upgrade, textX, boxY + 80, false);
    return true;
}
}
#endif

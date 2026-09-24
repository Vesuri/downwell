#ifndef DOWNWELL_CORE_SHOP_MENU_TEXT_H
#define DOWNWELL_CORE_SHOP_MENU_TEXT_H
#include "ShopMenuLayout.h"
namespace downwell {
// Sink.mask(id,x,y,pen) draws a pre-rasterized center/middle-anchored mask.
// Pens follow the native white/red/opaque-black convention (1/2/3).
// Four whole black text draws precede the foreground, as in scrDrawBorderText.
template<class Sink>
void drawShopMenuText(Sink& sink, ShopMenuText kind, unsigned value,
                      int x, int y, bool red) {
    int id = 0;
    switch (kind) {
    case ShopMenuText::exit: id = 65410; break;
    case ShopMenuText::buy: id = 65411; break;
    case ShopMenuText::purchased: id = 65412; break;
    case ShopMenuText::back: id = 65413; break;
    case ShopMenuText::itemName: id = 65420 + int(value) - 100; break;
    case ShopMenuText::itemDescription: id = 65430 + int(value) - 100; break;
    case ShopMenuText::price: break;
    }
    unsigned char digits[10]; // full 32-bit unsigned domain, not just current shop prices
    unsigned count = 0;
    if (kind == ShopMenuText::price) {
        do { digits[count++] = value % 10; value /= 10; } while (value);
    }
    const int dx[] = {1, -1, 0, 0, 0}, dy[] = {0, 0, 1, -1, 0};
    for (unsigned pass = 0; pass < 5; ++pass) {
        const unsigned pen = pass == 4 ? (red ? 2 : 1) : 3;
        if (kind != ShopMenuText::price) {
            sink.mask(id, x + dx[pass], y + dy[pass], pen);
            continue;
        }
        // Source string(value)+" G": digits, one eight-pixel space and G.
        // Packed single glyph masks have originX=4, so emit glyph centers.
        const int first = x - int(count + 2) * 4 + 4;
        for (unsigned i = 0; i < count; ++i)
            sink.mask(65440 + digits[count - 1 - i], first + int(i) * 8 + dx[pass],
                      y + dy[pass], pen);
        sink.mask(65450, first + int(count + 1) * 8 + dx[pass], y + dy[pass], pen);
    }
}
}
#endif

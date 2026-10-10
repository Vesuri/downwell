#ifndef DOWNWELL_CORE_SHOP_DRAW_STATE_H
#define DOWNWELL_CORE_SHOP_DRAW_STATE_H
#include "MotionBinary64.h"
namespace downwell {
// Self variables assigned by object 277's Draw, not backend layout caches.
struct ShopDrawState {
    MotionBinary64 boxX{},boxY{},textX{},nameY{},menuX[3]{},menuY{},arrowX{};
    MotionBinary64 sellX[2]{},iconX[3]{},sellY{},iconY{},wobbleY{};
    unsigned amount=1,sellPrice=100;
};
}
#endif

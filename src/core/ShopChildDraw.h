#ifndef DOWNWELL_CORE_SHOP_CHILD_DRAW_H
#define DOWNWELL_CORE_SHOP_CHILD_DRAW_H
#include "MotionBinary64.h"
namespace downwell {
struct ShopChildDraw {
    // Table, case and item inherit ordinary sprite drawing. Keeper Draw adds
    // the area-three hat after draw_self, without inheriting its transform.
    template<class Ops> static bool draw(int object,Ops& ops){
        if(object!=278&&object!=279&&object!=280&&object!=283)return false;
        if(!ops.self())return false;
        if(object!=280)return true;
        unsigned area;if(!ops.area(area))return false;
        if(area!=3)return true;
        auto y=ops.y();if(!y.add(MotionBinary64::fromInteger(-6)))return false;
        return ops.sprite(725,0,ops.x(),y);
    }
};
}
#endif

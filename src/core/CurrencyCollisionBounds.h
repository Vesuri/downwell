#ifndef DOWNWELL_CORE_CURRENCY_COLLISION_BOUNDS_H
#define DOWNWELL_CORE_CURRENCY_COLLISION_BOUNDS_H
#include "EnemyContact.h"
#include "RunnerSpriteBounds.h"
namespace downwell {
// Unit, unrotated currency masks. Narrow position to binary32 before edge
// arithmetic; rounding the centre first is not the bounds algorithm.
struct CurrencyCollisionBounds {
    static bool calculate(int sprite,MotionBinary64 x,MotionBinary64 y,ContactBounds& out){
        if(sprite!=133&&sprite!=134)return false;
        RunnerImageValue px,py;RunnerSpriteBounds::Box margins,edges;
        const auto local=contactSpriteBounds(sprite);
        if(!px.assign(x)||!py.assign(y)||!margins.left.assignInteger(local.left)||
            !margins.right.assignInteger(local.right-1)||!margins.top.assignInteger(local.top)||
            !margins.bottom.assignInteger(local.bottom-1))return false;
        const RunnerImageValue unit{0x3f800000u};
        auto round=[](RunnerImageValue value,std::int32_t& n){return value.roundPixel(n);};
        if(!RunnerSpriteBounds::unrotated(margins,0,0,px,py,unit,unit,false,round,edges))return false;
        ContactBounds next;
        auto pixel=[](RunnerImageValue value,int& out){
            std::int32_t n;if(!value.roundPixel(n))return false;out=int(n);return true;
        };
        if(!pixel(edges.left,next.left)||!pixel(edges.right,next.right)||
            !pixel(edges.top,next.top)||!pixel(edges.bottom,next.bottom))return false;
        out=next;return true;
    }
};
}
#endif

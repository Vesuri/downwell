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
        return calculateMask(sprite,x,y,out);
    }
    // Any verified unrotated, unit-scale rectangle mask (spawner and
    // steering-family place_meeting queries use 153, 174 and 702).
    static bool calculateMask(int sprite,MotionBinary64 x,MotionBinary64 y,ContactBounds& out){
        RunnerImageValue px,py;RunnerSpriteBounds::Box margins,edges;
        const auto local=contactSpriteBounds(sprite);
        if(local.left>=local.right||local.top>=local.bottom)return false;
        if(!px.assign(x)||!py.assign(y))return false;
        {   // Integer margins at unit scale: the same edges without the
            // binary32 margin conversions, where sumInteger admits them.
            RunnerSpriteBounds::Box fast;ContactBounds next;
            if(RunnerSpriteBounds::unitIntegerAxis(local.left,local.right-1,px,fast.left,fast.right)&&
               RunnerSpriteBounds::unitIntegerAxis(local.top,local.bottom-1,py,fast.top,fast.bottom)){
                std::int32_t l,r,t,b;
                if(!fast.left.roundPixel(l)||!fast.right.roundPixel(r)||
                   !fast.top.roundPixel(t)||!fast.bottom.roundPixel(b))return false;
                next.left=int(l);next.right=int(r);next.top=int(t);next.bottom=int(b);
                out=next;return true;
            }
        }
        if(!margins.left.assignInteger(local.left)||
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

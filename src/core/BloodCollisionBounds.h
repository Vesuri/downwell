#ifndef DOWNWELL_CORE_BLOOD_COLLISION_BOUNDS_H
#define DOWNWELL_CORE_BLOOD_COLLISION_BOUNDS_H
#include "BloodEffect.h"
#include "EnemyContact.h"
#include "RunnerSpriteBounds.h"
namespace downwell {
// Object 220 / sprite 84, unit scales, compatibility collision mode. Half-open
// result converts the runner's rounded inclusive right/bottom back to edges.
// The selected integer conversion assumes the runner's nearest-even FP mode.
struct BloodCollisionBounds {
    static bool calculate(const BloodEffect& effect,ContactBounds& result){
        RunnerImageValue radians,c,s;
        MotionBinary64 cosine,sine;
        if(!SmokeDirection::radians(effect.imageAngle,radians) ||
           !smokeTrig(radians.promoted(),cosine,sine) || !c.assign(cosine) || !s.assign(sine))return false;
        // Origin (12,12), inclusive margins (10,11)..(13,13).
        const RunnerSpriteBounds::Box margins{{0x41200000},{0x41300000},{0x41500000},{0x41500000}};
        const RunnerImageValue unit{0x3f800000};
        RunnerSpriteBounds::Box edges;
        auto round=[](RunnerImageValue value,std::int32_t& pixel){return value.roundPixel(pixel);};
        // Keep half-open integer output: rounding the fractional far edge
        // directly avoids an inclusive decrement followed by increment.
        if(!(effect.imageAngle.bits&0x7fffffffu)){
            if(!RunnerSpriteBounds::unrotated(margins,12,12,effect.x,effect.y,unit,unit,false,round,edges))return false;
        }else if(!RunnerSpriteBounds::rotated(margins,12,12,effect.x,effect.y,unit,unit,c,s,false,round,edges))return false;
        auto pixel=[](RunnerImageValue value,int& out){
            std::int32_t rounded;
            if(!value.roundPixel(rounded))return false;
            out=rounded;return true;
        };
        ContactBounds next;
        if(!pixel(edges.left,next.left)||!pixel(edges.right,next.right)||
           !pixel(edges.top,next.top)||!pixel(edges.bottom,next.bottom))return false;
        result=next;return true;
    }
};
}
#endif

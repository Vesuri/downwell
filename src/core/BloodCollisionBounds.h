#ifndef DOWNWELL_CORE_BLOOD_COLLISION_BOUNDS_H
#define DOWNWELL_CORE_BLOOD_COLLISION_BOUNDS_H
#include "BloodEffect.h"
#include "EnemyContact.h"
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
        RunnerImageValue cl,cr,st,sb,ct,cb,sl,sr;
        // Origin (12,12), inclusive margins (10,11)..(13,13).
        if(!product(c,-2,cl) || !product(c,2,cr) || !product(s,-1,st) || !product(s,2,sb) ||
           !product(c,-1,ct) || !product(c,2,cb) || !product(s,-2,sl) || !product(s,2,sr))return false;
        sort(cl,cr);sort(st,sb);sort(ct,cb);sort(sl,sr);
        sr.bits^=0x80000000u;sl.bits^=0x80000000u;
        ContactBounds next;
        if(!edge(effect.x,cl,st,next.left) || !edge(effect.x,cr,sb,next.right) ||
           !edge(effect.y,ct,sr,next.top) || !edge(effect.y,cb,sl,next.bottom))return false;
        result=next;return true;
    }
private:
    static bool product(RunnerImageValue a,int b,RunnerImageValue& out){
        MotionBinary64 n;
        return multiplyBinary64(a.promoted(),MotionBinary64::fromInteger(b),n) && out.assign(n);
    }
    static unsigned key(RunnerImageValue a){return a.bits&0x80000000u?~a.bits:a.bits^0x80000000u;}
    static void sort(RunnerImageValue& a,RunnerImageValue& b){if(key(a)>key(b)){const auto t=a;a=b;b=t;}}
    static bool edge(RunnerImageValue origin,RunnerImageValue a,RunnerImageValue b,int& out){
        auto sum=origin.promoted();RunnerImageValue narrowed;
        if(!sum.add(a.promoted()) || !narrowed.assign(sum))return false;
        sum=narrowed.promoted();
        if(!sum.add(b.promoted()) || !narrowed.assign(sum))return false;
        std::int32_t pixel;
        if(!narrowed.promoted().roundPixel(pixel))return false;
        out=pixel;return true;
    }
};
}
#endif

#ifndef DOWNWELL_CORE_FROG_MOTION_H
#define DOWNWELL_CORE_FROG_MOTION_H
#include "FrogHorizontalMotion.h"
#include "FrogVerticalMotion.h"
#include "FrogHitState.h"
namespace downwell {
// enmFrog Step block 37 calls round(xx), round(yy) directly. Do not round
// a Q8/binary32 presentation mirror first: it can manufacture an exact half
// and change the published pixel. This retains the full shared fixed state;
// it does not make the existing Q16 rebound approximation binary64-exact.
inline long frogPublishedPixel(long position, long unit) {
    const long whole=position/unit, fraction=position%unit, half=unit/2;
    if(fraction>half||(fraction==half&&whole%2))return whole+1;
    if(fraction<-half||(fraction==-half&&whole%2))return whole-1;
    return whole;
}
struct NoFrogHit {bool operator()()const{return true;}};
struct NoFrogScale {bool operator()(int)const{return true;}};
struct NoFrogLanding {void operator()(bool)const{}};
struct FrogStepEffects {bool hit;bool succeeded;};
// One shared ordering for both adapters. The terrain callback translates
// coordinates only; it must not implement an alternative movement policy.
// Collision probes remain provisional (not the complete source mask solver).
template<class Solid,class Hit=NoFrogHit,class Scale=NoFrogScale,class Landing=NoFrogLanding>
FrogStepEffects stepFrogMotion(FrogHorizontalMotion& horizontal,
                    FrogVerticalMotion& vertical, FrogHitState& hitState,
                    bool integrate, Solid solid,Hit onHit={},Scale scale={},Landing landing={}) {
    const bool hit = hitState.consume();
    if (hit) horizontal.velocity = FrogHitState::dampHorizontal(horizontal.velocity);
    vertical.applyGravity(hit);
    if(hit&&!onHit())return {true,false};
    vertical.resolve([&](int y) { return solid(horizontal.pixel(), y); });
    horizontal.resolve([&](int x) { return solid(x, vertical.pixel()); });
    if(horizontal.velocity!=0&&!scale(horizontal.velocity<0?-1:1))return {hit,false};
    landing(vertical.grounded);
    if(integrate){
        if(!vertical.grounded)horizontal.position+=horizontal.velocity;
        vertical.position+=vertical.velocity;
    }
    return {hit,true};
}
} // namespace downwell
#endif

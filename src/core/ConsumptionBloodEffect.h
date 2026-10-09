#ifndef DOWNWELL_CORE_CONSUMPTION_BLOOD_EFFECT_H
#define DOWNWELL_CORE_CONSUMPTION_BLOOD_EFFECT_H
#include "RandomBinary64.h"
#include "DivideBinary64.h"
#include "RunnerImageValue.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// Object 223 (fxBlood), not object 220. Default Draw uses sprite 92.
// Global writes/reads and builtin setters remain observable to the owner.
struct ConsumptionBloodEffect {
    RunnerImageValue x{},y{},imageSpeed{},imageIndex{},timeline{};
    MotionBinary64 xsp{},ysp{};
    int alarm0=-1;
    bool valid=true,destroyRequested=false;
    bool drawFrame(unsigned& frame)const{
        const unsigned magnitude=imageIndex.bits&0x7fffffffu;
        if(!valid||magnitude>=0x41000000u||((imageIndex.bits>>31)&&magnitude))return false;
        const unsigned exponent=(imageIndex.bits>>23)&255u;
        frame=exponent<127?0:((imageIndex.bits&0x7fffffu)|0x800000u)>>(150u-exponent);return true;
    }
    template<class Random,class Publish> bool create(MotionBinary64 px,MotionBinary64 py,Random& random,Publish& publish){
        *this={};
        if(!x.assign(px)||!y.assign(py)||
           !randomBinary64(random.nextU32(),MotionBinary64::fromInteger(3),ysp)||
           !ysp.add(MotionBinary64::fromInteger(-2))||
           !randomBinary64(random.nextU32(),MotionBinary64::fromInteger(1),xsp))return valid=false;
        if(random.chooseIndexSmall(2))xsp.high^=0x80000000u;
        alarm0=120;
        if(!publish.gravity(MotionBinary64{0x3fc99999u,0x9999999au}))return valid=false;
        imageSpeed={0x3e99999au};
        if(!publish.imageSpeed(imageSpeed))return valid=false;
        if(!imageIndex.assignInteger(int(random.irandomSmall(7)))||!publish.imageIndex(imageIndex))return valid=false;
        return true;
    }
    template<class Publish> bool advanceImage(Publish& publish){
        if(!valid)return false;
        if(destroyRequested)return true;
        RunnerImageValue evaluated;bool reconcile=false,crossed=false;
        if(!UnitSequenceFrames::evaluate(timeline,8,evaluated)||
           !UnitSequenceFrames::needsReconcile(imageIndex,evaluated,reconcile))return valid=false;
        auto next=reconcile?imageIndex:timeline;
        if(!UnitSequenceFrames::advance(next,imageSpeed,8,evaluated,crossed))return valid=false;
        timeline=next;imageIndex=evaluated;
        return publish.imageIndex(imageIndex)||(valid=false); // No Animation End handler.
    }
    template<class Environment> bool step(Environment& env){
        if(!valid)return false;
        if(destroyRequested)return true;
        auto px=x.promoted();
        if(!px.add(xsp)||!x.assign(px)||!env.position(x,y))return valid=false;
        auto py=y.promoted();
        if(!py.add(ysp)||!y.assign(py)||!env.position(x,y))return valid=false;
        if(!divideBinary64(xsp,{0x3ff028f5u,0xc28f5c29u},xsp)||!ysp.add(env.gravity()))return valid=false;
        auto fall=env.fallSpeed();fall.high^=0x80000000u;py=y.promoted();
        if(!py.add(fall)||!y.assign(py)||!env.position(x,y))return valid=false;
        return true;
    }
    template<class Publish> bool kill(Publish& publish){
        if(!valid)return false;
        if(destroyRequested)return true;
        destroyRequested=true;return publish.destroy()||(valid=false);
    }
};
}
#endif

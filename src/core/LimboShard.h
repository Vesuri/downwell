#ifndef DOWNWELL_CORE_LIMBO_SHARD_H
#define DOWNWELL_CORE_LIMBO_SHARD_H
#include "RandomBinary64.h"
#include "MultiplyBinary64.h"
#include "RunnerImageValue.h"
namespace downwell {
// Object 379, codes 2769--2772. Not an fxTimeShard subtype: it has no
// parent or alarm handlers. Owners retain numeric IDs, scheduler membership,
// animation and alarm countdowns. Callbacks publish setters synchronously;
// failed events retain the accepted prefix and must not be retried.
struct LimboShard {
    RunnerImageValue x{},y{},imageSpeed{},imageIndex{};
    MotionBinary64 xx{},yy{},xsp{},ysp{},ascendSpeed{},savedSpeed{};
    MotionBinary64 gravity{0x3fb47ae1u,0x47ae147bu},hardGravity{0x3fb99999u,0x9999999au};
    int shardLimit=16,dflash=-1,alarm2=-1;
    bool disappearing=false,noTouch=false,destroyRequested=false;
    struct Player {bool fired=false;MotionBinary64 x{},y{};};

    template<class Random,class Ops> bool create(MotionBinary64 px,MotionBinary64 py,bool lowSpec,Random& random,Ops& ops){
        if(!x.assign(px)||!y.assign(py))return false;
        xsp=MotionBinary64::fromInteger(int(random.irandomSmall(4))-2);
        static constexpr int vertical[]={0,-1,-1,-1,-2,-2,-3};
        ysp=MotionBinary64::fromInteger(vertical[random.chooseIndexSmall(7)]);
        if(!range(random,{0x3fb99999u,0x9999999au},{0x3fe33333u,0x33333333u},ascendSpeed))return false;
        shardLimit=lowSpec?5:16;xx=x.promoted();yy=y.promoted();
        gravity={0x3fb47ae1u,0x47ae147bu};hardGravity={0x3fb99999u,0x9999999au};
        // The VM evaluates the magnitude choose before the sign choose.
        static constexpr MotionBinary64 rates[]={{},{0x3f847ae1u,0x47ae147bu},{0x3fa99999u,0x9999999au}};
        savedSpeed=rates[random.chooseIndexSmall(3)];
        const auto sign=MotionBinary64::fromInteger(random.chooseIndexSmall(2)?1:-1);
        if(!scale(savedSpeed,sign)||!speed(savedSpeed,ops))return false;
        MotionBinary64 index;RunnerImageValue published;
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(7),index)||
            !published.assign(index)||!ops.image(published))return false;
        imageIndex=published;disappearing=false;dflash=-1;noTouch=false;return true;
    }

    // allowed must come from the real TimeStopBound query at this event's
    // entry. A blocked event skips ascent and wall queries, but NOT inertia.
    template<class Random,class Ops> bool step(bool allowed,const Player& player,Random& random,Ops& ops){
        if(destroyRequested)return true;
        if(allowed){
            if(player.fired){
                auto bottom=player.y,left=player.x,right=player.x;
                if(!bottom.add(MotionBinary64::fromInteger(160))||
                    !left.add(MotionBinary64::fromInteger(-16))||!right.add(MotionBinary64::fromInteger(16)))return false;
                if(less(player.y,y.promoted())&&less(y.promoted(),bottom)&&
                    less(left,x.promoted())&&less(x.promoted(),right)&&!noTouch){
                    auto delta=x.promoted();auto neg=player.x;neg.high^=0x80000000u;
                    if(!delta.add(neg)||!kick(delta,random,xsp)||
                        !range(random,MotionBinary64::fromInteger(2),MotionBinary64::fromInteger(5),ysp))return false;
                    noTouch=true;
                    if(!ops.alarm(2,5))return false;
                    alarm2=5;
                }
            }
            if(!speed(savedSpeed,ops))return false;
            auto probe=xx;bool hit=false;
            if(!probe.add(xsp)||!ops.meeting(probe,yy,56,hit))return false;
            if(hit){xsp.high^=0x80000000u;if(!scale(xsp,{0x3fe66666u,0x66666666u}))return false;}
            auto ascent=ascendSpeed;ascent.high^=0x80000000u;
            if(!yy.add(ascent))return false;
            dflash=disappearing?-dflash:-1;
        }else if(!speed({},ops))return false;
        if(small(xsp,{0x3f999999u,0x9999999au})){
            // sign(0) * .025 is zero, not the positive minimum.
            if(!xsp.zero())xsp={0x3f999999u|(xsp.high&0x80000000u),0x9999999au};
            else xsp={};
        }
        if(!xx.add(xsp)||!yy.add(ysp)||!scale(xsp,{0x3fef0a3du,0x70a3d70au})||
            !scale(ysp,{0x3fecccccu,0xcccccccdu}))return false;
        std::int32_t px,py;RunnerImageValue published;
        if(!xx.roundPixel(px)||!published.assignInteger(px)||!ops.positionX(published))return false;
        x=published;
        if(!yy.roundPixel(py)||!published.assignInteger(py)||!ops.positionY(published))return false;
        y=published;
        if(small(xsp,{0x3fa99999u,0x9999999au}))xsp={};
        if(small(ysp,{0x3fa99999u,0x9999999au}))ysp={};
        unsigned population=0;if(!ops.population(population))return false;
        if(population>unsigned(shardLimit)){
            bool outside=false;
            if(!ops.outside(x,y,outside))return false;
            if(outside&&!destroy(ops))return false;
        }
        // The second destroy call is not short-circuited by the first mark.
        MotionBinary64 top;if(!ops.viewTop(top))return false;
        return !less(y.promoted(),top)||destroy(ops);
    }
    template<class Random,class Ops> bool bulletContact(MotionBinary64 vx,MotionBinary64 vy,Random& random,Ops& ops){
        if(noTouch)return true;
        if(!kick(vx,random,xsp))return false;
        ysp=vy;noTouch=true;
        if(!ops.alarm(2,5))return false;
        alarm2=5;return true;
    }
    // VM numeric truth is positive, not C++ nonzero. In the authored -1/+1
    // domain, !dflash draws the negative phase (and also accepts zero).
    bool drawsSelf()const{return dflash<=0;}
private:
    template<class Ops> bool speed(MotionBinary64 value,Ops& ops){
        RunnerImageValue published;
        if(!published.assign(value)||!ops.speed(published))return false;
        imageSpeed=published;return true;
    }
    template<class Ops> bool destroy(Ops& ops){
        if(!ops.destroy())return false;
        destroyRequested=true;return true;
    }
    template<class Random> static bool range(Random& random,MotionBinary64 a,MotionBinary64 b,MotionBinary64& out){
        auto negative=a;negative.high^=0x80000000u;
        if(!b.add(negative)||!randomBinary64(random.nextU32(),b,out))return false;
        return out.add(a);
    }
    template<class Random> static bool kick(MotionBinary64 direction,Random& random,MotionBinary64& out){
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(2),out))return false;
        if(direction.zero())out={};else out.high|=direction.high&0x80000000u;
        return true;
    }
    static bool less(MotionBinary64 a,MotionBinary64 b){
        if(a.zero()&&b.zero())return false;
        const bool an=a.high>>31,bn=b.high>>31;
        if(an!=bn)return an;
        const bool magnitude=(a.high&0x7fffffffu)<(b.high&0x7fffffffu)||
            ((a.high&0x7fffffffu)==(b.high&0x7fffffffu)&&a.low<b.low);
        return an?(!magnitude&&(a.high!=b.high||a.low!=b.low)):magnitude;
    }
    static bool small(MotionBinary64 value,MotionBinary64 limit){value.high&=0x7fffffffu;return less(value,limit);}
    static bool scale(MotionBinary64& value,MotionBinary64 factor){
        MotionBinary64 out;if(!multiplyBinary64(value,factor,out))return false;value=out;return true;
    }
};
}
#endif

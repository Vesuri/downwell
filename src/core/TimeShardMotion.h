#ifndef DOWNWELL_CORE_TIME_SHARD_MOTION_H
#define DOWNWELL_CORE_TIME_SHARD_MOTION_H
#include "TimeShardLifecycle.h"
#include "MultiplyBinary64.h"
namespace downwell {
struct NoTimeShardDestroy {bool operator()()const{return true;}};
struct TimeShardMotion : TimeShardLifecycle {
    // meeting is the source sprite-mask query against sParentSolid (56).
    // count is queried after embedded retirement. outside implements outOfView
    // using the OLD published y, including its rmMain-only destruction rule.
    // A destroy request does not abort the remainder of this source event.
    template<class Random,class Meeting,class Count,class Outside,class Destroy=NoTimeShardDestroy>
    bool step(bool stopped,Random& random,Meeting meeting,Count count,Outside outside,Destroy destroy={}){
        if(destroyRequested)return true;
        auto requestDestroy=[&](){
            if(destroyRequested)return true;
            destroyRequested=true;return destroy();
        };
        if(!xsp.valid() || !ysp.valid())return false;
        if(!stopped){
            if(!ysp.add(less(ysp,MotionBinary64::fromInteger(-1))?hardGravity:gravity) ||
               !imageSpeed.assign(savedSpeed))return false;
        }else{
            imageSpeed={};
            if(alarm0>0)++alarm0;
            if(alarm1>0)++alarm1;
        }
        auto probe=xx;bool hit=false;
        if(!probe.add(xsp) || !meeting(probe,yy,hit))return false;
        // The final query below repeats this one when the shard ends where
        // the probe was (a resting shard); nothing in between moves solids.
        const MotionBinary64 firstX=probe,firstY=yy;const bool firstHit=hit;
        if(hit){xsp.high^=0x80000000u;if(!scale(xsp,{0x3fe66666u,0x66666666u}))return false;}
        probe=yy;
        if(!probe.add(ysp) || !meeting(xx,probe,hit))return false;
        if(hit){
            if(ysp.zero() || (ysp.high&0x80000000u)){
                if(!scale(ysp,{}))return false;
            }else{
                // Runner truncates BOTH fractional bounds to int64 zero.
                // irandom_range(0,0) still consumes two RNG words.
                ysp=MotionBinary64::fromInteger(int(random.irandomSmall(0)));
            }
            if(!scale(xsp,{0x3fe66666u,0x66666666u}))return false;
            static constexpr MotionBinary64 rates[]={
                {0xbfd33333u,0x33333333u},{0xbfb99999u,0x9999999au},{},
                {0x3fb99999u,0x9999999au},{0x3fd33333u,0x33333333u}};
            if(!imageSpeed.assign(rates[random.chooseIndexSmall(5)]))return false;
        }
        if(small(xsp))xsp=xsp.zero()?MotionBinary64{}:
            MotionBinary64{0x3fa99999u|(xsp.high&0x80000000u),0x9999999au};
        if(!xx.add(xsp) || !yy.add(ysp) || !scale(xsp,{0x3fecccccu,0xcccccccdu}) ||
           !scale(ysp,{0x3fecccccu,0xcccccccdu}))return false;
        if(small(xsp))xsp={};
        if(small(ysp))ysp={};
        if(same(xx,firstX)&&same(yy,firstY))hit=firstHit;
        else if(!meeting(xx,yy,hit))return false;
        if(hit){
            probe=xx;
            if(!probe.add(MotionBinary64::fromInteger(8)) || !meeting(probe,yy,hit))return false;
            if(!hit)xx=probe;
            else{
                probe=xx;
                if(!probe.add(MotionBinary64::fromInteger(-8)) || !meeting(probe,yy,hit))return false;
                if(!hit)xx=probe;
            }
            probe=yy;
            if(!probe.add(MotionBinary64::fromInteger(-8)) || !meeting(xx,probe,hit))return false;
            if(!hit)yy=probe;
            else{
                probe=yy;
                if(!probe.add(MotionBinary64::fromInteger(8)) || !meeting(xx,probe,hit))return false;
                if(!hit)yy=probe;
            }
            if(!meeting(xx,yy,hit))return false;
            if(hit&&!requestDestroy())return false;
        }
        dflash=disappearing?-dflash:-1;
        // Negative count is an adapter failure, never a VM population value.
        const int population=count();
        if(population<0)return false;
        if(population>shardLimit&&!requestDestroy())return false;
        bool retire=false;
        if(!outside(*this,retire))return false;
        if(retire&&!requestDestroy())return false;
        std::int32_t px,py;
        return xx.roundPixel(px) && yy.roundPixel(py) &&
            x.assign(MotionBinary64::fromInteger(px)) && y.assign(MotionBinary64::fromInteger(py));
    }
private:
    static bool less(MotionBinary64 a,MotionBinary64 b){
        if(a.zero() && b.zero())return false;
        const bool an=a.high>>31,bn=b.high>>31;
        if(an!=bn)return an;
        const bool magnitude=(a.high&0x7fffffffu)<(b.high&0x7fffffffu) ||
            ((a.high&0x7fffffffu)==(b.high&0x7fffffffu) && a.low<b.low);
        return an?(!magnitude && (a.high!=b.high || a.low!=b.low)):magnitude;
    }
    static bool same(MotionBinary64 a,MotionBinary64 b){return a.high==b.high&&a.low==b.low;}
    static bool small(MotionBinary64 v){v.high&=0x7fffffffu;return less(v,{0x3fa99999u,0x9999999au});}
    static bool scale(MotionBinary64& v,MotionBinary64 factor){MotionBinary64 out;if(!multiplyBinary64(v,factor,out))return false;v=out;return true;}
};
}
#endif

#ifndef DOWNWELL_CORE_CURRENCY_CREATE_VERIFICATION_H
#define DOWNWELL_CORE_CURRENCY_CREATE_VERIFICATION_H
#include "CurrencyCreate.h"
#include "CurrencyStepEnvelope.h"
namespace downwell {
// Controlled maximum-range tape for the 68000. Real RNG state/order and the
// scalar floating-point oracle are covered separately by CurrencyCreateTest.
struct CurrencyCreateVerification {
    struct Random {
        unsigned words=0,choices=0,integers=0;
        std::uint32_t nextU32(){++words;return 0;}
        unsigned chooseIndexSmall(unsigned){++choices;return 0;}
        unsigned irandomSmall(unsigned maximum){++integers;return maximum;}
    };
    struct Queries {
        unsigned walls=0,tiles=0,writes=0;
        bool wall(MotionBinary64,MotionBinary64,bool& hit){++walls;hit=true;return true;}
        bool tile(MotionBinary64,MotionBinary64,bool& hit){++tiles;hit=true;return true;}
        bool publish(RunnerImageValue,RunnerImageValue){++writes;return true;}
    };
    static bool run(){
        for(unsigned failure=0;failure<6;++failure){
            CurrencyCreate state;Random random;bool good=true;
            state.stuckKamo=2;state.disappearing=true;state.xx={0x40290000u,0};state.yy={0xc00c0000u,0}; // 12.5, -3.5
            state.x.assign(state.xx);state.y.assign(state.yy);state.imageSpeed.bits=0x3f000000u;
            state.alarms.set(0,1);state.alarms.set(1,0);state.alarms.set(2,-1);state.alarms.set(3,9);state.alarms.set(4,5);
            struct Ops {
                unsigned failure,calls=0,writes=0;
                bool next(){return ++calls!=failure;}
                bool wall(MotionBinary64,MotionBinary64,bool& hit){hit=true;return next();}
                bool destroy(){return next();}
                bool timeAllowed(bool& allowed){allowed=false;return next();}
                bool freeMotion(CurrencyCreate&){return false;}
                bool attractedMotion(CurrencyCreate&){return false;}
                bool publish(RunnerImageValue x,RunnerImageValue y){
                    if(x.bits!=0x41400000u||y.bits!=(writes?0xc0800000u:0xc0600000u))return false;
                    ++writes;return next();
                }
            } ops{failure};
            if(CurrencyStepEnvelope::run(state,random,ops,good)!=(failure==0)||good!=(failure==0)||
                random.choices!=1||ops.calls!=(failure?failure:5))return false;
            if(!failure&&(state.stuckKamo!=3||!state.obtainable||state.scriptIndex!=4||state.imageSpeed.bits||state.flash!=1||
                state.alarms.get(0)!=2||state.alarms.get(1)!=0||state.alarms.get(2)!=-1||state.alarms.get(3)!=10||state.alarms.get(4)!=5))return false;
            if(failure&&(CurrencyStepEnvelope::run(state,random,ops,good)||ops.calls!=failure||random.choices!=1))return false;
        }
        for(int area=1;area<=5;++area){
            Random random;Queries queries;CurrencyCreate birth;
            if(!birth.create({0x40288000u,0},{0xc00c0000u,0},area,true,random,queries)||
                birth.x.bits!=0x42a80000u||birth.y.bits!=0|| // 16+68, 0
                birth.xsp.high!=0xc0000000u||birth.xsp.low||
                birth.ysp.high!=(area==5?0x80000000u:0xc0000000u)||birth.ysp.low||
                birth.direction!=10||birth.imageSpeed.bits!=0xbf000000u||birth.attracted||
                birth.attractionRadius!=48||birth.alarms.get(3)!=30||
                birth.radius!=68||birth.maxRadius!=32||birth.attempts!=33||
                queries.walls!=34||queries.tiles||queries.writes!=4||
                random.words!=unsigned(area==5?3:2)||random.choices!=2||random.integers!=68)return false;
        }
        return true;
    }
};
}
#endif

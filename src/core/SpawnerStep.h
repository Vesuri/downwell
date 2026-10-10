#ifndef DOWNWELL_CORE_SPAWNER_STEP_H
#define DOWNWELL_CORE_SPAWNER_STEP_H
#include "RunnerRealComparison.h"
#include "MultiplyBinary64.h"
namespace downwell {
// Caller-owned state for spawnerStep. Script active is deliberately separate
// from runner activation. Channel settings are authored integral values;
// countdown and fall depth retain source binary64 arithmetic.
struct SpawnerStep {
    using Real=MotionBinary64;
    struct Channel {
        Real countdown{};
        int interval=0,amplitude=0,minimum=0,maximum=0;
        bool above=false;
        unsigned enemyCount=0;
        int enemies[16]{};
    };
    Channel channels[8]{};
    unsigned channelCount=0;
    Real depthMeasure{},fallDepthMemory{},randxGrid{},randyGrid{},snappyViewh{},snappyViewy{};
    int enemyMaxCount=10,totalDepthCount=500,fromAbove=0;
    int chan=0,i=0,spawnAmt=0,spawningEnemy=0,maskIndex=-1,wl=0;
    bool scriptActive=false;

    // Synchronous initialization of a NEW area-one spawner. Other areas use
    // different scripts and must not silently receive this configuration.
    bool createCaverns(int level,bool hard){
        if(level<1||level>3)return false;
        *this={};enemyMaxCount=20;
        auto channel=[&](int start,int interval,int amplitude,int minimum,int maximum,bool above,
            unsigned count,int a,int b=0,int c=0,int d=0,int e=0,int f=0){
            chan=int(channelCount++);auto& out=channels[chan];
            out.countdown=Real::fromInteger(start);out.interval=interval;out.amplitude=amplitude;
            out.minimum=minimum;out.maximum=maximum;out.above=above;out.enemyCount=count;
            const int enemies[]={a,b,c,d,e,f};
            for(unsigned j=0;j<count;++j)out.enemies[j]=enemies[j];
        };
        if(!hard){
            if(level==1)channel(160,160,32,1,1,false,1,115);
            if(level==2){
                channel(160,240,120,1,1,false,1,115);
                channel(2000,360,32,1,1,false,6,118,116,116,116,116,116);
            }
            if(level==3){
                channel(160,320,64,1,1,false,2,115,115);
                channel(800,560,100,1,1,false,2,116,116);
                channel(1300,800,100,1,1,false,3,118,118,118);
            }
        }else{
            if(level==1){
                channel(160,240,64,1,2,false,4,115,116,114,118);
                channel(160,360,32,1,1,true,1,173);
            }
            if(level==2){
                channel(160,300,100,1,1,true,1,173);
                channel(160,220,32,1,2,false,4,118,116,114,115);
            }
            if(level==3){
                channel(160,240,64,1,1,true,2,173,173);
                channel(800,200,32,1,1,false,3,118,116,114);
            }
        }
        return true;
    }

    // Exact signed ceil/round of the small world-coordinate grid. Keep the
    // view addition before division. Comparisons for ceil are NOT VM's epsilon
    // comparisons. Values outside representable grid storage fail explicitly.
    static bool viewGrid(Real y,Real height,bool above,Real& output){
        if(!above&&!y.add(height))return false;
        Real scaled;
        if(!multiplyBinary64(y,{0x3fb00000u,0},scaled))return false;
        std::int32_t n;
        if(!scaled.roundPixel(n))return false;
        if(!above){
            auto delta=Real::fromInteger(n);delta.high^=0x80000000u;
            if(!delta.add(scaled))return false;
            if(!delta.zero()&&!(delta.high&0x80000000u))++n;
            ++n;
        }
        if(n < -0x07ffffff || n > 0x07ffffff)return false;
        output=Real::fromInteger(n*16);return true;
    }

    template<class Random,class Ops> bool step(Real y,Random& random,Ops& ops){
        Real value;int order;
        if(!scriptActive){
            if(!ops.playerY(value)||!compareRunnerReal(value,y,order))return false;
            if(order>0){int area;if(!ops.area(area))return false;if(area!=5)scriptActive=true;}
        }
        if(!scriptActive)return true;
        if(!ops.fallDepth(value))return false;
        depthMeasure=fallDepthMemory;depthMeasure.high^=0x80000000u;
        if(!depthMeasure.add(value))return false;
        // Source reads global.falldepth again for this assignment.
        if(!ops.fallDepth(fallDepthMemory))return false;
        bool guard;
        if(!ops.death(guard))return false;
        if(guard)return true;
        if(!ops.endReached(guard))return false;
        if(guard)return true;
        int count;if(!ops.enemyCount(count))return false;
        if(count>=enemyMaxCount)return true;
        if(!channelCount||channelCount>8)return false;
        for(i=0;i<int(channelCount);++i){
            auto& c=channels[i];
            auto delta=depthMeasure;delta.high^=0x80000000u;
            if(!c.countdown.add(delta)||!compareRunnerReal(c.countdown,{},order))return false;
            if(order>0)continue;
            if(c.amplitude<0||c.amplitude>65534||c.minimum<0||c.maximum<c.minimum||
               c.maximum>65534||!c.enemyCount||c.enemyCount>16)return false;
            const int jitter=int(random.irandomSmall(unsigned(c.amplitude)));
            const int sign=random.chooseIndexSmall(2)?1:-1;
            c.countdown=Real::fromInteger(c.interval);
            if(!c.countdown.add(Real::fromInteger(jitter*sign)))return false;
            spawnAmt=c.minimum+int(random.irandomSmall(unsigned(c.maximum-c.minimum)));
            // repeat captures its counter, even though spawnAmt is caller state.
            for(int remaining=spawnAmt;remaining>0;--remaining){
                spawningEnemy=c.enemies[random.irandomSmall(c.enemyCount-1)];
                int mask;
                if(!ops.objectMask(spawningEnemy,mask)||!ops.mask(mask))return false;
                maskIndex=mask;
                randxGrid=Real::fromInteger(160+16*(2+int(random.irandomSmall(6))));
                Real viewY,height;
                if(!ops.viewY(viewY)||(!c.above&&!ops.viewHeight(height)))return false;
                auto& grid=c.above?snappyViewy:snappyViewh;
                if(!viewGrid(viewY,height,c.above,grid))return false;
                auto chooseY=[&](){
                    randyGrid=grid;
                    const int offset=16*(1+int(random.irandomSmall(4)));
                    return randyGrid.add(Real::fromInteger(c.above?-offset:offset));
                };
                if(!chooseY())return false;
                wl=0;
                for(;;){
                    bool hit;
                    if(!ops.meeting(randxGrid,randyGrid,57,hit))return false;
                    if(!hit&&!ops.meeting(randxGrid,randyGrid,83,hit))return false;
                    if(!hit&&!c.above&&!ops.tile(100,randxGrid,randyGrid,hit))return false;
                    if(!hit)break;
                    randxGrid=Real::fromInteger(160+16*(c.above?
                        2+int(random.irandomSmall(6)):1+int(random.irandomSmall(8))));
                    if(!chooseY())return false;
                    if(++wl>32)break;
                }
                bool wall;
                if(!ops.meeting(randxGrid,randyGrid,57,wall))return false;
                if(!wall&&wl<32){
                    unsigned child;
                    if(!ops.create(spawningEnemy,randxGrid,randyGrid,child)||!ops.deactivate(child))return false;
                }
            }
        }
        return true;
    }
};
}
#endif

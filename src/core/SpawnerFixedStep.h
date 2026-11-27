#ifndef DOWNWELL_CORE_SPAWNER_FIXED_STEP_H
#define DOWNWELL_CORE_SPAWNER_FIXED_STEP_H
#include <cstdint>
namespace downwell {
// spawnerStep on whole pixels and Q8 depths for the compact world (W3.20).
// The source's countdown and fall depth are sums of Q8 player positions,
// which binary64 holds exactly, and the VM's 1e-5 comparison band is finer
// than a Q8 step, so every decision equals SpawnerStep's, its VM form and
// oracle. Ops: playerYQ8(value), area(value), fallDepthQ8(value),
// death(value), endReached(value), enemyCount(value), objectMask(object,
// mask), mask(value), viewYQ8(value), viewHeight(pixels), meeting(x, y,
// object, hit), tile(depth, x, y, hit), create(object, x, y, child) and
// deactivate(child), positions in whole room pixels.
struct SpawnerFixedStep {
    struct Channel {
        std::int32_t countdownQ8=0;
        int interval=0,amplitude=0,minimum=0,maximum=0;
        bool above=false;
        unsigned enemyCount=0;
        int enemies[16]{};
    };
    Channel channels[8]{};
    unsigned channelCount=0;
    std::int32_t depthMeasureQ8=0,fallDepthMemoryQ8=0;
    int randxGrid=0,randyGrid=0,snappyViewh=0,snappyViewy=0;
    int enemyMaxCount=10,totalDepthCount=500,fromAbove=0;
    int chan=0,i=0,spawnAmt=0,spawningEnemy=0,maskIndex=-1,wl=0;
    bool scriptActive=false;

    // Synchronous initialization of a NEW area-one spawner (SpawnerStep's).
    bool createCaverns(int level,bool hard){
        if(level<1||level>3)return false;
        *this={};enemyMaxCount=20;
        auto channel=[&](int start,int interval,int amplitude,int minimum,int maximum,bool above,
            unsigned count,int a,int b=0,int c=0,int d=0,int e=0,int f=0){
            chan=int(channelCount++);auto& out=channels[chan];
            out.countdownQ8=start*256;out.interval=interval;out.amplitude=amplitude;
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

    // SpawnerStep::viewGrid on Q8: round(viewy / 16) * 16 above (halves to
    // even), (ceil((viewy + viewh) / 16) + 1) * 16 below.
    static bool viewGrid(std::int32_t yQ8,int height,bool above,int& output){
        if(yQ8< -0x10000000||yQ8>0x10000000||height<0||height>0x10000)return false;
        if(!above)yQ8+=height*256;
        std::int32_t n=yQ8>>12;const std::int32_t rest=yQ8&4095;
        if(above){if(rest>2048||(rest==2048&&(n&1)))++n;}
        else n+=(rest?1:0)+1;
        output=n*16;return true;
    }

    template<class Random,class Ops> bool step(int y,Random& random,Ops& ops){
        std::int32_t value;
        if(!scriptActive){
            if(!ops.playerYQ8(value))return false;
            if(value>y*256){int area;if(!ops.area(area))return false;if(area!=5)scriptActive=true;}
        }
        if(!scriptActive)return true;
        if(!ops.fallDepthQ8(value))return false;
        depthMeasureQ8=value-fallDepthMemoryQ8;
        // Source reads global.falldepth again for this assignment.
        if(!ops.fallDepthQ8(fallDepthMemoryQ8))return false;
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
            if(depthMeasureQ8< -0x20000000||depthMeasureQ8>0x20000000||
               c.countdownQ8< -0x20000000||c.countdownQ8>0x20000000)return false;
            c.countdownQ8-=depthMeasureQ8;
            if(c.countdownQ8>0)continue;
            if(c.amplitude<0||c.amplitude>65534||c.minimum<0||c.maximum<c.minimum||
               c.maximum>65534||!c.enemyCount||c.enemyCount>16||c.interval<0||c.interval>65534)return false;
            const int jitter=int(random.irandomSmall(unsigned(c.amplitude)));
            const int sign=random.chooseIndexSmall(2)?1:-1;
            c.countdownQ8=(c.interval+jitter*sign)*256;
            spawnAmt=c.minimum+int(random.irandomSmall(unsigned(c.maximum-c.minimum)));
            // repeat captures its counter, even though spawnAmt is caller state.
            for(int remaining=spawnAmt;remaining>0;--remaining){
                spawningEnemy=c.enemies[random.irandomSmall(c.enemyCount-1)];
                int mask;
                if(!ops.objectMask(spawningEnemy,mask)||!ops.mask(mask))return false;
                maskIndex=mask;
                randxGrid=160+16*(2+int(random.irandomSmall(6)));
                std::int32_t viewY;int height=0;
                if(!ops.viewYQ8(viewY)||(!c.above&&!ops.viewHeight(height)))return false;
                int& grid=c.above?snappyViewy:snappyViewh;
                if(!viewGrid(viewY,height,c.above,grid))return false;
                auto chooseY=[&](){
                    const int offset=16*(1+int(random.irandomSmall(4)));
                    randyGrid=grid+(c.above?-offset:offset);
                };
                chooseY();
                wl=0;
                for(;;){
                    bool hit;
                    if(!ops.meeting(randxGrid,randyGrid,57,hit))return false;
                    if(!hit&&!ops.meeting(randxGrid,randyGrid,83,hit))return false;
                    if(!hit&&!c.above&&!ops.tile(100,randxGrid,randyGrid,hit))return false;
                    if(!hit)break;
                    randxGrid=160+16*(c.above?2+int(random.irandomSmall(6)):1+int(random.irandomSmall(8)));
                    chooseY();
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

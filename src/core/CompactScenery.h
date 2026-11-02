#ifndef DOWNWELL_CORE_COMPACT_SCENERY_H
#define DOWNWELL_CORE_COMPACT_SCENERY_H
#include "DivideBinary64.h"
#include "MultiplyBinary64.h"
#include "RandomBinary64.h"
#include "RunnerRealComparison.h"
#include "ScatteredEvents.h"
#include <cstdint>
namespace downwell {
// Compact native records for world one's builder scenery. The builder has
// already consumed every Create RNG draw (Scattered16 scale/frame, gem chunk
// alarms); adoption repeats none of them. Events are the VM bodies; the world
// supplies queries, child births and publication through Ops.
struct CompactScatteredRecord:ScatteredScriptState {
    int x=0,y=0,scaleX=1;
    int myGem=-1; // compact gem slot of the latest scrCurrencySpawn birth
    std::uint32_t birth=0; // world tick of Create: Step starts on the next
    bool live=false;
};

// gemChunk (293): subparentShootableWall with wallHp 30. Bullets lower
// wallHp; every three points release one gem and flash, and the last opens it.
struct CompactGemChunkRecord {
    int x=0,y=0;
    int wallHp=30,prvWallHp=30,hit=0;
    int alarm[3]={-1,-1,-1};
    int sprite=569,myGem=-1;
    // image_index/image_speed in 1/65536 frames: presentation only, no RNG.
    std::int32_t imageIndex=0,imageSpeed=0;
    std::uint32_t birth=0;
    bool destroyed=false,flashing=false,opened=false,live=false;
    unsigned frame()const{return unsigned(imageIndex>>16);}
    unsigned frames()const{return sprite==569?4u:2u;}
    // Runner animation precedes Begin Step; Other 7 stops the shine.
    void advanceImage(){
        if(!imageSpeed)return;
        imageIndex+=imageSpeed;
        const std::int32_t end=std::int32_t(frames())<<16;
        if(imageIndex>=end){imageIndex-=end;imageSpeed=0;}
    }
    void setImage(unsigned index){imageIndex=std::int32_t(index)<<16;}
    template<class Random,class Ops> bool alarm0(Random&,Ops& ops){
        if(opened)return true;
        sprite=569;setImage(0);flashing=false;
        return ops.appearanceChanged();
    }
    // emitMovingFx's arguments are pushed last-first: rate, sprite, y, x.
    template<class Random,class Ops> bool alarm1(Random& random,Ops& ops){
        MotionBinary64 rate,px,py;
        if(!randomBinary64(random.nextU32(),{0x3fb99999u,0x9999999au},rate)||
           !rate.add({0x3fb99999u,0x9999999au}))return false;
        const int sprite=random.chooseIndexSmall(2)?572:571;
        const int dy=-16+int(random.irandomSmall(16));
        const int dx=14+int(random.irandomSmall(2));
        const int sign=random.chooseIndexSmall(2)?1:-1;
        px=MotionBinary64::fromInteger(x+dx*sign);py=MotionBinary64::fromInteger(y+dy);
        if(!ops.movingFx(px,py,sprite,rate,MotionBinary64{},MotionBinary64{}))return false;
        if(!destroyed)alarm[1]=30+int(random.irandomSmall(30));
        return true;
    }
    template<class Random,class Ops> bool alarm2(Random& random,Ops&){
        if(!destroyed){
            imageSpeed=19661; // 0.3
            alarm[2]=180+int(random.irandomSmall(180));
        }
        return true;
    }
    // Step draws no RNG itself: currency Creates use the world stream.
    template<class Random,class Ops> bool step(Random&,Ops& ops){
        if(destroyed){
            if(frame()!=1){setImage(1);if(!ops.appearanceChanged())return false;}
            if(opened)return true;
            opened=true;
            if(!ops.sound(162,85,0,1))return false;
            sprite=570;setImage(1);
            if(!ops.openMask()||!ops.appearanceChanged())return false;
            const auto px=MotionBinary64::fromInteger(x),py=MotionBinary64::fromInteger(y);
            if(!ops.movingFx(px,py,118,{0x3fe66666u,0x66666666u},MotionBinary64{},MotionBinary64{})||
               !ops.flash(px,py))return false;
            // global.lowSpec is not admitted: ten large gems worth 10.
            // Velocity ranges are in the compact gem's 1/12800 pixel units:
            // random_range(-2.5,2.5) and random_range(-2,-1).
            for(unsigned i=0;i<10;++i)
                if(!ops.burstGem(64000,-32000,12800,-25600,10,true))return false;
            return true;
        }
        if(prvWallHp>=wallHp+3&&!flashing){
            sprite=570;setImage(0);alarm[0]=4;flashing=true;
            if(!ops.appearanceChanged())return false;
            // random_range(-2,2) and random_range(-2.5,-1); Create's amount 2.
            if(!ops.burstGem(51200,-25600,19200,-32000,2,false)||!ops.sound(161,50,0,1))return false;
            prvWallHp-=3;
            if(prvWallHp<=0)destroyed=true;
        }
        return true;
    }
};

// Torch (296): its first Step chooses the wall it hangs from and creates
// its TorchFire, or destroys itself. Afterwards it has no behaviour.
struct CompactTorchRecord {
    int x=0,y=0,placement=-1;
    std::uint32_t birth=0;
    bool allSet=false,live=false;
    template<class Ops> bool step(Ops& ops){
        if(allSet)return true;
        bool hit;
        if(!ops.solid(x,y,hit))return false;
        bool destroy=hit;
        if(!destroy){
            if(!ops.solid(x,y+8,hit))return false;
            if(hit)placement=0;
            else{
                if(!ops.wall(x+8,y,hit))return false;
                if(hit)placement=1;
                else{
                    if(!ops.wall(x-8,y,hit))return false;
                    if(hit)placement=2;else destroy=true;
                }
            }
        }
        if(destroy&&!ops.destroy())return false;
        if(placement!=-1&&!ops.createFire(x,y-8))return false;
        allSet=true;
        return ops.appearanceChanged();
    }
};

// TorchFire (295): nine fire particles while in view, stompable/shootable.
struct CompactTorchFireRecord {
    using Real=MotionBinary64;
    static constexpr unsigned parts=9; // fPartAmt 8, inclusive loops
    struct Part {Real x,y,life,vx,vy,rate;};
    Part part[parts];
    int x=0,y=0,objHp=1,hit=0,stomped=0,myGem=-1;
    std::uint32_t birth=0;
    bool active=true,masked=true,live=false;
    void create(int px,int py){
        x=px;y=py;objHp=1;hit=0;stomped=0;active=true;masked=true;myGem=-1;
        for(unsigned i=0;i<parts;++i){
            auto& p=part[i];
            p.x=Real::fromInteger(x);p.y=Real::fromInteger(y);
            p.life=Real::fromInteger(int(i/2));
            if(i&1u)p.life.add({0x3fe00000u,0});
            p.vx={0x3fd33333u,0x33333333u};p.vy={0xbfc99999u,0x9999999au};p.rate={0x3fc99999u,0x9999999au};
        }
    }
    // spRand/spRandx/spRandy/posRand and the new rate, in source order.
    template<class Random> bool respawn(Part& p,Random& random,Real spread,Real low,Real high){
        Real speed,divisor,quotient,horizontal,vertical,position,rate;
        if(!randomBinary64(random.nextU32(),{0x3fd33333u,0x33333334u},speed)||
           !speed.add({0x3fc99999u,0x9999999au}))return false;
        if(!randomBinary64(random.nextU32(),{0x40040000u,0},divisor)||
           !divisor.add({0x40040000u,0})||!divideBinary64(speed,divisor,quotient))return false;
        horizontal=quotient;
        if(!random.chooseIndexSmall(2))horizontal.high^=0x80000000u;
        auto magnitude=horizontal;magnitude.high&=0x7fffffffu;magnitude.high^=0x80000000u;
        vertical=speed;if(!vertical.add(magnitude))return false;
        auto doubled=spread;if(!doubled.add(spread))return false;
        if(!randomBinary64(random.nextU32(),doubled,position))return false;
        auto negative=spread;negative.high^=0x80000000u;if(!position.add(negative))return false;
        auto range=high;auto lowNegative=low;lowNegative.high^=0x80000000u;if(!range.add(lowNegative))return false;
        if(!randomBinary64(random.nextU32(),range,rate)||!rate.add(low))return false;
        p.life={};
        p.x=Real::fromInteger(x);if(!p.x.add(position))return false;
        p.y=Real::fromInteger(y);if(!p.y.add(position))return false;
        p.vx=horizontal;p.vy=vertical;p.vy.high^=0x80000000u;p.rate=rate;
        return true;
    }
    template<class Random,class Ops> bool step(Random& random,Ops& ops){
        bool visible;
        if(!ops.inView(x,y,visible))return false;
        if(visible)for(auto& p:part){
            int order;
            if(!compareRunnerReal(p.life,Real::fromInteger(8),order))return false;
            if(order<0){
                if(!p.life.add(p.rate)||!p.x.add(p.vx)||!p.y.add(p.vy))return false;
            }else if(active&&!respawn(p,random,{0x40000000u,0},{0x3fc99999u,0x9999999au},{0x3fd99999u,0x9999999au}))
                return false;
        }
        bool retained;
        if(!ops.outOfView(y,retained))return false;
        if(objHp>0||!active)return true;
        if(!ops.sound(359,50,0,1))return false;
        active=false;masked=false;
        const auto px=Real::fromInteger(x),py=Real::fromInteger(y);
        Real angle;
        if(!randomBinary64(random.nextU32(),Real::fromInteger(20),angle)||
           !ops.movingFx(px,py,84,{0x3fe00000u,0},angle,Real::fromInteger(1)))return false;
        if(!randomBinary64(random.nextU32(),Real::fromInteger(20),angle))return false;
        angle.high^=0x80000000u;
        if(!angle.add(Real::fromInteger(180))||
           !ops.movingFx(px,py,84,{0x3fe00000u,0},angle,Real::fromInteger(1)))return false;
        if(!ops.currency(2))return false;
        if(stomped&&!ops.attractGem())return false;
        for(auto& p:part)
            if(!respawn(p,random,{0x40100000u,0},{0x3fc33333u,0x33333333u},{0x3fd33333u,0x33333333u}))return false;
        return true;
    }
};

// Fixed per-room storage. Capacity overflow is terminal for the world.
struct CompactScenery {
    static constexpr unsigned scatteredCapacity=64,gemChunkCapacity=16;
    static constexpr unsigned torchCapacity=32,fireCapacity=32;
    CompactScatteredRecord scattered[scatteredCapacity];
    CompactGemChunkRecord gemChunks[gemChunkCapacity];
    CompactTorchRecord torches[torchCapacity];
    CompactTorchFireRecord fires[fireCapacity];
    unsigned scatteredEnd=0,gemChunkEnd=0,torchEnd=0,fireEnd=0;
    void clear(){scatteredEnd=gemChunkEnd=torchEnd=fireEnd=0;}
    bool empty()const{return !scatteredEnd&&!gemChunkEnd&&!torchEnd&&!fireEnd;}
    template<class Record,unsigned Capacity> static Record* append(Record (&records)[Capacity],unsigned& end){
        unsigned kept=0; // Stable compaction keeps creation order.
        for(unsigned i=0;i<end;++i)if(records[i].live){if(kept!=i)records[kept]=records[i];++kept;}
        end=kept;
        if(end==Capacity)return nullptr;
        records[end]=Record{};records[end].live=true;
        return &records[end++];
    }
};
}
#endif

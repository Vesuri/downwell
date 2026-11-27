#ifndef DOWNWELL_CORE_COMPACT_SCENERY_H
#define DOWNWELL_CORE_COMPACT_SCENERY_H
#include "FixedPoint.h"
#include "RunnerImageValue.h"
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
        // random(0.1) + 0.1, narrowed once to the binary32 image_speed.
        RunnerImageValue rate;
        if(!rate.assignQ16((Fixed16::random(random.nextU32(),Fixed16::raw(6554))+Fixed16::raw(6554)).v))return false;
        const int sprite=random.chooseIndexSmall(2)?572:571;
        const int dy=-16+int(random.irandomSmall(16));
        const int dx=14+int(random.irandomSmall(2));
        const int sign=random.chooseIndexSmall(2)?1:-1;
        if(!ops.movingFx(Fixed16::fromInteger(x+dx*sign),Fixed16::fromInteger(y+dy),sprite,rate,Fixed16{},Fixed16{}))return false;
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
            const auto px=Fixed16::fromInteger(x),py=Fixed16::fromInteger(y);
            if(!ops.movingFx(px,py,118,RunnerImageValue{0x3f333333u},Fixed16{},Fixed16{})||
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
// Fixed point throughout (W3.20). Position and velocity are Q16 sums; the
// respawn draws are nearest samples in the VM's order and the speed split a
// rounded 32/32 division. Life decides each respawn, an RNG draw, so it is
// Q27 (life stays below 16): Q16 0.2 lies below 0.2 and would leave a
// particle born at 0 a tick short of 8 after 40 sums, where the VM's
// binary64 sum reaches 8 and its 1e-5 band (1342 Q27 units) is honoured.
struct CompactTorchFireRecord {
    static constexpr unsigned parts=9; // fPartAmt 8, inclusive loops
    static constexpr std::int32_t lifeOne=1<<27;
    // word / 2^32 * scale for a Q27 scale (nearest, halves up).
    static std::int32_t random27(std::uint32_t word,std::int32_t scale){
        return std::int32_t((std::uint64_t(word)*std::uint32_t(scale)+0x80000000u)>>32);
    }
    struct Part {
        std::int32_t x=0,y=0,vx=0,vy=0; // Q16
        std::int32_t life=0,rate=0; // Q27
        // compareRunnerReal(life, 8) < 0: life - 8 below -1e-5.
        bool lifeBelow8(bool& below)const{below=life<8*lifeOne-1342;return true;}
        // life += rate; x += vx; y += vy.
        bool advance(){life+=rate;x+=vx;y+=vy;return true;}
        bool pixels(std::int32_t& px,std::int32_t& py)const{
            px=Fixed16::roundQ16(x);py=Fixed16::roundQ16(y);return true;
        }
        // draw_sprite's subimage: life's integer part, 0 below one or negative.
        bool frame(std::int32_t& out)const{out=life<lifeOne?0:life>>27;return true;}
    };
    Part part[parts];
    int x=0,y=0,objHp=1,hit=0,stomped=0,myGem=-1;
    std::uint32_t birth=0;
    bool active=true,masked=true,live=false;
    void create(int px,int py){
        x=px;y=py;objHp=1;hit=0;stomped=0;active=true;masked=true;myGem=-1;
        for(unsigned i=0;i<parts;++i){
            auto& p=part[i];
            p.x=x*65536;p.y=y*65536;
            p.life=std::int32_t(i)*(lifeOne/2); // i / 2, halves for the odd parts
            p.vx=19661;p.vy=-13107;p.rate=26843546; // 0.3, -0.2, Q27 0.2
        }
    }
    // spRand/spRandx/spRandy/posRand and the new rate, in source order:
    // speed random(0.3) + 0.2, divisor random(2.5) + 2.5, the sign, the
    // position random(spread * 2) - spread and the rate random_range(low,
    // high), low and high in Q27.
    template<class Random> bool respawn(Part& p,Random& random,Fixed16 spread,std::int32_t low,std::int32_t high){
        const auto speed=Fixed16::random(random.nextU32(),Fixed16::raw(19661))+Fixed16::raw(13107);
        const auto divisor=Fixed16::random(random.nextU32(),Fixed16::raw(163840))+Fixed16::raw(163840);
        // speed < 1/2, so speed << 16 fits 32 bits; divisor >= 2.5.
        const std::uint32_t n=std::uint32_t(speed.v)<<16,d=std::uint32_t(divisor.v);
        std::int32_t horizontal=std::int32_t((n+d/2)/d);
        const std::int32_t vertical=speed.v-horizontal;
        if(!random.chooseIndexSmall(2))horizontal=-horizontal;
        const auto position=Fixed16::random(random.nextU32(),spread+spread)-spread;
        const std::int32_t rate=random27(random.nextU32(),high-low)+low;
        p.life=0;p.x=x*65536+position.v;p.y=y*65536+position.v;
        p.vx=horizontal;p.vy=-vertical;p.rate=rate;
        return true;
    }
    template<class Random,class Ops> bool step(Random& random,Ops& ops){
        bool visible;
        if(!ops.inView(x,y,visible))return false;
        if(visible)for(auto& p:part){
            bool below;
            if(!p.lifeBelow8(below))return false;
            if(below){
                if(!p.advance())return false;
            }else if(active&&!respawn(p,random,Fixed16::fromInteger(2),26843546,53687091))
                return false; // spread 2, rate 0.2..0.4
        }
        bool retained;
        if(!ops.outOfView(y,retained))return false;
        if(objHp>0||!active)return true;
        if(!ops.sound(359,50,0,1))return false;
        active=false;masked=false;
        const auto px=Fixed16::fromInteger(x),py=Fixed16::fromInteger(y);
        constexpr RunnerImageValue half{0x3f000000u};
        auto angle=Fixed16::random(random.nextU32(),Fixed16::fromInteger(20));
        if(!ops.movingFx(px,py,84,half,angle,Fixed16::fromInteger(1)))return false;
        angle=Fixed16::fromInteger(180)-Fixed16::random(random.nextU32(),Fixed16::fromInteger(20));
        if(!ops.movingFx(px,py,84,half,angle,Fixed16::fromInteger(1)))return false;
        if(!ops.currency(2))return false;
        if(stomped&&!ops.attractGem())return false;
        for(auto& p:part) // spread 4, rate 0.15..0.3
            if(!respawn(p,random,Fixed16::fromInteger(4),20132659,40265318))return false;
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

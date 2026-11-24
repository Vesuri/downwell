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
    // A particle moves by repeated binary64 sums: life += rate, x += vx and
    // y += vy, each rounded. Only three decisions read them: life < 8, the
    // pixel each coordinate rounds to and life's integer part. Each value
    // keeps its exact start and a Q32 running sum. After n sums the Q32 sum
    // is within 2n+2 units of the binary64 one: under one unit from
    // truncating the start, under one per sum from truncating the step and
    // at most 1/4 per sum from binary64 rounding, as every value stays below
    // 2^20. A decision clear of that bound is the binary64 one; otherwise the
    // n binary64 sums are replayed. Values outside the gate (and a particle
    // past maxSteps) keep the binary64 sums themselves.
    struct Part {
        Real x,y,life,vx,vy,rate; // exact: current values; else at step 0
        std::int64_t xq=0,yq=0,lifeq=0;
        std::int32_t vxq=0,vyq=0,rateq=0;
        std::uint16_t steps=0;
        bool exact=true;
        static constexpr unsigned maxSteps=1024;
        // Zero or 2^-60 <= |v| < 2^19: every sum is a normal multiple of
        // 2^-112 below 2^20, so no binary64 sum can fail.
        static bool fixed(Real v,unsigned maxExponent,std::int64_t& out){
            if(v.zero()){out=0;return true;}
            const int exponent=int((v.high>>20)&2047)-1023;
            if(exponent<-60||exponent>int(maxExponent))return false;
            // Q32 = significand * 2^(exponent-20), truncated; exponent < 19.
            const unsigned shift=unsigned(20-exponent);
            std::uint32_t high=(v.high&0xfffffu)|0x100000u,low=v.low;
            if(shift>=64)high=low=0;
            else if(shift>=32){low=high>>(shift-32);high=0;}
            else{low=(low>>shift)|(high<<(32-shift));high>>=shift;}
            const auto magnitude=std::int64_t((std::uint64_t(high)<<32)|low);
            out=v.high>>31?-magnitude:magnitude;return true;
        }
        static bool fixedStep(Real v,std::int32_t& out){ // |v| < 1/2
            std::int64_t q;
            if(!fixed(v,0,q)||q>=0x80000000ll||q<=-0x80000000ll)return false;
            out=std::int32_t(q);return true;
        }
        void begin(){
            steps=0;
            exact=!(fixed(x,18,xq)&&fixed(y,18,yq)&&fixed(life,18,lifeq)&&
                    fixedStep(vx,vxq)&&fixedStep(vy,vyq)&&fixedStep(rate,rateq));
            settle();
        }
        // Every decision until the next step must be clear of the bound, or
        // the particle continues on binary64 sums until it respawns: a sum
        // can sit on a pixel or frame boundary for many updates (create()'s
        // 0.3 steps reach k+0.5), and the draw must not replay it each time.
        bool settle(){
            if(exact)return true;
            constexpr std::int64_t threshold=0x7ffff583all; // floor((8-epsilon)*2^32)
            const auto b=bound();
            const bool below=lifeq+b<=threshold-1,above=lifeq-b>=threshold+1;
            std::int32_t v;
            if(pixel(xq,b,v)&&pixel(yq,b,v)&&(above||(below&&frameFixed(lifeq,b,v))))return true;
            Real cx,cy,cl;
            if(!current(cx,cy,cl))return false;
            x=cx;y=cy;life=cl;exact=true;return true;
        }
        static bool replay(Real v,Real step,unsigned n,Real& out){
            for(unsigned i=0;i<n;++i)if(!v.add(step))return false;
            out=v;return true;
        }
        bool current(Real& cx,Real& cy,Real& cl)const{
            if(exact){cx=x;cy=y;cl=life;return true;}
            return replay(life,rate,steps,cl)&&replay(x,vx,steps,cx)&&replay(y,vy,steps,cy);
        }
        // life += rate; x += vx; y += vy.
        bool advance(){
            if(!exact&&steps==maxSteps){
                Real cx,cy,cl;
                if(!current(cx,cy,cl))return false;
                x=cx;y=cy;life=cl;exact=true;
            }
            if(exact)return life.add(rate)&&x.add(vx)&&y.add(vy);
            ++steps;lifeq+=rateq;xq+=vxq;yq+=vyq;
            return settle();
        }
        std::int64_t bound()const{return 2*std::int64_t(steps)+2;}
        // compareRunnerReal(life, 8) < 0: fl(life-8) below -epsilon, with
        // epsilon the double nearest 1e-5. Its threshold 8-epsilon lies in
        // [threshold, threshold+1) Q32 units; one unit further either side
        // is far beyond fl(life-8)'s rounding.
        bool lifeBelow8(bool& below)const{
            if(!exact){
                constexpr std::int64_t threshold=0x7ffff583all; // floor((8-epsilon)*2^32)
                if(lifeq+bound()<=threshold-1){below=true;return true;}
                if(lifeq-bound()>=threshold+1){below=false;return true;}
            }
            Real cl=life;int order;
            if(!exact&&!replay(life,rate,steps,cl))return false;
            if(!compareRunnerReal(cl,Real::fromInteger(8),order))return false;
            below=order<0;return true;
        }
        // Nearest pixel, ties to even (MotionBinary64::roundPixel).
        static bool pixel(std::int64_t q,std::int64_t bound,std::int32_t& out){
            const auto fraction=std::uint32_t(q);
            const auto whole=std::int32_t(q>>32);
            if(fraction+std::uint64_t(bound)<0x80000000u){out=whole;return true;}
            if(fraction>0x80000000u+std::uint64_t(bound)){out=whole+1;return true;}
            return false;
        }
        bool pixels(std::int32_t& px,std::int32_t& py)const{
            if(!exact&&pixel(xq,bound(),px)&&pixel(yq,bound(),py))return true;
            Real cx=x,cy=y;
            if(!exact&&(!replay(x,vx,steps,cx)||!replay(y,vy,steps,cy)))return false;
            return cx.roundPixel(px)&&cy.roundPixel(py);
        }
        // draw_sprite's subimage: life's integer part, 0 below one or
        // negative. Life is below 8 here.
        // max(floor(life), 0) at both ends of (q-bound, q+bound).
        static bool frameFixed(std::int64_t q,std::int64_t bound,std::int32_t& out){
            auto low=std::int32_t((q-bound)>>32),high=std::int32_t((q+bound-1)>>32);
            if(low<0)low=0;
            if(high<0)high=0;
            if(low!=high)return false;
            out=low;return true;
        }
        bool frame(std::int32_t& out)const{
            if(!exact&&frameFixed(lifeq,bound(),out))return true;
            Real cl=life;
            if(!exact&&!replay(life,rate,steps,cl))return false;
            const int exponent=int((cl.high>>20)&0x7ffu)-1023;
            if((cl.high>>31)||exponent<0)out=0;
            else out=std::int32_t(((cl.high&0xfffffu)|0x100000u)>>(20-exponent));
            return true;
        }
    };
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
            p.begin();
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
        p.begin();
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

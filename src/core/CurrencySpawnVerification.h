#ifndef DOWNWELL_CORE_CURRENCY_SPAWN_VERIFICATION_H
#define DOWNWELL_CORE_CURRENCY_SPAWN_VERIFICATION_H
#include "CurrencySpawnScript.h"
#include "CurrencyCreate.h"
#include "CurrencyReward.h"
#include "GameMakerRandom.h"
namespace downwell {
// Fixed-capacity source-script/Create integration tape, not world admission.
struct CurrencySpawnVerification {
    struct Ops {
        using Handle=std::uint32_t;
        CurrencySpawnSource<Handle>& source;
        CurrencyCreate children[3];
        GameMakerRandom random{9};
        unsigned count=0;
        explicit Ops(CurrencySpawnSource<Handle>& caller):source(caller){}
        bool parent(int object,int& p){p=83;return object==93;}
        bool create(int object,MotionBinary64 x,MotionBinary64 y,Handle& h){
            if(object!=248||count==3)return false;
            struct Queries {
                CurrencySpawnSource<Handle>& source;Handle previous;
                bool wall(MotionBinary64,MotionBinary64,bool& hit){hit=false;return source.myGem==previous;}
                bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=false;return source.myGem==previous;}
                bool publish(RunnerImageValue,RunnerImageValue){return false;}
            } queries{source,source.myGem};
            if(!children[count].create(x,y,1,false,random,queries))return false;
            h=++count;return true;
        }
        bool amount(Handle h,int n){if(!h||h>count||source.myGem!=h)return false;children[h-1].getAmount=n;return true;}
        bool size(Handle h,int n){if(!h||h>count)return false;children[h-1].size=n;return true;}
        bool attracted(Handle h,bool n){if(!h||h>count)return false;children[h-1].attracted=n;return true;}
    };
    static bool run(){
        // Exercise the shared live reward wrapper as well as the exact Create
        // helper. Its fixed-point velocities remain provisional, but area
        // branches must consume the identical RNG stream before later births.
        for(int area=1;area<=5;++area){
            GameMakerRandom random(47),oracle(47);bool valid=true;unsigned count=0;
            const auto result=spawnCurrencyReward(13,12,-3,false,3,random,[&](const CurrencyBirth& birth){
                struct Queries {
                    bool wall(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                    bool tile(MotionBinary64,MotionBinary64,bool& hit){hit=false;return true;}
                    bool publish(RunnerImageValue,RunnerImageValue){return false;}
                } queries;
                CurrencyCreate source;
                valid=valid&&source.create(MotionBinary64::fromInteger(12),MotionBinary64::fromInteger(-3),
                    area,false,oracle,queries);
                const bool slow=area==3||area==4;
                std::int32_t direction;valid=valid&&source.direction.roundPixel(direction);
                valid=valid&&birth.amount==(count?2:10)&&birth.large==!count&&
                    birth.motion.gravity==(slow?112:448)&&birth.motion.maximumGravity==(slow?6400:51200)&&
                    birth.motion.animation.speedBits()==source.imageSpeed.bits&&
                    birth.motion.homing.direction==GemHoming::normalize(direction*65536L)&&
                    (area!=5||birth.motion.vy<=0);
                ++count;
            },false,area);
            if(!valid||!result.complete||result.emitted!=3||count!=3||
                random.drawCount()!=oracle.drawCount()||random.stateIndex()!=oracle.stateIndex())return false;
            for(unsigned i=0;i<16;++i)if(random.stateWord(i)!=oracle.stateWord(i))return false;
            GemMotion moving;moving.initialize(12,-3,false,random,false,area);
            moving.vx=0;moving.vy=moving.maximumGravity-1;
            const auto beforeY=moving.y;
            auto clear=[](int,int){return false;};
            if(!moving.step(100000,100000,clear,random)||
               moving.vy!=moving.maximumGravity-1+moving.gravity||moving.y!=beforeY+moving.vy||
               !moving.step(100000,100000,clear,random)||moving.vy!=moving.maximumGravity)return false;
            // Reusing a record must restore ordinary-area constants.
            moving.initialize(0,0,false,random);
            if(moving.gravity!=448||moving.maximumGravity!=51200)return false;
        }
        for(unsigned full=0;full<2;++full){
            CurrencySpawnSource<std::uint32_t> source;
            source.objectIndex=93;source.stomped=MotionBinary64::fromInteger(1);
            source.x={0x40288000u,0};source.y={0xc00c0000u,0};source.myGem=77;
            Ops ops{source};bool good=true;
            if(CurrencySpawnScript::run(source,MotionBinary64::fromInteger(full?40:13),ops,good)==bool(full)||
                good==bool(full)||ops.count!=3||source.myGem!=3||!source.atrState||ops.random.drawCount()!=24)return false;
            for(unsigned i=0;i<3;++i){const auto& gem=ops.children[i];
                if(gem.getAmount!=(full||i==0?10:2)||gem.size!=int(full||i==0)||!gem.attracted||
                    gem.x.bits!=0x41440000u||gem.y.bits!=0xc0600000u)return false;
            }
            if(source.moniAmount.high!=(full?0x40240000u:0xbff00000u)||source.moniAmount.low)return false;
            if(full&&(CurrencySpawnScript::run(source,MotionBinary64::fromInteger(40),ops,good)||
                ops.random.drawCount()!=24||ops.count!=3))return false;
        }
        return true;
    }
};
}
#endif

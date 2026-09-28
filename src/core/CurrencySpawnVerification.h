#ifndef DOWNWELL_CORE_CURRENCY_SPAWN_VERIFICATION_H
#define DOWNWELL_CORE_CURRENCY_SPAWN_VERIFICATION_H
#include "CurrencySpawnScript.h"
#include "CurrencyCreate.h"
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

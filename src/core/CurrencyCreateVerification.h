#ifndef DOWNWELL_CORE_CURRENCY_CREATE_VERIFICATION_H
#define DOWNWELL_CORE_CURRENCY_CREATE_VERIFICATION_H
#include "CurrencyCreate.h"
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

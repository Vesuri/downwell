#ifndef DOWNWELL_CORE_CATACOMB_BUILDER_DISPATCH_H
#define DOWNWELL_CORE_CATACOMB_BUILDER_DISPATCH_H
#include "CatacombBuilderSymbols.h"
#include "RandomBinary64.h"
#include "RunnerRealComparison.h"
namespace downwell {
// Source builder-instance state for area-specific cells. Common terrain and
// pattern traversal remain with the caller. Child Create uses this same RNG
// synchronously; never precompute choices past a child callback.
struct CatacombBuilderDispatch {
    int spawnChance=0;
    bool succeeded=true;
    template<class Random,class Create> bool cell(char symbol,int x,int y,unsigned level,
        bool hard,Random& random,Create create){
        if(!succeeded)return false;
        if(executing_)return succeeded=false;
        executing_=true;
        struct Ops {
            CatacombBuilderDispatch& state;Random& random;Create& emit;
            int choose(const int* values,unsigned count){return values[random.chooseIndexSmall(count)];}
            int irandom(unsigned upper){return int(random.irandomSmall(upper));}
            bool spawnChance(int value){state.spawnChance=value;return state.succeeded;}
            bool randomFiveBelow(unsigned level){
                MotionBinary64 value;int order=0;
                if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(5),value)||
                   !compareRunnerReal(value,MotionBinary64::fromInteger(level),order)){
                    state.succeeded=false;return false;
                }
                return order<0;
            }
            bool create(int object,int x,int y){return state.succeeded&&emit(object,x,y);}
        } ops{*this,random,create};
        const bool ok=emitCatacombSymbol(symbol,x,y,level,hard,ops);
        executing_=false;return succeeded=succeeded&&ok;
    }
private:
    bool executing_=false;
};
}
#endif

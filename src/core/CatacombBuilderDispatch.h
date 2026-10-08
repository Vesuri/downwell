#ifndef DOWNWELL_CORE_CATACOMB_BUILDER_DISPATCH_H
#define DOWNWELL_CORE_CATACOMB_BUILDER_DISPATCH_H
#include "CatacombBuilderSymbols.h"
#include "RandomBinary64.h"
#include "RunnerRealComparison.h"
#include "BuilderPatternPack.h"
#include "BuilderStringTraversal.h"
#include "BuilderCommonDispatch.h"
namespace downwell {
// Source builder-instance state for area-specific cells. Common terrain and
// pattern traversal remain with the caller. Child Create uses this same RNG
// synchronously; never precompute choices past a child callback.
struct CatacombBuilderDispatch {
    int spawnChance=0;
    bool succeeded=true;
    template<class Random,class Ops>
    bool build(const BuilderPatternPack& pack,unsigned id,BuilderStringTraversal& traversal,
               BuilderCommonDispatch& common,unsigned level,bool hard,unsigned extraRows,
               int roomWidth,int playerHp,unsigned style,Random& random,Ops& ops){
        if(!common.succeeded)return succeeded=false;
        CommonOps<Random,Ops> shared{random,ops};
        return pattern(pack,id,traversal,level,hard,extraRows,random,
            [&](int object,int x,int y){return ops.create(object,x,y,[](auto){return true;});},
            [&](char c,int x,int y){return common.cell(c,x,y,roomWidth,playerHp,style,shared);},
            [&](int x,int y){return common.pillar(x,y,roomWidth,shared);});
    }
    template<class Random,class Create,class Common,class Pillar>
    bool pattern(const BuilderPatternPack& pack,unsigned id,BuilderStringTraversal& traversal,
                 unsigned level,bool hard,unsigned extraRows,Random& random,
                 Create create,Common common,Pillar pillar){
        PackedBuilderPattern pattern;
        const std::uint8_t* text=nullptr;std::uint32_t length=0;
        if(!succeeded || executing_ || traversing_ || level<1 || level>3 ||
           !pack.pattern(id,pattern) || unsigned(pattern.family)<6 || unsigned(pattern.family)>11 ||
           !pack.literalData(id,text,length))return succeeded=false;
        traversing_=true;
        const bool ok=traversal.run(text,length,extraRows,
            [&](char symbol,int x,int y){return cell(symbol,x,y,level,hard,random,create);},
            [&](char symbol,int x,int y){return common(symbol,x,y)&&succeeded;},
            [&](int x,int y){return pillar(x,y)&&succeeded;});
        traversing_=false;
        return succeeded=succeeded&&ok;
    }
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
    template<class Random,class Owner> struct CommonOps {
        Random& random;Owner& owner;
        int choose(const int* values,unsigned count){return values[random.chooseIndexSmall(count)];}
        template<class Post> bool create(int object,int x,int y,Post post){return owner.create(object,x,y,post);}
        template<class Child> bool write(Child child,BuilderChildField field,int value){return owner.write(child,field,value);}
        template<class Child> bool deactivate(Child child){return owner.deactivate(child);}
        template<class Child> bool rightOfMiddle(Child child,int width,bool& right){return owner.rightOfMiddle(child,width,right);}
    };
    bool executing_=false;
    bool traversing_=false;
};
}
#endif

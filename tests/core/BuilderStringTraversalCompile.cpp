#include "core/CatacombBuilderDispatch.h"
#include "core/GameMakerRandom.h"
// Instantiate the same shared traversal and dispatcher for the freestanding
// target; this is a compiler check, not a native gameplay integration test.
bool compileBuilderTraversal(downwell::CatacombBuilderDispatch& dispatch,
    const downwell::BuilderPatternPack& pack,downwell::BuilderStringTraversal& cursor,
    downwell::GameMakerRandom& random,unsigned id,
    bool (*create)(int,int,int),bool (*common)(char,int,int),bool (*pillar)(int,int)){
    return dispatch.pattern(pack,id,cursor,1,false,4,random,create,common,pillar);
}

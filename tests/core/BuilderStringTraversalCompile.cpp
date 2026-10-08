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
struct CompileBuilderOwner {
    bool birth(int,int,int,int&);
    template<class Post> bool create(int object,int x,int y,Post post){
        int child=0;return birth(object,x,y,child)&&post(child);
    }
    bool write(int,downwell::BuilderChildField,int);
    bool deactivate(int);
    bool rightOfMiddle(int,int,bool&);
};
bool compileCompleteBuilderDispatch(downwell::CatacombBuilderDispatch& dispatch,
    const downwell::BuilderPatternPack& pack,downwell::BuilderStringTraversal& cursor,
    downwell::BuilderCommonDispatch& common,downwell::GameMakerRandom& random,
    CompileBuilderOwner& owner,unsigned id){
    return dispatch.build(pack,id,cursor,common,1,false,4,480,4,0,random,owner);
}

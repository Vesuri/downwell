#include "core/GameplayTerrainInstances.h"
#include <cassert>
#include <climits>
#include <cstdio>
#include <vector>
using namespace downwell;
struct Lifecycle {
    using H=InstanceHandle;using R=GameplayTerrainInstances::Record;using V=RunnerImageValue;
    int retireRemaining=-1;
    bool birth(H h,const R& r){
        assert(h.generation==r.lifetimeId&&r.lifetimeId);
        assert(r.spriteId==(r.objectId==99?46:-1)&&r.imageIndex==0&&r.imageSpeed==1&&!r.destroyRequested&&!r.wallNeedsReset);
        const auto& w=r.wall;
        assert(w.checked==0&&w.pillar==0&&w.neighbors==0&&w.imageIndex==0&&w.checkThing==0&&
            w.checkDistance==0&&!w.surrounded&&!w.deactivate&&w.lights==0);
        return true;
    }
    bool retire(H,const R&){if(!retireRemaining)return false;if(retireRemaining>0)--retireRemaining;return true;}
    bool activation(H,const R&,bool){return true;}bool membership(H,const R&,bool){return true;}
    bool move(H,const R&,V,V){return true;}bool sprite(H,const R&,int){return true;}
    bool imageIndex(H,const R&,V){return true;}bool markDestroyed(H,const R&){return true;}
};
static Lifecycle lifecycle;
static GameplayTerrainInstances terrain(GameplayTerrainInstances::Binding::bindDestruction(lifecycle));
static unsigned examined=0;
static void check(int first,int end,int sprite=70,const GameplayTerrainInstances& store=terrain){
    std::vector<unsigned> expected,actual;
    for(unsigned i=0;i<store.size();++i){
        const auto& r=*store.find(store.handle(i));
        if(wallIntersectsRows(sprite,r.y,first,end))expected.push_back(r.lifetimeId);
    }
    assert(store.visitWallRows(first,end,[&](const auto& r){
        ++examined;
        if(wallIntersectsRows(sprite,r.y,first,end))actual.push_back(r.lifetimeId);
        return true;
    }));
    assert(actual==expected);
    unsigned calls=0;
    const bool completed=store.visitWallRows(first,end,[&](const auto&){++calls;return false;});
    assert(calls<=1&&completed==(calls==0));
}
int main(){
    // Dirty recycled records must not take the untouched-slot fast path.
    {
        static GameplayTerrainInstances reused(GameplayTerrainInstances::Binding::bindDestruction(lifecycle));
        GameMakerRandom rng(5);
        for(unsigned i=0;i<96;++i){
            const int object=i%3==0?60:i%3==1?93:99;
            assert(reused.append(object,int(i)*16,-int(i)*16,i%2,1000+i,object==60?676:-1));
            const auto h=reused.handle(0);const auto* r=reused.find(h);
            assert(h.slot==0&&r&&r->x==int(i)*16&&r->y==-int(i)*16&&r->active==bool(i%2)&&
                r->sourceInstanceId==1000+i&&r->creationCodeId==(object==60?676:-1));
            assert(reused.createProperties(h,74,true,rng)&&reused.roomCreation(h)&&reused.imageIndex(h,7));
            if(object==60){
                assert(r->wall.pillar==-1&&r->wallNeedsReset);
                assert(reused.createProperties(h,74,true,rng)&&r->wall.pillar==0&&!r->wallNeedsReset);
                assert(reused.roomCreation(h)&&r->wall.pillar==-1&&r->wallNeedsReset);
            }
            assert(reused.activation(h,false)&&reused.requestDestroy(h)&&reused.retireMarked(h));
            assert(!reused.find(h)&&reused.size()==0);
        }
    }
    for(unsigned i=0;i<1024;++i)assert(terrain.append(i%3==0?93:60,240,int(i/2)*16,true));
    check(2048,2050);
    assert(examined<terrain.size()/4); // Reject whole unrelated birth-order groups.
    for(int first=-32;first<8256;first+=7){check(first,first+3,51);check(first,first+284);}
    unsigned random=97;
    for(unsigned step=0;step<2000;++step){
        random=random*1664525u+1013904223u;
        const auto h=terrain.handle(random%terrain.size());
        assert(terrain.requestDestroy(h)&&terrain.retireMarked(h));
        // Reused identity slots append at the end, not at slot order.
        const int y=int((random>>16)%700)*16-1000;
        assert(terrain.append(60,240,y,true));
        check(y-16,y);check(y,y+1);check(y+16,y+17);check(-16,9000);
    }
    assert(terrain.clear());
    for(int y:{INT_MIN,INT_MIN+16,-16,0,16,INT_MAX-16,INT_MAX})
        assert(terrain.append(60,0,y,true));
    check(INT_MIN,INT_MIN+1);check(INT_MAX-1,INT_MAX);check(0,1);check(1,1);check(2,1);
    GameMakerRandom rng(1);
    assert(terrain.createProperties(terrain.handle(0),987,false,rng));
    unsigned unbounded=0;
    assert(terrain.visitWallRows(500,501,[&](const auto&){++unbounded;return true;}));
    assert(unbounded==terrain.size());
    assert(terrain.clear()&&terrain.append(60,0,0,true));
    unsigned reset=0;
    assert(terrain.visitWallRows(500,501,[&](const auto&){++reset;return true;}));
    assert(!reset);
    Lifecycle failure;failure.retireRemaining=17;
    static GameplayTerrainInstances partial(GameplayTerrainInstances::Binding::bindDestruction(failure));
    for(unsigned i=0;i<160;++i)assert(partial.append(60,0,int(i)*64,true));
    assert(!partial.clear()&&!partial.valid()&&partial.size()==143);
    for(int first=0;first<11000;first+=13)check(first,first+3,70,partial);
    std::puts("Terrain row selection: ordered footprint equivalence, 2000 retire/reuse mutations, limits and unknown-art fallback passed");
}

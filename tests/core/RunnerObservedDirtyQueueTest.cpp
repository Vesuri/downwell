#include "core/GameplayTerrainInstancesRuns.h"
#include "core/RunnerObservedDirtyQueue.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
struct Terrain:GameplayTerrainInstancesRuns {
    using GameplayTerrainInstancesRuns::GameplayTerrainInstancesRuns;
    mutable unsigned observations=0;
    template<unsigned Which,class Visit> bool visitDirty(Visit visit)const{
        ++observations;return GameplayTerrainInstancesRuns::visitDirty<Which>(visit);
    }
};
struct Observer {
    template<class... A> bool birth(const A&...){return true;}
    template<class... A> bool retire(const A&...){return true;}
    template<class... A> bool activation(const A&...){return true;}
    template<class... A> bool membership(const A&...){return true;}
    template<class... A> bool move(const A&...){return true;}
    template<class... A> bool sprite(const A&...){return true;}
    template<class... A> bool imageIndex(const A&...){return true;}
    template<class... A> bool markDestroyed(const A&...){return true;}
};
static constexpr unsigned Capacity=40,First=3,Last=35;
template<unsigned Which> using Queue=RunnerObservedDirtyQueue<Capacity,First,Last,Terrain,Which>;
using Reference=RunnerSpatialDirtyQueue<Capacity>;
template<class Q> static std::vector<unsigned> contents(const Q& queue){
    std::vector<unsigned> values;
    for(unsigned n=queue.first();n!=queue.End;n=queue.next(n)){
        assert(values.size()<Capacity&&queue.contains(n));values.push_back(n);
    }
    assert(values.size()==queue.size());return values;
}
template<class Q> static void equal(const Q& queue,const Reference& reference){
    assert(queue.size()==reference.size()&&queue.first()==reference.first());
    assert(contents(queue)==contents(reference));
    for(unsigned slot=0;slot<=Capacity;++slot){
        assert(queue.contains(slot)==reference.contains(slot));
        assert(queue.next(slot)==reference.next(slot));
    }
}
int main(){
    // A borrowed constructor interval publishes precisely its successful
    // prefix, including low-word carry. No topology is built while recording.
    for(unsigned prefix=0;prefix<Last-First;++prefix)
        for(auto clock:{RunnerRegistrationOrder{},RunnerRegistrationOrder{0xfffffffcu,2}}){
            Terrain terrain;Queue<0> queue(clock);Reference reference;assert(queue.bindTerrain(terrain));
            assert(queue.mark(0)&&reference.mark(0));
            for(unsigned i=0;i<=prefix;++i){
                assert(terrain.appendInitializedCell(93,int(i)*16,0,true,
                    [&](InstanceHandle h,auto&,bool&,bool inherited){
                        if(!i)assert(queue.markAllocatedTerrain(h.slot+First,false));
                        else {assert(inherited);terrain.dirtyInherit<0>(h.slot);}
                        assert(reference.mark(h.slot+First));return true;
                    }));
            }
            queue.completeInheritedTerrain(First+prefix,prefix);
            assert(!terrain.observations);equal(queue,reference);
            assert(queue.mark(1)&&reference.mark(1));equal(queue,reference);
            assert(queue.mark(First)&&reference.mark(First));equal(queue,reference);
        }
    // Independent publication clocks, low-word carry and complete wrap. No
    // traversal is requested while admitted births only expose memberships.
    for(auto clock:{RunnerRegistrationOrder{},RunnerRegistrationOrder{0xfffffffcu,2},RunnerRegistrationOrder{0xfffffffcu,0xffffffffu}}){
        Terrain terrain;Queue<0> queue(clock);Reference reference;assert(queue.bindTerrain(terrain));
        for(unsigned i=0;i<Last-First;++i){
            assert(terrain.appendInitializedCell(93,int(i)*16,0,true,[&](InstanceHandle h,auto&,bool&,bool inherited){
                assert(queue.markAllocatedTerrain(h.slot+First,i!=0&&inherited));
                assert(reference.mark(h.slot+First));return true;
            }));
            assert(queue.first()==reference.first()&&queue.size()==reference.size());
            if(clock.high!=0xffffffffu)assert(terrain.observations==0);
        }
        if(clock.high!=0xffffffffu)assert(terrain.ownershipRunCount()==1);
        equal(queue,reference);
        // Another publication can split the shared run without invalidating
        // queue chronology or a previously derived successor table.
        RunnerSpatialState changed;changed.flags=3;terrain.spatialLiteral(7,changed);
        equal(queue,reference);
        for(unsigned i=0;i<Capacity;++i){assert(queue.remove(i)==reference.remove(i));equal(queue,reference);}
    }
    Observer observer;Terrain terrain(Terrain::Binding::bindDestruction(observer));
    Queue<0> spatial;Queue<1> collision;Reference spatialReference,collisionReference;
    assert(spatial.bindTerrain(terrain)&&collision.bindTerrain(terrain));
    for(unsigned i=0;i<Last-First;++i)
        assert(terrain.appendInitializedCell(93,int(i)*16,0,true,[](auto,auto&,bool&,bool){return true;}));
    unsigned random=0x8192517u,checks=0;
    auto exercise=[&](auto& queue,auto& reference,unsigned action,unsigned slot){
        if(action<5)assert(queue.mark(slot)==reference.mark(slot));
        else if(action<9)assert(queue.remove(slot)==reference.remove(slot));
        else{queue.clear();reference.clear();}
    };
    for(unsigned i=0;i<20000;++i){
        random=random*1664525u+1013904223u;
        const unsigned slot=(random>>8)%(Capacity+1),action=(random>>24)%10;
        if(random&1)exercise(spatial,spatialReference,action,slot);
        else exercise(collision,collisionReference,action,slot);
        equal(spatial,spatialReference);equal(collision,collisionReference);++checks;
    }
    // Read successor AFTER refresh. Moving a successor to the head leaves it
    // for a later flush. Include both actor/terrain directions and failures.
    for(unsigned first:{0u,3u,7u,36u})for(unsigned other:{1u,4u,8u,37u}){
        spatial.clear();spatialReference.clear();
        for(unsigned slot:{2u,other,first}){assert(spatial.mark(slot)&&spatialReference.mark(slot));}
        std::vector<unsigned> actual,expected;
        const auto flags=[](unsigned){return 0u;};
        assert(spatial.flush(flags,[&](unsigned slot){actual.push_back(slot);if(slot==first)assert(spatial.mark(other));return true;}));
        assert(spatialReference.flush(flags,[&](unsigned slot){expected.push_back(slot);if(slot==first)assert(spatialReference.mark(other));return true;}));
        assert(actual==expected);equal(spatial,spatialReference);
        assert(spatial.mark(first)&&spatialReference.mark(first));actual.clear();expected.clear();
        assert(!spatial.flush(flags,[&](unsigned slot){actual.push_back(slot);return slot!=other;}));
        assert(!spatialReference.flush(flags,[&](unsigned slot){expected.push_back(slot);return slot!=other;}));
        assert(actual==expected);equal(spatial,spatialReference);
    }
    // Retirement/reuse must remove the old queue membership before replacing
    // its identity, while the other cells keep their original order keys.
    spatial.clear();spatialReference.clear();
    const auto retired=terrain.handle(5);assert(retired.slot==5);
    for(unsigned slot:{First+4,First+5,First+6})assert(spatial.mark(slot)&&spatialReference.mark(slot));
    assert(spatial.remove(First+5)&&spatialReference.remove(First+5));
    assert(terrain.requestDestroy(retired)&&terrain.retireMarked(retired));
    assert(terrain.appendInitializedCell(93,80,0,true,[&](InstanceHandle h,auto&,bool&,bool){
        assert(h.slot==5&&h.generation>retired.generation);
        return spatial.mark(h.slot+First)&&spatialReference.mark(h.slot+First);
    }));
    equal(spatial,spatialReference);
    std::printf("Observed dirty queues: %u mixed actor/terrain mutations, lazy traversal, clock wrap, callback order and reuse passed\n",checks);
}

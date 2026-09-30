#include "core/GameplayTerrainInstancesRuns.h"
#include "core/RunnerObservedRegistration.h"
#include "core/RunnerSpatialInstances.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

using namespace downwell;
static std::vector<unsigned char> read(const char* path){
    std::ifstream file(path,std::ios::binary);assert(file);
    return {std::istreambuf_iterator<char>(file),{}};
}

// Observe the completed or failed transaction, never introduce a query into
// the callback-free interval being replaced. Keep the full-record constructor
// as the oracle, including failures BEFORE its initial bounds publication.
struct BirthGraph {
    unsigned value=0,preparedValue=0,calls=0;
    bool reject=false;
    bool valid()const{return true;}
    const unsigned* flags(unsigned)const{return &value;}
    bool prepareBirth(unsigned){++calls;value=preparedValue;return !reject;}
};

template<unsigned Nodes> static void transactions(const PreparedObjectDefaults<386>& defaults,
    const PreparedTerrainBirthBounds& geometry){
    using Terrain=GameplayTerrainInstancesRuns;
    using Reference=RunnerSpatialInstances<2050,Nodes,true,1,2049,true,
        RunnerSpatialLifetimes<2050>,RunnerRunMembership<2050,1,2049>,true>;
    using Native=RunnerSpatialInstances<2050,Nodes,true,1,2049,true,
        RunnerSpatialLifetimes<2050,1,2049,Terrain>,
        RunnerObservedRegistration<2050,1,2049,Terrain>,true,Terrain>;
    unsigned comparisons=0,indexFailures=0;
    constexpr unsigned cells=Nodes==1?8:3;
    for(int object:{60,93,99})for(bool active:{false,true})
        for(unsigned flags=0;flags<16;++flags)for(unsigned preparedFlags=0;preparedFlags<16;++preparedFlags)
        for(unsigned failure=0;failure<3;++failure){
            auto terrain=std::make_unique<Terrain>();
            auto reference=std::make_unique<Reference>();
            auto native=std::make_unique<Native>();
            assert(native->bindTerrainLifetimes(*terrain));
            native->prepareTerrainStates(defaults);
            assert(native->bindTerrainGeometry(geometry)&&reference->bindTerrainGeometry(geometry));
            const auto* d=defaults.get(object);assert(d);
            // Warm chronology and shared descriptors before the rejected
            // cell. A first-slot-only fixture misses split-aware publication.
            for(unsigned cell=0;cell<cells;++cell){
            const unsigned stop=cell+1==cells?failure:0;
            const int x=64+int(cell)*16;
            typename Reference::State state;typename Reference::Box bounds;
            RunnerSpatialSplit::PixelBox pixels;PreparedTerrainBirthBounds::Key key;
            bool precise=false;
            assert(geometry.at(object,x,256,state.x,state.y,bounds,precise,&pixels,&key));
            state.flags=8u|(active?0u:2u);
            BirthGraph rg{d->propertyFlags|(flags<<3),d->propertyFlags|(preparedFlags<<3),0,stop==2},ng=rg;
            bool np=false;unsigned nproperties=0,rroom=0,nroom=0;
            const InstanceHandle shared{cell+1,cell+1};
            typename Reference::Properties properties;
            const bool expected=reference->attachPreparedTerrainBirthReference(shared,object,state,properties,defaults,rg,
                bounds,pixels,precise,[&](RunnerImageValue depth){
                    assert(depth.bits==d->creationDepth.bits);++rroom;return stop!=1;
                });
            const bool actual=terrain->appendInitializedCell(object,x,256,active,
                [&](InstanceHandle local,auto&,bool&,bool){
                    assert(local.slot==cell&&local.generation==cell+1);
                    return native->attachGeneratedTerrainCell(shared,object,!active,*d,ng,&ng.value,pixels,
                        [&]{++nproperties;},[&](RunnerImageValue depth){
                            assert(depth.bits==d->creationDepth.bits);++nroom;return stop!=1;
                        },key,np,false);
                });
            assert(actual==expected);
            if constexpr(Nodes!=1)assert(actual==(stop==0));
            if(!actual&&!stop)++indexFailures;
            assert(nproperties==1&&rroom==nroom&&rroom==1);
            assert(rg.calls==ng.calls&&rg.calls==(stop==1?0u:1u)&&np==(stop==0));
            assert(rg.value==ng.value);
            assert(reference->contains(shared)&&native->contains(shared));
            const auto r=reference->resolve(shared);const auto n=native->resolve(shared);
            assert(r&&n&&r->same(*n));
            assert(reference->pendingSpatial()==native->pendingSpatial());
            assert(reference->pendingCollision()==native->pendingCollision());
            assert(reference->pendingSpatialIndex()==native->pendingSpatialIndex());
            assert(reference->bindingRevision()==native->bindingRevision());
            assert(reference->captureRevision()==native->captureRevision());
            for(unsigned earlier=0;earlier<cell;++earlier){
                const InstanceHandle previous{earlier+1,earlier+1};
                const auto old=native->resolve(previous);
                assert(old&&reference->resolve(previous)->same(*old));
            }
            ++comparisons;
            if(!actual)break;
            }
        }
    if constexpr(Nodes==1)assert(indexFailures);
    std::printf("Terrain birth transaction: %u prefixes, %u index-capacity failures match full-record reference (nodes=%u)\n",
        comparisons,indexFailures,Nodes);
}
int main(){
    const auto ob=read("generated/downwell-objects.dwob");
    const auto sb=read("generated/downwell-sprites.dwsc");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    PreparedObjectDefaults<386> defaults;assert(defaults.prepare(objects,sprites));
    PreparedTerrainBirthBounds geometry;assert(geometry.prepare(defaults,sprites));
    transactions<4100>(defaults,geometry);
    transactions<1>(defaults,geometry);
}

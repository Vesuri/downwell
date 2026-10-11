#include "core/RunnerLogicalLayerOrder.h"
#include "core/RunnerLayerDraw.h"
#include "core/RunnerPreparedDrawList.h"
#include "core/NativeTerrainLifetimes.h"
#include <vector>
#include <cstdio>
#include <cstdlib>
using namespace downwell;
static void check(bool ok,unsigned line){if(!ok){std::fprintf(stderr,"logical layer failure at %u\n",line);std::abort();}}
#define REQUIRE(x) check((x),__LINE__)
struct Terrain {
    static constexpr unsigned capacity=4;
    RunnerRetainedLayerState states[capacity]{};
    RunnerRetainedLayerRead layerState(unsigned slot)const{return slot<capacity?RunnerRetainedLayerRead(states[slot]):nullptr;}
    RunnerRetainedLayerRead layerState(InstanceHandle h)const{return h.generation==1?layerState(h.slot):nullptr;}
    void setLayerState(unsigned slot,const RunnerRetainedLayerState& value){states[slot]=value;}
    void publishInheritedLayer(unsigned){REQUIRE(false);}
    template<class Visit> bool visitLayerCutoffs(unsigned layer,const RunnerLayerOrderKey* after,Visit visit)const{
        for(const auto& s:states)if(s.bound()&&s.layer==layer&&s.inactive&&(!after||after->before(s.order)))
            if(!visit(s.order))return false;
        return true;
    }
    template<class Visit> bool visitLayerStates(Visit visit){
        for(unsigned i=0;i<capacity;++i)if(!visit(InstanceHandle{i,1},states[i]))return false;return true;
    }
};
using Logical=RunnerLogicalLayerOrder<3,8,Terrain>;
using Reference=RunnerLayers<3,8>;
static void compare(Logical& logical,const Reference& reference){
    for(unsigned layer=0;layer<3;++layer){
        REQUIRE(bool(logical.get(layer))==bool(reference.get(layer)));
        if(!reference.get(layer))continue;
        REQUIRE(logical.elementCount(layer)==reference.elementCount(layer));
        std::vector<unsigned> a,b;
        REQUIRE(reference.visitElements(layer,[&](unsigned e){a.push_back(e);return true;}));
        REQUIRE(logical.visitLogical(layer,[&](bool terrain,unsigned e){b.push_back(e+(terrain?4:0));return true;}));
        REQUIRE(a==b);
    }
}
static void inheritedCapacity(){
    for(unsigned prefix=0;prefix<4;++prefix){
        Terrain terrain;Logical logical;Reference reference;REQUIRE(logical.bindTerrain(terrain));
        unsigned layer,other;REQUIRE(logical.insertAfterEquals(1,0,true,layer));
        REQUIRE(reference.insertAfterEquals(1,0,true,other)&&layer==other);
        for(unsigned e=0;e<4;++e){
            REQUIRE(logical.attach(layer,e,true,false,[](unsigned){return false;}));
            REQUIRE(reference.attach(layer,e,true,false,[](unsigned){return false;}));
        }
        REQUIRE(logical.retainAllocated(0,layer,false));
        REQUIRE(reference.attach(layer,4,true,false,[](unsigned){return false;}));
        REQUIRE(logical.retainedAvailable(layer)==3);
        for(unsigned i=1;i<=prefix;++i){
            terrain.states[i]=terrain.states[0].shifted(i);
            REQUIRE(reference.attach(layer,4+i,true,false,[](unsigned){return false;}));
        }
        REQUIRE(logical.retainInheritedCount(layer,prefix));compare(logical,reference);
        const unsigned remaining=3-prefix;
        REQUIRE(logical.retainedAvailable(layer)==remaining);
        REQUIRE(!logical.retainInheritedCount(layer,remaining+1));compare(logical,reference);
        if(remaining){
            REQUIRE(logical.attach(layer,7,true,false,[](unsigned){return false;}));
            REQUIRE(reference.attach(layer,7,true,false,[](unsigned){return false;}));
            compare(logical,reference);
        }
    }
}
static void merges(){
    for(unsigned split=0;split<=8;++split)for(unsigned mask=0;mask<256;++mask){
        Terrain terrain;Logical logical;Reference reference;REQUIRE(logical.bindTerrain(terrain));
        unsigned layers[2];
        for(unsigned i=0;i<2;++i){unsigned r;REQUIRE(logical.insertAfterEquals(i+1,int(i)*10,true,layers[i]));
            REQUIRE(reference.insertAfterEquals(i+1,int(i)*10,true,r)&&r==layers[i]);}
        for(unsigned e=0;e<8;++e){const unsigned layer=layers[e<split?0:1];
            REQUIRE(reference.attach(layer,e,true,false,[](unsigned){return false;}));
            if(e<4)REQUIRE(logical.attach(layer,e,true,false,[](unsigned){return false;}));
            else if(e&1)REQUIRE(logical.retainAllocated(e-4,layer,(mask&(1u<<e))!=0));
            else REQUIRE(logical.retainInstance({e-4,1},layer,(mask&(1u<<e))!=0));
        }
        compare(logical,reference);
        std::vector<unsigned> physicalA,physicalB;
        REQUIRE(reference.move(layers[0],10,true,[&](unsigned e,int id){if(e<4)physicalA.push_back(e);REQUIRE(id==2);return true;},
            [&](unsigned e){return (mask&(1u<<e))!=0;}));
        REQUIRE(logical.move(layers[0],10,true,[&](unsigned e,int id){physicalB.push_back(e);REQUIRE(id==2);return true;},
            [&](unsigned e){return (mask&(1u<<e))!=0;}));
        REQUIRE(physicalA==physicalB);compare(logical,reference);
        for(unsigned s=0;s<4;++s)REQUIRE(terrain.states[s].publishedId==2&&terrain.states[s].layer==layers[1]);
        for(unsigned e=0;e<8;++e){
            REQUIRE(reference.detach(e));
            REQUIRE(e<4?logical.detach(e):logical.releaseInstance({e-4,1}));
            compare(logical,reference);
        }
    }
}
static void mutations(){
    Terrain terrain;Logical logical;Reference reference;REQUIRE(logical.bindTerrain(terrain));
    unsigned layer,r;REQUIRE(logical.insertAfterEquals(1,0,false,layer)&&reference.insertAfterEquals(1,0,false,r));
    bool present[8]{},inactive[8]{};unsigned random=782191;
    for(unsigned step=0;step<10000;++step){
        random=random*1664525u+1013904223u;const unsigned e=(random>>9)&7,op=(random>>20)%5;
        if(op==0&&!present[e]){
            REQUIRE(reference.attach(layer,e,true,false,[](unsigned){return false;}));
            REQUIRE(e<4?logical.attach(layer,e,true,false,[](unsigned){return false;}):logical.retainInstance({e-4,1},layer,false));
            present[e]=true;inactive[e]=false;
        }else if(op==1&&present[e]){
            REQUIRE(reference.detach(e));REQUIRE(e<4?logical.detach(e):logical.releaseInstance({e-4,1}));present[e]=false;
        }else if(op==2&&present[e]){
            inactive[e]=!inactive[e];if(e>=4)REQUIRE(logical.publishInactive({e-4,1},inactive[e]));
        }else if(op==3&&present[e]){
            REQUIRE(reference.reconcileInstanceActivation(e,inactive[e]));
            REQUIRE(e<4?logical.reconcileInstanceActivation(e,inactive[e]):logical.reconcileRetained({e-4,1}));
        }else if(op==4){
            std::vector<unsigned> a,b;
            REQUIRE(reference.normalizeRoomInstances([&](unsigned n){if(n<4)a.push_back(n);return inactive[n];}));
            REQUIRE(logical.normalizeRoomInstances([&](unsigned n){b.push_back(n);return inactive[n];}));
            REQUIRE(a==b);
        }
        compare(logical,reference);
        std::vector<unsigned> a,b;
        reference.visitElements(layer,[&](unsigned n){if(inactive[n])return false;if(n<4)a.push_back(n);return true;});
        RunnerLayerOrderKey after;bool haveAfter=false;
        logical.visitElements(layer,[&](unsigned n){const auto key=logical.physicalOrder(n);
            if(logical.cutsOff(layer,haveAfter?&after:nullptr,key)||inactive[n])return false;
            b.push_back(n);after=key;haveAfter=true;return true;
        });REQUIRE(a==b);
    }
}
static std::vector<unsigned> drawMutation(bool useLogical,bool birth,bool prepared=false){
    Terrain terrain;Logical logical;Reference reference;REQUIRE(logical.bindTerrain(terrain));
    RunnerPreparedDrawList<3,8> projection;
    unsigned layer,r;REQUIRE(logical.insertAfterEquals(1,0,false,layer)&&reference.insertAfterEquals(1,0,false,r));
    // [actor 3, retained 4, actor 0], with room for a new retained head.
    for(unsigned e:{0u,4u,3u}){
        REQUIRE(reference.attach(layer,e,true,false,[](unsigned){return false;}));
        REQUIRE(e<4?logical.attach(layer,e,true,false,[](unsigned){return false;}):logical.retainInstance({e-4,1},layer,false));
        if(e<4){REQUIRE(projection.attach(layer,e,projection.End));
            REQUIRE(projection.publish(e,e,decltype(projection)::draw));}
    }
    bool inactive[8]{};std::vector<unsigned> out;
    auto draw=[&](unsigned e){out.push_back(e);if(e!=3)return;
        if(birth){REQUIRE(reference.attach(layer,5,true,false,[](unsigned){return false;}));
            REQUIRE(logical.retainInstance({1,1},layer,true));inactive[5]=true;
        }else{inactive[4]=true;REQUIRE(logical.publishInactive({0,1},true));}
    };
    if(useLogical){
        RunnerLayerOrderKey after;bool haveAfter=false;
        auto before=[&](unsigned e){const auto key=logical.physicalOrder(e);
            if(logical.cutsOff(layer,haveAfter?&after:nullptr,key))return false;
            after=key;haveAfter=true;return true;
        };
        if(prepared)REQUIRE(projection.visitOrdered(layer,before,[&](unsigned e){draw(e);return true;}));
        else logical.visitDrawElements(layer,[&](unsigned e){if(!before(e))return false;draw(e);return true;});
    }else reference.visitDrawElements(layer,[&](unsigned e){if(inactive[e])return false;if(e<4)draw(e);return true;});
    return out;
}
static void preparedCutoffs(){
    for(unsigned mask=0;mask<256;++mask){
        Terrain terrain;Logical logical;Reference reference;REQUIRE(logical.bindTerrain(terrain));
        RunnerPreparedDrawList<3,8> projection;
        unsigned layer,r;REQUIRE(logical.insertAfterEquals(1,0,false,layer)&&reference.insertAfterEquals(1,0,false,r));
        for(unsigned e:{0u,4u,1u,5u,2u,6u,3u,7u}){
            REQUIRE(reference.attach(layer,e,true,false,[](unsigned){return false;}));
            if(e<4){
                REQUIRE(logical.attach(layer,e,true,false,[](unsigned){return false;}));
                REQUIRE(projection.attach(layer,e,projection.End));
                REQUIRE(projection.publish(e,e,(mask&(1u<<e))?decltype(projection)::endLayer:decltype(projection)::draw));
            }else REQUIRE(logical.retainInstance({e-4,1},layer,(mask&(1u<<e))!=0));
        }
        std::vector<unsigned> expected,actual;
        reference.visitElements(layer,[&](unsigned e){if(mask&(1u<<e))return false;if(e<4)expected.push_back(e);return true;});
        RunnerLayerOrderKey after;bool haveAfter=false;
        const auto before=[&](unsigned e){const auto key=logical.physicalOrder(e);
            if(logical.cutsOff(layer,haveAfter?&after:nullptr,key))return false;
            after=key;haveAfter=true;return true;};
        REQUIRE(projection.visitOrdered(layer,before,[&](unsigned e){actual.push_back(e);return true;}));
        REQUIRE(actual==expected);
        if(!expected.empty()){
            after={};haveAfter=false;unsigned calls=0;
            REQUIRE(!projection.visitOrdered(layer,before,[&](unsigned){++calls;return false;}));
            REQUIRE(calls==1);
        }
    }
}
static void failedPublication(){
    for(unsigned failure=0;failure<4;++failure){
        Terrain terrain;Logical logical;Reference reference;REQUIRE(logical.bindTerrain(terrain));
        unsigned source,destination,r;
        REQUIRE(logical.insertAfterEquals(1,0,true,source)&&reference.insertAfterEquals(1,0,true,r)&&r==source);
        REQUIRE(logical.insertAfterEquals(2,10,true,destination)&&reference.insertAfterEquals(2,10,true,r)&&r==destination);
        int published[8]={1,1,1,1,1,1,1,1},physical[4]={1,1,1,1};
        for(unsigned e:{0u,4u,1u,5u,2u,6u,3u,7u}){
            REQUIRE(reference.attach(source,e,true,false,[](unsigned){return false;}));
            REQUIRE(e<4?logical.attach(source,e,true,false,[](unsigned){return false;}):logical.retainInstance({e-4,1},source,false));
        }
        REQUIRE(!reference.move(source,10,true,[&](unsigned e,int id){
            if(e==failure)return false;published[e]=id;return true;
        },[](unsigned){return false;}));
        REQUIRE(!logical.move(source,10,true,[&](unsigned e,int id){
            if(e==failure)return false;physical[e]=id;return true;
        },[](unsigned){return false;}));
        REQUIRE(!logical.succeeded()&&!reference.succeeded());
        compare(logical,reference);
        REQUIRE(logical.get(source)->depth==10&&logical.elementCount(destination)==0);
        for(unsigned e=0;e<4;++e){
            REQUIRE(physical[e]==published[e]);
            REQUIRE(terrain.states[e].publishedId==published[e+4]);
            REQUIRE(terrain.states[e].layer==source);
        }
    }
}
static void nativePublication(){
    struct Fixture {
        NativeTerrainLifetimes<8> terrain;
        RunnerLogicalLayerOrder<3,16,NativeTerrainLifetimes<8>> layers;
        unsigned layer=0,stamp=0;
    } fixture;
    REQUIRE(fixture.layers.bindTerrain(fixture.terrain));
    REQUIRE(fixture.layers.insertAfterEquals(1,100,false,fixture.layer));
    REQUIRE(fixture.terrain.bind({&fixture,
        [](void*,unsigned){return true;},
        [](void* p,const NativeTerrainLifetimes<8>::Identity& id,bool active,std::uint32_t& stamp,std::uint32_t&){
            auto& f=*static_cast<Fixture*>(p);stamp=++f.stamp;
            REQUIRE(f.terrain.layerState(id.handle)&&!f.terrain.layerState(id.handle)->bound());
            return f.layers.retainInstance(id.handle,f.layer,!active);
        },
        [](void* p,unsigned slot){auto& f=*static_cast<Fixture*>(p);
            return f.layers.releaseInstance(f.terrain.currentHandle(slot));},
        [](void*,unsigned){return true;}
    }));
    for(unsigned i=0;i<8;++i)REQUIRE(fixture.terrain.append(60,int(i)*16,0));
    for(unsigned i=0;i<8;++i){
        const auto h=fixture.terrain.handle(i);const auto before=*fixture.terrain.layerState(h);
        REQUIRE(before.bound()&&before.order.serial.low==i+1);
        if(i==2)REQUIRE(fixture.terrain.rebindOrder(h,100+i));
        REQUIRE(fixture.terrain.layerState(h)->same(before));
    }
    REQUIRE(fixture.layers.publishInactive(fixture.terrain.handle(3),true));
    REQUIRE(fixture.layers.reconcileRetained(fixture.terrain.handle(3)));
    unsigned count=0;
    REQUIRE(fixture.terrain.visitLayerStates([&](InstanceHandle,const RunnerRetainedLayerState& s){
        REQUIRE(s.bound());++count;return true;
    }));REQUIRE(count==8);
    // Releasing layer publications splits runs during whole-room teardown.
    REQUIRE(fixture.terrain.clear()&&fixture.layers.elementCount(fixture.layer)==0);
    REQUIRE(fixture.terrain.append(60,0,0));
    REQUIRE(fixture.terrain.layerState(fixture.terrain.handle(0))->bound());
    REQUIRE(fixture.terrain.clear());
}
static void nativeRunPrefixes(){
    for(unsigned prefix=0;prefix<=8;++prefix){
        struct Fixture {
            NativeTerrainLifetimes<8> terrain;
            RunnerLogicalLayerOrder<3,16,NativeTerrainLifetimes<8>> layers;
            unsigned layer=0,pending=0,births=0,ends=0;
        } f;
        REQUIRE(f.layers.bindTerrain(f.terrain)&&f.layers.insertAfterEquals(1,100,false,f.layer));
        NativeTerrainLifetimes<8>::Binding binding;
        binding.context=&f;binding.admit=[](void*,unsigned){return true;};
        binding.birth=[](void* p,const NativeTerrainLifetimes<8>::Identity& id,bool,std::uint32_t& stamp,std::uint32_t&){
            auto& f=*static_cast<Fixture*>(p);stamp=++f.births;
            if(f.births==1)return f.layers.retainAllocated(id.handle.slot,f.layer,false);
            f.terrain.publishInheritedLayer(id.handle.slot);++f.pending;return true;
        };
        binding.mark=[](void*,unsigned){return true;};
        binding.retire=[](void* p,unsigned slot){auto& f=*static_cast<Fixture*>(p);
            return f.layers.releaseInstance(f.terrain.currentHandle(slot));};
        binding.beginRun=[](void* p){auto& f=*static_cast<Fixture*>(p);return !f.births&&!f.pending;};
        binding.endRun=[](void* p){auto& f=*static_cast<Fixture*>(p);++f.ends;
            return !f.pending||f.layers.retainInheritedCount(f.layer,f.pending);};
        REQUIRE(f.terrain.bind(binding));
        const bool complete=f.terrain.publishRun([&]{
            for(unsigned i=0;i<prefix;++i){
                REQUIRE(f.terrain.append(60,int(i)*16,0));
                REQUIRE(f.terrain.layerState(f.terrain.handle(i))->order.serial.low==i+1);
            }
            return prefix==8;
        });
        REQUIRE(complete==(prefix==8)&&f.ends==1&&f.terrain.size()==prefix);
        REQUIRE(f.layers.elementCount(f.layer)==prefix);
        unsigned observed=0;
        REQUIRE(f.layers.visitLogical(f.layer,[&](bool retained,unsigned slot){
            REQUIRE(retained&&slot==prefix-1-observed);++observed;return true;
        }));REQUIRE(observed==prefix);
    }
}
static void sparsePreparedDraw(){
    using List=RunnerPreparedDrawList<3,512>;
    List list;
    unsigned layer[512];
    for(unsigned i=0;i<512;++i){layer[i]=i%3;REQUIRE(list.attach(layer[i],i,List::End));}
    unsigned random=12345;
    const auto compare=[&](){
        for(unsigned l=0;l<3;++l){
            std::vector<unsigned> a,b,aa,bb;
            REQUIRE(list.visitOrdered(l,[&](unsigned e){aa.push_back(e);return true;},
                [&](unsigned t){a.push_back(t);return true;}));
            REQUIRE(list.visitOrderedReference(l,[&](unsigned e){bb.push_back(e);return true;},
                [&](unsigned t){b.push_back(t);return true;}));
            REQUIRE(a==b&&aa==bb);
        }
    };
    for(unsigned n=0;n<10000;++n){
        random=random*1664525u+1013904223u;
        const unsigned e=(random>>8)%512,op=random%5;
        if(op<3)REQUIRE(list.publish(e,random%65536,op==0?List::skip:op==1?List::draw:List::endLayer));
        else if(op==3){
            REQUIRE(list.detach(layer[e],e));layer[e]=(layer[e]+1)%3;
            REQUIRE(list.attach(layer[e],e,List::End));
        }else REQUIRE(list.reposition(layer[e],e,List::End));
        compare();
        if(n%101==0){
            list.beginRelink();
            for(unsigned i=0;i<512;++i)REQUIRE(list.prependExisting(layer[i],i));
            compare();
        }
    }
    // Mutations from an executing Draw must see a newly activated successor,
    // but not a newly inserted head. Include removing/hiding the current slot.
    for(unsigned mode=0;mode<4;++mode){
        const auto run=[&](bool reference){
            List live;std::vector<unsigned> out;
            for(unsigned i=0;i<4;++i)REQUIRE(live.attach(0,i,List::End));
            REQUIRE(live.publish(3,3,List::draw)&&live.publish(0,0,List::draw));
            const auto draw=[&](unsigned token){
                out.push_back(token);
                if(token==3){
                    REQUIRE(live.publish(2,2,List::draw));
                    if(mode==0)REQUIRE(live.publish(3,3,List::skip));
                    if(mode==1)REQUIRE(live.detach(0,3));
                    if(mode==2)REQUIRE(live.publish(2,2,List::endLayer));
                    REQUIRE(live.attach(0,4,List::End)&&live.publish(4,4,List::endLayer));
                }
                return true;
            };
            const auto before=[](unsigned){return true;};
            REQUIRE(reference?live.visitOrderedReference(0,before,draw):live.visitOrdered(0,before,draw));
            return out;
        };
        REQUIRE(run(false)==run(true));
    }
}
int main(){inheritedCapacity();merges();mutations();failedPublication();preparedCutoffs();nativePublication();nativeRunPrefixes();sparsePreparedDraw();
    REQUIRE(drawMutation(true,true)==drawMutation(false,true));
    REQUIRE(drawMutation(true,false)==drawMutation(false,false));
    REQUIRE(drawMutation(true,true)==std::vector<unsigned>({3,0}));
    REQUIRE(drawMutation(true,false)==std::vector<unsigned>({3}));
    REQUIRE(drawMutation(true,true,true)==drawMutation(false,true));
    REQUIRE(drawMutation(true,false,true)==drawMutation(false,false));
    std::puts("Logical retained layer order: 2304 merges, 10000 mutations, 256 prepared cutoffs, 10000 sparse-chain mutations, failed publication prefixes and live Draw cutoffs passed");
}

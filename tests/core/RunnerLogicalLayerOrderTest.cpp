#include "core/RunnerLogicalLayerOrder.h"
#include "core/RunnerLayerDraw.h"
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
static std::vector<unsigned> drawMutation(bool useLogical,bool birth){
    Terrain terrain;Logical logical;Reference reference;REQUIRE(logical.bindTerrain(terrain));
    unsigned layer,r;REQUIRE(logical.insertAfterEquals(1,0,false,layer)&&reference.insertAfterEquals(1,0,false,r));
    // [actor 3, retained 4, actor 0], with room for a new retained head.
    for(unsigned e:{0u,4u,3u}){
        REQUIRE(reference.attach(layer,e,true,false,[](unsigned){return false;}));
        REQUIRE(e<4?logical.attach(layer,e,true,false,[](unsigned){return false;}):logical.retainInstance({e-4,1},layer,false));
    }
    bool inactive[8]{};std::vector<unsigned> out;
    auto draw=[&](unsigned e){out.push_back(e);if(e!=3)return;
        if(birth){REQUIRE(reference.attach(layer,5,true,false,[](unsigned){return false;}));
            REQUIRE(logical.retainInstance({1,1},layer,true));inactive[5]=true;
        }else{inactive[4]=true;REQUIRE(logical.publishInactive({0,1},true));}
    };
    if(useLogical){
        RunnerLayerOrderKey after;bool haveAfter=false;
        logical.visitDrawElements(layer,[&](unsigned e){const auto key=logical.physicalOrder(e);
            if(logical.cutsOff(layer,haveAfter?&after:nullptr,key))return false;
            draw(e);after=key;haveAfter=true;return true;
        });
    }else reference.visitDrawElements(layer,[&](unsigned e){if(inactive[e])return false;if(e<4)draw(e);return true;});
    return out;
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
int main(){inheritedCapacity();merges();mutations();failedPublication();
    REQUIRE(drawMutation(true,true)==drawMutation(false,true));
    REQUIRE(drawMutation(true,false)==drawMutation(false,false));
    REQUIRE(drawMutation(true,true)==std::vector<unsigned>({3,0}));
    REQUIRE(drawMutation(true,false)==std::vector<unsigned>({3}));
    std::puts("Logical retained layer order: 2304 merges, 10000 mutations, failed publication prefixes and live Draw cutoffs passed");
}

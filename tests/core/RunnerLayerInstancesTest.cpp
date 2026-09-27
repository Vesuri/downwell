#include "core/RunnerLayerInstances.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <vector>
namespace {
using namespace downwell;
void require(bool v,const char* m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
using Registry=RunnerLayerInstances<2,5>;using Layers=Registry::Layers;
std::vector<unsigned> ids(const Layers& layers,const Registry& registry,unsigned layer){
    std::vector<unsigned> result;
    require(layers.visitElements(layer,[&](unsigned e){
        const auto* r=registry.get(e);require(r!=nullptr,"registered instance element");result.push_back(r->sourceId);return result.size()<=5;
    }),"bounded elements");return result;
}
void permutations(){
    unsigned order[]={0,1,2,3};unsigned count=0;
    do {
        Registry registry;Layers layers;unsigned layer=2;
        require(layers.insertAfterEquals(17,-50000,false,layer),"fixed layer");
        for(unsigned i=0;i<4;++i)require(registry.reserve(layers,layer,100+i),"serialized placeholder");
        for(unsigned i:order){
            RunnerDepthService::Instance instance;
            // Deliberately different source IDs, element slots and store slots.
            const InstanceHandle h{20+i,3};
            require(registry.attach(layers,layer,100+i,h,instance,false),"fill existing element");
            require(instance.attached&&instance.layerId==17,"published layer identity");
            require(instance.depth.bits==0xc7435000u,"published layer depth");
            require(registry.find(h)==registry.find(100+i),"lifetime lookup");
        }
        require(ids(layers,registry,layer)==std::vector<unsigned>({103,102,101,100}),"creation permutation cannot change serialized element order");
        ++count;
    }while(std::next_permutation(order,order+4));
    require(count==24,"all creation orders");
}
void lifecycle(){
    Registry registry;Layers layers;unsigned layer=2;
    require(layers.insertAfterEquals(17,0,false,layer),"lifecycle layer");
    for(unsigned i=0;i<3;++i)require(registry.reserve(layers,layer,100+i),"lifecycle reserve");
    RunnerDepthService::Instance inactive;inactive.append=false;
    require(registry.attach(layers,layer,101,{50,1},inactive,true),"inactive placeholder moves to tail");
    require(ids(layers,registry,layer)==std::vector<unsigned>({102,100,101}),"inactive reuse order");
    RunnerDepthService::Instance live;
    require(registry.attach(layers,layer,100,{51,1},live,false),"active placeholder stays in place");
    require(ids(layers,registry,layer)==std::vector<unsigned>({102,100,101}),"active reuse order");
    RunnerDepthService::Instance duplicate;
    require(!registry.attach(layers,layer,100,{52,1},duplicate,false)&&!duplicate.attached,"duplicate source rejected");
    require(!registry.attach(layers,layer,200,{51,1},duplicate,false),"duplicate lifetime rejected");
    require(!registry.retire(layers,101,{50,2})&&registry.retire(layers,101,{50,1}),"generation checked retirement");
    RunnerDepthService::Instance replacement;
    require(registry.attach(layers,layer,101,{50,2},replacement,false),"new lifetime creates head element");
    require(registry.find(InstanceHandle{50,1})==Registry::End,"old lifetime cannot resolve replacement");
    require(ids(layers,registry,layer)==std::vector<unsigned>({101,102,100}),"new element does not inherit retired position");
    RunnerDepthService::Instance dormant;dormant.append=true;
    require(registry.attach(layers,layer,300,{99,1},dormant,false),"new dormant registration appends independently of inactive flag");
    require(ids(layers,registry,layer)==std::vector<unsigned>({101,102,100,300}),"append uses registration flag");
}
void bounds(){
    RunnerLayerInstances<1,1> registry;RunnerLayers<1,1> layers;unsigned layer=1;
    require(layers.insertAfterEquals(1,0,false,layer)&&registry.reserve(layers,layer,10),"capacity seed");
    require(!registry.reserve(layers,layer,10)&&!registry.reserve(layers,layer,11),"duplicate/capacity reserve");
    RunnerDepthService::Instance state;
    require(!registry.attach(layers,layer,11,{1,1},state,false)&&!state.attached,"capacity leaves instance unchanged");
    require(!registry.attach(layers,layer,10,{},state,false),"invalid handle rejected");
}
void roomFallback(){
    using RoomRegistry=RunnerLayerInstances<4,8>;
    RoomRegistry registry;RoomRegistry::Layers layers;
    unsigned fixed=4,dynamic=4;unsigned creations=0;
    require(layers.insertAfterEquals(17,-50,false,fixed)&&
        layers.insertAfterEquals(18,7,true,dynamic),"room fallback layers");
    auto create=[&](int depth,unsigned& slot){
        ++creations;return layers.insertAfterEquals(100+creations,depth,true,slot);
    };
    RunnerDepthService::Instance saved;saved.layerId=17;saved.depth.bits=0x40f00000u; // 7.5
    require(registry.attachRoomFallback(layers,100,{0,1},saved,false,create),"saved fixed ID takes priority");
    require(saved.layerId==17&&saved.depth.bits==0xc2480000u&&creations==0,"saved layer depth published");
    RunnerDepthService::Instance missing;missing.layerId=999;missing.depth.bits=0x40f00000u;
    require(registry.attachRoomFallback(layers,101,{1,1},missing,false,create),"missing saved ID uses truncated depth");
    require(missing.layerId==18&&missing.depth.bits==0x40e00000u&&creations==0,"reuse matching dynamic layer");
    RunnerDepthService::Instance fresh;fresh.depth.bits=0xc2480000u; // fixed layer exists at -50
    require(registry.attachRoomFallback(layers,102,{2,1},fresh,false,create),"fixed depth match cannot be reused");
    require(fresh.layerId==101&&creations==1,"new dynamic layer created at fixed peer depth");
    RunnerDepthService::Instance dormant;dormant.depth=fresh.depth;dormant.append=true;
    require(registry.attachRoomFallback(layers,103,{3,1},dormant,true,create)&&creations==1,"reuse created dynamic layer");
    std::vector<unsigned> order;
    require(layers.visitElements(layers.find(101),[&](unsigned e){order.push_back(registry.get(e)->sourceId);return true;}),"fallback order");
    require(order==std::vector<unsigned>({102,103}),"fallback preserves append registration flag");
    // Already attached is a true no-op, not a second birth or reordering.
    require(registry.attachRoomFallback(layers,103,{3,1},dormant,true,
        [](int,unsigned&){return false;})&&creations==1,"attached instance skipped");
    RunnerDepthService::Instance failure;failure.depth.bits=0x42c80000u; // 100
    require(!registry.attachRoomFallback(layers,104,{4,1},failure,false,
        [](int,unsigned&){return false;})&&!failure.attached&&registry.find(104)==RoomRegistry::End,
        "failed layer allocation does not attach instance");
    require(!registry.attachRoomFallback(layers,104,{4,1},failure,false,
        [&](int,unsigned& slot){slot=fixed;return true;})&&!failure.attached,
        "allocator cannot substitute a fixed layer");
    RunnerDepthService::Instance invalid;invalid.depth.bits=0x7fc00000u;
    require(!registry.attachRoomFallback(layers,105,{5,1},invalid,false,create),"invalid fallback conversion fails");
    invalid.layerId=17;
    require(registry.attachRoomFallback(layers,105,{5,1},invalid,false,create)&&invalid.depth.bits==0xc2480000u,
        "saved ID path never converts old depth");
}
void serializedInitialization(){
    Registry registry;Layers layers;unsigned layer=2;
    require(layers.insertAfterEquals(17,-50,false,layer),"initialization layer");
    for(unsigned id:{100u,101u,102u})require(registry.reserve(layers,layer,id),"initialization placeholders");
    RunnerDepthService::Instance state;state.depth.bits=0x40f00000u;
    state.layerId=900;state.attached=true;state.append=true;
    unsigned lookups=0,releases=0;
    auto lookup=[&](unsigned id,InstanceHandle& h,RunnerDepthService::Instance*& out){
        ++lookups;require(id==101,"lookup uses source ID");h={50,3};out=&state;return true;
    };
    auto release=[&](InstanceHandle h,const auto& old){
        ++releases;require(h.slot==50&&h.generation==3&&old.layerId==900&&old.attached,
            "release sees prior attachment before publication");return true;
    };
    const unsigned element=registry.find(101);
    require(registry.initializeReserved(layers,element,lookup,release),"initialize existing global instance");
    require(lookups==1&&releases==1&&state.attached&&state.layerId==17&&state.depth.bits==0x40f00000u&&state.append,
        "initialization preserves fractional depth and registration state");
    require(ids(layers,registry,layer)==std::vector<unsigned>({102,101,100}),"initialization preserves placeholder position");
    require(registry.initializeReserved(layers,element,lookup,release)&&lookups==1&&releases==1,
        "initialized element never rebinds");
    require(registry.initializeReserved(layers,registry.find(100),
        [](unsigned,InstanceHandle&,RunnerDepthService::Instance*&){return true;},release)&&releases==1,
        "missing global instance is a successful no-op");
    require(!registry.get(registry.find(100))->instance.generation,"missing instance leaves placeholder unbound");
    RunnerDepthService::Instance fresh;fresh.depth.bits=0xc0200000u; // -2.5
    auto freshLookup=[&](unsigned,InstanceHandle& h,RunnerDepthService::Instance*& out){h={51,1};out=&fresh;return true;};
    require(registry.initializeReserved(layers,registry.find(100),freshLookup,release)&&releases==1&&fresh.depth.bits==0xc0200000u,
        "unattached initialization skips release and preserves depth");
    RunnerDepthService::Instance failed;failed.attached=true;failed.layerId=123;failed.depth.bits=0x3f800000u;
    require(!registry.initializeReserved(layers,registry.find(102),
        [&](unsigned,InstanceHandle& h,RunnerDepthService::Instance*& out){h={52,1};out=&failed;return true;},
        [](InstanceHandle,const auto&){return false;}),"release failure stops publication");
    require(failed.layerId==123&&failed.depth.bits==0x3f800000u&&!registry.get(registry.find(102))->instance.generation,
        "release failure leaves target unbound");
    require(!registry.initializeReserved(layers,registry.find(102),
        [](unsigned,InstanceHandle&,RunnerDepthService::Instance*&){return false;},release),"lookup failure propagated");
    require(!registry.initializeReserved(layers,registry.find(102),
        [&](unsigned,InstanceHandle&,RunnerDepthService::Instance*& out){out=&failed;return true;},release),"invalid lifetime rejected");
    require(registry.reserve(layers,layer,0xffffffffu)&&registry.initializeReserved(layers,registry.find(0xffffffffu),
        [](unsigned,InstanceHandle&,RunnerDepthService::Instance*&){return false;},release),"negative source ID is skipped before lookup");
}
void rebindRelease(){
    for(bool dynamic:{false,true}){
        Registry registry;Layers layers;unsigned layer=2,other=2;
        require(layers.insertAfterEquals(17,10,dynamic,layer)&&layers.insertAfterEquals(18,20,false,other),"rebind layers");
        RunnerDepthService::Instance state;
        require(registry.attach(layers,layer,100,{50,1},state,false),"rebind original binding");
        state.depth.bits=0x41280000u; // 10.5, not layer's integer depth
        auto unchanged=[&](){return state.attached&&state.layerId==17&&state.depth.bits==0x41280000u;};
        require(registry.releaseForRebind(layers,{50,2},state)&&registry.find(100)!=Registry::End,"stale generation cannot release");
        auto missing=state;missing.layerId=999;
        require(registry.releaseForRebind(layers,{50,1},missing)&&registry.find(100)!=Registry::End,"missing layer no-op");
        missing.layerId=18;
        require(registry.releaseForRebind(layers,{50,1},missing)&&registry.find(100)!=Registry::End,"wrong layer no-op");
        missing.layerId=-1;
        require(registry.releaseForRebind(layers,{50,1},missing)&&registry.find(100)!=Registry::End,"unset saved layer no-op");
        missing=state;missing.attached=false;
        require(registry.releaseForRebind(layers,{50,1},missing)&&registry.find(100)!=Registry::End,"unattached flag no-op");
        require(registry.releaseForRebind(layers,{50,1},state)&&unchanged(),"release preserves instance fields");
        require(layers.get(layer)&&layers.elementCount(layer)==0&&registry.find(100)==Registry::End,
            "release retains empty dynamic and fixed layers but frees element");
        require(registry.releaseForRebind(layers,{50,1},state)&&unchanged(),"repeated missing-element release no-op");
        require(registry.reserve(layers,other,100),"same source ID can acquire a new placeholder after release");
        require(registry.initializeReserved(layers,registry.find(100),
            [&](unsigned id,InstanceHandle& h,RunnerDepthService::Instance*& out){
                require(id==100,"rebind source ID preserved");h={50,1};out=&state;return true;
            }),"shared release adapter binds destination without a second instance birth");
        require(state.attached&&state.layerId==18&&state.depth.bits==0x41280000u&&layers.get(layer),
            "rebinding publishes new layer only, not its depth");
        require(registry.find(InstanceHandle{50,1})==registry.find(100),"instance lifetime survives rebind");
    }
}
}
int main(){permutations();lifecycle();bounds();roomFallback();serializedInitialization();rebindRelease();std::cout<<"Serialized layer initialization, rebind release, room fallback and lifetime-checked attachment verified\n";}

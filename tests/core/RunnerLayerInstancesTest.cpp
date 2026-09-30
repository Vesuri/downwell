#include "core/RunnerLayerInstances.h"
#include "core/RunnerLayerAllocationVerification.h"
#include "core/InstanceStore.h"
#include "core/RunnerLayerIds.h"
#include "core/RunnerLayerLifetimes.h"
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
    require(RunnerLayerAllocationVerification::run(8192),"mixed owner allocation equals full scan");
    // Compare the bounded lookups with a full-capacity oracle through sparse
    // mixed membership, retirement, slot reuse and complete room reset.
    {
        using R=RunnerLayerInstances<1,96>;
        R r;R::Layers l;unsigned layer=1;
        auto check=[&](){
            for(unsigned id=0;id<130;++id){
                unsigned expected=R::End;
                for(unsigned e=0;e<R::End;++e){const auto* p=r.get(e);
                    if(id&&p&&p->sourceId==id){expected=e;break;}}
                require(r.find(id)==expected,"bounded source lookup equals full scan");
            }
            for(unsigned slot=0;slot<8;++slot)for(unsigned generation=0;generation<20;++generation){
                const InstanceHandle h{slot,generation};unsigned expected=R::End;
                for(unsigned e=0;e<R::End;++e){const auto* p=r.get(e);
                    if(generation&&p&&p->instance.slot==slot&&p->instance.generation==generation){expected=e;break;}}
                require(r.find(h)==expected,"bounded lifetime lookup equals full scan");
            }
        };
        for(unsigned room=0;room<2;++room){
            require(l.insertAfterEquals(1,0,false,layer),"sparse layer");
            for(unsigned e=0;e<80;++e)
                require(l.attach(layer,e,false,false,[](unsigned){return false;}),"non-instance prefix");
            require(r.reserve(l,layer,100)&&r.find(100u)==80,"reserve after mixed prefix");
            for(unsigned generation=1;generation<20;++generation){
                RunnerDepthService::Instance state;
                const unsigned id=generation==1?100:0;
                const InstanceHandle h{generation%8,generation};
                require(r.attach(l,layer,id,h,state,false),"sparse attach");check();
                require(!r.discardUninitialized(),"live binding prevents reset");check();
                require(r.retire(l,id,h),"sparse retire");check();
                if(generation==1){
                    for(unsigned e=0;e<80;++e)require(l.detach(e),"release mixed prefix");
                }
            }
            require(r.discardUninitialized()&&l.discardUninitialized(),"reset sparse room");check();
        }
    }
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
void identityCollisions(){
    using R=RunnerLayerInstances<1,96>;
    R registry;R::Layers layers;unsigned layer;
    require(layers.insertAfterEquals(1,0,false,layer),"identity collision layer");
    InstanceHandle handles[64];
    for(unsigned i=0;i<64;++i){
        // All collide in the compact index; slots exceed element capacity.
        handles[i]={7+256*i,9};RunnerDepthService::Instance state;
        require(registry.attach(layers,layer,0,handles[i],state,false),"colliding identity attach");
    }
    auto compare=[&](){
        for(auto h:handles){
            unsigned expected=R::End;
            for(unsigned e=0;e<R::End;++e){const auto* entry=registry.get(e);
                if(entry&&entry->instance.slot==h.slot&&entry->instance.generation==h.generation){expected=e;break;}}
            require(registry.find(h)==expected,"colliding lookup matches complete scan");
            require(registry.find(InstanceHandle{h.slot,h.generation+1})==R::End,"collision still checks generation");
        }
    };
    compare();
    for(unsigned i=0;i<64;++i){
        const unsigned index=(i*17)%64;
        require(registry.retire(layers,0,handles[index]),"unlink hash head/middle/tail");compare();
    }
    for(unsigned generation=1;generation<=32;++generation){
        RunnerDepthService::Instance state;
        require(registry.attach(layers,layer,0,{42,generation},state,false),"same slot distinct lifetime keys");
    }
    for(unsigned generation=1;generation<=32;++generation)
        require(registry.find(InstanceHandle{42,generation})!=R::End&&registry.retire(layers,0,{42,generation}),"full-key lifetime lookup");
    require(registry.discardUninitialized(),"identity index room reset");compare();
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
void globalFallbackOrder(){
    InstanceStore<4> store;RunnerStepSnapshot<4> snapshot;
    using R=RunnerLayerInstances<2,4>;
    R registry;R::Layers layers;RunnerLayerIds sourceIds;
    require(sourceIds.observeLoaded(131),"catalog layer ID floor");
    RunnerDepthService::Instance states[4];
    for(unsigned id:{513u,512u,1025u,1024u}){
        require(store.create(id,10,7,true)==CreateInstanceResult::success,"persistent fallback instance");
    }
    // Storage order starts in bucket 1, but bucket 0 must create its layer first.
    states[0].depth.bits=states[2].depth.bits=0x41200000u; // 10
    states[1].depth.bits=states[3].depth.bits=0x41a00000u; // 20
    store.enterRoom(8);
    require(store.captureGlobalInstanceOrder(snapshot),"capture persistent global order");
    for(unsigned i=0;i<snapshot.size();++i){
        const auto h=snapshot.handle(i);const auto* record=store.resolve(h);
        require(record&&registry.attachRoomFallback(layers,record->instanceId,h,states[h.slot],false,
            [&](int depth,unsigned& slot){return sourceIds.createDynamic(layers,depth,slot);}),
            "global iteration drives fallback attachment");
    }
    require(states[1].layerId==132&&states[3].layerId==132&&
        states[0].layerId==133&&states[2].layerId==133,"hash traversal determines dynamic source IDs");
    std::vector<unsigned> order;
    require(layers.visitElements(layers.find(132),[&](unsigned e){order.push_back(registry.get(e)->sourceId);return true;}),"bucket zero elements");
    require(order==std::vector<unsigned>({1024,512}),"head attachment reverses same-bucket insertion order");
    order.clear();
    require(layers.visitElements(layers.find(133),[&](unsigned e){order.push_back(registry.get(e)->sourceId);return true;}),"bucket one elements");
    require(order==std::vector<unsigned>({1025,513}),"second bucket keeps independent element order");
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
void roomInitializationFailures(){
    for(unsigned mode=0;mode<3;++mode){
        Registry registry;Layers layers;unsigned layer=Layers::End;
        require(layers.insertAfterEquals(17,0,false,layer)&&registry.reserve(layers,layer,100)&&
            registry.reserve(layers,layer,101),"room initialization seed");
        RunnerDepthService::Instance existing,missing;missing.layerId=17;
        unsigned globals=0,normalizations=0;
        const bool ok=registry.initializeRoom(layers,
            [&](unsigned id,InstanceHandle& h,RunnerDepthService::Instance*& state){
                if(mode==1)return false;
                if(id==100){h={50,1};state=&existing;}
                return true;
            },[&](auto visit){
                ++globals;
                require(existing.attached,"global pass follows serialized binding");
                if(!visit(100,{50,1},existing,false)||!visit(101,{51,1},missing,true))return false;
                return mode!=2;
            },[&](unsigned e){++normalizations;return registry.get(e)->sourceId==101;},
            [](int,unsigned&){return false;});
        require(ok==(mode==0),"initialization propagates stage failure");
        if(mode==1)require(!globals&&!normalizations&&!existing.attached&&!missing.attached,
            "serialized failure suppresses global pass and normalization");
        else{
            require(globals==1&&existing.attached&&missing.attached,"completed publications survive later failure");
            require(normalizations==(mode==0?2u:0u),"global failure suppresses normalization");
            require(ids(layers,registry,layer)==std::vector<unsigned>({100,101}),"bound entries retain valid ownership");
        }
    }
}
void lifetimeDepthIntegration(){
    using R=RunnerLayerInstances<4,8>;
    R registry;R::Layers layers;RunnerLayerIds ids;require(ids.observeLoaded(131),"lifetime catalog floor");
    unsigned fixed=4;require(layers.insertAfterEquals(17,0,false,fixed)&&registry.reserve(layers,fixed,100),"lifetime fixed reservation");
    RunnerLayerLifetimes<4,8,4> owner(layers,registry,ids);
    require(owner.birth({0,1},100,{0x41200000u},false)&&owner.resolve({0,1})->depth.layerId==17&&
        owner.resolve({0,1})->depth.depth.bits==0,"placed birth fills reservation and publishes its depth");
    require(owner.birth({1,1},101,{},false)&&owner.birth({2,1},102,{},false),"dynamic peer births");
    require(owner.resolve({1,1})->depth.layerId==132&&owner.resolve({2,1})->depth.layerId==132,
        "dynamic births do not reuse fixed layer at matching depth");
    require(owner.writeDepth({1,1},MotionBinary64::fromInteger(5))&&owner.pendingDepths()==1&&
        owner.resolve({1,1})->depth.layerId==132,"setter publishes before deferred migration");
    require(owner.serviceDepths()&&owner.resolve({1,1})->depth.layerId==133,"shared-layer departure allocates destination");
    require(owner.writeDepth({2,1},MotionBinary64::fromInteger(5))&&owner.serviceDepths()&&
        owner.resolve({2,1})->depth.layerId==133&&layers.find(132)==R::Layers::End,"singleton move merges and publishes identity");
    require(owner.publishInactive({2,1},true)&&owner.reconcileActivation({2,1}),"activation relinks same lifetime");
    require(owner.writeDepth({2,1},MotionBinary64::fromInteger(-60000))&&owner.retire({2,1})&&
        owner.pendingDepths()==0&&!owner.resolve({2,1}),"retirement cancels pending depth before slot reuse");
    require(owner.birth({2,2},103,{},false)&&!owner.resolve({2,1})&&owner.resolve({2,2}),"generation checked slot reuse");
    require(owner.writeDepth({1,1},MotionBinary64::fromInteger(-60000))&&owner.serviceDepths()&&
        layers.get(layers.find(owner.resolve({1,1})->depth.layerId))->depth==-60000,"large depth jump preserves source migration");
    require(!owner.writeDepth({2,1},MotionBinary64::fromInteger(1))&&!owner.succeeded()&&
        !owner.retire({2,2})&&owner.resolve({2,2}),"stale lifetime stops owner without retiring replacement");
    R fullRegistry;R::Layers fullLayers;RunnerLayerIds fullIds;
    RunnerLayerLifetimes<4,8,0> full(fullLayers,fullRegistry,fullIds);
    require(!full.birth({0,1},1,{},false)&&!full.succeeded(),"lifetime capacity fails terminally");
}
void lifetimeRoomInitialization(){
    using R=RunnerLayerInstances<4,8>;using Owner=RunnerLayerLifetimes<4,8,4>;
    for(unsigned flags=0;flags<8;++flags){
        R copyRegistry;R::Layers copyLayers;RunnerLayerIds copyIds;
        Owner copyOwner(copyLayers,copyRegistry,copyIds);
        const Owner::State input{{2,0x12345678u},0xabcdef12u,
            {{0x80000000u},-60000,bool(flags&1),bool(flags&2)},bool(flags&4)};
        require(copyOwner.adopt(input),"adopt all combinations of lifetime flags");
        const auto* output=copyOwner.resolve(input.handle);
        require(output&&output->handle.slot==input.handle.slot&&
            output->handle.generation==input.handle.generation&&output->sourceId==input.sourceId&&
            output->depth.depth.bits==input.depth.depth.bits&&output->depth.layerId==input.depth.layerId&&
            output->depth.attached==input.depth.attached&&output->depth.append==input.depth.append&&
            output->inactive==input.inactive,
            "adoption preserves every field including signed zero and high identity bits");
    }
    R registry;R::Layers layers;RunnerLayerIds catalog;
    require(catalog.observeLoaded(131),"room lifetime catalog");
    unsigned fixed=4;require(layers.insertAfterEquals(17,10,false,fixed)&&registry.reserve(layers,fixed,100),
        "room lifetime placeholder");
    Owner owner(layers,registry,catalog);
    Owner::State survivor{{2,7},100,{{0x41280000u},99,true,false},true}; // 10.5
    Owner::State fallback{{0,3},512,{{0x41a40000u},17,false,false},false}; // 20.5
    require(owner.adopt(survivor)&&owner.adopt(fallback)&&registry.find(survivor.handle)==R::End,
        "adoption preserves fields without attaching or running Create");
    require(owner.initializeRoom([&](auto visit){
        require(owner.resolve(survivor.handle)->depth.layerId==17&&
            owner.resolve(survivor.handle)->depth.depth.bits==0x41280000u,
            "serialized initialization preserves fractional depth before globals");
        return visit(fallback.handle)&&visit(survivor.handle);
    },[](unsigned){return false;}),"shared owner initializes surviving room lifetimes");
    require(owner.resolve(fallback.handle)->depth.layerId==17&&
        owner.resolve(fallback.handle)->depth.depth.bits==0x41200000u,
        "fallback honors saved fixed layer then publishes its depth");
    std::vector<unsigned> order;
    require(layers.visitElements(fixed,[&](unsigned e){order.push_back(registry.get(e)->sourceId);return true;})&&
        order==std::vector<unsigned>({512,100}),"inactive survivor follows active fallback");
    require(owner.writeDepth(survivor.handle,MotionBinary64::fromInteger(-60000))&&owner.serviceDepths()&&
        owner.resolve(survivor.handle)->depth.layerId==132,"adopted lifetime enters ordinary depth service");
    require(owner.retire(survivor.handle)&&owner.birth({2,8},101,{},false),"adopted lifetime retires and slot is reusable");
    require(!owner.initializeRoom([](auto visit){return visit(InstanceHandle{2,7});},[](unsigned){return false;})&&
        !owner.succeeded()&&owner.resolve({2,8}),"stale global enumeration fails without losing replacement");
    R duplicates;R::Layers duplicateLayers;RunnerLayerIds duplicateIds;Owner duplicate(duplicateLayers,duplicates,duplicateIds);
    require(duplicate.adopt(survivor)&&!duplicate.birth({1,1},100,{},false)&&!duplicate.resolve({1,1}),
        "birth cannot duplicate an adopted source identity before initialization");
}
void roomUninitialization(){
    using R=RunnerLayerInstances<4,8>;using Owner=RunnerLayerLifetimes<4,8,4>;
    R registry;R::Layers layers;RunnerLayerIds catalog;Owner owner(layers,registry,catalog);
    unsigned layer=4;require(layers.insertAfterEquals(17,10,false,layer)&&registry.reserve(layers,layer,100),"exit seed");
    require(owner.birth({0,1},100,{},false)&&owner.writeDepth({0,1},MotionBinary64::fromInteger(15)),"pending exit setter");
    const unsigned element=registry.find(100);
    require(owner.uninitializeRoom([](unsigned){return false;}),"uninitialize room instance elements");
    const auto* survivor=owner.resolve({0,1});
    require(survivor&&!survivor->depth.attached&&survivor->depth.layerId==-1&&
        survivor->depth.depth.bits==0x41700000u&&owner.pendingDepths()==1,
        "uninitialization clears attachment but preserves lifetime, depth and pending write");
    require(registry.find(100)==element&&!registry.get(element)->instance.generation&&
        layers.elementOwner(element)==layer&&layers.elementCount(layer)==1,
        "uninitialized element remains in its original layer");
    require(owner.uninitializeRoom([](unsigned){return false;})&&owner.pendingDepths()==1,
        "uninitialization repeat is a no-op");
    require(owner.initializeRoom([](auto visit){return visit(InstanceHandle{0,1});},[](unsigned){return false;})&&
        owner.resolve({0,1})->depth.depth.bits==0x41700000u&&owner.serviceDepths()&&
        layers.get(layers.find(owner.resolve({0,1})->depth.layerId))->depth==15,
        "retained depth request migrates after serialized reinitialization");
    require(owner.writeDepth({0,1},MotionBinary64::fromInteger(20))&&
        owner.uninitializeRoom([](unsigned){return false;})&&owner.serviceDepths()&&owner.pendingDepths()==0,
        "depth service skips unattached survivors without requiring an element binding");

    Registry r;Layers l;unsigned fixed=2;require(l.insertAfterEquals(7,0,false,fixed),"lookup exit seed");
    RunnerDepthService::Instance original,replacement;replacement.attached=true;replacement.layerId=99;
    replacement.depth.bits=0x41280000u;
    require(r.attach(l,fixed,100,{0,1},original,false),"lookup exit bound element");
    const unsigned e=r.find(100);
    require(!r.uninitialize(l,e,[](unsigned,InstanceHandle&,RunnerDepthService::Instance*&){return false;})&&
        r.get(e)->instance.generation==1,"failed lookup leaves binding intact");
    require(r.uninitialize(l,e,[&](unsigned id,InstanceHandle& h,RunnerDepthService::Instance*& out){
        require(id==100,"uninitialize source identity");h={3,2};out=&replacement;return true;
    })&&original.attached&&!replacement.attached&&replacement.layerId==-1&&replacement.depth.bits==0x41280000u,
        "uninitialize resolves source ID rather than clearing the cached lifetime");
    require(r.uninitialize(l,e,[](unsigned,InstanceHandle&,RunnerDepthService::Instance*&){return false;}),
        "uninitialized element does not repeat global lookup");
    RunnerDepthService::Instance missing;
    require(r.attach(l,fixed,101,{1,1},missing,false)&&
        r.uninitialize(l,r.find(101),[](unsigned,InstanceHandle&,RunnerDepthService::Instance*&){return true;})&&
        !r.get(r.find(101))->instance.generation&&missing.attached,
        "absent global instance still clears the element without touching cached state");
}
void unnumberedLifetimes(){
    using namespace downwell;
    for(bool retireBeforeExit:{false,true}){
        using Registry=RunnerLayerInstances<4,4>;
        Registry registry;Registry::Layers layers;RunnerLayerIds ids;
        RunnerLayerLifetimes<4,4,3> owner(layers,registry,ids);
        const InstanceHandle a{0,1},b{1,1};
        require(owner.birthUnnumbered(a,{},false)&&owner.birthUnnumbered(b,{},false),"distinct unnumbered births share a layer");
        require(registry.find(0u)==Registry::End&&registry.find(a)!=registry.find(b)&&
            owner.resolve(a)->sourceId==0&&owner.resolve(b)->sourceId==0,"unknown IDs are not fabricated or aliased");
        require(owner.writeDepth(a,MotionBinary64::fromInteger(-60000))&&owner.serviceDepths(),"unnumbered depth migration uses lifetime identity");
        require(layers.get(layers.find(owner.resolve(a)->depth.layerId))->depth==-60000&&
            layers.get(layers.find(owner.resolve(b)->depth.layerId))->depth==0,"migration does not affect another unnumbered lifetime");
        if(retireBeforeExit)require(owner.retire(a)&&owner.retire(b),"retire unnumbered actors before room exit");
        require(owner.uninitializeRoom([](unsigned){return false;})==retireBeforeExit,
            "unrecovered numeric IDs cannot silently survive source room lookup");
    }
}
template<bool Direct> std::vector<unsigned> presenceWitness(){
    using namespace downwell;
    using R=RunnerLayerInstances<2,32,Direct?24:0>;
    R registry;typename R::Layers layers;unsigned layer=2;
    require(layers.insertAfterEquals(17,0,false,layer),"presence layer");
    RunnerDepthService::Instance states[24];InstanceHandle handles[24];
    std::vector<unsigned> trace;
    auto capture=[&](){
        for(unsigned e=0;e<32;++e){
            const auto* entry=registry.get(e);trace.push_back(entry!=nullptr);
            if(entry){trace.push_back(entry->sourceId);trace.push_back(entry->instance.slot);
                trace.push_back(entry->instance.generation);}
        }
        require(layers.visitElements(layer,[&](unsigned e){trace.push_back(e);return true;}),"presence order");
        trace.push_back(0xffffffffu);
    };
    capture();
    for(unsigned i=0;i<4;++i){require(registry.reserve(layers,layer,101+i),"presence placeholder");capture();}
    for(unsigned i=0;i<24;++i){
        handles[i]={i,1};
        require(registry.attach(layers,layer,i<4?101+i:0,handles[i],states[i],i%3==0),"presence attach");
        capture();
    }
    for(unsigned i=0;i<4;++i){
        const unsigned element=registry.find(101+i);
        require(registry.uninitialize(layers,element,[&](unsigned source,InstanceHandle& h,
            RunnerDepthService::Instance*& state){
            require(source==101+i,"presence uninitialize ID");h=handles[i];state=&states[i];return true;
        }),"presence uninitialize");
        require(registry.get(element)&&!registry.get(element)->instance.generation,
            "unbound named placeholder stays present");capture();
        ++handles[i].generation;
        require(registry.attach(layers,layer,101+i,handles[i],states[i],false),"presence reinitialize");capture();
    }
    for(unsigned i=0;i<24;i+=2){
        const auto old=handles[i];const unsigned source=i<4?101+i:0;
        require(registry.retire(layers,source,old),"presence retire");capture();
        require(registry.find(old)==R::End,"presence old generation gone");
        states[i]={};++handles[i].generation;
        require(registry.attach(layers,layer,source,handles[i],states[i],false),"presence reuse");capture();
        RunnerDepthService::Instance duplicate;
        require(!registry.attach(layers,layer,0,handles[i],duplicate,false),"presence duplicate rejection");capture();
    }
    return trace;
}
template<bool Compact> std::vector<unsigned> compactLifetimeWitness(){
    using namespace downwell;
    using R=RunnerLayerInstances<8,32,24>;
    R registry;R::Layers layers;RunnerLayerIds ids;
    RunnerLayerLifetimes<8,32,24,true,Compact> owner(layers,registry,ids);
    std::vector<unsigned> trace;
    InstanceHandle handles[24];
    auto capture=[&](){
        trace.push_back(owner.succeeded());trace.push_back(ids.dynamicMaximum);
        require(layers.visitLayers([&](unsigned layer,const auto& record){
            trace.push_back(layer);trace.push_back(record.id);trace.push_back(record.depth);
            require(layers.visitElements(layer,[&](unsigned e){
                const auto* entry=registry.get(e);require(entry,"live entry");
                trace.push_back(e);trace.push_back(entry->instance.slot);trace.push_back(entry->instance.generation);
                return true;
            }),"compact element order");trace.push_back(0xffffffffu);return true;
        }),"compact layer order");
    };
    for(unsigned i=0;i<24;++i){
        handles[i]={i,1};RunnerImageValue depth;depth.assignInteger(int(i%3)*16);
        require(owner.birthUnnumbered(handles[i],depth,i%5==0,true),"compact birth");capture();
    }
    // Keep most lifetimes unobserved while activation and merges change links.
    for(unsigned i=0;i<24;i+=3){
        require(owner.publishInactive(handles[i],true)&&owner.reconcileActivation(handles[i]),"compact inactive");capture();
        require(owner.publishInactive(handles[i],false)&&owner.reconcileActivation(handles[i]),"compact active");capture();
    }
    for(unsigned i=1;i<24;i+=4){
        require(owner.writeDepth(handles[i],MotionBinary64::fromInteger(int((i+1)%3)*16))&&owner.serviceDepths(),"compact depth migration");capture();
    }
    for(unsigned i=0;i<24;i+=2){
        require(owner.retire(handles[i])&&!owner.resolve(handles[i]),"compact retirement");capture();
        const auto old=handles[i];++handles[i].generation;
        require(owner.birthUnnumbered(handles[i],{},false,true)&&!owner.resolve(old),"compact generation reuse");capture();
    }
    for(unsigned i=0;i<24;++i){
        int layer;require(owner.attachedLayer(handles[i],layer),"derived layer lookup");trace.push_back(layer);
        const auto* state=owner.resolve(handles[i]);require(state,"materialized lifetime");
        trace.push_back(state->sourceId);trace.push_back(state->depth.depth.bits);trace.push_back(state->depth.layerId);
        trace.push_back(state->depth.attached);trace.push_back(state->depth.append);trace.push_back(state->inactive);
        require(owner.publishInactive(handles[i],!state->inactive)&&owner.resolve(handles[i])==state,"materialized pointer stays stable");
        trace.push_back(state->inactive);
    }
    for(auto h:handles)require(owner.retire(h),"materialized retirement");capture();
    return trace;
}
template<bool Compact> std::vector<unsigned> compactFailureWitness(){
    using namespace downwell;
    using R=RunnerLayerInstances<1,1,2>;
    R registry;R::Layers layers;RunnerLayerIds ids;
    RunnerLayerLifetimes<1,1,2,true,Compact> owner(layers,registry,ids);
    require(owner.birthUnnumbered({0,1},{},false,true),"fill compact attachment capacity");
    require(!owner.birthUnnumbered({1,1},{},true,true)&&!owner.succeeded(),"failed compact attachment");
    const auto* state=owner.resolve({1,1});require(state,"failure keeps adopted lifetime");
    return {state->handle.slot,state->handle.generation,state->sourceId,state->depth.depth.bits,
        unsigned(state->depth.layerId),state->depth.attached,state->depth.append,state->inactive};
}
template<bool Fast> std::vector<unsigned> freshAttachmentWitness(unsigned mode){
    using namespace downwell;
    using R=RunnerLayerInstances<3,4,5>;
    R registry;R::Layers layers;
    const RunnerImageValue depth{mode==3?0x7fc00000u:0x41780000u}; // NaN or 15.5.
    InstanceHandle h{mode==2?5u:mode==8?4u:0u,1};
    unsigned calls=0,failureCalls=0;
    auto create=[&](int value,unsigned& slot){
        ++calls;if(mode==4)return false;
        const bool made=layers.insertAfterEquals(100+calls,value+(mode==6),mode!=5,slot);
        return mode==7?false:made;
    };
    if(mode==8||mode==9||mode==10){
        unsigned layer;require(layers.insertAfterEquals(50,15,true,layer),"prepare existing dynamic layer");
        if(mode==8)for(unsigned s=0;s<4;++s){
            RunnerDepthService::Instance state;
            require(registry.attach(layers,layer,0,{s,1},state,false),"fill compact element registry");
        }
        if(mode==9){
            RunnerDepthService::Instance state;
            require(registry.attach(layers,layer,0,h,state,false),"pre-existing compact identity");
        }
    }
    if(mode==11){unsigned layer;require(layers.insertAfterEquals(50,15,false,layer),"fixed equal-depth layer");}
    RunnerDepthService::Instance state{depth,-1,false,false};
    bool ok;
    if constexpr(Fast)ok=registry.attachFreshUnnumbered(layers,h,depth,create,
        [&](RunnerImageValue d,int id,bool attached){++failureCalls;state={d,id,attached,false};});
    else ok=registry.attachRoomFallback(layers,0,h,state,mode==1,create);
    if constexpr(Fast)require(failureCalls==unsigned(!ok),"compact failure published exactly once, never on success");
    std::vector<unsigned> trace{unsigned(ok),calls};
    if(!ok)trace.insert(trace.end(),{state.depth.bits,unsigned(state.layerId),unsigned(state.attached),unsigned(state.append)});
    require(layers.visitLayers([&](unsigned l,const auto& value){
        trace.insert(trace.end(),{l,unsigned(value.id),unsigned(value.depth),unsigned(value.dynamic)});
        return layers.visitElements(l,[&](unsigned e){trace.push_back(e);return true;});
    }),"capture compact layer topology");
    for(unsigned e=0;e<4;++e){const auto* entry=registry.get(e);
        trace.push_back(bool(entry));
        if(entry)trace.insert(trace.end(),{entry->sourceId,entry->instance.slot,entry->instance.generation});
    }
    return trace;
}
template<bool Retained> std::vector<unsigned> logicalCapacityFailure(){
    using Storage=RunnerLayers<4,4,Retained>;
    using R=RunnerLayerInstances<4,4,2,Storage>;
    using Lifetimes=RunnerLayerLifetimes<4,4,2,true,true,Storage>;
    Storage layers;R registry;RunnerLayerIds ids;Lifetimes lives(layers,registry,ids);
    unsigned full=4;require(layers.insertAfterEquals(5,0,true,full)&&ids.observeLoaded(10),"logical capacity fixture");
    if constexpr(Retained)require(layers.retain(full,4),"retain full logical capacity");
    else for(unsigned e=0;e<4;++e)require(layers.attach(full,e,true,false,[](unsigned){return false;}),"fill physical reference");
    RunnerImageValue depth;depth.assignInteger(20);InstanceHandle h{0,17};
    require(!lives.birthUnnumbered(h,depth,false,true)&&!lives.succeeded(),"capacity failure is terminal");
    const auto* state=lives.resolve(h);require(state,"failed birth retains adopted lifetime");
    std::vector<unsigned> out{unsigned(ids.dynamicMaximum),state->handle.slot,state->handle.generation,
        state->depth.depth.bits,unsigned(state->depth.layerId),unsigned(state->depth.attached),
        registry.find(h),layers.elementCount(full)};
    layers.visitLayers([&](unsigned,const auto& layer){out.insert(out.end(),{unsigned(layer.id),unsigned(layer.depth)});return true;});
    require(ids.dynamicMaximum==11&&!state->depth.attached&&state->depth.layerId==-1&&registry.find(h)==R::End,
        "logical exhaustion consumes layer ID but publishes no element or attached state");
    return out;
}
void retainedLifetimeIntegration(){
    using Storage=RunnerLayers<4,8,true>;
    using R=RunnerLayerInstances<4,8,4,Storage>;
    using Lifetimes=RunnerLayerLifetimes<4,8,4,true,true,Storage>;
    Storage layers;R registry;RunnerLayerIds ids;Lifetimes lives(layers,registry,ids);
    unsigned terrain=4;require(ids.createDynamic(layers,0,terrain)&&layers.retain(terrain,3),"retained-only dynamic layer");
    const int terrainId=layers.get(terrain)->id;const InstanceHandle actor{0,23};
    require(lives.birthUnnumbered(actor,{},false,true)&&layers.elementCount(terrain)==4,"actor shares retained layer");
    require(lives.writeDepth(actor,MotionBinary64::fromInteger(10))&&lives.serviceDepths(),"depth service sees logical population");
    const auto* state=lives.resolve(actor);
    require(state&&state->depth.layerId!=terrainId&&layers.get(terrain)->depth==0&&layers.elementCount(terrain)==3,
        "ordinary lifetime owner cannot move retained terrain as a singleton actor");
    require(lives.retire(actor)&&layers.get(terrain)&&layers.releaseRetained(terrain,3)&&!layers.get(terrain),
        "actor and retained retirement dispose only their own empty layers");
    require(logicalCapacityFailure<true>()==logicalCapacityFailure<false>(),"logical/physical capacity failure witnesses");
}
int main(){retainedLifetimeIntegration();permutations();lifecycle();identityCollisions();bounds();roomFallback();globalFallbackOrder();serializedInitialization();rebindRelease();roomInitializationFailures();lifetimeDepthIntegration();lifetimeRoomInitialization();roomUninitialization();unnumberedLifetimes();
    static_assert(sizeof(downwell::RunnerLayerEntry<true>)==12,"derived entry contains only identity fields");
    require(presenceWitness<true>()==presenceWitness<false>(),"derived/explicit presence lifecycle trace");
    require(compactLifetimeWitness<true>()==compactLifetimeWitness<false>(),"compact/full lifetime mutation trace");
    require(compactFailureWitness<true>()==compactFailureWitness<false>(),"compact/full attachment failure prefix");
    for(unsigned mode=0;mode<12;++mode)
        require(freshAttachmentWitness<true>(mode)==freshAttachmentWitness<false>(mode),"fresh compact/full attachment publication prefix");
    std::cout<<"Serialized layer initialization, rebind release, room fallback and lifetime-checked attachment verified\n";}

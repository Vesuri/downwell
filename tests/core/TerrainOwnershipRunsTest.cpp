#include "core/TerrainOwnershipRuns.h"
#include "core/NativeTerrainLifetimes.h"
#include "core/GameplayTerrainInstancesRuns.h"
#include "core/RunnerSpatialLifetimes.h"
#include "core/RunnerObservedRegistration.h"
#include "core/RunnerObservedRoomMembership.h"
#include "core/RunnerRoomMembership.h"
#include <tuple>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
using Store=downwell::TerrainOwnershipRuns<64>;
static void requireAt(bool ok,unsigned line){if(!ok){std::fprintf(stderr,"terrain check failed at line %u\n",line);std::abort();}}
#define require(ok) requireAt((ok),__LINE__)
static bool same(const Store::Identity& a,const Store::Identity& b){
    return a.handle.slot==b.handle.slot&&a.handle.generation==b.handle.generation&&
        a.object==b.object&&a.x==b.x&&a.y==b.y&&a.creationCode==b.creationCode&&a.source==b.source;
}
// Stand-in for independently captured publication order and common state.
// The ownership container must never reconstruct it from current geometry.
struct Publication {
    downwell::RunnerRegistrationOrder order;
    unsigned flags=0;
    Publication shifted(unsigned offset)const{
        Publication result=*this;
        result.order.low+=offset;if(result.order.low<order.low)++result.order.high;
        return result;
    }
    bool same(const Publication& b)const{
        return order.low==b.order.low&&order.high==b.order.high&&flags==b.flags;
    }
};
static void publicationRuns(){
    using Runs=downwell::TerrainOwnershipRuns<64,Publication>;
    Runs runs;
    struct Cell {Store::Identity id;Publication publication;};
    std::vector<Cell> cells;
    unsigned comparisons=0;
    auto compare=[&]{
        require(runs.size()==cells.size());unsigned ordinal=0;
        require(runs.visit([&](const auto& id){
            const auto& expected=cells.at(ordinal++);
            require(id.handle.slot==expected.id.handle.slot&&id.handle.generation==expected.id.handle.generation&&
                id.object==expected.id.object&&id.x==expected.id.x&&id.y==expected.id.y&&
                id.creationCode==expected.id.creationCode&&id.source==expected.id.source);
            Publication p;require(runs.publication(id.handle,p)&&p.same(expected.publication));
            return true;
        }));
        require(ordinal==cells.size()&&runs.runCount()<=runs.size());
        for(unsigned i=0;i<cells.size();++i){const auto h=runs.handle(i);
            require(h.slot==cells[i].id.handle.slot&&h.generation==cells[i].id.handle.generation);
        }
        ++comparisons;
    };
    auto append=[&](unsigned slot,unsigned generation,int x,Publication p){
        require(runs.append({slot,generation},60,x,32,0,-1,p));
        cells.push_back({{{slot,generation},60,x,32,-1,0},p});compare();
    };
    auto change=[&](unsigned ordinal,Publication p){
        require(runs.changePublication(cells[ordinal].id.handle,p));
        cells[ordinal].publication=p;compare();
    };
    auto retire=[&](unsigned ordinal){
        require(runs.retire(cells[ordinal].id.handle));cells.erase(cells.begin()+ordinal);compare();
    };
    const Publication base{{0xffffffe0u,7},0x8008};
    for(unsigned i=0;i<64;++i)append(i,i+1,int(i)*16,base.shifted(i));
    require(runs.runCount()==1);
    // No-op updates must not fragment the authoritative run.
    for(unsigned i=0;i<64;++i)change(i,cells[i].publication);
    require(runs.runCount()==1);
    // A changed cell separates common state on both sides, even at full
    // population. There is no smaller exception pool that can exhaust first.
    change(31,{{5,19},2});require(runs.runCount()==3);
    for(unsigned i=0;i<64;++i)change(i,{{i*17u,29},i});
    require(runs.runCount()==64);
    while(!cells.empty())retire(0);
    // Splits must ignore holes whose slots now belong to newer identities.
    for(unsigned i=0;i<64;++i)append(i,100+i,-int(i)*16,base.shifted(i));
    for(unsigned i=63;i;--i)if(i&1)retire(i);
    for(unsigned i=1;i<64;i+=2)append(i,200+i,int(i)*32,{{i,0},99});
    for(unsigned i=0;i<32;++i)change(i,{{i,1},17});
    unsigned random=91823,generation=1000;
    for(unsigned i=0;i<4096;++i){
        random=random*1664525u+1013904223u;
        const unsigned ordinal=(random>>8)%cells.size();
        if(random&1)change(ordinal,{{random,generation},random>>24});
        else {const unsigned slot=cells[ordinal].id.handle.slot;retire(ordinal);
            append(slot,generation++,int(slot)*16,{{random,9},0});}
    }
    const auto old=cells[0].id.handle;retire(0);
    Publication untouched{{11,12},13};
    require(!runs.publication(old,untouched)&&untouched.same({{11,12},13}));
    require(!runs.changePublication(old,{}));compare();
    while(!cells.empty())retire(0);
    // Trim all-hole prefixes/suffixes without leaking descriptors.
    for(unsigned pass=0;pass<80;++pass){
        for(unsigned i=0;i<64;++i)append(i,generation++,int(i)*16,base.shifted(i));
        while(cells.size()>1)retire(pass&1?0:unsigned(cells.size()-1));
        change(0,{{pass,18},1});require(runs.runCount()==1);retire(0);
    }
    std::printf("Authoritative terrain publication runs: %u identity/order/state comparisons passed\n",comparisons);
}
struct TerrainObserver {
    template<class... A> bool birth(const A&...){return true;}
    template<class... A> bool retire(const A&...){return true;}
    template<class... A> bool activation(const A&...){return true;}
    template<class... A> bool membership(const A&...){return true;}
    template<class... A> bool move(const A&...){return true;}
    template<class... A> bool sprite(const A&...){return true;}
    template<class... A> bool imageIndex(const A&...){return true;}
    template<class... A> bool markDestroyed(const A&...){return true;}
};
struct InitializedObserver {
    using Terrain=downwell::GameplayTerrainInstancesRuns;
    Terrain* store=nullptr;
    unsigned failure=0,calls=0;
    bool initializedTerrainBirth(downwell::InstanceHandle h,const Terrain::Record& r,int tile,
        downwell::GameMakerRandom&,Terrain::CreateValues& values,bool&){
        ++calls;require(store->ownsIdentity(h,r)&&store->size()==1&&r.spriteId==-1);
        if(failure==1)return false;
        values.spriteId=tile;
        require(store->find(h)->spriteId==tile); // Writes are live, not committed at return.
        if(failure==2)return false;
        values.imageSpeed=0;return true;
    }
};
struct IdentityObserver:InitializedObserver {
    bool useIdentity=false;
    unsigned identityCalls=0;
    bool initializedTerrainIdentity(downwell::InstanceHandle h,int object,int x,int y,bool active,int tile,
        Terrain::CreateValues& values,bool&,bool& admitted){
        ++identityCalls;
        require(object==60&&x==16&&y==32&&active&&store->find(h)->spriteId==-1);
        if(!useIdentity)return false;
        admitted=true;
        if(failure==1)return false;
        values.spriteId=tile;
        require(store->find(h)->spriteId==tile);
        if(failure==2)return false;
        values.imageSpeed=0;return true;
    }
};
static void initializedBoundary(){
    using Terrain=downwell::GameplayTerrainInstancesRuns;
    for(unsigned failure=0;failure<3;++failure){
        Terrain store;InitializedObserver observer{&store,failure};downwell::GameMakerRandom random;
        require(store.bindInitializedBirth(observer)&&!store.bindInitializedBirth(observer));
        require(store.appendInitialized(60,16,32,true,51,random)==(failure==0));
        require(observer.calls==1&&store.size()==1&&store.valid()==(failure==0));
        const auto record=store.find(store.handle(0));
        require(record&&record->spriteId==(failure==1?-1:51)&&record->imageSpeed==(failure?1:0));
        if(failure)require(!store.appendInitialized(60,32,32,true,51,random)&&observer.calls==1);
    }
    for(bool direct:{false,true})for(unsigned failure=0;failure<3;++failure){
        Terrain store;IdentityObserver observer;observer.store=&store;
        observer.useIdentity=direct;observer.failure=failure;downwell::GameMakerRandom random;
        require(store.bindInitializedIdentity(observer)&&!store.bindInitializedBirth(observer));
        require(store.appendInitialized(60,16,32,true,51,random)==(failure==0));
        require(observer.identityCalls==1&&observer.calls==(direct?0u:1u));
        const auto record=store.find(store.handle(0));
        require(store.size()==1&&store.valid()==(failure==0)&&random.drawCount()==0);
        require(record&&record->spriteId==(failure==1?-1:51)&&record->imageSpeed==(failure?1:0));
        if(failure)require(!store.appendInitialized(60,32,32,true,51,random)&&observer.identityCalls==1);
    }
    TerrainObserver observer;
    for(int object:{60,93,99})for(int tile:{-1,51,100000}){
        Terrain runs(Terrain::Binding::bindDestruction(observer));
        downwell::GameplayTerrainInstancesScalar scalar;
        downwell::GameMakerRandom a,b;
        require(runs.appendInitialized(object,32,48,false,tile,a));
        require(scalar.append(object,32,48,false)&&scalar.createProperties(scalar.handle(0),tile,false,b));
        const auto x=runs.find(runs.handle(0));const auto* y=scalar.find(scalar.handle(0));
        require(x->spriteId==y->spriteId&&x->imageIndex==y->imageIndex&&x->imageSpeed==y->imageSpeed&&
            x->active==y->active&&x->lifetimeId==y->lifetimeId&&a.drawCount()==b.drawCount());
        for(unsigned i=0;i<16;++i)require(a.stateWord(i)==b.stateWord(i));
    }
}
static void spatialIdentity(){
    using Terrain=downwell::GameplayTerrainInstancesRuns;
    using Shared=downwell::RunnerSpatialLifetimes<2052,2,2050,Terrain>;
    static_assert(sizeof(Shared)<128,"No spatial identity array for terrain");
    TerrainObserver observer;
    Terrain terrain(Terrain::Binding::bindDestruction(observer));Shared spatial;
    require(!spatial.canAttach({2,1},60));
    require(spatial.bind(terrain)&&!spatial.bind(terrain));
    for(unsigned iteration=0;iteration<8;++iteration){
        require(terrain.append(60,32,48,true));const auto local=terrain.handle(0);
        const downwell::InstanceHandle h{local.slot+2,local.generation};
        require(terrain.find(local)&&!spatial.contains(h));
        require(!spatial.canAttach({h.slot,h.generation+1},60)&&!spatial.canAttach(h,93));
        require(spatial.canAttach(h,60));spatial.publish(h,60);
        require(spatial.contains(h)&&spatial.object(h.slot)==60&&spatial.handle(h.slot).generation==h.generation);
        require(!spatial.canAttach(h,60)&&spatial.rebind(h.slot,60)&&!spatial.rebind(h.slot,93));
        require(terrain.requestDestroy(local));
        require(spatial.contains(h)); // A destruction mark is not retirement.
        if(iteration&1)spatial.clear();else spatial.retire(h.slot);
        require(!spatial.contains(h)&&spatial.object(h.slot)==-1&&!spatial.canAttach(h,60));
        require(bool(terrain.find(local))); // Spatial retirement precedes typed retirement.
        require(terrain.retireMarked(local)&&!spatial.contains(h));
    }
    // The actor ranges on both sides retain the ordinary generation high-water
    // marks, including after clear. Terrain cannot alias either compressed slot.
    for(unsigned slot:{0u,1u,2050u,2051u}){
        require(spatial.canAttach({slot,17},35));spatial.publish({slot,17},35);
        require(spatial.contains({slot,17})&&spatial.rebind(slot,92)&&spatial.object(slot)==92);
        spatial.retire(slot);require(!spatial.canAttach({slot,17},35)&&spatial.canAttach({slot,18},35));
        spatial.publish({slot,18},35);
    }
    spatial.clear();
    for(unsigned slot:{0u,1u,2050u,2051u})require(!spatial.contains({slot,18})&&!spatial.canAttach({slot,18},35));
    require(!spatial.canAttach({2052,1},35)&&!spatial.contains({2052,1}));
}
static void observedRegistration(){
    using Terrain=downwell::GameplayTerrainInstancesRuns;
    TerrainObserver observer;
    Terrain terrain(Terrain::Binding::bindDestruction(observer));
    downwell::RunnerObservedRegistration<2052,2,2050,Terrain> observed;
    downwell::RunnerObjectMembership<2052> scalar;
    require(observed.bindTerrain(terrain)&&!observed.bindTerrain(terrain));
    using Entry=std::tuple<unsigned,unsigned,int>;
    auto isA=[](int child,int parent){return parent==100&&(child==60||child==93);};
    auto compare=[&](){
        std::vector<Entry> a,b;
        require(observed.visitRegistrations([&](unsigned s,unsigned id,int o){a.emplace_back(s,id,o);return true;}));
        require(scalar.visitRegistrations([&](unsigned s,unsigned id,int o){b.emplace_back(s,id,o);return true;}));
        require(a==b&&observed.size()==scalar.size());
        for(int object:{-1,35,60,93,99,100,101}){
            require(observed.count(object,isA)==scalar.count(object,isA));
            a.clear();b.clear();
            observed.visit(object,isA,[&](unsigned s,unsigned id){a.emplace_back(s,id,0);return true;});
            scalar.visit(object,isA,[&](unsigned s,unsigned id){b.emplace_back(s,id,0);return true;});
            require(a==b);
        }
        for(unsigned first:{0u,2u,2050u}){
            a.clear();b.clear();const unsigned last=first==0?2:first==2?2050:2052;
            require(observed.visitRegistrationRange(first,last,[&](unsigned s,unsigned id,int o){a.emplace_back(s,id,o);return true;}));
            require(scalar.visitRegistrationRange(first,last,[&](unsigned s,unsigned id,int o){b.emplace_back(s,id,o);return true;}));
            require(a==b);
        }
    };
    auto bind=[&](unsigned s,int object,unsigned id){require(observed.bind(s,object,id)==scalar.bind(s,object,id));compare();};
    bind(0,35,7);
    for(unsigned i=0;i<64;++i){
        int object=i%3==0?60:i%3==1?93:99;
        require(terrain.append(object,int(i)*16,32,true));const auto h=terrain.handle(i);
        if(i&1){
            require(observed.bindAllocatedTerrain(h.slot+2,object));
            require(scalar.bind(h.slot+2,object,h.generation));compare();
        }else bind(h.slot+2,object,h.generation);
        if(i%7==0)bind(2050,35,i+1);
    }
    unsigned random=77123;
    for(unsigned i=0;i<2000;++i){
        random=random*1664525u+1013904223u;
        const auto h=terrain.handle((random>>8)%64);const auto r=terrain.find(h);
        if(random&1)bind(h.slot+2,r->objectId,h.generation);
        else {require(observed.unbind(h.slot+2)==scalar.unbind(h.slot+2));compare();}
    }
    // Direct typed retirement invalidates a previously observed projection.
    const auto old=terrain.handle(0);bind(old.slot+2,terrain.find(old)->objectId,old.generation);
    require(terrain.requestDestroy(old)&&terrain.retireMarked(old));require(scalar.unbind(old.slot+2));compare();
    require(terrain.append(93,0,48,true));const auto fresh=terrain.handle(terrain.size()-1);
    require(fresh.slot==old.slot&&fresh.generation!=old.generation);bind(fresh.slot+2,93,fresh.generation);
    require(!observed.bind(old.slot+2,93,old.generation));compare();
    unsigned calls=0;require(!observed.visitRegistrations([&](auto,auto,auto){++calls;return false;})&&calls==1);
    using Projection=decltype(observed)::RegistrationProjection<2,2050>;
    Projection projection;
    require(!observed.visitRegistrationRange(projection,0,2,[&](auto,auto,auto){observed.repeatFreshTailBinding();return true;}));
    require(!observed.visitRetainedRegistrationRange(2,3,[](auto,auto,auto){return true;}));
    observed.clear();scalar.clear();compare();
    bind(2051,35,0);bind(fresh.slot+2,93,fresh.generation);
    downwell::RunnerRegistrationOrder order{0xffffffffu,7};order.advance();
    require(order.low==0&&order.high==8&&order.present());
    std::puts("Observed terrain registration: mixed order, rebind, retirement and reuse passed");
}
static void observedRoom(){
    using Terrain=downwell::GameplayTerrainInstancesRuns;
    TerrainObserver observer;Terrain terrain(Terrain::Binding::bindDestruction(observer));
    downwell::RunnerObservedRoomMembership<2052,2,2050,Terrain> room;
    downwell::RunnerRoomMembership<2052> scalar;
    require(room.bindTerrain(terrain));
    std::vector<unsigned> slots{0,1,2050,2051};
    for(unsigned i=0;i<64;++i){require(terrain.append(60,int(i)*16,32,true));slots.push_back(terrain.handle(i).slot+2);}
    auto compare=[&](){
        for(unsigned s:slots)require(room.contains(s)==scalar.contains(s)&&room.active(s)==scalar.active(s)&&room.marked(s)==scalar.marked(s));
        for(unsigned mode=0;mode<6;++mode){
            std::vector<unsigned> a,b;
            auto run=[&](auto& owner,auto& out){
                auto visit=[&](unsigned s){out.push_back(s);return true;};
                if(mode==0)return owner.visitAnimationChecked(visit);
                if(mode==1)return owner.visitDrawChecked(visit);
                if(mode==2)return owner.visitCleanupChecked(visit);
                if(mode==3)return owner.visitMarkedCleanupChecked(visit);
                bool dirty=false;
                bool ok=owner.visitAnimationRetainedChecked(2,2050,dirty,[&](unsigned s){
                    out.push_back(s);if(mode==5&&out.size()==1)dirty=true;return true;
                });out.push_back(dirty?1:0);return ok;
            };
            require(run(room,a)==run(scalar,b)&&a==b);
        }
    };
    unsigned random=71413;
    for(unsigned round=0;round<8;++round){
        room.clear();scalar.clear();compare();
        for(unsigned s:slots){
            if(s>=2&&s<2050&&(s&1))room.bindAllocatedTerrain(s,{});
            else require(room.bind(s,{}));
            require(scalar.bind(s,{}));compare();
        }
        for(unsigned step=0;step<512;++step){
            random=random*1664525u+1013904223u;
            const unsigned slot=slots[(random>>8)%slots.size()],op=(random>>20)%6;
            std::vector<unsigned> a,b;
            auto mutate=[&](auto& owner,auto& log){
                if(op==0){downwell::RunnerImageValue key;key.assignInteger(int((random>>24)%7)-3);return owner.bind(slot,key);}
                if(op==1)return owner.remove(slot);
                if(op==2)return owner.mark(slot);
                unsigned calls=0;
                if(op==3)return owner.reconcileChecked([&](unsigned s){log.push_back(s);return ((s+step)%7)<3;},
                    [&](unsigned s,bool active){log.push_back(4096+s+4096*active);return step%13||++calls!=2;});
                auto visit=[&](unsigned s){log.push_back(s);if(step%17==0&&++calls==2)return false;return owner.remove(s);};
                return op==4?owner.visitMarkedCleanupChecked(visit):owner.visitCleanupChecked(visit);
            };
            require(mutate(room,a)==mutate(scalar,b)&&a==b);compare();
        }
    }
    std::puts("Observed terrain room order: animation, Draw, activation, cleanup and failure prefixes passed");
}
static void combinedPublicationBoundaries(){
    using Terrain=downwell::GameplayTerrainInstancesRuns;
    for(bool direct:{false,true})for(unsigned lastStage=0;lastStage<=3;++lastStage){
        TerrainObserver observer;Terrain terrain(Terrain::Binding::bindDestruction(observer));
        downwell::RunnerObservedRegistration<2052,2,2050,Terrain> registration;
        downwell::RunnerObservedRoomMembership<2052,2,2050,Terrain> room;
        downwell::RunnerLogicalLayerOrder<2,128,Terrain> layers;
        require(registration.bindTerrain(terrain)&&room.bindTerrain(terrain)&&layers.bindTerrain(terrain));
        unsigned layer;require(layers.insertAfterEquals(1,0,true,layer));
        // The three clocks are independent, not aliases of numeric instance ID.
        require(registration.bind(0,35,7)&&registration.bind(0,35,7));
        require(room.bind(0,{}));
        require(layers.attach(layer,100,true,false,[](unsigned){return false;}));
        for(unsigned i=0;i<32;++i){
            require(terrain.append(60,int(i)*16,32,true));const auto h=terrain.handle(i);
            const unsigned shared=h.slot+2,stage=i==31?lastStage:3;
            // Inherited run metadata is not future membership publication.
            require(!registration.contains(shared)&&!room.contains(shared)&&!terrain.layerState(h)->bound());
            require(!terrain.canContinuePublication(h,1));
            if(stage>=1){require(registration.bindAllocatedTerrain(shared,60,direct&&i));
                require(!room.contains(shared)&&!terrain.layerState(h)->bound());}
            if(stage>=2){room.bindAllocatedTerrain(shared,{},direct&&i);require(!terrain.layerState(h)->bound());}
            if(stage>=3)require(layers.retainAllocated(h.slot,layer,false,direct&&i));
            require(terrain.canContinuePublication(h,31-i)==(stage>=2));
            for(unsigned j=0;j<=i;++j){const auto old=terrain.handle(j);const unsigned completed=j==i?stage:3;
                require(registration.contains(old.slot+2)==(completed>=1));
                require(room.contains(old.slot+2)==(completed>=2));
                const auto state=terrain.layerState(old);
                require(state&&state->bound()==(completed>=3));
                if(completed>=1)require(terrain.registrationOrder(old.slot).low==j+3);
                if(completed>=2)require(terrain.roomState(old.slot).order.low==j+2);
                if(completed>=3)require(state->order.serial.low==j+2);
            }
            require(terrain.ownershipRunCount()==1);
        }
        const auto changed=terrain.handle(11);
        const auto membership=terrain.registrationOrder(changed.slot);
        const auto drawing=*terrain.layerState(changed);
        require(room.mark(changed.slot+2));
        require(terrain.registrationOrder(changed.slot).same(membership)&&terrain.layerState(changed)->same(drawing));
        require(layers.publishInactive(changed,true));
        require(room.marked(changed.slot+2)&&terrain.registrationOrder(changed.slot).same(membership));
        require(registration.unbind(changed.slot+2));
        require(room.marked(changed.slot+2)&&terrain.layerState(changed)->inactive);
        require(terrain.ownershipRunCount()==3);
    }
    std::puts("Combined terrain publication: independent clocks, partial birth visibility and isolated mutations passed");
}
static void runCutoffs(){
    using namespace downwell;
    TerrainObserver observer;
    GameplayTerrainInstancesRuns terrain(GameplayTerrainInstancesRuns::Binding::bindDestruction(observer));
    for(unsigned i=0;i<32;++i){
        require(terrain.append(60,int(i)*16,0,true));const auto h=terrain.handle(i);
        terrain.setLayerState(h.slot,{{{0xfffffff0u+i,7u+(i>=16)},false},17,1,true});
    }
    require(terrain.ownershipRunCount()==1);
    auto compare=[&]{
        for(unsigned layer=0;layer<3;++layer)for(unsigned i=0;i<70;++i){
            RunnerLayerOrderKey after{{0xffffffe8u+i,7u+(i>=24)},bool(i&1)};
            const auto* boundary=i?&after:nullptr;
            bool expected=false,actual=false;RunnerLayerOrderKey a,b;
            terrain.visitLayerStates([&](InstanceHandle,const RunnerRetainedLayerState& state){
                if(state.bound()&&state.inactive&&state.layer==layer&&(!boundary||boundary->before(state.order)))
                    if(!expected||state.order.before(a)){expected=true;a=state.order;}
                return true;
            });
            require(terrain.visitLayerCutoffs(layer,boundary,[&](RunnerLayerOrderKey key){
                if(!actual||key.before(b)){actual=true;b=key;}return true;
            }));
            require(expected==actual&&(!actual||(a.serial.same(b.serial)&&a.tail==b.tail)));
            unsigned calls=0;
            require(terrain.visitLayerCutoffs(layer,boundary,[&](auto){++calls;return false;})==!actual);
            require(calls==unsigned(actual));
        }
    };
    compare();
    for(unsigned i=0;i<32;++i){
        const auto h=terrain.handle(i);auto state=*terrain.layerState(h);
        state.order.tail=(i&1)!=0;state.inactive=(i%3)!=0;state.layer=i%2;
        terrain.setLayerState(h.slot,state);compare();
    }
    while(terrain.size()){
        const auto h=terrain.handle(terrain.size()/2);
        require(terrain.requestDestroy(h)&&terrain.retireMarked(h));compare();
    }
    std::puts("Run-level layer cutoffs: painter directions, carries, exclusions, holes and early stops passed");
}
static void inheritedAdmission(){
    using namespace downwell;
    GameplayTerrainInstancesRuns terrain;
    const int coordinates[]={0,16,32,16,0,32};
    const bool joins[]={false,true,true,false,true,false};
    for(unsigned i=0;i<6;++i){
        require(terrain.appendInitializedCell(60,coordinates[i],0,true,
            [&](InstanceHandle h,auto&,bool&,bool inherited){
                require(inherited==joins[i]);
                require(!terrain.canContinuePublication(h,1));
                return true;
            }));
    }
    const auto h=terrain.handle(5);
    require(terrain.registerInstance(h,60,{0xffffffffu,0xffffffffu}));
    terrain.setRoomState(h.slot,{{1,0},{},0,false});
    require(terrain.canContinuePublication(h,0)&&!terrain.canContinuePublication(h,1));
    require(terrain.registerInstance(h,60,{1,0}));
    terrain.setRoomState(h.slot,{{0xfffffffeu,0xffffffffu},{},0,false});
    require(terrain.canContinuePublication(h,1)&&!terrain.canContinuePublication(h,2));
    terrain.setRoomState(h.slot,{{1,0},{},0,false});
    terrain.setLayerState(h.slot,{{{0xffffffffu,0xffffffffu},false},1,0,false});
    require(terrain.canContinuePublication(h,0)&&!terrain.canContinuePublication(h,1));
    terrain.setLayerState(h.slot,{});require(terrain.canContinuePublication(h,100));
    require(!terrain.canContinuePublication(h,100,true,false));
    require(terrain.dirtyMark<0>(h.slot,{1,0}));
    require(terrain.canContinuePublication(h,100,true,false)&&!terrain.canContinuePublication(h,100,true,true));
    require(terrain.dirtyMark<1>(h.slot,{0xffffffffu,0xffffffffu}));
    require(terrain.canContinuePublication(h,0,true,true)&&!terrain.canContinuePublication(h,1,true,true));
    terrain.dirtyRemove<1>(h.slot);
    terrain.setRoomState(h.slot,{{1,0},{},1,false});require(!terrain.canContinuePublication(h,1));
    terrain.setRoomState(h.slot,{{1,0},{},0,true});require(!terrain.canContinuePublication(h,1));
    require(!terrain.canContinuePublication({h.slot,h.generation+1},0));
    std::puts("Run continuation admission: geometry breaks, unpublished stages and independent clock wrap gates passed");
}
static void constructorPublication(){
    using namespace downwell;
    GameplayTerrainInstancesRuns terrain;
    const TerrainAppearance a{51,{0x3f800000u},2},b{52,{0x40000000u},2};
    InstanceHandle first,second;
    require(terrain.appendInitializedCell(60,0,32,true,[&](auto h,auto&,bool&,bool){
        first=h;terrain.setAppearance(h.slot,a);
        require(terrain.appearance(h.slot).same(a));
        // A nested adjacent birth shares the descriptor. The outer writer
        // must be invalidated before resuming a mutation that splits siblings.
        require(terrain.appendInitializedCell(60,16,32,true,[&](auto next,auto&,bool&,bool){
            second=next;terrain.setAppearance(next.slot,a);return true;
        }));
        terrain.setAppearance(h.slot,b);
        require(terrain.appearance(second.slot).same(a));
        return true;
    }));
    require(terrain.appearance(first.slot).same(b));
    require(terrain.appearance(second.slot).same(a));
    // Failed construction retains its exact published prefix and drops the
    // writer. Subsequent shared-state edits still isolate the sibling.
    GameplayTerrainInstancesRuns failed;
    require(!failed.appendInitializedCell(60,0,0,true,[&](auto h,auto&,bool&,bool){
        failed.setAppearance(h.slot,a);return false;
    }));
    require(failed.appearance(0).same(a));
}
int main(){
    static_assert(sizeof(downwell::PackedWallState)==4,"No extra per-wall cache allocation");
    for(const auto delta:std::vector<std::pair<int,int>>{{0,0},{0,-16},{16,0},{0,16},{-16,0}}){
        downwell::PackedWallState packed;
        require(packed.storeCachedOffset(delta.first,delta.second));
        for(int frame=0;frame<16;++frame){
            downwell::WallEndStepState state;state.imageIndex=frame;state.neighbors=15-frame;
            state.checked=frame%3;state.pillar=frame%3-1;state.lights=frame;
            require(packed.store(state));int x,y;require(packed.cachedOffset(x,y));
            require(x==delta.first&&y==delta.second&&packed.load().imageIndex==frame&&
                packed.load().neighbors==15-frame&&packed.load().pillar==state.pillar);
        }
        const auto before=packed.bits;
        require(!packed.storeCachedOffset(16,16)&&packed.bits==before);
        downwell::NativeTerrainLifetimes<2> live;
        require(live.append(60,240,160,true,100900,676));const auto h=live.handle(0);
        require(live.publishCachedOffset(h,delta.first,delta.second)&&live.createProperties(h,51)&&live.roomCreation(h));
        int x,y;require(live.cachedOffset(h,x,y)&&x==delta.first&&y==delta.second);
        require(live.setWallDeactivation(h,true)&&live.cachedOffset(h,x,y)&&x==delta.first&&y==delta.second);
        require(live.publishRefreshedBounds(h,0x8000)&&live.cachedOffset(h,x,y)&&!x&&!y);
        require(live.publishCachedOffset(h,delta.first,delta.second)&&live.markDestroyed(h)&&live.retire(h));
        require(live.append(60,240,160)&&!live.cachedOffset(h,x,y));
        require(live.cachedOffset(live.handle(0),x,y)&&!x&&!y);
    }
    for(int code:{-1,676,677,678}){
        downwell::NativeTerrainLifetimes<8> live;
        require(live.append(60,240,160,true,100900,code));
        const auto h=live.handle(0);downwell::WallEndStepState state;
        require(!live.initialWallState(h,state));
        require(live.createProperties(h,51)&&live.initialWallState(h,state)&&state.pillar==0);
        require(live.roomCreation(h)&&live.initialWallState(h,state));
        downwell::WallEndStepState expected;require(downwell::wallRoomCreation(code,expected));
        require(state.pillar==expected.pillar&&!state.checked&&!state.surrounded);
        // Spatial refresh must retain the independent room-script stage.
        require(live.publishRefreshedBounds(h,0x8000)&&live.initialWallState(h,state)&&state.pillar==expected.pillar);
        require(live.markDestroyed(h)&&live.retire(h)&&live.append(60,240,160));
        const auto replacement=live.handle(0);
        require(replacement.slot==h.slot&&replacement.generation!=h.generation&&!live.initialWallState(h,state));
        require(live.createProperties(replacement,51)&&live.initialWallState(replacement,state)&&!state.pillar);
    }
    for(unsigned failure=0;failure<3;++failure){
        downwell::NativeTerrainLifetimes<8> live;
        require(live.append(failure==2?93:60,240,160,true,100900,failure==1?679:676));
        const auto h=live.handle(0);
        if(failure)require(live.createProperties(h,51));
        require(!live.roomCreation(h)&&!live.valid());
    }
    {
        struct Clock {unsigned next=10,births=0,marks=0,retired=0;} clock;
        using Native=downwell::NativeTerrainLifetimes<8>;
        Native live;
        Native::Binding binding{&clock,
            [](void* p,unsigned count){return count<=0xffffffffu-static_cast<Clock*>(p)->next;},
            [](void* p,const Native::Identity&,bool,std::uint32_t& order,std::uint32_t&){auto& c=*static_cast<Clock*>(p);order=c.next++;++c.births;return true;},
            [](void* p,unsigned){++static_cast<Clock*>(p)->retired;return true;},
            [](void* p,unsigned){++static_cast<Clock*>(p)->marks;return true;}};
        require(live.bind(binding)&&live.append(60,240,160)&&live.append(60,256,160));
        require(live.runCount()==1&&live.registrationOrder(live.handle(0))==10&&live.registrationOrder(live.handle(1))==11);
        ++clock.next; // A different family is born between adjacent walls.
        require(live.append(60,272,160)&&live.runCount()==2&&live.registrationOrder(live.handle(2))==13);
        const auto old=live.handle(0);
        require(live.markDestroyed(old)&&live.markDestroyed(old)&&clock.marks==1);
        require(live.retire(old)&&clock.retired==1&&live.clear()&&clock.retired==3);
        require(live.bind(binding)&&live.append(60,240,160)&&live.registrationOrder(live.handle(0))==14);
        require(!live.contains(old)&&clock.births==4);
    }
    {
        downwell::NativeTerrainLifetimes<8> live;
        require(live.append(60,240,160,true,100854)&&live.append(60,240,160,false,100855));
        const auto first=live.handle(0),second=live.handle(1);
        require(live.append(60,256,176)&&live.append(60,272,176)&&live.append(60,288,176));
        require(live.size()==5&&live.runCount()==3&&live.canAppend(3)&&!live.canAppend(4));
        downwell::NativeTerrainLifetimes<8>::Identity id;bool active,destroyed;
        require(live.read(second,id,active,destroyed)&&!active&&!destroyed&&id.source==100855);
        require(live.markDestroyed(first)&&live.markDestroyed(first)&&live.read(first,id,active,destroyed)&&destroyed);
        require(live.retire(first)&&!live.read(first,id,active,destroyed));
        require(live.append(93,240,160)&&live.handle(4).slot==first.slot&&live.handle(4).generation!=first.generation);
        require(live.read(second,id,active,destroyed)&&id.source==100855&&!active);
        const auto latest=live.handle(4);
        require(live.clear()&&!live.size()&&!live.read(latest,id,active,destroyed));
        require(live.append(60,240,160)&&live.handle(0).generation>latest.generation);
        for(unsigned i=1;i<8;++i)require(live.append(60,240+int(i)*16,160));
        require(live.runCount()==1&&!live.canAppend(1)&&!live.append(60,0,0)&&!live.valid());
    }
    constructorPublication();
    publicationRuns();
    combinedPublicationBoundaries();
    runCutoffs();
    inheritedAdmission();
    observedRoom();
    observedRegistration();
    initializedBoundary();
    spatialIdentity();
    Store runs;std::vector<Store::Identity> scalar;
    unsigned comparisons=0;
    auto compare=[&](){
        require(runs.size()==scalar.size());unsigned count=0;
        require(runs.visit([&](const auto& r){require(count<scalar.size()&&same(r,scalar[count++]));return true;}));
        require(count==scalar.size());
        require(runs.handle(count).generation==0);
        // Compare run-level row selection with the scalar predicate after
        // every insertion/removal, including fragmented runs and slot reuse.
        for(int first: {-2147483647-1,-16,16,32,48,64,2048,2147483646}){
            const int end=first==2147483646?2147483647:first+1;
            std::vector<Store::Identity> expected;
            for(const auto& r:scalar)
                if(!((r.y<first&&unsigned(first)-unsigned(r.y)>=16u)||
                    (r.y>=end&&unsigned(r.y)-unsigned(end)>=16u)))expected.push_back(r);
            unsigned selected=0;
            require(runs.visitRows(first,end,[&](const auto& r){
                require(selected<expected.size()&&same(r,expected[selected++]));return true;
            }));
            require(selected==expected.size());
            unsigned stopped=0;
            require(runs.visitRows(first,end,[&](const auto&){++stopped;return false;})==expected.empty());
            require(stopped==(expected.empty()?0u:1u));
        }
        require(runs.visitRows(32,32,[&](const auto&){require(false);return false;}));
        require(runs.visitRows(64,32,[&](const auto&){require(false);return false;}));
        for(unsigned i=0;i<count;++i){const auto h=runs.handle(i);
            require(h.slot==scalar[i].handle.slot&&h.generation==scalar[i].handle.generation);
        }
        for(unsigned s=0;s<64;++s){
            auto found=std::find_if(scalar.begin(),scalar.end(),[&](const auto& r){return r.handle.slot==s;});
            require(runs.occupied(s)==(found!=scalar.end()));
            require(runs.object(s)==(found==scalar.end()?-1:found->object));
            const auto current=runs.currentHandle(s);
            require(current.generation==(found==scalar.end()?0:found->handle.generation));
            if(found!=scalar.end()){
                Store::Identity actual;
                require(runs.contains(found->handle)&&runs.read(found->handle,actual)&&same(actual,*found));
                actual.x=12345;
                require(!runs.read({s,found->handle.generation+1},actual)&&actual.x==12345);
            }
        }
        ++comparisons;
    };
    auto append=[&](unsigned slot,unsigned gen,int object,int x,int y,unsigned source=0,int code=-1){
        const bool present=std::any_of(scalar.begin(),scalar.end(),[&](const auto& r){return r.handle.slot==slot;});
        const bool expected=slot<64&&gen&&!present&&(object==60||object==93||object==99);
        require(runs.append({slot,gen},object,x,y,source,code)==expected);
        if(expected)scalar.push_back({{slot,gen},object,x,y,code,source});compare();
    };
    auto retire=[&](downwell::InstanceHandle h){
        const auto found=std::find_if(scalar.begin(),scalar.end(),[&](const auto& r){return r.handle.slot==h.slot&&r.handle.generation==h.generation;});
        require(runs.retire(h)==(found!=scalar.end()));
        if(found!=scalar.end())scalar.erase(found);compare();
    };
    for(unsigned s=0;s<64;++s)append(s,1+s,60,16*int(s),32);
    require(runs.runCount()==1);
    for(unsigned s=1;s<64;s+=2)retire({s,1+s});
    require(runs.runCount()==1); // Destruction does not split survivor geometry.
    for(unsigned s=1;s<64;s+=2)append(s,100+s,93,-16*int(s),48);
    require(runs.size()==64);
    while(!scalar.empty())retire(scalar.front().handle);
    require(!runs.runCount());
    for(unsigned s=0;s<64;++s)append(s,200+s,60,1024-16*int(s),64);
    require(runs.runCount()==1);
    unsigned stopped=0;require(!runs.visit([&](const auto&){return ++stopped!=3;})&&stopped==3);
    while(!scalar.empty())retire(scalar.back().handle);
    // Authored identities and scripts must never be folded into generated runs.
    for(unsigned s=0;s<64;++s)append(s,300+s,60,16*int(s),32,s+1,676);
    require(runs.runCount()==64);
    unsigned random=71913,next=1000;
    for(unsigned i=0;i<20000;++i){
        random=random*1664525u+1013904223u;const unsigned slot=(random>>9)&63;
        auto found=std::find_if(scalar.begin(),scalar.end(),[&](const auto& r){return r.handle.slot==slot;});
        if((random&1)&&found!=scalar.end())retire(found->handle);
        else append(slot,next++,random&2?60:93,int(slot)*16,int((random>>16)&127)*16);
    }
    while(!scalar.empty())retire(scalar.front().handle);
    append(0,0xffffffffu,60,2147483647,0);
    append(1,1,60,(-2147483647-1),0); // No signed coordinate/generation wrap merge.
    append(64,1,60,0,0);append(2,0,60,0,0);append(2,1,35,0,0);
    std::printf("Terrain identity/geometry/order runs: %u scalar comparisons passed\n",comparisons);
    std::printf("Terrain store bytes (host): scalar=%zu runs=%zu\n",
        sizeof(downwell::GameplayTerrainInstancesScalar),sizeof(downwell::GameplayTerrainInstancesRuns));
}

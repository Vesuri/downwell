#include "core/TerrainOwnershipRuns.h"
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
int main(){
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

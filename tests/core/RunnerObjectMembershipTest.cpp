#include "core/RunnerObjectMembership.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
static void require(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
    downwell::RunnerObjectMembership<8> registry;
    downwell::RunnerObjectMembership<8>::RegistrationProjection<2,6> projection;
    // Synthetic immutable hierarchy: objects 1/2 derive from 0; 3 is unrelated.
    auto isA=[](int child,int parent){return parent==0 && (child==1 || child==2);};
    struct Record {unsigned slot;int object;std::uint32_t identity;};
    std::vector<Record> reference;
    std::uint32_t random=17,identity=0;
    for(unsigned step=0;step<8192;++step){
        random=random*1664525u+1013904223u;
        const unsigned slot=(random>>12)%8;
        auto old=std::find_if(reference.begin(),reference.end(),[&](const Record& r){return r.slot==slot;});
        const bool existed=old!=reference.end();
        if(old!=reference.end())reference.erase(old);
        if(random&0x100u){
            const int object=(random>>20)%4;
            require(registry.bind(slot,object,++identity),"bind succeeds for a valid stable slot");
            reference.push_back({slot,object,identity});
        }else require(registry.unbind(slot)==existed,"unbind preserves membership presence");
        require(registry.size()==reference.size(),"registry size follows successful bindings");
        require(!registry.bind(8,0,999) && !registry.bind(slot,-1,999),"invalid binding is rejected atomically");
        for(unsigned first=0;first<=8;++first)for(unsigned last=first;last<=8;++last){
            std::vector<std::uint32_t> expected,actual;
            for(const auto& r:reference)if(r.slot>=first&&r.slot<last)expected.push_back(r.identity);
            require(registry.visitRegistrationRange(first,last,[&](unsigned s,std::uint32_t id,int object){
                auto found=std::find_if(reference.begin(),reference.end(),[&](const Record& r){return r.slot==s;});
                require(found!=reference.end()&&found->object==object,"range retains object identity");
                actual.push_back(id);return true;
            })&&actual==expected,"range filtering preserves source registration order");
            unsigned calls=0;
            const bool complete=registry.visitRegistrationRange(first,last,[&](unsigned,std::uint32_t id,int){
                require(id==expected.front(),"range failure begins at first registration");++calls;return false;
            });
            require(complete==expected.empty()&&calls==(expected.empty()?0u:1u),"range callback failure stops traversal");
            std::vector<std::uint32_t> projected;
            const bool eligible=first==last||last<=2||first>=6;
            const bool projectedOk=registry.visitRegistrationRange(projection,first,last,
                [&](unsigned s,std::uint32_t id,int object){
                    auto found=std::find_if(reference.begin(),reference.end(),[&](const Record& r){return r.slot==s;});
                    require(found!=reference.end()&&found->object==object,"projection retains live resource identity");
                    projected.push_back(id);return true;
                });
            require(projectedOk==eligible&&projected==(eligible?expected:std::vector<std::uint32_t>{}),
                "cached projection matches raw capture or rejects overlapping request");
            if(eligible){
                calls=0;
                const bool done=registry.visitRegistrationRange(projection,first,last,[&](auto,auto,auto){++calls;return false;});
                require(done==expected.empty()&&calls==(expected.empty()?0u:1u),"projected callback failure stops capture");
            }
        }
        require(!registry.visitRegistrationRange(3,2,[](auto,auto,auto){return true;})&&
            !registry.visitRegistrationRange(0,9,[](auto,auto,auto){return true;}),"invalid registration ranges fail");
        for(int object=0;object<4;++object){
            std::vector<std::uint32_t> expected,actual;
            for(const auto& r:reference)if(r.object==object || isA(r.object,object))expected.push_back(r.identity);
            registry.visit(object,isA,[&](unsigned s,std::uint32_t id){
                require(registry.contains(s),"visited slot remains bound");actual.push_back(id);return true;});
            require(actual==expected && registry.count(object,isA)==expected.size(),
                "ancestor and own lists retain registration order across removals and rebinds");
            unsigned calls=0;
            registry.visit(object,isA,[&](unsigned,std::uint32_t id){
                require(!expected.empty() && id==expected.front(),"first hit follows registration order");++calls;return false;});
            require(calls==(expected.empty()?0u:1u),"first-hit callback stops traversal");
        }
    }
    registry.clear();require(!registry.size(),"clear retires all membership");
    unsigned projectedCalls=0;
    require(registry.visitRegistrationRange(projection,0,2,[&](auto,auto,auto){++projectedCalls;return true;})&&
        !projectedCalls,"clear invalidates projected registrations");
    require(registry.bind(0,2,123),"registry reusable after clear");
    registry.clear();require(!registry.unbind(0),"clear removes stale slot links");
    registry.bind(0,1,10);registry.bind(1,2,20);registry.bind(0,1,10);
    std::vector<std::uint32_t> rebound;
    registry.visit(0,isA,[&](unsigned,std::uint32_t id){rebound.push_back(id);return true;});
    require(rebound==std::vector<std::uint32_t>({20,10}),
        "rebinding the same identity moves membership to the ancestor-list tail");
    rebound.clear();require(registry.visitRegistrationRange(projection,0,2,[&](auto,std::uint32_t id,auto){
        rebound.push_back(id);return true;
    })&&rebound==std::vector<std::uint32_t>({20,10}),"projection follows same-identity rebind order");
    for(unsigned repeat=0;repeat<32;++repeat){
        require(registry.bind(0,1,10),"same tail can rebind repeatedly");
        rebound.clear();
        require(registry.visitRegistrationRange(projection,0,2,[&](auto,std::uint32_t id,auto){
            rebound.push_back(id);return true;
        })&&rebound==std::vector<std::uint32_t>({20,10})&&registry.size()==2,
            "same tail rebind preserves links, payload, size and projected order");
    }
    unsigned tailMutations=0;
    require(!registry.visitRegistrationRange(projection,0,2,[&](auto,auto,auto){
        ++tailMutations;registry.bind(0,1,10);return true;
    })&&tailMutations==1,"same tail rebind still invalidates capture revision");
    unsigned mutations=0;
    require(!registry.visitRegistrationRange(projection,0,2,[&](auto,auto,auto){
        ++mutations;registry.bind(7,1,777);return true;
    })&&mutations==1,"capture detects forbidden membership mutation");
    downwell::RunnerObjectMembership<0> empty;
    downwell::RunnerObjectMembership<8>::RegistrationProjection<2,2> unfiltered;
    unsigned allCalls=0;
    require(registry.visitRegistrationRange(unfiltered,0,8,[&](auto,auto,auto){++allCalls;return true;})&&
        allCalls==registry.size(),"empty excluded range projects every registration");
    require(!empty.bind(0,0,1) && !empty.count(0,isA),"zero capacity rejects births");
    std::cout<<"runner object membership tests passed\n";
}

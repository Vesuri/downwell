#include "core/RunnerObjectMembership.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
static void require(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
    downwell::RunnerObjectMembership<8> registry;
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
    require(registry.bind(0,2,123),"registry reusable after clear");
    registry.clear();require(!registry.unbind(0),"clear removes stale slot links");
    registry.bind(0,1,10);registry.bind(1,2,20);registry.bind(0,1,10);
    std::vector<std::uint32_t> rebound;
    registry.visit(0,isA,[&](unsigned,std::uint32_t id){rebound.push_back(id);return true;});
    require(rebound==std::vector<std::uint32_t>({20,10}),
        "rebinding the same identity moves membership to the ancestor-list tail");
    downwell::RunnerObjectMembership<0> empty;
    require(!empty.bind(0,0,1) && !empty.count(0,isA),"zero capacity rejects births");
    std::cout<<"runner object membership tests passed\n";
}

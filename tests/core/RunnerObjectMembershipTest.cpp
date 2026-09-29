#include "core/RunnerObjectMembership.h"
#include "core/RunnerRunMembership.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
#include <type_traits>
static void require(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
template<bool Retained,bool Runs=false> void checkRegistry(){
    using Registry=typename std::conditional<Runs,downwell::RunnerRunMembership<8,Retained?2:0,Retained?6:0>,
        downwell::RunnerObjectMembership<8,Retained?2:0,Retained?6:0>>::type;
    Registry registry;
    typename Registry::template RegistrationProjection<2,6> projection;
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
            if constexpr(Retained){
                std::vector<std::uint32_t> retained;
                const bool ok=registry.visitRetainedRegistrationRange(first,last,
                    [&](unsigned s,std::uint32_t id,int object){
                        auto found=std::find_if(reference.begin(),reference.end(),[&](const Record& r){return r.slot==s;});
                        require(found!=reference.end()&&found->object==object,"retained identity is live");
                        retained.push_back(id);return true;
                    });
                require(ok==eligible&&retained==projected,"maintained order equals rebuilt projection");
                if(eligible){
                    unsigned visits=0;
                    const bool done=registry.visitRetainedRegistrationRange(first,last,[&](auto,auto,auto){++visits;return false;});
                    require(done==expected.empty()&&visits==(expected.empty()?0u:1u),"maintained capture stops on failure");
                }
            }
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
    typename Registry::template RegistrationProjection<2,2> unfiltered;
    unsigned allCalls=0;
    require(registry.visitRegistrationRange(unfiltered,0,8,[&](auto,auto,auto){++allCalls;return true;})&&
        allCalls==registry.size(),"empty excluded range projects every registration");
    require(!empty.bind(0,0,1) && !empty.count(0,isA),"zero capacity rejects births");
    if constexpr(Retained){
        auto copy=registry;
        registry.clear();
        std::vector<std::uint32_t> raw,kept;
        copy.visitRegistrationRange(0,2,[&](auto,auto id,auto){raw.push_back(id);return true;});
        require(copy.visitRetainedRegistrationRange(0,2,[&](auto,auto id,auto){kept.push_back(id);return true;})&&raw==kept,
            "copied owner retains independent actor order");
        unsigned calls=0;
        require(!copy.visitRetainedRegistrationRange(0,2,[&](auto,auto,auto){
            ++calls;copy.bind(3,0,99);return true;
        })&&calls==1,"omitted-domain mutation still invalidates a capture");
        copy.clear();
        require(copy.bind(1,0,101)&&copy.bind(0,0,102),"retained owner reusable after clear");
        calls=0;
        require(!copy.visitRetainedRegistrationRange(0,2,[&](auto,auto,auto){
            ++calls;copy.bind(0,0,102);return true;
        })&&calls==1,"same-tail rebind still invalidates maintained capture");
    }
}
int main(){
    checkRegistry<false>();checkRegistry<true>();
    checkRegistry<false,true>();checkRegistry<true,true>();
    {
        // Exercise long runs, all split positions, re-registration order and
        // identity arithmetic against the original scalar owner.
        downwell::RunnerObjectMembership<128,8,120> scalar;
        downwell::RunnerRunMembership<128,8,120> runs;
        auto compare=[&](){
            std::vector<std::uint32_t> a,b;
            scalar.visitRegistrations([&](auto s,auto id,auto object){a.insert(a.end(),{s,id,unsigned(object)});return true;});
            runs.visitRegistrations([&](auto s,auto id,auto object){b.insert(b.end(),{s,id,unsigned(object)});return true;});
            require(a==b&&scalar.size()==runs.size(),"expanded run history matches scalar registrations");
            for(unsigned s=0;s<128;++s)require(scalar.contains(s)==runs.contains(s),"run locator matches membership");
            auto isA=[](int child,int parent){return parent==0&&(child==1||child==2||child==60);};
            for(int object : {-1,0,1,2,3,60,99}){
                a.clear();b.clear();std::vector<std::uint32_t> c;
                scalar.visit(object,isA,[&](auto s,auto id){a.insert(a.end(),{s,id});return true;});
                runs.visit(object,isA,[&](auto s,auto id){b.insert(b.end(),{s,id});return true;});
                runs.visitReference(object,isA,[&](auto s,auto id){c.insert(c.end(),{s,id});return true;});
                require(a==b&&b==c,"run-selected family preserves full slot/generation order");
                require(scalar.count(object,isA)==runs.count(object,isA)&&
                    runs.count(object,isA)==runs.countReference(object,isA),"run population matches expanded references");
            }
        };
        unsigned random=19;
        for(unsigned round=0;round<64;++round){
            auto clearedReference=runs;
            clearedReference.clearReference();
            scalar.clear();runs.clear();
            require(!runs.size()&&!runs.runCount(),"whole-run clear empties registration");
            for(unsigned s=0;s<128;++s)require(!runs.contains(s),"whole-run clear removes locators");
            for(unsigned s=0;s<128;++s){
                const std::uint32_t id=0xffffffc0u+s; // Includes modulo-32-bit identities.
                require(scalar.bind(s,60,id)&&runs.bind(s,60,id)&&clearedReference.bind(s,60,id),"dense run birth");compare();
                std::vector<std::uint32_t> a,b;
                runs.visitRegistrations([&](auto slot,auto identity,auto object){a.insert(a.end(),{slot,identity,unsigned(object)});return true;});
                clearedReference.visitRegistrations([&](auto slot,auto identity,auto object){b.insert(b.end(),{slot,identity,unsigned(object)});return true;});
                require(a==b&&runs.runCount()==clearedReference.runCount(),"run clear and scalar clear refill identically");
            }
            for(unsigned step=0;step<512;++step){
                random=random*1664525u+1013904223u;const unsigned s=(random>>8)&127;
                if(random&1)require(scalar.unbind(s)==runs.unbind(s),"split removal result");
                else require(scalar.bind(s,int((random>>16)&3),random)==runs.bind(s,int((random>>16)&3),random),"split rebind result");
                compare();
            }
        }
        downwell::RunnerRunMembership<0,0,0> empty;
        require(!empty.bind(0,0,0)&&!empty.unbind(0)&&empty.size()==0,"empty run owner");
        std::cout<<"Registration storage (128 slots): scalar="<<sizeof(scalar)<<" runs="<<sizeof(runs)<<'\n';
    }
    {
        // Cold terrain query: reject unrelated runs without walking cells,
        // count matching runs without expanding them, and preserve every
        // possible early-stop prefix (including generation wrap).
        downwell::RunnerRunMembership<2048,0,2048> runs;
        for(unsigned s=0;s<2048;++s)require(runs.bind(s,s<1024?60:93,0xfffffc00u+s),"cold query birth");
        unsigned selectedTests=0,referenceTests=0;
        auto selected=[&](int child,int parent){++selectedTests;return parent==59&&child==60;};
        auto reference=[&](int child,int parent){++referenceTests;return parent==59&&child==60;};
        require(runs.count(59,selected)==1024&&runs.countReference(59,reference)==1024,
            "ancestor population expands no instances");
        require(selectedTests==2&&referenceTests==2048,"ancestry checks scale with runs rather than cells");
        for(unsigned stop=1;stop<=1025;++stop){
            std::vector<std::uint32_t> a,b;unsigned na=0,nb=0;
            runs.visit(59,selected,[&](auto s,auto id){a.insert(a.end(),{s,id});return ++na<stop;});
            runs.visitReference(59,reference,[&](auto s,auto id){b.insert(b.end(),{s,id});return ++nb<stop;});
            require(a==b,"every ancestor-query stop prefix remains identical");
        }
        selectedTests=referenceTests=0;unsigned callbacks=0;
        runs.visit(99,selected,[&](auto,auto){++callbacks;return true;});
        runs.visitReference(99,reference,[&](auto,auto){++callbacks;return true;});
        require(!callbacks&&selectedTests==2&&referenceTests==2048,"unrelated family skips complete terrain runs");
    }
    {
        downwell::RunnerRunMembership<64,8,56> runs;
        for(unsigned i=8;i<56;++i)require(runs.bind(i,60,100+i),"terrain run append");
        require(runs.runCount()==1,"48 terrain registrations share one descriptor");
        for(unsigned i=9;i<56;i+=2)require(runs.unbind(i),"interior removal splits without reordering");
        require(runs.size()==24&&runs.runCount()==24,"split every remaining terrain cell");
        for(unsigned i=0;i<64;++i)require(runs.bind(i,60,200+i),"full capacity after fragmentation");
        require(runs.size()==64,"descriptor reserve preserves full slot capacity");
        runs.clear();require(runs.size()==0&&runs.runCount()==0,"release fragmented runs");
    }
    downwell::RunnerObjectMembership<8,0,8> omitted;
    require(omitted.bind(3,0,1)&&omitted.unbind(3),"fully omitted domain needs no retained entries");
    std::cout<<"runner object membership tests passed\n";
}

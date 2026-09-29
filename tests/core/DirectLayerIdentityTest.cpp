#include "core/RunnerLayerInstances.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
int main(){
    using Hash=RunnerLayerInstances<1,32>;
    using Direct=RunnerLayerInstances<1,32,48>;
    Hash hash;Direct direct;Hash::Layers a,b;
    RunnerDepthService::Instance sa[48]{},sb[48]{};
    unsigned generation[48]{};bool live[48]{};
    unsigned random=17;
    auto compare=[&](){
        for(unsigned e=0;e<32;++e){
            const auto* x=hash.get(e);const auto* y=direct.get(e);
            assert(bool(x)==bool(y)&&a.elementOwner(e)==b.elementOwner(e));
            if(x)assert(x->sourceId==y->sourceId&&x->instance.slot==y->instance.slot&&
                x->instance.generation==y->instance.generation);
        }
        for(unsigned s=0;s<48;++s){
            assert(hash.find(InstanceHandle{s,generation[s]})==direct.find(InstanceHandle{s,generation[s]}));
            assert(direct.find(InstanceHandle{s,generation[s]+1})==Direct::End);
            if(generation[s])assert(direct.find(InstanceHandle{s,generation[s]-1})==Direct::End);
            assert(hash.find(100+s)==direct.find(100+s));
            assert(sa[s].attached==sb[s].attached&&sa[s].layerId==sb[s].layerId&&
                sa[s].depth.bits==sb[s].depth.bits&&sa[s].append==sb[s].append);
        }
        std::vector<unsigned> x,y;
        assert(a.visitElements(0,[&](unsigned e){x.push_back(e);return true;}));
        assert(b.visitElements(0,[&](unsigned e){y.push_back(e);return true;}));
        assert(x==y);
    };
    for(unsigned room=0;room<2;++room){
        unsigned la,lb;
        assert(a.insertAfterEquals(17,0,false,la)&&b.insertAfterEquals(17,0,false,lb)&&la==lb);
        for(unsigned s=0;s<4;++s)assert(hash.reserve(a,la,100+s)&&direct.reserve(b,lb,100+s));
        for(unsigned step=0;step<4096;++step){
            random=random*1664525u+1013904223u;
            const unsigned slot=(random>>12)%48;const InstanceHandle old{slot,generation[slot]};
            if(live[slot]){
                if(random&0x100u){
                    assert(hash.retire(a,100+slot,old)&&direct.retire(b,100+slot,old));
                    sa[slot]={};sb[slot]={};live[slot]=false;
                }else{
                    assert(a.reconcileInstanceActivation(hash.find(old),random&0x200u));
                    assert(b.reconcileInstanceActivation(direct.find(old),random&0x200u));
                    RunnerDepthService::Instance duplicate;
                    assert(!direct.attach(b,lb,0,{slot,generation[slot]+1},duplicate,false)&&!duplicate.attached);
                }
            }else{
                const InstanceHandle next{slot,++generation[slot]};
                sa[slot]={};sb[slot]={};sa[slot].append=sb[slot].append=random&0x200u;
                const bool ok=hash.attach(a,la,100+slot,next,sa[slot],random&0x400u);
                assert(ok==direct.attach(b,lb,100+slot,next,sb[slot],random&0x400u));
                live[slot]=ok;
            }
            compare();
        }
        auto lookupA=[&](unsigned id,InstanceHandle& h,RunnerDepthService::Instance*& state){
            const unsigned slot=id-100;assert(slot<48);h={slot,generation[slot]};state=&sa[slot];return true;
        };
        auto lookupB=[&](unsigned id,InstanceHandle& h,RunnerDepthService::Instance*& state){
            const unsigned slot=id-100;assert(slot<48);h={slot,generation[slot]};state=&sb[slot];return true;
        };
        for(unsigned pass=0;pass<2;++pass){
            for(unsigned e=0;e<32;++e)if(hash.get(e)){
                assert(hash.uninitialize(a,e,lookupA)&&direct.uninitialize(b,e,lookupB));compare();
                if(!pass){
                    assert(hash.initializeReserved(a,e,lookupA)&&direct.initializeReserved(b,e,lookupB));compare();
                }
            }
        }
        RunnerDepthService::Instance outside;
        assert(!direct.attach(b,lb,0,{48,1},outside,false)&&!outside.attached);
        assert(direct.find(InstanceHandle{48,1})==Direct::End);
        assert(hash.discardUninitialized()&&direct.discardUninitialized());
        assert(a.discardUninitialized()&&b.discardUninitialized());
        for(auto& v:live)v=false;
    }
    static_assert(sizeof(RunnerLayerInstances<64,3323,3067>)<sizeof(RunnerLayerInstances<64,3323>));
    std::printf("Direct layer identities: 8192 mutations, placeholders, capacity, activation, rebind, generations and room reset match hash reference\n");
}

#include "core/TerrainPublicationClock.h"
#include "core/RunnerLogicalLayerOrder.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;

static void counters(){
    for(unsigned failedPrefix=0;failedPrefix<17;++failedPrefix){
        TerrainPublicationClock room,layer;
        RunnerRegistrationOrder roomReference,layerReference;
        unsigned terrain=0,random=17291;
        // Exercise carries without billions of publications. Both clocks
        // receive the same already-published ordinary prefix as the oracle.
        room.advanceOther(0xfffffff0u);roomReference.low=0xfffffff0u;
        layer.advanceOther(0xffffffe0u);layerReference.low=0xffffffe0u;
        for(unsigned event=0;event<4096;++event){
            random=random*1664525u+1013904223u;
            switch((random>>16)%3){
            case 0:{
                const unsigned requested=(random>>24)%17;
                const unsigned completed=requested<failedPrefix?requested:failedPrefix;
                const auto roomBasis=room.basis(),layerBasis=layer.basis();
                for(unsigned i=0;i<completed;++i){
                    ++terrain;roomReference.advance();layerReference.advance();
                    assert(TerrainPublicationClock::serial(roomBasis,terrain).same(roomReference));
                    assert(TerrainPublicationClock::serial(layerBasis,terrain).same(layerReference));
                }
                // A failed suffix is not published. Reading the current
                // state sees precisely the synchronous successful prefix.
                assert(room.current(terrain).same(roomReference));
                assert(layer.current(terrain).same(layerReference));
                break;
            }
            case 1:roomReference.advance();assert(room.advance(terrain).same(roomReference));break;
            case 2:layerReference.advance();assert(layer.advance(terrain).same(layerReference));break;
            }
        }
    }
}

// The candidate stores source facts only. It constructs the compatibility
// state on a read, or retains a changed value at an actual layer mutation.
// This fixture exercises the real logical-layer consumer, not only a model
// of integer addition. Production source-record storage is a separate step.
template<bool Facts> struct Terrain {
    static constexpr unsigned capacity=32;
    struct Birth {RunnerRegistrationOrder basis;unsigned ordinal=0,layer=0;int id=0;};
    Birth births[capacity]{};
    RunnerRetainedLayerState retained[capacity]{};
    bool changed[capacity]{};
    unsigned published=0;
    unsigned sourcePublicationCount()const{return Facts?published:0;}
    RunnerRetainedLayerRead layerState(unsigned slot)const{
        if(slot>=capacity)return nullptr;
        if constexpr(Facts){
            const auto& b=births[slot];
            if(b.ordinal&&!changed[slot])return RunnerRetainedLayerState{
                {TerrainPublicationClock::serial(b.basis,b.ordinal),false},b.id,b.layer,false};
        }
        return retained[slot];
    }
    RunnerRetainedLayerRead layerState(InstanceHandle h)const{return h.generation==1?layerState(h.slot):nullptr;}
    void setLayerState(unsigned slot,const RunnerRetainedLayerState& s){assert(slot<capacity);changed[slot]=true;retained[slot]=s;}
    void publishInheritedLayer(unsigned){assert(false);}
    template<class Visit> bool visitLayerStates(Visit visit)const{
        for(unsigned slot=0;slot<capacity;++slot)if(!visit(InstanceHandle{slot,1},*layerState(slot)))return false;
        return true;
    }
    template<class Visit> bool visitLayerCutoffs(unsigned layer,const RunnerLayerOrderKey* after,Visit visit)const{
        return visitLayerStates([&](InstanceHandle,const RunnerRetainedLayerState& s){
            return !s.bound()||s.layer!=layer||!s.inactive||(after&&!after->before(s.order))||visit(s.order);
        });
    }
};

static void layerConsumer(){
    Terrain<true> source;Terrain<false> reference;
    RunnerLogicalLayerOrder<4,96,Terrain<true>,true> actual;
    RunnerLogicalLayerOrder<4,96,Terrain<false>> oracle;
    assert(actual.bindTerrain(source)&&oracle.bindTerrain(reference));
    unsigned layer,other;
    assert(actual.insertAfterEquals(31,100,true,layer));
    assert(oracle.insertAfterEquals(31,100,true,other)&&layer==other);
    unsigned born=0,actors=0;
    auto compare=[&]{
        assert(actual.elementCount(layer)==oracle.elementCount(layer));
        std::vector<unsigned> a,b;
        assert(actual.visitLogical(layer,[&](bool native,unsigned n){a.push_back(n+(native?96:0));return true;}));
        assert(oracle.visitLogical(layer,[&](bool native,unsigned n){b.push_back(n+(native?96:0));return true;}));
        assert(a==b);
        for(unsigned n=0;n<born;++n)assert(source.layerState(n)->same(*reference.layerState(n)));
        for(unsigned n=0;n<actors;++n){const auto x=actual.physicalOrder(n),y=oracle.physicalOrder(n);
            assert(x.serial.same(y.serial)&&x.tail==y.tail);
        }
    };
    for(unsigned group=0;group<8;++group){
        const unsigned count=1+group%4;
        const auto basis=actual.sourcePublicationBasis();
        assert(actual.retainSourceCount(layer,count));
        RunnerRetainedLayerState first;
        assert(oracle.retainConstructedSpan(layer,count,first));
        for(unsigned i=0;i<count;++i){
            source.births[born+i]={basis,source.published+i+1,layer,31};
            reference.retained[born+i]=first.shifted(i);
        }
        source.published+=count;born+=count;compare();
        assert(actual.attach(layer,actors,true,false,[](unsigned){return false;}));
        assert(oracle.attach(layer,actors,true,false,[](unsigned){return false;}));
        ++actors;compare();
        const InstanceHandle changed{born-1,1};
        assert(actual.publishInactive(changed,true)&&oracle.publishInactive(changed,true));compare();
        assert(actual.reconcileRetained(changed)&&oracle.reconcileRetained(changed));compare();
        assert(actual.publishInactive(changed,false)&&oracle.publishInactive(changed,false));
        assert(actual.reconcileRetained(changed)&&oracle.reconcileRetained(changed));compare();
    }
    for(unsigned n=0;n<born;n+=2){
        assert(actual.releaseInstance({n,1})&&oracle.releaseInstance({n,1}));compare();
    }
}
int main(){
    counters();layerConsumer();
    std::puts("Terrain source chronology: interleavings, failed prefixes, carries and live layer mutations match the retained oracle");
}

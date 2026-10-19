#include "core/TerrainBirthRecords.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
struct State {
    static unsigned live;
    unsigned value;
    explicit State(unsigned n):value(n){++live;}
    State(const State& s):value(s.value){++live;}
    ~State(){--live;}
};
unsigned State::live=0;
using Store=TerrainBirthRecords<64,State>;
struct Expected {InstanceHandle h;unsigned registration,ordinal,value;bool changed;};
static TerrainBirthFacts facts(const unsigned char* bytes,unsigned slot,unsigned generation,unsigned registration){
    return {bytes,slot,generation,registration,160,51,17,2,0xfffffff0u,3,0xffffffe0u,7,0x42c80000u,{4,4}};
}
static void compare(const Store& store,const std::vector<Expected>& expected){
    for(unsigned slot=0;slot<=64;++slot){
        for(unsigned count:{0u,1u,2u,7u,32u,64u,65u})
            assert(store.canAppend({slot,0xffff0000u},count)==store.canAppendReference({slot,0xffff0000u},count));
        assert(store.lookupMatchesReference({slot,0}));
        assert(store.lookupMatchesReference(store.currentHandle(slot)));
    }
    assert(store.size()==expected.size());unsigned n=0,changed=0;
    assert(store.visit([&](InstanceHandle h,const Store::Read& read){
        const auto& e=expected.at(n++);
        assert(h.slot==e.h.slot&&h.generation==e.h.generation);
        assert(read.registration()==e.registration&&read.ordinal==e.ordinal);
        assert(read.object()==((h.slot&1)?93:60)&&read.x()==int(h.slot)*16);
        assert(read.layerSerial().same(TerrainPublicationClock::serial({0xfffffff0u,3},e.ordinal)));
        assert(read.roomSerial().same(TerrainPublicationClock::serial({0xffffffe0u,7},e.ordinal)));
        const auto* state=store.mutation(h);assert(bool(state)==e.changed);
        if(state){assert(state->value==e.value);++changed;}
        const auto current=store.currentHandle(h.slot);
        assert(current.slot==h.slot&&current.generation==h.generation);
        return true;
    }));
    assert(n==expected.size()&&State::live==changed);
}
int main(){
    unsigned char bytes[128];
    for(unsigned i=0;i<64;++i){bytes[2*i]=static_cast<unsigned char>(32+i+((i&1)?128:0));bytes[2*i+1]=0;}
    {
        Store store;
        assert(store.reserve({4,1},3));
        assert(store.canAppend({4,1},3)&&store.canAppendReference({4,1},3));
        store.abandon();
        assert(store.append(facts(bytes+8,4,1,100),3));
        assert(store.retire({5,2}));
        assert(store.canAppend({5,4},1)&&store.canAppendReference({5,4},1));
        assert(!store.canAppend({5,4},2)&&!store.canAppendReference({5,4},2));
        for(unsigned generation:{0u,1u,3u,4u,0xfffffffeu,0xffffffffu})
            for(unsigned count:{0u,1u,2u,64u,65u})
                assert(store.canAppend({0,generation},count)==store.canAppendReference({0,generation},count));
        store.clear();
        assert(store.canAppend({4,4},3)&&store.canAppendReference({4,4},3));
    }
    // Every successful prefix is immediately visible; unused suffix cells
    // have no identity, mutable state or consumer publication serial.
    for(unsigned prefix=1;prefix<=64;++prefix){
        Store store;assert(!State::live);
        assert(store.append(facts(bytes,0,1,100),prefix));
        std::vector<Expected> expected;
        for(unsigned i=0;i<prefix;++i)expected.push_back({{i,i+1},100+i,i+1,0,false});
        compare(store,expected);assert(!State::live);
        assert(!store.append(facts(bytes,0,1000,2000),1)); // No overlapping ownership.
        assert(!store.currentHandle(prefix).generation);
    }
    {
        Store store;std::vector<Expected> expected;
        assert(store.append(facts(bytes,0,1,100),64));
        for(unsigned i=0;i<64;++i)expected.push_back({{i,i+1},100+i,i+1,0,false});
        unsigned generation=65,registration=164,random=31873,initialCalls=0;
        std::vector<InstanceHandle> retired;
        for(unsigned iteration=0;iteration<12000;++iteration){
            random=random*1664525u+1013904223u;
            const unsigned slot=(random>>16)&63;
            const auto current=store.currentHandle(slot);
            if(!current.generation){
                assert(store.append(facts(bytes+2*slot,slot,generation,registration),1));
                expected.push_back({{slot,generation++},registration++,store.sourcePublicationCount(),0,false});
            }else{
                unsigned index=0;while(expected[index].h.slot!=slot)++index;
                auto& e=expected[index];
                if(random&1){
                    const auto before=initialCalls;
                    auto* value=store.materialize(current,[&](const Store::Read& source){++initialCalls;return State(source.registration());});
                    assert(value&&initialCalls==before+(e.changed?0:1));
                    if(!e.changed)assert(value->value==e.registration);
                    e.value=value->value=random;e.changed=true;
                }else{
                    assert(store.retire(current)&&!store.retire(current));
                    retired.push_back(current);expected.erase(expected.begin()+index);
                }
            }
            compare(store,expected);
            // Slot reuse must never resurrect an earlier generation even
            // while other cells in that old immutable segment remain live.
            for(const auto& h:retired){Store::Read read;
                assert(store.lookupMatchesReference(h)&&!store.read(h,read)&&!store.mutation(h));}
        }
        const auto published=store.sourcePublicationCount();store.clear();
        assert(!store.size()&&!State::live&&store.sourcePublicationCount()==published);
        assert(store.append(facts(bytes,0,generation,registration),1));
        Store::Read read;assert(store.read({0,generation},read)&&read.ordinal==published+1);
        assert(!store.append(facts(bytes+2,1,generation,registration+1),1));
    }
    assert(!State::live);
    std::puts("Source terrain records: no mutable births; exact prefix/order reads, mutation, retirement, stale handles and reuse passed");
}

#include "core/RunnerSpatialDirtyQueue.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>
int main(){
    using Queue=downwell::RunnerSpatialDirtyQueue<16>;
    Queue queue;std::vector<unsigned> expected;
    auto check=[&](){
        assert(queue.size()==expected.size());unsigned slot=queue.first();
        for(auto item:expected){assert(slot==item);slot=queue.next(slot);}
        assert(slot==Queue::End);
        for(unsigned i=0;i<16;++i)assert(queue.contains(i)==(std::find(expected.begin(),expected.end(),i)!=expected.end()));
    };
    std::uint32_t state=0x27d3ab49;
    for(unsigned i=0;i<65536;++i){
        state=state*1664525u+1013904223u;unsigned slot=(state>>16)%18;
        auto it=std::find(expected.begin(),expected.end(),slot);
        if(state&1){
            assert(queue.mark(slot)==(slot<16));
            if(slot<16){if(it!=expected.end())expected.erase(it);expected.insert(expected.begin(),slot);}
        }else{
            assert(queue.remove(slot)==(it!=expected.end()));
            if(it!=expected.end())expected.erase(it);
        }
        check();
        if(i%1024==0){queue.clear();expected.clear();check();}
    }
    queue.clear();assert(queue.mark(1)&&queue.mark(2)&&queue.mark(3)&&queue.mark(1));
    expected={1,3,2};check(); // Repeated mutation moves the instance to the head.
    assert(queue.remove(3));assert(queue.mark(3));expected={3,1,2};check();
    for(unsigned object=0;object<64;++object)for(unsigned instance=0;instance<64;++instance){
        bool queueExpected=bool(object&8)||bool(object&32)||bool(instance&32);
        assert(!Queue::shouldQueue(false,object,instance));
        assert(Queue::shouldQueue(true,object,instance)==queueExpected);
        for(bool all:{false,true})for(unsigned extra:{0u,0x100000u,0x8000u,0x80000u}){
            unsigned flags=instance|extra;
            bool update=!(flags&1)&&!(flags&2)&&!(flags&0x100000)&&bool(object&16)&&
                (all||bool(object&8)||bool(object&32)||bool(flags&32));
            assert(Queue::shouldUpdateSpriteIndex(object,flags,all)==update);
        }
    }
    std::puts("Spatial dirty queue: 65536 vector comparisons, move-to-front/retire/reuse and 32768 sprite eligibility combinations passed");
}

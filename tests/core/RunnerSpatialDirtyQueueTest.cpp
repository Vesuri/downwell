#include "core/RunnerSpatialDirtyQueue.h"
#include "core/RunnerSpatialBoundsUpdate.h"
#include "core/RunnerSpatialIndex.h"
#include "core/RunnerSpatialPosition.h"
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
            bool publish=(bool(flags&0x8000)||(!(flags&1)&&!(flags&2)&&!(flags&0x100000)))&&bool(object&16)&&
                (all||bool(object&8)||bool(object&32)||bool(flags&32));
            assert(Queue::shouldPublishBounds(object,flags,all)==publish);
        }
    }
    using Update=downwell::RunnerSpatialBoundsUpdate;
    downwell::RunnerSpatialIndex<8> index;
    std::uint32_t flags[3]={};
    for(unsigned id=0;id<3;++id)assert(Update::publish(index,id,0x30,flags[id],false,{},{}));
    for(auto flag:flags)assert(flag==0x8000);
    flags[0]|=0x100003;
    assert(!Queue::shouldUpdateSpriteIndex(0x30,flags[0],false));
    assert(Update::publish(index,0,0x30,flags[0],false,{},{}));
    unsigned order[]={2,1,0},cursor=0;
    assert(index.query({},[&](unsigned id){assert(id==order[cursor++]);return true;}));assert(cursor==3);
    // No cached-bounds equality shortcut: publication still changes leaf order.
    assert(Update::publish(index,1,0x30,flags[1],false,{},{}));
    unsigned second[]={2,0,1};cursor=0;
    assert(index.query({},[&](unsigned id){assert(id==second[cursor++]);return true;}));assert(cursor==3);
    std::uint32_t absent=0x8000;
    assert(Update::publish(index,99,0x30,absent,false,{},{})); // Not-found then insert.
    using Real=downwell::RunnerImageValue;
    Update::Box reversed{Real{0x40000000},Real{0x40000000},Real{},Real{}};
    assert(Update::publish(index,99,0x30,absent,false,{},reversed));
    unsigned hit=0;
    assert(index.query({Real{0x3f800000},Real{0x3f800000},Real{0x3f800000},Real{0x3f800000}},
        [&](unsigned id){assert(id==99);++hit;return true;}));assert(hit==1);
    Update::Box invalid{};invalid.left.bits=0x7fc00000;
    std::uint32_t skipped=1;
    assert(Update::publish(index,100,0x30,skipped,false,invalid,invalid)&&skipped==1);
    assert(!Update::publish(index,99,0x30,absent,false,{},invalid)&&absent==0x8000);
    Queue spatial,collision;
    Real x{},y{},one{0x3f800000};std::uint32_t positionFlags=0;
    using Position=downwell::RunnerSpatialPosition;
    assert(Position::assign(1,x,y,positionFlags,one,one,false,true,0x1a,spatial,collision));
    assert(x.bits==one.bits&&y.bits==one.bits&&positionFlags==8);
    assert(spatial.first()==1&&collision.first()==1);
    assert(spatial.mark(2)&&collision.mark(2));
    assert(Position::assign(1,x,y,positionFlags,one,one,false,true,0x1a,spatial,collision));
    assert(spatial.first()==2&&collision.first()==2); // Ordinary no-op preserves ordering.
    assert(Position::assign(1,x,y,positionFlags,one,one,true,true,0x1a,spatial,collision));
    assert(spatial.first()==1&&collision.first()==1); // Special state bypasses comparison.
    x={};y={};positionFlags=0;spatial.clear();collision.clear();
    assert(Position::assign(1,x,y,positionFlags,Real{0x80000000},y,false,true,0x1a,spatial,collision));
    assert(x.bits==0&&positionFlags==0&&spatial.size()==0); // SSE +0 equals -0.
    assert(Position::assign(1,x,y,positionFlags,one,y,false,true,0x12,spatial,collision));
    assert(positionFlags==8&&spatial.size()==0&&collision.size()==0); // Bounds dirty without queue eligibility.
    positionFlags|=0x20;
    assert(Position::assign(1,x,y,positionFlags,{},y,false,true,0x12,spatial,collision));
    assert(spatial.first()==1&&collision.size()==0); // Instance opt-in is spatial only.
    Real before=x;auto beforeFlags=positionFlags;
    assert(!Position::assign(Queue::End,x,y,positionFlags,one,one,false,true,0x1a,spatial,collision));
    assert(x.bits==before.bits&&positionFlags==beforeFlags&&spatial.first()==1);
    assert(!Position::assign(1,x,y,positionFlags,Real{0x7f800000},one,false,true,0x1a,spatial,collision));
    assert(x.bits==before.bits&&positionFlags==beforeFlags);
    std::puts("Spatial mutation: no-op/special position, dual memberships, bounds publication; 65536 queue comparisons and 32768 eligibility combinations passed");
}

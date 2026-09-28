#include "core/RunnerSpatialDirtyQueue.h"
#include "core/RunnerSpatialBoundsUpdate.h"
#include "core/RunnerSpatialIndex.h"
#include "core/RunnerSpatialPosition.h"
#include "core/RunnerSpatialObjectPreparation.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <vector>
int main(){
    {
        using Prepare=downwell::RunnerSpatialObjectPreparation;
        using Path=Prepare::Path;
        auto number=[](int n){downwell::RunnerImageValue v;assert(v.assign(downwell::MotionBinary64::fromInteger(n)));return v;};
        const Prepare::Box reversed{number(4),number(6),number(0),number(0)};
        for(unsigned population=0;population<8;++population)
        for(unsigned height=0;height<8;++height)
        for(unsigned object=0;object<64;++object)
        for(unsigned all=0;all<2;++all){
            unsigned marked=0,visited=0;
            const auto path=Prepare::prepare(population,height,object,all,
                [&]{++marked;return true;},[&](auto){++visited;},
                [](auto,auto&,auto&){assert(false);return false;},
                [](auto,auto){assert(false);return false;});
            assert(path==(!population?Path::none:population<height?Path::list:Path::spatial));
            const bool needs=population && population>=height && !(object&0x28) && !all;
            assert(marked==needs && visited==needs);
        }
        const std::uint32_t excluded[]={1,2,0x8000,0x100000};
        for(auto flags:excluded){
            auto box=reversed;
            assert(Prepare::prepare(1,0,0,false,[]{return true;},[&](auto emit){emit(1,flags,box);},
                [](auto,auto&,auto&){assert(false);return false;},
                [](auto,auto){assert(false);return false;})==Path::spatial);
        }
        downwell::RunnerSpatialIndex<8> index;
        std::uint32_t flags[3]={8,0,2};
        Prepare::Box boxes[3]={reversed,reversed,reversed};
        std::vector<unsigned> calls;
        auto prepare=[&](bool fail){return Prepare::prepare(3,index.height(),0,false,
            [&]{calls.push_back(100);return true;},
            [&](auto emit){for(unsigned i=0;i<3;++i)if(!emit(i,flags[i],boxes[i]))break;},
            [&](unsigned i,auto& f,auto&){calls.push_back(10+i);f&=~8u;return true;},
            [&](auto box,unsigned i){calls.push_back(i);return !fail && index.insert(box,i);});};
        assert(prepare(false)==Path::spatial);
        assert((calls==std::vector<unsigned>{100,10,0,1}));
        assert(flags[0]==0x8000 && flags[1]==0x8000 && flags[2]==2);
        std::vector<unsigned> hits;
        assert(index.query({number(0),number(0),number(4),number(6)},[&](unsigned i){hits.push_back(i);return true;}));
        assert((hits==std::vector<unsigned>{0,1}));
        calls.clear();assert(prepare(false)==Path::spatial);
        assert((calls==std::vector<unsigned>{100})); // Already indexed: no duplicate insert.
        flags[0]=8;calls.clear();assert(prepare(true)==Path::failed);
        assert(!(flags[0]&0x8000) && (calls==std::vector<unsigned>{100,10,0}));
        assert(Prepare::prepare(1,0,0,false,[]{return false;},
            [](auto){assert(false);},[](auto,auto&,auto&){return true;},
            [](auto,auto){return true;})==Path::failed);
        auto cached=reversed;std::uint32_t dirty=8;
        assert(Prepare::prepare(1,0,0,false,[]{return true;},
            [&](auto emit){emit(0,dirty,cached);},[](auto,auto&,auto&){return false;},
            [](auto,auto){assert(false);return true;})==Path::failed);
        assert(dirty==8);
        cached.left.bits=0x7f800000u;dirty=0;
        assert(Prepare::prepare(1,0,0,false,[]{return true;},
            [&](auto emit){emit(0,dirty,cached);},[](auto,auto&,auto&){assert(false);return true;},
            [](auto,auto){assert(false);return true;})==Path::failed);
        assert(dirty==0);
    }
    using Queue=downwell::RunnerSpatialDirtyQueue<16>;
    {
        Queue pending;
        std::uint32_t flags[6]={0,8,1,2,0x100000,0x8000};
        for(unsigned i=6;i-->0;)assert(pending.mark(i));
        std::vector<unsigned> refreshed;
        assert(pending.flush([&](unsigned i){return flags[i];},[&](unsigned i){
            assert(pending.contains(i));refreshed.push_back(i);return true;
        }));
        assert((refreshed==std::vector<unsigned>{0,1,5}) && pending.size()==0);
        // Read next after refresh, not before. Moving 1 ahead of the current
        // entry leaves it for the next flush; the original successor becomes 2.
        assert(pending.mark(2)&&pending.mark(1)&&pending.mark(0));
        refreshed.clear();
        assert(pending.flush([](unsigned){return 0u;},[&](unsigned i){
            refreshed.push_back(i);if(i==0)assert(pending.mark(1));return true;
        }));
        assert((refreshed==std::vector<unsigned>{0,2}));
        assert(pending.size()==1 && pending.first()==1);
        assert(pending.mark(0));refreshed.clear();
        assert(!pending.flush([](unsigned){return 0u;},[&](unsigned i){
            refreshed.push_back(i);return i!=1;
        }));
        assert((refreshed==std::vector<unsigned>{0,1}));
        assert(pending.size()==1 && pending.first()==1);
        pending.clear();
        assert(pending.flush([](unsigned){assert(false);return 0u;},
            [](unsigned){assert(false);return false;}));
        // Clean entries still refresh and republish unchanged bounds. Removing
        // and appending changes first-hit order even though geometry is equal.
        downwell::RunnerSpatialIndex<8> tree;
        using Update=downwell::RunnerSpatialBoundsUpdate;
        Update::Box boxes[3]={};std::uint32_t indexed[3]={};
        for(unsigned i=0;i<3;++i)
            assert(Update::publish(tree,i,0x30,indexed[i],false,{},{}));
        assert(pending.mark(0));
        assert(pending.flush([&](unsigned i){return indexed[i];},[&](unsigned i){
            return Update::finishRefresh(tree,i,0x30,indexed[i],false,boxes[i],
                &boxes[i],{}, {},false,true);
        }));
        std::vector<unsigned> hits;
        assert(tree.query({},[&](unsigned i){hits.push_back(i);return true;}));
        assert((hits==std::vector<unsigned>{2,1,0}));
        // No publication eligibility: refresh still clears the dirty bit and
        // resolves missing-resource bounds instead of silently skipping it.
        indexed[1]=8;assert(pending.mark(1));
        assert(pending.flush([&](unsigned i){return indexed[i];},[&](unsigned i){
            return Update::finishRefresh(tree,i,0,indexed[i],false,boxes[i],
                nullptr,{0x3f800000},{},false,true);
        }));
        assert(indexed[1]==0 && boxes[1].left.bits==0x3f800000);
    }
    Queue queue;std::vector<unsigned> expected;
    auto check=[&](){
        assert(queue.size()==expected.size());unsigned slot=queue.first();
        for(auto item:expected){assert(slot==item);slot=queue.next(slot);}
        assert(slot==Queue::End);
        for(unsigned i=0;i<16;++i)assert(queue.contains(i)==(std::find(expected.begin(),expected.end(),i)!=expected.end()));
    };
    std::uint32_t state=0x27d3ab49;
    // Repeat the head mutation at every population, then remove interior/tail
    // members: validate backlinks as well as the visible forward order.
    for(unsigned population=1;population<=16;++population){
        queue.clear();expected.clear();
        for(unsigned slot=0;slot<population;++slot){
            assert(queue.mark(slot));expected.insert(expected.begin(),slot);
        }
        for(unsigned repeat=0;repeat<32;++repeat){assert(queue.mark(queue.first()));check();}
        for(unsigned slot=0;slot<population;++slot){
            assert(queue.remove(slot));
            expected.erase(std::find(expected.begin(),expected.end(),slot));check();
        }
    }
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
            bool refresh=!(flags&1)&&!(flags&2)&&!(flags&0x100000);
            assert(Queue::shouldRefreshQueued(flags)==refresh);
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
    assert(!Queue::shouldRefreshQueued(flags[0]));
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
    // Missing-resource refresh updates the cache, but leaves even an old index
    // entry untouched. Restoring a resource must use the removal retry path.
    downwell::RunnerSpatialIndex<8> refreshed;
    Update::Box cached{},calculated{Real{0x40000000},Real{0x40000000},Real{0x40000000},Real{0x40000000}};
    std::uint32_t refreshFlags=0x208;
    assert(Update::finishRefresh(refreshed,7,0x30,refreshFlags,false,cached,nullptr,
        Real{0x3f800000},Real{},true,true));
    assert(refreshFlags==0 && cached.left.bits==0x3f800000 && cached.right.bits==0x3f800000);
    unsigned refreshHits=0;
    assert(refreshed.query(cached,[&](unsigned){++refreshHits;return true;}));assert(refreshHits==0);
    assert(Update::finishRefresh(refreshed,7,0x30,refreshFlags,false,cached,&calculated,
        Real{},Real{},true,true));
    assert(refreshFlags==0x8200);
    refreshFlags|=8;
    assert(Update::finishRefresh(refreshed,7,0x30,refreshFlags,false,cached,nullptr,
        Real{},Real{},false,true));
    assert(refreshFlags==0x8000 && cached.left.bits==0);
    assert(refreshed.query(calculated,[&](unsigned id){assert(id==7);++refreshHits;return true;}));
    assert(refreshHits==1); // The entry remains at the previous resource bounds.
    Update::Box origin{};
    assert(Update::finishRefresh(refreshed,7,0x30,refreshFlags,false,cached,&origin,
        Real{},Real{},false,true));
    refreshHits=0;
    assert(refreshed.query(calculated,[&](unsigned){++refreshHits;return true;}));assert(refreshHits==0);
    assert(refreshed.query(origin,[&](unsigned id){assert(id==7);++refreshHits;return true;}));assert(refreshHits==1);
    refreshFlags|=8;
    assert(Update::finishRefresh(refreshed,7,0x30,refreshFlags,false,cached,&calculated,
        Real{},Real{},true,false));
    assert(refreshFlags==0x8200 && cached.left.bits==calculated.left.bits);
    refreshHits=0;
    assert(refreshed.query(origin,[&](unsigned){++refreshHits;return true;}));assert(refreshHits==1);
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
    std::puts("Spatial mutation: 8192 preparation gates, ordered initial publication and failure handling; 65536 queue comparisons and 32768 eligibility combinations passed");
}

#include "core/RunnerSpatialInstances.h"
#include "core/InstanceStore.h"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
struct Graph {
    std::uint32_t values[4]={0x10,0x10,0x10,0x10};
    bool valid()const{return true;}
    const std::uint32_t* flags(unsigned object)const{return object<4?&values[object]:nullptr;}
    bool isA(int child,int parent)const{return child==parent || (parent==0&&(child==1||child==2));}
    bool markSpatialFamily(unsigned object){
        if(object>=4)return false;
        for(unsigned i=0;i<4;++i)if(isA(i,object)&&!(values[i]&0x28))values[i]|=0x20;
        return true;
    }
    bool prepareBirth(unsigned object){
        if(object>=4)return false;
        if((object==1||object==2)&&!(values[object]&0x28)&&(values[0]&0x28))values[object]|=0x20;
        return true;
    }
};
int main(){
    using Owner=RunnerSpatialInstances<64,64>;
    Owner owner;Graph graph;InstanceStore<64> store;
    auto attach=[&](unsigned id,int object){
        assert(store.create(id,object,0,false)==CreateInstanceResult::success);
        auto h=store.handleAt(store.size()-1);assert(owner.attach(h,object,{}));return h;
    };
    auto a=attach(10,1),b=attach(11,1),c=attach(12,2),self=attach(13,3);
    std::vector<unsigned> refreshOrder,preciseOrder;
    std::vector<bool> modes;
    auto refresh=[&](InstanceHandle h,bool publish){
        auto* s=owner.resolve(h);assert(s);
        auto* record=store.resolve(h);assert(record);
        refreshOrder.push_back(h.slot);
        Owner::Box bounds{s->x,s->y,s->x,s->y};
        return owner.refresh(h,*graph.flags(record->objectId),false,&bounds,false,publish);
    };
    auto precise=[&](InstanceHandle query,InstanceHandle candidate,bool publish,bool& hit){
        preciseOrder.push_back(candidate.slot);modes.push_back(publish);
        auto* q=owner.resolve(query);auto* p=owner.resolve(candidate);assert(q&&p);
        if(q->flags&8){if(!refresh(query,publish))return false;}
        if(p->flags&8){if(!refresh(candidate,publish))return false;}
        hit=RunnerSpatialSplit::equal(q->x,p->x)&&RunnerSpatialSplit::equal(q->y,p->y);
        return true;
    };
    Owner::Result result;
    assert(owner.place(self,0,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.slot==a.slot&&result.handle.generation==a.generation);
    assert((refreshOrder==std::vector<unsigned>{a.slot,b.slot,c.slot,self.slot}));
    assert(modes.size()==1&&!modes[0]);
    assert(owner.move(a,{0x3f800000},{},*graph.flags(1)));
    assert(owner.pendingSpatial()==1);
    assert(owner.place(self,0,{0x3f800000},{},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.slot==a.slot);
    assert(owner.resolve(self)->x.bits==0 && owner.resolve(self)->bounds.left.bits==0);
    assert(owner.move(a,{}, {},*graph.flags(1)));
    assert(owner.place(self,0,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.slot==c.slot); // Swap-last + append order.
    assert(owner.setExclusions(c,1));
    assert(owner.place(self,0,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.slot==b.slot);
    assert(owner.setExclusions(b,0x100000));
    assert(owner.place(self,0,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.slot==a.slot);
    // Cached missing-resource bounds no longer locate the old index entry.
    // Retirement must retry and clear both queues before external slot reuse.
    assert(owner.move(a,{0x42c80000},{},0x38));
    assert(owner.pendingSpatial()==1&&owner.pendingCollision()==1);
    assert(owner.refresh(a,0x38,false,nullptr,false,true));
    assert(owner.retire(a));assert(!owner.resolve(a));
    assert(!owner.pendingSpatial()&&!owner.pendingCollision());
    assert(store.erase(10)); // Compaction must not affect b/c/self handles.
    assert(store.resolve(b)&&store.resolve(c)&&store.resolve(self));
    auto replacement=attach(14,1);
    assert(replacement.slot==a.slot&&replacement.generation!=a.generation);
    assert(!owner.retire(a)&&!owner.move(a,{}, {},0x38));
    assert(owner.valid()&&owner.resolve(replacement));
    assert(refresh(replacement,true));
    assert(owner.place(self,0,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.generation==replacement.generation&&result.handle.slot==replacement.slot);
    // Clear cannot make an old external handle valid again.
    owner.clear();assert(!owner.attach(replacement,1,{}));
    store.clear();graph=Graph{};
    self=attach(20,3);a=attach(21,1);
    // Grow the unrelated tree enough to select the ancestor-list branch for
    // the one-member object. Its precise callback must request publication.
    graph.values[3]=0x30;
    for(unsigned i=0;i<40;++i){auto h=attach(30+i,3);assert(refresh(h,true));}
    preciseOrder.clear();modes.clear();refreshOrder.clear();
    assert(owner.place(self,1,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.slot==a.slot&&modes.size()==1&&modes[0]);
    assert(owner.registerObject(a,false));
    preciseOrder.clear();
    assert(owner.place(self,1,{}, {},false,graph,refresh,precise,result));
    assert(!result.found&&preciseOrder.empty());
    assert(owner.registerObject(a,true));
    assert(owner.move(self,{0x3f800000},{},0x38));
    assert(!owner.flush([](InstanceHandle,bool){return false;}));
    assert(!owner.valid());
    const auto saved=result;
    assert(!owner.place(self,1,{}, {},false,graph,refresh,precise,result));
    assert(result.found==saved.found&&result.handle.slot==saved.handle.slot);
    // Capacity failure invalidates the owner instead of querying a partial tree.
    RunnerSpatialInstances<8,1> small;
    RunnerSpatialInstances<8,1>::Box zero{};
    for(unsigned i=0;i<7;++i){
        assert(small.attach({i,1},1,{}));
        assert(small.refresh({i,1},0x30,false,&zero,false,true)==(i<6));
    }
    assert(!small.valid());
    {
        Owner births;Graph family;family.values[0]=0x30;
        assert(births.attach({0,1},1,{}));
        assert(births.publishBirth({0,1},family,[&](InstanceHandle h,bool publish){
            assert(family.values[1]==0x30 && family.values[2]==0x10 && publish);
            return births.refresh(h,family.values[1],false,&zero,false,publish);
        }));
        assert(births.resolve({0,1})->flags==0x8000);
        assert(!births.pendingSpatial()&&!births.pendingCollision());
        family.values[2]=0x18;
        assert(births.attach({1,1},2,{}));
        assert(births.publishBirth({1,1},family,[&](InstanceHandle h,bool publish){
            assert(!births.pendingCollision());
            return births.refresh(h,family.values[2],false,&zero,false,publish);
        }));
        assert(!births.pendingSpatial()&&births.pendingCollision()==1);
        assert(births.retire({1,1})&&!births.pendingCollision());
    }
    std::puts("Spatial instances: lifetime reuse, ordered first hits, query branches, queue retirement and failure latching passed");
}

#include "core/RunnerSpatialInstances.h"
#include "core/InstanceStore.h"
#include "core/RoomLoader.h"
#include "core/EventScheduler.h"
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
    {
        Owner sprites;InstanceHandle h{0,1};Owner::State clean;clean.flags=0;
        assert(sprites.attach(h,1,clean));
        Owner::Sprite sprite;sprite.spriteId=208;sprite.imageIndex={0x40800000u};
        Owner::Real timeline{0x40800000u},cached{0x40000000u};
        assert(sprites.changeSprite(h,sprite,timeline,cached,208,6,0x38));
        assert(!sprites.pendingSpatial()&&!sprites.pendingCollision()&&sprites.resolve(h)->flags==0);
        assert(cached.bits==0x40000000u&&timeline.bits==0x40800000u);
        assert(sprites.applyBindingProperties(h,0x96,false,[&](InstanceHandle live){
            return sprites.changeSprite(live,sprite,timeline,cached,205,1,0x96);
        }));
        assert(sprite.spriteId==205&&!sprite.imageIndex.bits&&!timeline.bits&&cached.bits==0xbf800000u);
        assert(!sprites.pendingSpatial()&&(sprites.resolve(h)->flags&8u));
        sprite.imageIndex={0xbe800000u};timeline={0x40000000u};
        assert(sprites.changeSprite(h,sprite,timeline,cached,-1,0,0x38));
        assert(!sprite.imageIndex.bits&&timeline.bits==0x40000000u);
        assert(sprites.pendingSpatial()==1&&sprites.pendingCollision()==1);
    }
    // Source object-property ordering, independently enumerated over all low
    // flag bits and both global preparation modes. Sprite callback inspects the
    // intermediate state; final dirty/managed flags must not be applied early.
    for(unsigned objectFlags=0;objectFlags<256;++objectFlags)for(bool allPrepared:{false,true}){
        RunnerSpatialInstances<2,8> bound;
        const InstanceHandle h{0,1};
        RunnerSpatialInstances<2,8>::State state;state.flags=0x200070u;
        assert(bound.attach(h,1,state));
        const RunnerSpatialInstances<2,8>::Box zero{};
        assert(bound.refresh(h,0x38,false,&zero,false,true));
        assert(bound.move(h,{0x3f800000},{},0x38));
        const unsigned before=bound.resolve(h)->flags;
        const bool remove=!(objectFlags&1u)&&!allPrepared&&!(objectFlags&0x28u);
        unsigned intermediate=before;
        if(objectFlags&1u)intermediate|=0x28u;else intermediate&=~0x20u;
        if(remove)intermediate&=~0x8000u;
        intermediate=(intermediate&~0x50u)|((objectFlags&2u)?0x10u:0u)|((objectFlags&4u)?0x40u:0u);
        unsigned calls=0;
        assert(bound.applyBindingProperties(h,objectFlags,allPrepared,[&](InstanceHandle live){
            ++calls;assert(live.slot==h.slot&&live.generation==h.generation);
            assert(bound.resolve(h)->flags==intermediate);
            assert(bound.pendingSpatial()==(remove?0u:1u));
            return true;
        }));
        const unsigned expected=(intermediate&~0x200000u)|((objectFlags&0x80u)?0x200000u:0u)|8u;
        assert(calls==1&&bound.resolve(h)->flags==expected);
        assert(bound.pendingSpatial()==(remove?0u:1u));
        assert(bound.pendingCollision()==(remove?0u:1u));
        assert(bound.resolve(h)&&bound.valid());
        RunnerSpatialInstances<2,8> fresh;
        RunnerSpatialInstances<2,8>::State clean;clean.flags=0;
        assert(fresh.attach(h,1,clean));
        assert(fresh.applyBindingProperties(h,objectFlags,allPrepared,[&](InstanceHandle){
            assert(fresh.pendingSpatial()==((objectFlags&1u)?1u:0u));
            assert(fresh.pendingCollision()==((objectFlags&1u)&&(objectFlags&8u)?1u:0u));
            return true;
        }));
        const bool queue=(objectFlags&0x28u)||(objectFlags&1u);
        assert(fresh.pendingSpatial()==(queue?1u:0u));
        assert(fresh.pendingCollision()==(queue&&(objectFlags&8u)?1u:0u));
    }
    {
        RunnerSpatialInstances<2,8> failed;
        assert(failed.attach({0,1},1,{}));
        assert(!failed.applyBindingProperties({0,1},0x86,false,[](InstanceHandle){return false;}));
        assert(!failed.valid()&&!(failed.resolve({0,1})->flags&0x200000u));
    }
    {
        InstanceStore<64> records;Owner space;
        records.create(1,1,7,false);
        const auto h=records.handleAt(0);
        assert(space.attach(h,1,{}));
        auto activate=[&](InstanceHandle handle,const InstanceRecord& old,bool active){
            assert(records.resolve(handle)==&old);
            assert(bool(space.resolve(handle)->flags&2u)==!old.active);
            return space.setActive(handle,active,0x38);
        };
        auto membership=[&](InstanceHandle handle,const InstanceRecord&,bool registered){
            return space.registerObject(handle,registered);
        };
        const auto exact=[](int,int){return false;};
        assert(records.requestActivation(h,false,activate));
        assert(!records.resolve(h)->active&&records.resolve(h)->activationPending);
        assert(records.registeredObjectCount(1,exact)==1); // Still registered this phase.
        assert(records.reconcileObjectMembership(membership));
        assert(records.registeredObjectCount(1,exact)==0&&(space.resolve(h)->flags&0x80000));
        assert(records.requestActivation(h,true,activate));
        assert(space.pendingSpatial()==1&&space.pendingCollision()==1);
        unsigned begins=0;
        assert(EventScheduler<64>::runStep(records,
            [](const InstanceRecord&){return AlarmBank::FiredMask(0);},
            [&](InstanceRecord&,StepEvent event){
                if(event.phase==StepEventPhase::beginStep){
                    ++begins;assert(records.registeredObjectCount(1,exact)==0);
                }else assert(records.registeredObjectCount(1,exact)==1);
            },[](StepEventPhase){},membership));
        assert(begins==1&&!(space.resolve(h)->flags&0x80002));
        assert(!records.requestActivation(h,false,[](InstanceHandle,const InstanceRecord&,bool){return false;}));
        assert(records.resolve(h)->active&&!records.resolve(h)->activationPending);
        assert(records.erase(1,[&](InstanceHandle handle,const InstanceRecord&){return space.retire(handle);}));
        records.create(2,1,7,false);
        bool called=false;
        assert(!records.requestActivation(h,false,[&](InstanceHandle,const InstanceRecord&,bool){called=true;return true;}));
        assert(!called&&records.at(0).active);
    }
    {
        Owner activation;Owner::State clean;clean.flags=0;
        InstanceHandle a{0,1},b{1,1},c{2,1};
        assert(activation.attach(a,1,clean)&&activation.attach(b,1,clean)&&activation.attach(c,1,clean));
        assert(activation.setActive(a,false,0x38)&&activation.setActive(b,false,0x38));
        assert(!activation.pendingSpatial()&&!activation.pendingCollision());
        assert(activation.setActive(a,true,0x38)&&activation.setActive(b,true,0x38));
        assert(activation.pendingSpatial()==2&&activation.pendingCollision()==2);
        assert(activation.resolve(a)->flags==0); // No invented dirty bit.
        assert(activation.setActive(a,true,0x38)); // Must not move a to head.
        std::vector<unsigned> order;
        assert(activation.flush([&](InstanceHandle h,bool publish){
            assert(publish);order.push_back(h.slot);return true;
        }));
        assert((order==std::vector<unsigned>{1,0}));
        assert(activation.setActive(c,false,0x10)&&activation.setActive(c,true,0x10));
        assert(!activation.pendingSpatial()); // Unprepared object is ineligible.
        assert(activation.registerObject(c,false));
        assert(activation.resolve(c)->flags==0x80000u);
        assert(activation.setActive(c,false,0x30)&&activation.setActive(c,true,0x30));
        assert(activation.resolve(c)->flags==0x80000u); // Deferred flag remains.
        assert(activation.pendingSpatial()==1&&activation.pendingCollision()==2);
        assert(activation.registerObject(c,true)&&activation.resolve(c)->flags==0);
        assert(activation.setActive(c,false,0x30));
        order.clear();
        assert(activation.flush([&](InstanceHandle,bool){order.push_back(99);return true;}));
        assert(order.empty()&&!activation.pendingSpatial()); // Deactivated queue entries are skipped.
        assert(!activation.setActive({0,2},true,0x38)&&activation.valid());
    }
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
    assert(store.erase(10,[&](InstanceHandle h,const InstanceRecord& record){
        assert(h.slot==a.slot&&h.generation==a.generation&&record.instanceId==10);
        assert(store.resolve(h)==&record);return owner.retire(h);
    }));
    assert(!owner.resolve(a)&&!store.resolve(a));
    assert(!owner.pendingSpatial()&&!owner.pendingCollision());
    // Compaction must not affect b/c/self handles.
    assert(store.resolve(b)&&store.resolve(c)&&store.resolve(self));
    auto replacement=attach(14,1);
    assert(replacement.slot==a.slot&&replacement.generation!=a.generation);
    assert(!owner.retire(a)&&!owner.move(a,{}, {},0x38));
    assert(owner.valid()&&owner.resolve(replacement));
    assert(refresh(replacement,true));
    assert(owner.place(self,0,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.generation==replacement.generation&&result.handle.slot==replacement.slot);
    // Room retirement keeps persistent handles and spatial membership intact.
    store.resolve(self)->persistent=true;
    assert(store.enterRoom(2,[&](InstanceHandle h,const InstanceRecord& record){
        assert(!record.persistent&&store.resolve(h)==&record);return owner.retire(h);
    }));
    assert(store.size()==1&&store.resolve(self)&&owner.resolve(self));
    assert(!owner.resolve(replacement)&&!owner.resolve(b)&&!owner.resolve(c));
    assert(store.clear([&](InstanceHandle h,const InstanceRecord& record){
        assert(store.resolve(h)==&record);return owner.retire(h);
    }));
    assert(!owner.resolve(self)&&store.empty());
    // Clear cannot make an old external handle valid again.
    owner.clear();assert(!owner.attach(replacement,1,{}));
    graph=Graph{};
    self=attach(20,3);a=attach(21,1);
    // Grow the unrelated tree enough to select the ancestor-list branch for
    // the one-member object. Its precise callback must request publication.
    graph.values[3]=0x30;
    for(unsigned i=0;i<40;++i){auto h=attach(30+i,3);assert(refresh(h,true));}
    preciseOrder.clear();modes.clear();refreshOrder.clear();
    assert(owner.place(self,1,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.slot==a.slot&&modes.size()==1&&modes[0]);
    // Object binding can unlink spatial state without retiring object lists.
    // Mismatch cached bounds deliberately, exercising removal's root retry.
    assert(owner.refresh(a,0x30,false,&owner.resolve(a)->bounds,false,true));
    assert(owner.move(a,{0x42c80000},{},0x38));
    assert(owner.refresh(a,0x38,false,nullptr,false,true));
    const auto preserved=*owner.resolve(a);
    assert(preserved.flags&0x8000u);
    assert(owner.removeSpatialMembership(a));
    assert(owner.resolve(a)&&owner.resolve(a)->flags==(preserved.flags&~0x8000u));
    assert(owner.resolve(a)->x.bits==preserved.x.bits&&owner.resolve(a)->bounds.left.bits==preserved.bounds.left.bits);
    assert(!owner.pendingSpatial()&&!owner.pendingCollision());
    assert(owner.removeSpatialMembership(a)); // Idempotent, not lifetime retirement.
    assert(owner.move(a,{}, {},0x38));
    modes.clear();
    assert(owner.place(self,1,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.slot==a.slot&&result.handle.generation==a.generation&&modes[0]);
    assert(!(owner.resolve(a)->flags&0x8000u)); // List hit need not prepare the object.
    const auto republished=owner.resolve(a)->bounds;
    assert(owner.refresh(a,0x30,false,&republished,false,true));
    assert(owner.resolve(a)->flags&0x8000u); // Eligible refresh republishes same lifetime.
    assert(!owner.removeSpatialMembership({a.slot,a.generation+1})&&owner.valid());
    assert(owner.registerObject(a,false));
    preciseOrder.clear();
    assert(owner.place(self,1,{}, {},false,graph,refresh,precise,result));
    assert(!result.found&&preciseOrder.empty());
    assert(owner.registerObject(a,true));
    b=attach(90,2);
    auto rebind=[&](InstanceHandle h,const InstanceRecord& old,int object,bool registered){
        assert(store.resolve(h)==&old);
        return owner.rebindObjectMembership(h,object,registered);
    };
    modes.clear();
    assert(owner.place(self,0,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.slot==a.slot&&modes[0]);
    const auto oldFlags=owner.resolve(a)->flags;
    assert(store.bindObject(a,1,true,rebind)); // Same object still appends.
    modes.clear();
    assert(owner.place(self,0,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.slot==b.slot&&modes[0]);
    assert(owner.resolve(a)->flags==oldFlags&&store.resolve(a)->objectId==1);
    assert(store.bindObject(a,2,true,rebind));
    assert(owner.place(self,1,{}, {},false,graph,refresh,precise,result)&&!result.found);
    assert(store.bindObject(b,2,false,rebind));
    assert(owner.place(self,2,{}, {},false,graph,refresh,precise,result));
    assert(result.found&&result.handle.slot==a.slot&&owner.resolve(b));
    assert(!store.bindObject(a,1,true,[](InstanceHandle,const InstanceRecord&,int,bool){return false;}));
    assert(store.resolve(a)->objectId==2);
    assert(!owner.rebindObjectMembership({a.slot,a.generation+1},1,true));
    assert(!owner.rebindObjectMembership(a,-1,true)&&owner.valid());
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
        InstanceStore<4> lifetimes;RunnerSpatialInstances<4,8> memberships;
        for(unsigned i=0;i<3;++i){
            assert(lifetimes.create(i,1,7,i==1)==CreateInstanceResult::success);
            assert(memberships.attach(lifetimes.handleAt(i),1,{}));
        }
        const auto first=lifetimes.handleAt(0),persistent=lifetimes.handleAt(1),last=lifetimes.handleAt(2);
        unsigned calls=0;
        assert(!lifetimes.erase(99,[&](auto,const auto&){++calls;return true;})&&!calls);
        assert(!lifetimes.erase(0,[&](auto h,const auto& record){
            ++calls;assert(lifetimes.resolve(h)==&record);return false;
        }));
        assert(calls==1&&lifetimes.size()==3&&memberships.resolve(first));
        calls=0;
        assert(!lifetimes.enterRoom(8,[&](auto h,const auto& record){
            ++calls;assert(record.roomId==7&&!record.persistent);
            return h.slot==first.slot?memberships.retire(h):false;
        }));
        assert(calls==2&&lifetimes.size()==3&&lifetimes.resolve(first)&&lifetimes.resolve(last));
        assert(lifetimes.resolve(persistent)->roomId==7&&memberships.resolve(persistent));
        assert(!memberships.resolve(first)&&memberships.resolve(last));
        // A failed batch is terminal for this simulation: do not replay the
        // first external retirement. Explicit teardown resets both domains.
        memberships.clear();lifetimes.clear();
        calls=0;assert(lifetimes.clear([&](auto,const auto&){++calls;return false;})&&!calls);
        assert(lifetimes.create(4,1,9,false)==CreateInstanceResult::success);
        assert(!lifetimes.clear([](auto,const auto&){return false;})&&lifetimes.size()==1);
    }
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
    {
        InstanceStore<64> lifetimes;Owner space;Graph family;family.values[1]=0x30;
        std::vector<unsigned> sequence;
        struct Lifecycle {
            InstanceStore<64>& store;Owner& owner;Graph& graph;std::vector<unsigned>& sequence;
            bool birth(InstanceHandle h,const InstanceRecord& record){
                assert(store.resolve(h)==&record&&record.spriteId==84&&record.maskId==597);
                Owner::State state;
                assert(state.x.assign(MotionBinary64::fromInteger(record.x)));
                assert(state.y.assign(MotionBinary64::fromInteger(record.y)));
                if(!owner.attach(h,record.objectId,state))return false;
                sequence.push_back(100+record.instanceId);
                return owner.publishBirth(h,graph,[&](InstanceHandle handle,bool publish){
                    const auto* s=owner.resolve(handle);assert(s);
                    Owner::Box bounds{s->x,s->y,s->x,s->y};
                    return owner.refresh(handle,*graph.flags(record.objectId),false,&bounds,false,publish);
                });
            }
            bool retire(InstanceHandle h,const InstanceRecord& record){
                assert(store.resolve(h)==&record);sequence.push_back(200+record.instanceId);
                return owner.retire(h);
            }
        } lifecycle{lifetimes,space,family,sequence};
        RoomInstanceDefinition firstRoom[2]={{1,1,true},{2,1,false}};
        RoomInstanceDefinition nextRoom[1]={{3,1,false}};
        for(auto& r:firstRoom){r.spriteId=84;r.maskId=597;}
        nextRoom[0].spriteId=84;nextRoom[0].maskId=597;
        auto events=[&](InstanceRecord& record,RoomInstancePhase phase){
            if(phase!=RoomInstancePhase::preCreate)return;
            for(unsigned i=0;i<lifetimes.size();++i)if(&lifetimes.at(i)==&record){
                const auto* state=space.resolve(lifetimes.handleAt(i));
                assert(state&&(state->flags&0x8000)&&!(state->flags&8));
            }
            sequence.push_back(300+record.instanceId);
        };
        auto noRoom=[](int){};auto noDeferred=[](auto&){};
        assert(RoomLoader<64>::enter(lifetimes,7,firstRoom,2,true,events,noRoom,noDeferred,lifecycle)==LoadRoomResult::success);
        const auto persistent=lifetimes.handleAt(0),retired=lifetimes.handleAt(1);
        assert(RoomLoader<64>::enter(lifetimes,8,nextRoom,1,false,events,noRoom,noDeferred,lifecycle)==LoadRoomResult::success);
        assert((sequence==std::vector<unsigned>{101,301,102,302,202,103,303}));
        assert(space.resolve(persistent)&&lifetimes.resolve(persistent)&&!space.resolve(retired));
        const auto fresh=lifetimes.handleAt(1);
        assert(fresh.slot==retired.slot&&fresh.generation!=retired.generation&&space.resolve(fresh));
    }
    std::puts("Spatial instances: lifetime reuse, ordered first hits, query branches, queue retirement and failure latching passed");
}

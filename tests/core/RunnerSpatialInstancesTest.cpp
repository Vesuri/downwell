#include "core/RunnerSpatialInstances.h"
#include "core/InstanceStore.h"
#include "core/RoomLoader.h"
#include "core/EventScheduler.h"
#include <cassert>
#include <cstdio>
#include <vector>
#include <cstring>
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
    {
        RunnerSpatialInstances<2,4> owner;
        assert(!owner.currentHandle(0).generation&&!owner.currentHandle(2).generation);
        assert(owner.attach({0,1},0,{}));
        const auto old=owner.currentHandle(0);
        assert(old.slot==0&&old.generation==1&&owner.resolve(old));
        assert(owner.registerObject(old,false));
        assert(owner.currentHandle(0).generation==1); // Dormant is still alive.
        assert(owner.retire(old)&&!owner.currentHandle(0).generation);
        assert(owner.attach({0,2},0,{}));
        assert(!owner.resolve(old)&&owner.currentHandle(0).generation==2);
        assert(owner.currentHandle(0).slot==0&&!owner.currentHandle(1).generation);
    }
    {
        RunnerSpatialInstances<8,16> owner;bool dirty=false;
        assert(!owner.watchCaptureRanges(2,5,4,8,dirty));
        assert(owner.watchCaptureRanges(2,4,6,8,dirty)&&dirty);
        for(unsigned slot=0;slot<8;++slot){
            dirty=false;assert(owner.attach({slot,1},0,{}));
            assert(dirty==(slot==2||slot==3||slot==6||slot==7));
        }
        for(unsigned slot=0;slot<8;++slot){
            const bool watched=slot==2||slot==3||slot==6||slot==7;
            const InstanceHandle h{slot,1};
            dirty=false;assert(owner.move(h,{0x3f800000u},{},0)&&!dirty);
            assert(owner.capturePrevious(h)&&!dirty);
            assert(owner.setActive(h,false,0)&&dirty==watched);
            dirty=false;assert(owner.setActive(h,false,0)&&!dirty);
            assert(owner.setActive(h,true,0)&&dirty==watched);
            dirty=false;assert(owner.setExclusions(h,1)&&dirty==watched);
            dirty=false;assert(owner.registerObject(h,false)&&dirty==watched);
            dirty=false;assert(owner.registerObject(h,false)&&!dirty);
            assert(owner.registerObject(h,true)&&dirty==watched);
            dirty=false;assert(owner.rebindObjectMembership(h,0,true)&&dirty==watched);
            dirty=false;assert(owner.retire(h)&&dirty==watched);
            dirty=false;assert(owner.attach({slot,2},0,{})&&dirty==watched);
        }
        dirty=false;owner.clear();assert(dirty);
    }
    {
        RunnerSpatialInstances<8,16> owner;bool dirty=false;
        assert(owner.watchSnapshotRange(2,4,dirty)&&dirty);dirty=false;
        assert(owner.attach({2,1},0,{})&&owner.attach({4,1},0,{}));
        assert(!dirty); // Birth already initializes the snapshot.
        assert(owner.move({4,1},{0x3f800000},{},0)&&!dirty);
        assert(owner.move({2,1},{0x3f800000},{},0)&&dirty);
        dirty=false;assert(owner.capturePrevious({2,1})&&!dirty);
        assert(owner.resolve({2,1})->previous.x.bits==0x3f800000);
        assert(owner.move({2,1},{0x3f800000},{},0)&&dirty); // Equal writes are conservative.
        dirty=false;assert(owner.retire({2,1})&&!owner.move({2,1},{},{},0)&&!dirty);
        assert(owner.attach({2,2},0,{})&&!dirty);
        assert(!owner.watchSnapshotRange(4,9,dirty));
    }
    {
        RunnerSpatialInstances<8,16> owner;
        assert(owner.attach({5,1},2,{})&&owner.attach({7,1},3,{})&&owner.attach({2,1},1,{}));
        const auto visit=[&](){
            std::vector<unsigned> seen;
            assert(owner.visitRegistrationRange(1,6,[&](InstanceHandle h,int object,const auto& state){
                assert(owner.resolve(h)==&state&&owner.objectId(h)==object);
                seen.push_back(h.slot);return true;
            }));return seen;
        };
        assert((visit()==std::vector<unsigned>{5,2}));
        assert(owner.setExclusions({5,1},2));
        assert((visit()==std::vector<unsigned>{5,2})); // Flags are the phase's decision.
        assert(owner.registerObject({5,1},false));assert((visit()==std::vector<unsigned>{2}));
        assert(owner.registerObject({5,1},true));assert((visit()==std::vector<unsigned>{2,5}));
        assert(owner.retire({2,1})&&owner.attach({2,2},1,{}));
        assert(!owner.resolve({2,1})&&(visit()==std::vector<unsigned>{5,2}));
        unsigned calls=0;
        assert(!owner.visitRegistrationRange(1,6,[&](auto h,int,const auto&){assert(h.slot==5);++calls;return false;})&&calls==1);
        assert(!owner.visitRegistrationRange(6,1,[&](auto,int,const auto&){++calls;return true;})&&calls==1);
    }
    using Owner=RunnerSpatialInstances<64,64>;
    for(unsigned mode=0;mode<4;++mode){
        Owner owner;Graph graph;const InstanceHandle first{9,1},second{4,1};
        assert(owner.attach(first,1,{})&&owner.attach(second,2,{}));
        unsigned refreshes=0,candidates=0;
        const Owner::Box box{{},{},{0x41800000},{0x41800000}};
        auto refresh=[&](InstanceHandle h,bool publish){++refreshes;return owner.refresh(h,*graph.flags(owner.objectId(h)),false,&box,true,publish);};
        if(mode==2)assert(owner.setExclusions(first,2));
        RunnerLineArguments args{{0xc0800000},{0x40800000},{0x41a00000},{0x40800000}};
        Owner::Result result{true,{55,99}};
        const bool ok=owner.collisionLine(first,0,args,mode==1,false,graph,refresh,
            [&](InstanceHandle h,const RunnerLineArguments& received,bool& hit){
                ++candidates;assert(h.slot==(mode==1||mode==2?4u:9u));
                assert(received.x1.bits==args.x1.bits&&received.x2.bits==args.x2.bits);
                hit=true;return mode!=3;
            },result);
        assert(candidates==1&&refreshes==(mode==2?1u:2u));
        assert(graph.values[0]&0x20); // Source family preparation ran.
        if(mode==3){assert(!ok&&!owner.valid()&&result.handle.slot==55);continue;}
        assert(ok&&result.found&&result.handle.slot==(mode==1||mode==2?4u:9u));
    }
    {
        Owner owner;Graph graph;graph.values[3]=0x18;
        for(unsigned i=0;i<40;++i){
            Owner::State state;state.flags=0;
            assert(state.x.assign(MotionBinary64::fromInteger(int(i)*32)));
            state.bounds={state.x,{},state.x,{0x41800000}};
            assert(owner.attach({i,1},3,state));
            assert(owner.publishBirth({i,1},graph,[&](InstanceHandle h,bool publish){
                return owner.refresh(h,graph.values[3],false,&state.bounds,true,publish);
            }));
        }
        Owner::State target;target.kind=7;
        assert(owner.attach({60,1},1,target));
        unsigned calls=0;Owner::Result result;
        assert(owner.collisionLine({},0,{},false,false,graph,
            [](auto,bool){assert(false);return false;},
            [&](InstanceHandle h,const auto&,bool& hit){++calls;assert(h.slot==60);hit=true;return true;},result));
        assert(calls==1&&result.found&&result.handle.slot==60&&graph.values[0]==0x10);
        assert(owner.resolve({60,1})->flags&8); // List candidate owns lazy refresh.
    }
    // collision_point does not share position_meeting's preparation/index
    // branch. Registration order and exclusion flags alone gate candidates.
    for(unsigned mode=0;mode<6;++mode){
        Owner owner;Graph graph;Owner::State state;state.kind=7;
        assert(owner.attach({9,1},1,state)&&owner.attach({4,1},2,state)&&owner.attach({2,1},3,state));
        assert(owner.move({9,1},{0x3f800000},{},0x18));
        assert(owner.move({4,1},{0x40000000},{},0x18));
        if(mode==1)assert(owner.setExclusions({9,1},2));
        if(mode==2){assert(owner.retire({9,1}));assert(owner.attach({9,2},1,state));}
        const auto pending=owner.pendingSpatial();
        const auto pendingCollision=owner.pendingCollision();
        unsigned calls=0;Owner::Result result{true,{55,99}};
        const bool ok=owner.collisionPointVm({9,1},0,MotionBinary64{0x3ff00000,1},
            MotionBinary64{0x80000000,0},mode==3,graph,
            [&](InstanceHandle h,auto x,auto y,bool& hit){
                ++calls;assert(x.bits==0x3f800000&&y.bits==0x80000000);
                assert(h.slot==((mode==1||mode==2||mode==3)?4u:calls==1?9u:4u));
                hit=mode!=4;return mode!=5;
            },result);
        assert(owner.pendingSpatial()==pending&&owner.pendingCollision()==pendingCollision);
        for(auto flags:graph.values)assert(flags==0x10); // No family preparation.
        if(mode==5){assert(!ok&&!owner.valid()&&result.handle.slot==55);continue;}
        assert(ok&&owner.valid());
        if(mode==4){assert(!result.found&&calls==2);continue;}
        assert(result.found&&calls==1&&result.handle.slot==((mode==1||mode==2||mode==3)?4u:9u));
    }
    {
        Owner owner;Graph graph;assert(owner.attach({1,1},1,{}));
        Owner::Result result{true,{55,99}};unsigned calls=0;
        const auto candidate=[&](auto,auto,auto,bool&){++calls;return true;};
        assert(!owner.collisionPointVm({},0,{0x7ff00000u,0},{},false,graph,candidate,result));
        assert(!owner.valid()&&calls==0&&result.handle.slot==55);
        assert(!owner.collisionPointVm({},0,{},{},false,graph,candidate,result)&&calls==0);
    }
    {
        Owner owner;Owner::State state;state.x={0x43704000};state.y={0x43308000};
        const InstanceHandle h{0,1};assert(owner.attach(h,0,state));
        assert(owner.resolve(h)->previous.x.bits==state.x.bits&&owner.resolve(h)->previous.y.bits==state.y.bits);
        assert(owner.move(h,{0x43708000},{0x4330c000},0x18));
        assert(owner.resolve(h)->previous.x.bits==state.x.bits);
        assert(owner.setExclusions(h,3)); // Still in active room list until reconciliation.
        const auto spatial=owner.pendingSpatial(),collision=owner.pendingCollision();
        assert(owner.capturePrevious(h)&&owner.resolve(h)->previous.x.bits==0x43708000&&
            owner.resolve(h)->previous.y.bits==0x4330c000);
        assert(owner.pendingSpatial()==spatial&&owner.pendingCollision()==collision);
        assert(!owner.capturePrevious({0,2}));
        assert(owner.retire(h)&&owner.attach({0,2},0,state));
        assert(owner.resolve({0,2})->previous.x.bits==state.x.bits);
    }
    {
        Owner owner;RunnerCollisionPairs<64,1> pairs;
        const InstanceHandle h{0,1};assert(owner.attach(h,0,{}));
        assert(owner.queueCollision(h,0x10)&&owner.pendingCollision()==0);
        assert(owner.queueCollision(h,8)&&owner.queueCollision(h,8));
        assert(owner.pendingCollision()==1&&owner.pendingSpatial()==0&&owner.resolve(h)->flags==8);
        assert(!owner.queueCollision({0,2},8)&&owner.pendingCollision()==1);
        assert(owner.collectCollisionPairs(pairs,true,
            [](InstanceHandle,bool){assert(false);return false;},
            [](InstanceHandle,InstanceHandle,bool&){assert(false);return false;},
            [](InstanceHandle,unsigned&){assert(false);return false;}));
        assert(owner.pendingCollision()==0&&pairs.size()==0);
    }
    // Actual index traversal and collision queue, not a supplied slot order.
    for(unsigned mode=0;mode<5;++mode){
        Owner owner;Graph graph;graph.values[1]=0x18;
        RunnerCollisionPairs<64,16> pairs;
        unsigned refreshed=0,lookups=0,framesRead=0;
        auto refresh=[&](InstanceHandle h,bool publish){
            ++refreshed;
            // Reversed cached endpoints must still query the normalized box.
            Owner::Box box{{0x3f800000},{0x3f800000},{},{}};
            return owner.refresh(h,0x18,false,&box,true,publish);
        };
        for(unsigned i=0;i<3;++i){
            assert(owner.attach({i,1},1,{}));
            assert(owner.publishBirth({i,1},graph,refresh));
        }
        // Queue is 2,1,0; insertion order within the leaf is 0,1,2.
        if(mode==1)assert(owner.setExclusions({2,1},2));
        if(mode==2)assert(owner.move({0,1},{0x3f800000},{},0x18));
        refreshed=0;
        auto event=[&](InstanceHandle,InstanceHandle,bool& found){
            ++lookups;found=true;return mode!=3;
        };
        auto frames=[&](InstanceHandle,unsigned& n){++framesRead;n=2;return mode!=4;};
        const bool ok=owner.collectCollisionPairs(pairs,mode!=2,refresh,event,frames);
        if(mode==3||mode==4){
            assert(!ok&&!owner.valid());
            const unsigned before=lookups;
            assert(!owner.collectCollisionPairs(pairs,true,refresh,event,frames));
            assert(lookups==before);
            continue;
        }
        assert(ok&&refreshed==0);
        if(mode==2){
            assert(pairs.size()==0&&lookups==0&&framesRead==0);
            assert(owner.pendingCollision()==0&&owner.pendingSpatial()==1);
            continue; // Absent global index skips even the bounds flush.
        }
        assert(pairs.size()==3&&owner.pendingCollision()==(mode==1?2u:3u));
        const unsigned expected[2][6]={{2,0,2,1,1,0},{1,0,1,2,0,2}};
        for(unsigned i=0;i<3;++i){
            const auto* p=pairs.at(i);
            assert(p->self.slot==expected[mode][2*i]&&p->other.slot==expected[mode][2*i+1]);
        }
        // An excluded current instance remains a broad-phase candidate; it is
        // neither stamped nor requeued. Precise/destruction checks come later.
        assert(framesRead==(mode==1?2u:3u));
        assert(owner.collectCollisionPairs(pairs,true,refresh,event,frames));
        for(unsigned i=0;i<3;++i){
            const auto* p=pairs.at(i);
            assert(p->self.slot==expected[mode][2*i]&&p->other.slot==expected[mode][2*i+1]);
        }
        assert(owner.retire({0,1}));
        assert(owner.collectCollisionPairs(pairs,true,refresh,event,frames));
        assert(pairs.size()==1); // No stale pair/queue membership after retirement.
    }
    {
        Owner owner;Graph graph;unsigned count=999;
        assert(owner.instanceNumber(0,graph,count)&&count==0);
        for(unsigned i=0;i<6;++i){
            Owner::State state;
            state.flags=i==2?1u:i==3?2u:i==4?0x40000u:i==5?0x100000u:0u;
            assert(owner.attach({i,1},i==0?0:1,state));
        }
        assert(owner.instanceNumber(0,graph,count)&&count==2);
        assert(owner.instanceNumber(1,graph,count)&&count==1);
        assert(owner.instanceNumber(2,graph,count)&&count==0);
        assert(owner.setExclusions({1,1},1u)&&owner.instanceNumber(0,graph,count)&&count==1);
        assert(owner.resolve({1,1})); // Marking excludes before physical cleanup.
        assert(owner.setActive({1,1},true,*graph.flags(1))&&owner.instanceNumber(0,graph,count)&&count==1);
        assert(owner.retire({1,1})&&owner.attach({1,2},1,{})&&owner.instanceNumber(0,graph,count)&&count==2);
        assert(owner.registerObject({1,2},false)&&owner.instanceNumber(0,graph,count)&&count==1);
        count=999;assert(!owner.instanceNumber(-3,graph,count)&&count==999);
        assert(!owner.instanceNumber(4,graph,count)&&count==999);
    }
    for(unsigned framesCount:{0u,1u,2u}){
        Owner owner;Graph graph;graph.values[1]=0x18;
        RunnerCollisionPairs<64,1> pairs;
        unsigned refreshes=0;
        auto refresh=[&](InstanceHandle h,bool publish){
            ++refreshes;const auto* s=owner.resolve(h);assert(s);
            Owner::Box box{s->x,s->y,s->x,s->y};
            return owner.refresh(h,0x18,false,&box,true,publish);
        };
        for(unsigned i=0;i<2;++i){
            assert(owner.attach({i,1},1,{}));
            assert(owner.publishBirth({i,1},graph,refresh));
            assert(owner.move({i,1},{0x40000000},{},0x18));
        }
        refreshes=0;
        auto event=[](InstanceHandle,InstanceHandle,bool& found){found=true;return true;};
        auto frames=[&](InstanceHandle,unsigned& n){n=framesCount;return true;};
        assert(owner.collectCollisionPairs(pairs,true,refresh,event,frames));
        assert(refreshes==2&&owner.pendingSpatial()==0&&pairs.size()==1);
        assert(pairs.at(0)->self.slot==1&&pairs.at(0)->other.slot==0);
        assert(owner.pendingCollision()==(framesCount>1?2u:0u));
        assert(owner.collectCollisionPairs(pairs,true,refresh,event,frames));
        assert(pairs.size()==(framesCount>1?1u:0u));
    }
    // VM coordinates retain fractions and narrow only once, in absolute room
    // space. Compare the boundary with a scalar double -> float conversion,
    // including both sides of binary32 ties and negative coordinates.
    for(bool listPath:{false,true})for(double x:{240.25,-240.25,0.0,-0.0,
            240.0+0x1p-17,240.0+0x1p-17-0x1p-40,240.0+0x1p-17+0x1p-40,
            240.0+0x3p-17,16777217.0}){
        Owner queries;Graph graph;
        const InstanceHandle self{0,1},candidate{1,1};
        assert(queries.attach(self,3,{})&&queries.attach(candidate,1,{}));
        // Keep a real earlier snapshot distinct from both the current and
        // temporary query positions, without mutating private test state.
        const RunnerImageValue previousX{0xc1800000u},previousY{0xc2000000u}; // -16, -32
        assert(queries.move(self,previousX,previousY,*graph.flags(3))&&queries.capturePrevious(self));
        assert(queries.move(self,{},{},*graph.flags(3)));
        auto refresh=[&](InstanceHandle h,bool publish){
            const auto* s=queries.resolve(h);assert(s);
            Owner::Box box{s->x,s->y,s->x,s->y};
            return queries.refresh(h,*graph.flags(unsigned(queries.objectId(h))),false,&box,false,publish);
        };
        if(listPath){
            graph.values[3]=0x30;
            for(unsigned i=2;i<42;++i){
                assert(queries.attach({i,1},3,{}));assert(refresh({i,1},true));
            }
        }
        float fx=static_cast<float>(x),fy=-176.25f;
        std::uint32_t bx,by;std::memcpy(&bx,&fx,4);std::memcpy(&by,&fy,4);
        assert(queries.move(candidate,{bx},{by},*graph.flags(1)));
        auto vm=[](double d){std::uint64_t bits;std::memcpy(&bits,&d,8);
            return MotionBinary64{std::uint32_t(bits>>32),std::uint32_t(bits)};};
        unsigned calls=0;Owner::Result result;
        assert(queries.placeVm(self,1,vm(x),vm(-176.25),false,graph,refresh,
            [&](InstanceHandle q,InstanceHandle c,bool publish,bool& hit){
                ++calls;assert(q.slot==self.slot&&c.slot==candidate.slot);
                assert(publish==listPath);
                assert(queries.resolve(q)->x.bits==bx&&queries.resolve(q)->y.bits==by);
                assert(queries.resolve(q)->previous.x.bits==previousX.bits&&
                    queries.resolve(q)->previous.y.bits==previousY.bits);
                hit=true;return true;
            },result));
        assert(calls==1&&result.found&&result.handle.slot==candidate.slot);
        assert(!queries.resolve(self)->x.bits&&!queries.resolve(self)->y.bits);
        assert(queries.resolve(self)->previous.x.bits==previousX.bits&&
            queries.resolve(self)->previous.y.bits==previousY.bits);
    }
    for(bool badY:{false,true})for(MotionBinary64 invalid:{MotionBinary64{0x7ff00000u,0},
            MotionBinary64{0x7ff80000u,1},MotionBinary64{0x47f00000u,0},MotionBinary64{0,1}}){
        Owner queries;Graph graph;const InstanceHandle self{0,1};
        assert(queries.attach(self,3,{}));
        Owner::Result result{true,{7,8}};unsigned calls=0;
        assert(!queries.placeVm(self,1,badY?MotionBinary64{}:invalid,badY?invalid:MotionBinary64{},
            false,graph,[&](InstanceHandle,bool){++calls;return true;},
            [&](InstanceHandle,InstanceHandle,bool,bool&){++calls;return true;},result));
        assert(!queries.valid()&&!calls&&!queries.pendingSpatial()&&!queries.pendingCollision());
        assert(result.found&&result.handle.slot==7&&result.handle.generation==8);
        assert(!queries.resolve(self)->x.bits&&!queries.resolve(self)->y.bits);
        assert(graph.values[1]==0x10);
    }
    for(auto field:{Owner::TransformField::scaleX,Owner::TransformField::scaleY,Owner::TransformField::angle})
    for(auto bits:{0u,0x80000000u,0x3f800000u,0x40000000u,0xbf800000u})
    for(auto alpha:{0x3f800000u,0x3f000000u})for(auto blend:{0xffffffu,0xff0000u}){
        Owner transforms;Owner::State clean;clean.flags=0;
        assert(transforms.attach({0,1},1,clean));Owner::Sprite sprite;
        auto& target=field==Owner::TransformField::scaleX?sprite.scaleX:
            field==Owner::TransformField::scaleY?sprite.scaleY:sprite.angle;
        target={0x40000000u};
        assert(transforms.changeTransform({0,1},sprite,field,{bits},{alpha},blend,0x38));
        assert(target.bits==bits);
        const bool changed=bits!=0x40000000u;
        const bool fast=sprite.scaleX.bits==0x3f800000u&&sprite.scaleY.bits==0x3f800000u&&
            !(sprite.angle.bits&0x7fffffffu)&&alpha==0x3f800000u&&blend==0xffffffu;
        assert(transforms.resolve({0,1})->flags==(changed?(8u|(fast?0x4000u:0u)):0u));
        assert(transforms.pendingSpatial()==unsigned(changed)&&transforms.pendingCollision()==unsigned(changed));
    }
    {
        Owner transforms;Owner::State clean;clean.flags=0x4000u;
        assert(transforms.attach({0,1},1,clean));Owner::Sprite sprite;
        assert(transforms.changeTransform({0,1},sprite,Owner::TransformField::angle,{0x80000000u},{0x3f800000u},0xffffff,0x38));
        assert(sprite.angle.bits==0&&!transforms.pendingSpatial()&&transforms.resolve({0,1})->flags==0x4000u);
        assert(transforms.changeMask({0,1},sprite,-1,0x38)&&!transforms.pendingSpatial());
        assert(transforms.changeMask({0,1},sprite,999999,0x38)&&sprite.maskId==999999);
        assert(transforms.pendingSpatial()==1&&transforms.pendingCollision()==1);
        const auto old=sprite.scaleX.bits;
        assert(!transforms.changeTransform({0,1},sprite,Owner::TransformField::scaleX,{0x7f800000u},{0x3f800000u},0xffffff,0x38));
        assert(!transforms.valid()&&sprite.scaleX.bits==old);
    }
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
        // Reactivation is reconciled after the Begin Step snapshot.
        assert(begins==0&&!(space.resolve(h)->flags&0x80002));
        assert(EventScheduler<64>::runStep(records,
            [](const InstanceRecord&){return AlarmBank::FiredMask(0);},
            [&](InstanceRecord&,StepEvent event){if(event.phase==StepEventPhase::beginStep)++begins;},
            [](StepEventPhase){},membership));
        assert(begins==1);
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
    bool snapshotDirty=false;
    assert(owner.watchSnapshotRange(self.slot,self.slot+1,snapshotDirty));snapshotDirty=false;
    Owner::Result result;
    assert(owner.place(self,0,{}, {},false,graph,refresh,precise,result));
    assert(snapshotDirty); // Temporary caller movement/restoration uses the observer.
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

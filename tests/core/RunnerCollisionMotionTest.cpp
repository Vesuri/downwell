#include "core/RunnerCollisionMotion.h"
#include "core/RunnerSpatialInstances.h"
#include "core/RunnerCollisionDispatch.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
using namespace downwell;
static RunnerImageValue real(float f){RunnerImageValue r;std::memcpy(&r.bits,&f,4);return r;}
int main(){
    unsigned cases=0;
    for(float x:{-16777216.f,-240.25f,-0.5f,-0.f,0.f,0.5f,240.25f,16777216.f})
    for(float v:{-3.f,-1.f,-0.00001f,-0.f,0.f,0.00001f,1.f,3.f}){
        volatile float xx=x,vv=v;
        const auto expected=real(xx+vv);
        unsigned calls=0;
        assert(RunnerCollisionMotion::advance(real(x),real(x),{real(v),real(v)},
            [&](auto a,auto b){++calls;return a.bits==expected.bits&&b.bits==expected.bits;}));
        assert(calls==1);++cases;
    }
    unsigned random=0x682000u;
    auto sample=[&]{random=random*1664525u+1013904223u;
        return RunnerImageValue{(random&0x807fffffu)|((70u+(random%110u))<<23)};};
    for(unsigned i=0;i<8192;++i){
        const auto a=sample(),b=sample();float fa,fb;
        std::memcpy(&fa,&a.bits,4);std::memcpy(&fb,&b.bits,4);
        volatile float va=fa,vb=fb;const auto expected=real(va+vb);
        assert(RunnerCollisionMotion::advance(a,b,{b,a},[&](auto x,auto y){
            return x.bits==expected.bits&&y.bits==expected.bits;
        }));++cases;
    }
    RunnerCollisionMotion::Path path{real(.75f),real(.25f)};
    unsigned calls=0;
    assert(!RunnerCollisionMotion::rollback({real(1),real(2)},&path,[&](auto,auto){++calls;return false;}));
    assert(calls==1&&path.position.bits==real(.75f).bits);
    assert(RunnerCollisionMotion::rollback({real(1),real(2)},&path,[&](auto x,auto y){
        assert(path.position.bits==real(.75f).bits);return x.bits==real(1).bits&&y.bits==real(2).bits;
    })&&path.position.bits==real(.25f).bits);
    for(auto invalid:{RunnerImageValue{1},RunnerImageValue{0x7f800000u},RunnerImageValue{0x7fc00000u}}){
        assert(!RunnerCollisionMotion::advance(invalid,{},{{},{}},[](auto,auto){assert(false);return true;}));
        assert(!RunnerCollisionMotion::rollback({{},invalid},nullptr,[](auto,auto){assert(false);return true;}));
    }
    assert(!RunnerCollisionMotion::advance({0x7f7fffffu},{},{{0x7f7fffffu},{}},[](auto,auto){assert(false);return true;}));
    // Real maintained-owner publication, retaining fractions and dirtiness.
    RunnerSpatialInstances<4,8> owner;const InstanceHandle h{0,1};
    assert(owner.attach(h,0,{}));
    auto publish=[&](auto x,auto y){return owner.move(h,x,y,0x18);};
    assert(RunnerCollisionMotion::advance(real(240.25f),real(176.5f),{real(.5f),real(-.25f)},publish));
    assert(owner.resolve(h)->x.bits==real(240.75f).bits&&owner.resolve(h)->y.bits==real(176.25f).bits);
    assert(owner.pendingSpatial()==1&&owner.pendingCollision()==1);
    assert(RunnerCollisionMotion::rollback({real(240.25f),real(176.5f)},nullptr,publish));
    assert(owner.resolve(h)->x.bits==real(240.25f).bits&&owner.resolve(h)->y.bits==real(176.5f).bits);
    // Compose the paired protocol with concrete binary32 movement. Handlers
    // modify engine speed AFTER the first rollback; reapplication must read
    // those new speeds, and a remaining overlap rolls back both instances.
    for(bool stillOverlapping:{false,true}){
        struct Ops {
            RunnerSpatialInstances<4,8> world;
            RunnerCollisionMotion::Previous previous[2];
            RunnerCollisionMotion::Velocity velocity[2]{};
            bool secondHit=false;unsigned queries=0;
            bool flags(InstanceHandle h,std::uint32_t& out){out=world.resolve(h)->flags;return true;}
            bool overlap(InstanceHandle,InstanceHandle,bool& hit){hit=queries++?secondHit:true;return true;}
            bool queueCollision(InstanceHandle h){return world.queueCollision(h,0x18);}
            bool rollback(InstanceHandle h){return RunnerCollisionMotion::rollback(previous[h.slot],nullptr,
                [&](auto x,auto y){return world.move(h,x,y,0x18);});}
            bool prepareMotion(InstanceHandle){return true;} // Fixture has no path.
            bool advanceMotion(InstanceHandle h){const auto* s=world.resolve(h);
                return RunnerCollisionMotion::advance(s->x,s->y,velocity[h.slot],
                    [&](auto x,auto y){return world.move(h,x,y,0x18);});}
            bool dispatch(InstanceHandle h,InstanceHandle){
                const auto* s=world.resolve(h);assert(s->x.bits==previous[h.slot].x.bits);
                velocity[h.slot]={real(.25f),real(-.5f)};return true;
            }
        } ops;
        ops.secondHit=stillOverlapping;
        for(unsigned i=0;i<2;++i){
            RunnerSpatialInstances<4,8>::State state;
            state.x=real(100.f+i);state.y=real(200.f);state.flags=0x20;
            assert(ops.world.attach({i,1},0,state));
            ops.previous[i]={real(90.f+i),real(190.f)};
        }
        RunnerCollisionDispatch pass;assert(pass.pair({0,1},{1,1},ops));
        for(unsigned i=0;i<2;++i){
            const auto* s=ops.world.resolve({i,1});
            assert(s->x.bits==real(90.f+i+(stillOverlapping?0.f:.25f)).bits);
            assert(s->y.bits==real(stillOverlapping?190.f:189.5f).bits);
        }
        assert(ops.queries==2&&ops.world.pendingSpatial()==2&&ops.world.pendingCollision()==2);
    }
    std::printf("Collision movement: %u binary32 sums, path rollback order, publication and invalid input checks passed\n",cases);
}

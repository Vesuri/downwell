#include "core/StoredWhiteJellyStates.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
using N=downwell::MotionBinary64;using F=downwell::RunnerImageValue;
using World=downwell::InstanceStore<8>;
using Owner=downwell::StoredWhiteJellyStates<8,2>;
struct Ops {
    World& world;downwell::InstanceHandle key;bool reject=false,compact=false;
    bool position(bool vertical,F value,F& accepted){
        if(compact){assert(world.erase(10));compact=false;}
        auto* r=world.resolve(key);assert(r);float v;std::memcpy(&v,&value.bits,4);
        if(vertical)r->preciseY=v;else r->preciseX=v;
        accepted=value;return !reject;
    }
    bool playerY(N& v){v=N::fromInteger(40);return true;}
    bool playerX(N& v){v=N::fromInteger(80);return true;}
    bool pointDirection(N,N,N,N,N& value){value=N::fromInteger(90);return true;}
    bool direction(N v,N& out){out=v;return true;}
    bool imageSpeed(F value,F& accepted){
        auto* r=world.resolve(key);assert(r);std::memcpy(&r->imageSpeed,&value.bits,4);accepted=value;return true;
    }
};
static downwell::InstanceHandle birth(World& w,unsigned id,int object=115){
    assert(w.create(id,object,11,false)==downwell::CreateInstanceResult::success);
    const auto h=w.handleAt(w.size()-1);auto* r=w.resolve(h);r->preciseX=24;r->preciseY=56;return h;
}
int main(){
    World world;Owner owner(world);downwell::GameMakerRandom random(8);
    birth(world,10,60);const auto first=birth(world,11);Ops ops{world,first,false,true};
    assert(owner.create(first,random,ops)&&owner.valid()&&world.size()==1&&random.drawCount()==1);
    assert(owner.state(11)&&world.resolve(first)->preciseX==32&&world.resolve(first)->preciseY==64);
    assert(owner.dispatch(first,[](auto& s){s.hitStun=true;return true;}));
    assert(owner.hitStunAlarm(first)&&!owner.state(11)->hitStun);
    // Source active is independent of runner activation.
    assert(owner.dispatch(first,[](auto& s){s.scriptActive=false;return true;}));
    unsigned calls=0;assert(owner.dispatch(first,[&](auto&){++calls;return true;})&&calls==1);
    world.resolve(first)->active=false;
    assert(owner.dispatch(first,[&](auto&){++calls;return true;})&&calls==1);
    world.resolve(first)->active=true;
    assert(owner.dispatch(first,[&](auto&){world.resolve(first)->destroyRequested=true;return true;}));
    assert(owner.state(11)&&owner.dispatch(first,[&](auto&){++calls;return true;})&&calls==1);
    assert(world.erase(11)&&!owner.state(11));
    const auto second=birth(world,11);Ops next{world,second};
    assert(owner.create(second,random,next)&&random.drawCount()==2&&!owner.state(11)->hitStun);
    assert(!owner.dispatch(first,[](auto&){return true;})&&!owner.valid());
    owner.clear();assert(owner.valid()&&!owner.state(11));
    assert(owner.create(second,random,next));
    // A failed partial Create remains inspectable, but may not be retried.
    const auto third=birth(world,12);Ops fail{world,third,true};const auto before=random.drawCount();
    assert(!owner.create(third,random,fail)&&!owner.valid()&&random.drawCount()==before);
    assert(owner.state(12)&&owner.state(12)->alive&&!owner.state(12)->initialized);
    assert(!owner.create(third,random,fail));
    // Duplicate live births fail before another random draw.
    owner.clear();assert(owner.create(second,random,next));const auto after=random.drawCount();
    assert(!owner.create(second,random,next)&&random.drawCount()==after);
    owner.clear();assert(owner.create(second,random,next));
    assert(!owner.dispatch(second,[&](auto&){assert(!owner.clear());return true;}));
    assert(!owner.valid()&&owner.state(11));
    std::puts("Stored white jellies: compaction, activation, destruction, stale handles and failed Create ownership passed");
}

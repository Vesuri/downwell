#include "core/MovingFxInstances.h"
#include <cassert>
#include <cstdio>
#include <vector>
using Pool=downwell::MovingFxInstances<2>;using R=Pool::Record;
using H=downwell::InstanceHandle;using F=downwell::RunnerImageValue;using N=downwell::MotionBinary64;
struct Audit {
    std::vector<int> calls;bool reject=false;
    bool birth(H,const R&){return false;}
    bool birthAt(H,const R& r,F,F){assert(r.busy&&!r.initialized&&r.spriteId==-1);calls.push_back(1);return true;}
    bool move(H,const R&,F,F){calls.push_back(2);return !reject;}
    bool retire(H,const R& r){assert(r.destroyRequested);calls.push_back(3);return true;}
    bool activation(H,const R& r,bool active){assert(r.active!=active);calls.push_back(4);return true;}
    bool membership(H,const R&,bool){return false;}
    bool markDestroyed(H,const R& r){assert(!r.destroyRequested);calls.push_back(5);return true;}
    bool sprite(H,const R&,int sprite){assert(sprite==704);calls.push_back(6);return true;}
    bool imageIndex(H,const R&,F){calls.push_back(7);return true;}
    bool mask(H,const R&,int){return false;}
    bool transform(H,const R&,downwell::RunnerTransformField field,F,F,unsigned){assert(field==downwell::RunnerTransformField::angle);calls.push_back(8);return true;}
};
int main(){
    Audit audit;Pool pool(Pool::Binding::bindPositionedActor(audit));H first;
    assert(pool.create(100,N::fromInteger(10),N::fromInteger(20),first)&&pool.size()==1);
    assert(pool.find(first)->initialized&&pool.sprite(first,704,2)&&pool.imageSpeed(first,{0x3fe00000u,0})&&
        pool.imageAngle(first,N::fromInteger(90))&&pool.velocity(first,true,N::fromInteger(-1)));
    assert(pool.step(first,[](const auto&,bool& allowed){allowed=true;return true;}));
    assert(pool.find(first)->y.bits==0x41980000u&&audit.calls==std::vector<int>({1,6,7,8,2,2}));
    assert(pool.activate(first,false));unsigned reads=0;
    assert(pool.step(first,[&](const auto&,bool&){++reads;return false;})&&!reads&&pool.advanceAnimation(first));
    assert(pool.find(first)->imageIndex.bits==0&&pool.activate(first,true));
    for(unsigned i=0;i<4;++i)assert(pool.advanceAnimation(first));
    assert(pool.find(first)->destroyRequested&&pool.retire(first)&&!pool.find(first)&&!pool.size());
    H second;assert(pool.create(101,N::fromInteger(10),N::fromInteger(20),second));
    assert(second.slot==first.slot&&second.generation!=first.generation);
    assert(!pool.imageSpeed(first,{})&&!pool.valid());
    Audit reject;Pool broken(Pool::Binding::bindPositionedActor(reject));H key;
    assert(broken.create(200,N::fromInteger(10),N::fromInteger(20),key));reject.reject=true;
    assert(!broken.step(key,[](const auto&,bool& allowed){allowed=false;return true;})&&!broken.valid());
    assert(broken.find(key)->imageSpeed.bits==0); // completed write survives failed position publication
    std::puts("Native moving effects: publication, activation, animation end, stale handles and failure prefixes passed");
}

#include "core/GameplayWhiteJellies.h"
#include "core/GameplayRedBalls.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <vector>
#include <cstdio>
#ifdef DOWNWELL_TEST_RED_BALL
using Pool=downwell::GameplayRedBalls;
constexpr int normalSprite=702,hitSprite=703,collisionMask=153;
#else
using Pool=downwell::GameplayWhiteJellies;
constexpr int normalSprite=176,hitSprite=175,collisionMask=174;
#endif
using R=Pool::Record;
using H=downwell::InstanceHandle;using F=downwell::RunnerImageValue;using N=downwell::MotionBinary64;
struct Audit {
    std::vector<int> calls;bool rejectMove=false;
    bool birth(H,const R&){assert(false);return false;}
    bool birthAt(H,const R& r,F x,F y){
        assert(!r.initialized&&!r.alive&&r.busy&&r.sourceId&&r.spriteId==normalSprite&&r.maskId==collisionMask);
        assert(x.bits==0x41c00000u&&y.bits==0x42600000u);calls.push_back(1);return true;
    }
    bool move(H,const R& r,F x,F y){
        assert(r.alive&&!r.initialized);calls.push_back(2);
        assert(x.bits==0x42000000u&&(y.bits==0x42600000u||y.bits==0x42800000u));return !rejectMove;
    }
    bool retire(H,const R&){calls.push_back(3);return true;}
    bool activation(H,const R& r,bool value){assert(r.active!=value);calls.push_back(4);return true;}
    bool membership(H,const R&,bool){return false;}
    bool markDestroyed(H,const R& r){assert(!r.destroyRequested);calls.push_back(5);return true;}
    bool sprite(H,const R&,int id){assert(id==hitSprite||id==normalSprite);calls.push_back(6);return true;}
    bool imageIndex(H,const R&,F){calls.push_back(7);return true;}
    bool mask(H,const R&,int){return false;}
    bool transform(H,const R&,downwell::RunnerTransformField,F,F,unsigned){return false;}
};
struct Ops {
    Pool& pool;H key;
    bool position(bool y,F value,F& out){return pool.position(key,y,value,out);}
    bool playerY(N& value){value={};return true;}bool playerX(N& value){value={};return true;}
    bool pointDirection(N,N,N,N,N& value){value=N::fromInteger(90);return true;}
    bool direction(N value,N& out){out=value;return true;}
    bool imageSpeed(F value,F& out){out=value;return true;}
    bool alarm(int number,int value){return pool.setAlarm(key,number,value);}
};
int main(){
    Audit audit;Pool pool(Pool::Binding::bindPositionedActor(audit));downwell::GameMakerRandom random(17);
    H first;auto make=[&](H h){return Ops{pool,h};};
    assert(pool.create(100,{0x41c00000u},{0x42600000u},random,make,first));
    assert(pool.size()==1&&random.drawCount()==1&&pool.find(first)->initialized);
#ifdef DOWNWELL_TEST_RED_BALL
    assert(pool.find(first)->health==30&&pool.find(first)->money==20&&
        pool.find(first)->alarms.get(0)==10&&pool.find(first)->alarms.get(2)==30);
#endif
    assert(audit.calls==std::vector<int>({1,2,2}));
    assert(pool.find(first)->x.bits==0x42000000u&&pool.find(first)->y.bits==0x42800000u);
    assert(pool.dispatch(first,[&](auto& r){
        assert(r.busy&&pool.publishMovingFx(first,100001)&&r.myFx==100001);
        return true;
    }));
    assert(pool.dispatch(first,[](auto& r){r.hitStun=true;return true;}));
    assert(pool.setAlarm(first,1,2)&&pool.advanceAlarm(first,1)&&pool.find(first)->hitStun);
    assert(pool.advanceAlarm(first,1)&&!pool.find(first)->hitStun);
    unsigned effectAlarms=0;
    assert(pool.setAlarm(first,0,1)&&pool.advanceAlarm(first,0,[&](auto&){
        ++effectAlarms;return pool.setAlarm(first,0,3);
    })&&effectAlarms==1&&pool.find(first)->alarms.get(0)==3);
    for(unsigned i=0;i<8;++i)assert(pool.advanceAnimation(first));
    assert(pool.find(first)->imageIndex.bits>0x3f800000u);
    assert(pool.sprite(first,hitSprite)&&pool.find(first)->imageIndex.bits==0&&pool.find(first)->timeline.bits==0);
    assert(pool.advanceAnimation(first)&&pool.find(first)->imageIndex.bits==0x3e4ccccdu);
    assert(pool.sprite(first,normalSprite)&&pool.find(first)->imageIndex.bits==0x3e4ccccdu);
    assert(pool.imageIndex(first,{})&&pool.find(first)->timeline.bits==0x3e4ccccdu);
    assert(pool.advanceAnimation(first)&&pool.find(first)->imageIndex.bits==0x3e4ccccdu);
    assert(pool.activate(first,false)&&!pool.find(first)->active&&pool.find(first)->scriptActive);
    unsigned events=0;assert(pool.dispatch(first,[&](auto&){++events;return true;})&&!events);
    assert(pool.activate(first,true));
    assert(pool.dispatch(first,[&](auto& r){
        ++events;assert(pool.markDestroyed(first)&&r.destroyRequested);return true;
    }));
    assert(events==1&&pool.find(first)&&pool.retireMarked(first)&&!pool.find(first)&&!pool.size());
    H second;assert(pool.create(101,{0x41c00000u},{0x42600000u},random,make,second));
    assert(second.slot==first.slot&&second.generation!=first.generation&&pool.valid());
    assert(pool.clear()&&!pool.size());
    assert(!pool.publishMovingFx(first,100002)&&!pool.valid());
    // Publication failure occurs before the random velocity draw; partial
    // state stays owned and the owner is poisoned, not rolled back/retried.
    Audit failing;failing.rejectMove=true;Pool broken(Pool::Binding::bindPositionedActor(failing));
    const auto before=random.drawCount();H rejected;
    assert(!broken.create(102,{0x41c00000u},{0x42600000u},random,[&](H h){return Ops{broken,h};},rejected));
    assert(!rejected.generation&&!broken.valid()&&broken.size()==1&&random.drawCount()==before);
    assert(broken.find(broken.handle(0))->alive&&!broken.find(broken.handle(0))->initialized);
    std::puts("Native steering owner: positioned birth, synchronous setters, activation and lifetime retirement passed");
}

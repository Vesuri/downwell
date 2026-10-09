#include "core/BlastActor.h"
#ifdef DOWNWELL_NATIVE_BLAST_PROBE
struct BlastEvents {
    bool shake(int,int);bool momentDelay();bool imageIndex(downwell::RunnerImageValue);
    bool imageSpeed(downwell::RunnerImageValue);bool choose(unsigned,unsigned&);
    bool xscale(downwell::RunnerImageValue);bool destroySelf();bool directParent(int&);
    bool targetId(std::uint32_t&);bool wallHealth(downwell::MotionBinary64&);
    bool setWallHealth(downwell::MotionBinary64);bool objectHealth(downwell::MotionBinary64&);
    bool setObjectHealth(downwell::MotionBinary64);downwell::MotionBinary64 callerX();
    downwell::MotionBinary64 callerY();bool flash(downwell::MotionBinary64,downwell::MotionBinary64,int,int,int);
    bool bulletThrough(bool&);bool bulletImmune(bool&);bool enemyHealth(downwell::MotionBinary64&);
    bool setEnemyHit();bool setEnemyDirection(downwell::MotionBinary64);bool setEnemyHealth(downwell::MotionBinary64);
    bool setEnemyStun();bool hitSound(int,int,bool,bool);bool setEnemyImpact(downwell::MotionBinary64);
    bool createExplosion(int,downwell::MotionBinary64,downwell::MotionBinary64);
    bool impactEffect(downwell::MotionBinary64,downwell::MotionBinary64,int,downwell::MotionBinary64,downwell::MotionBinary64,int);
};
bool nativeBlastProbe(downwell::BlastActor<32>& actor,BlastEvents& events,bool& good){
    return actor.create(events,good)&&actor.advanceAnimation(events,good)&&actor.step(good)&&
        actor.wallContact(events,good)&&actor.shootableContact(events,good)&&actor.enemyContact(events,good);
}
#else
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
static bool equal(MotionBinary64 a,MotionBinary64 b){return a.high==b.high&&a.low==b.low;}
struct BlastEvents {
    std::vector<int> calls;unsigned fail=0,choice=0;int parent=84;std::uint32_t id=100001;
    MotionBinary64 hp=MotionBinary64::fromInteger(60),impact{};
    bool through=false,immune=false,hit=false,stun=false;
    bool call(int tag){calls.push_back(tag);return calls.size()!=fail;}
    bool shake(int a,int d){assert(a==6&&d==8);return call(1);}
    bool momentDelay(){return call(2);}
    bool imageIndex(RunnerImageValue){return call(3);}
    bool imageSpeed(RunnerImageValue v){assert(v.bits==0x3f000000u);return call(4);}
    bool choose(unsigned count,unsigned& result){assert(count==2);result=choice;return call(5);}
    bool xscale(RunnerImageValue v){assert(v.bits==(choice?0xbf800000u:0x3f800000u));return call(6);}
    bool destroySelf(){return call(7);}
    bool directParent(int& out){out=parent;return call(8);}
    bool targetId(std::uint32_t& out){out=id;return call(9);}
    bool wallHealth(MotionBinary64& out){out=hp;return call(10);}
    bool setWallHealth(MotionBinary64 value){if(!call(11))return false;hp=value;return true;}
    bool objectHealth(MotionBinary64& out){out=hp;return call(12);}
    bool setObjectHealth(MotionBinary64 value){if(!call(13))return false;hp=value;return true;}
    MotionBinary64 callerY(){calls.push_back(14);return MotionBinary64{0x40388000u,0};}
    MotionBinary64 callerX(){calls.push_back(15);return MotionBinary64{0x40248000u,0};}
    bool flash(MotionBinary64 x,MotionBinary64 y,int count,int vary,int radius){
        assert(x.high==0x40248000u&&y.high==0x40388000u&&count==1&&!vary&&!radius);return call(16);
    }
    bool bulletThrough(bool& out){out=through;return call(17);}
    bool bulletImmune(bool& out){out=immune;return call(18);}
    bool enemyHealth(MotionBinary64& out){out=hp;return call(19);}
    bool setEnemyHit(){if(!call(20))return false;hit=true;return true;}
    bool setEnemyDirection(MotionBinary64 value){assert(equal(value,MotionBinary64::fromInteger(270)));return call(21);}
    bool setEnemyHealth(MotionBinary64 value){if(!call(22))return false;hp=value;return true;}
    bool setEnemyStun(){if(!call(23))return false;stun=true;return true;}
    bool hitSound(int id,int priority,bool loop,bool cut){assert(id==29&&priority==90&&!loop&&cut);return call(24);}
    bool setEnemyImpact(MotionBinary64 value){if(!call(25))return false;impact=value;return true;}
    bool impactEffect(MotionBinary64 x,MotionBinary64 y,int sprite,MotionBinary64 speed,MotionBinary64 direction,int depth){
        assert(x.high==0x40248000u&&y.high==0x40388000u&&sprite==605&&
            equal(speed,{0x3fd99999u,0x9999999au})&&equal(direction,MotionBinary64::fromInteger(270))&&!depth);
        return call(26);
    }
    bool createExplosion(int object,MotionBinary64 x,MotionBinary64 y){
        assert(object==272&&x.high==0x40248000u&&y.high==0x40388000u);return call(27);
    }
};
static BlastActor<4> fresh(){BlastActor<4> a;BlastEvents e;bool good=true;assert(a.create(e,good));return a;}
int main(){
    for(unsigned choice=0;choice<2;++choice)for(unsigned fail=0;fail<=6;++fail){
        BlastActor<4> a;BlastEvents e;e.choice=choice;e.fail=fail;bool good=true;
        assert(a.create(e,good)==!fail&&good==!fail&&e.calls.size()==(fail?fail:6));
        assert(a.allSet&&a.consistent&&a.wave&&!a.explode&&equal(a.damage,MotionBinary64::fromInteger(50))&&
            a.hits.count==0&&a.hits.ids[0]==-1&&a.shake.shakeAmt==6&&a.shake.shakeDur==8);
        if(fail){const auto before=e.calls.size();assert(!a.create(e,good)&&e.calls.size()==before);}
        else assert(e.calls==std::vector<int>({1,2,3,4,5,6}));
    }
    {
        auto a=fresh();BlastEvents e;bool good=true;
        for(unsigned frame=1;frame<=28;++frame){
            assert(a.advanceAnimation(e,good));
            if(frame<28){assert(a.step(good));assert(a.damageEnd==(frame>=7));assert(!a.destroyRequested);}
            else assert(a.destroyRequested&&e.calls.back()==7);
        }
        a.disregardWall=true;a.alarm0();assert(!a.disregardWall&&a.disregardWallWritten);
    }
    {
        ProjectileHitMemory<2> m;bool good=true;m.i=99;
        assert(m.check(100001,good)&&m.i==99&&m.count==1&&!m.hitIgnore);
        assert(m.check(100002,good)&&m.i==1&&m.count==2&&!m.hitIgnore);
        assert(m.check(100001,good)&&m.i==2&&m.count==2&&m.hitIgnore);
        assert(!m.check(100003,good)&&!good&&m.count==2&&m.checkTarget==100003&&!m.hitIgnore&&m.i==2);
        assert(!m.check(100001,good)&&m.checkTarget==100003);
    }
    for(unsigned fail=0;fail<=5;++fail){
        auto a=fresh();BlastEvents e;e.fail=fail;bool good=true;
        assert(a.wallContact(e,good)==!fail&&e.calls.size()==(fail?fail:5));
        if(!fail){assert(e.calls==std::vector<int>({8,9,10,10,11})&&equal(e.hp,MotionBinary64::fromInteger(10)));
            e.calls.clear();assert(a.shootableContact(e,good)&&e.calls==std::vector<int>({9}));}
    }
    {
        auto a=fresh();BlastEvents e;bool good=true;e.parent=57;
        assert(a.wallContact(e,good)&&a.hits.count==0&&e.calls==std::vector<int>({8}));
        e.parent=84;e.hp={};e.calls.clear();assert(a.wallContact(e,good)&&a.hits.count==1);
        e.hp=MotionBinary64::fromInteger(60);e.calls.clear();assert(a.wallContact(e,good)&&e.calls==std::vector<int>({8,9}));
    }
    for(unsigned fail:{0u,1u,2u,3u,4u,7u}){
        auto a=fresh();BlastEvents e;e.fail=fail;bool good=true;
        assert(a.shootableContact(e,good)==!fail&&e.calls.size()==(fail?fail:7));
        if(!fail)assert(e.calls==std::vector<int>({9,12,12,13,14,15,16})&&equal(e.hp,MotionBinary64::fromInteger(10)));
        if(fail==7)assert(equal(e.hp,MotionBinary64::fromInteger(10))&&a.hits.count==1);
    }
    for(unsigned fail:{0u,1u,2u,3u,4u,5u,6u,7u,8u,9u,10u,11u,12u,15u,16u}){
        auto a=fresh();BlastEvents e;e.fail=fail;bool good=true;
        assert(a.enemyContact(e,good)==!fail&&e.calls.size()==(fail?fail:16));
        if(!fail){assert(e.calls==std::vector<int>({17,18,19,9,20,21,19,22,23,2,24,25,14,15,26,2})&&
            equal(e.hp,MotionBinary64::fromInteger(10))&&e.hit&&e.stun&&equal(e.impact,MotionBinary64::fromInteger(10)));
            e.calls.clear();assert(a.enemyContact(e,good)&&e.calls==std::vector<int>({17,18,19,9}));}
        else {const auto before=e.calls.size();assert(!a.enemyContact(e,good)&&e.calls.size()==before);}
    }
    for(unsigned guard=0;guard<4;++guard){
        auto a=fresh();BlastEvents e;bool good=true;
        if(guard==0)a.damageEnd=true;else if(guard==1)e.through=true;else if(guard==2)e.immune=true;else e.hp={};
        assert(a.enemyContact(e,good)&&a.hits.count==0&&e.calls.size()==guard);
    }
    {
        auto a=fresh();BlastEvents e;bool good=true;a.consistent=false;a.explode=true;
        assert(a.enemyContact(e,good)&&a.destroyRequested&&a.hits.count==0&&
            e.calls==std::vector<int>({17,18,19,14,15,27,7,20,21,19,22,23,2,24,25,14,15,26,2}));
    }
    std::puts("Blast: Create prefixes, 28-frame lifecycle, shared hit memory, wall/object/enemy guards and ordered damage passed");
}
#endif

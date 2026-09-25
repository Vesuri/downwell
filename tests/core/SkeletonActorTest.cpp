#include "core/SkeletonActor.h"
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
#include <cstring>
#include <cmath>
#include <vector>
static downwell::MotionBinary64 number(double value) {
    std::uint64_t raw; std::memcpy(&raw,&value,8);
    return {std::uint32_t(raw>>32),std::uint32_t(raw)};
}
static void check(bool ok) { if (!ok) std::abort(); }
struct Events {
    downwell::SkeletonActor& actor;
    unsigned order=0;
    downwell::MotionBinary64 expectedImpact=number(9);
    void hurtSound(int n) { check(n==133); }
    bool throwBone(int) { return true; }
    void enemyDeath() { check(++order==1); }
    void blood(int a,int b) { check(++order==2 && a==10 && b==3); }
    void smoke(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int a,int b) {
        check(++order==3 && x.high==actor.xx.high && y.high==actor.yy.high && a==5 && b==1);
    }
    void flash(downwell::MotionBinary64,downwell::MotionBinary64,int a,int b,int c) {
        check(++order==4 && a==6 && b==2 && c==10);
    }
    void currency(int n) { check(++order==5 && n==10); }
    void deathSound(int n) { check(++order==6 && n==132); }
    void deadBody(int sprite,downwell::MotionBinary64 impact,int debris) { check(++order==7 && sprite==206 && impact.high==expectedImpact.high && impact.low==expectedImpact.low && debris==102); }
    void debris(int x,int y) { ++order; check((order==8 || order==9) && x==actor.x && y==actor.y-5); }
    void destroy() { check(++order==10 && actor.destroyRequested && actor.takenImpact.high==expectedImpact.high && actor.takenImpact.low==expectedImpact.low); }
};
struct Contact {
    bool grounded=false, mutate=false;
    unsigned stomps=0, damageCalls=0;
    bool skeletonBelowPlayer() { return true; }
    bool playerGrounded() { return grounded; }
    bool playerDescending() { return true; }
    bool playerBelowSkeleton() { return false; }
    void stomp() { ++stomps; if (mutate) grounded=true; }
    void damage(int a,int b,int c) { check(a==1 && b==3 && c==2); ++damageCalls; }
};
struct BulletEvents {
    downwell::SkeletonActor& actor;
    std::vector<int> calls;
    void destroyBullet(){calls.push_back(1);}
    downwell::MotionBinary64 impactAngle(int x,int y) {
        check(x==96 && y==192 && actor.hitPending); calls.push_back(2); return number(42);
    }
    void momentDelay(){check(actor.damageStun);calls.push_back(3);}
    void bulletHitSound(int id){check(id==29);calls.push_back(4);}
    void impactEffect(int sprite,int speed,downwell::MotionBinary64 direction,int depth) {
        check(sprite==605 && speed==4 && direction.high==number(270).high && depth==0);
        calls.push_back(5);
    }
};
int main() {
    for(int health:{-1,0,1,9,15,60}) for(int damage:{9,15})
    for(bool stunned:{false,true}) for(bool active:{false,true}) {
        downwell::SkeletonActor s; check(s.create(96,192));
        s.health=health; s.hitStun=stunned; s.active=active;
        BulletEvents events{s,{}};
        check(s.hitByMachineGun(damage,number(270),events));
        if(health>0) {
            std::vector<int> expected={1,2,3,4,5}; if(!stunned) expected.push_back(3);
            check(events.calls==expected && s.health==health-damage && s.hitDamage==damage);
            const auto impact=number(damage/5.0);
            check(s.takenImpact.high==impact.high && s.takenImpact.low==impact.low);
            check(s.hitPending && s.damageStun && s.hitAngle.high==number(42).high);
        } else check(events.calls.empty() && s.health==health && !s.hitPending && !s.damageStun);
        check(s.alive && !s.destroyRequested && s.active==active);
    }
    {
        downwell::SkeletonActor s; check(s.create(96,192)); BulletEvents events{s,{}};
        check(!s.hitByMachineGun(10,number(270),events) && events.calls.empty() && s.health==60);
        for(unsigned i=0;i<7;++i) check(s.hitByMachineGun(9,number(270),events));
        check(s.health==-3 && s.hitDamage==63 && s.alive && !s.destroyRequested);
        const auto count=events.calls.size();
        check(s.hitByMachineGun(9,number(270),events) && events.calls.size()==count);
        Events death{s}; death.expectedImpact=number(1.8);
        check(s.step(false,false,false,0,death));
        check(death.order==10 && s.destroyRequested && s.takenImpact.zero());
    }
    for(int coordinate=-4096;coordinate<=4096;++coordinate) {
        downwell::SkeletonActor s;
        check(s.create(coordinate,-coordinate));
        check(s.x==std::nearbyint(float(coordinate)/16.0f)*16 && s.y==-s.x);
    }
    {
        downwell::SkeletonActor s;
        check(s.create(16777217,-16777217) && s.x==16777216 && s.y==-16777216);
        check(!s.create(2147483647,0) && s.x==16777216); // Unsupported published integer: atomic failure.
        check(s.create(-2147483647-1,0) && s.x==(-2147483647-1));
        s.timefieldImmune=true; s.collideTimeField(); check(s.health==60);
        s.timefieldImmune=false; s.collideTimeField();
        check(s.health==0 && s.alive && !s.destroyRequested && !s.hitPending);
    }
    for(double dx:{-0.00002,-0.000001,0.0,0.000001,0.00002})
    for(double dy:{-0.00002,-0.000001,0.0,0.000001,0.00002})
    for(double vx:{-0.00002,-0.000001,0.0,0.000001,0.00002}) {
        downwell::SkeletonActor s; s.create(100,200); s.active=false; s.xsp=number(vx);
        Events e{s}; check(s.stepAtPlayer(false,number(96+dx),number(96+dy),e));
        const bool active=(96+dy)-96>0.00001;
        int facing=active && 96-(96+dx)>0.00001?-1:1;
        if(std::abs(vx)>0.00001) facing=vx<0?-1:1;
        check(s.active==active && s.facing==facing);
    }
    struct Retirement { unsigned calls=0; void destroy(){++calls;} };
    for(int y:{-100,200,300000}) for(bool mainRoom:{false,true})
    for(bool upper:{false,true}) for(double delta:{-1.0,-0.00002,-0.000001,0.0,0.000001,0.00002,1.0}) {
        downwell::SkeletonActor s; s.create(100,y); Retirement events;
        const double view=s.y+(upper?160.0:-3000.0)+delta;
        const bool outside=double(s.y)-(view-160)<-0.00001 || double(s.y)-(view+3000)>0.00001;
        check(s.endStep(mainRoom,number(view),events));
        check(s.destroyRequested==(mainRoom && outside) && events.calls==unsigned(mainRoom && outside));
        check(s.alive && s.health==60); // Culling is not a kill/reward event.
    }
    {
        int result=7;
        check(!downwell::compareRunnerReal({0x7ff00000u,0},{},result) && result==7);
    }
    for (double vx : {-1.2,-0.8,0.0,0.8,1.2}) for (double vy : {-0.2,0.0,0.2}) {
        downwell::SkeletonActor s; s.create(100,200); s.xsp=number(vx); s.ysp=number(vy);
        double x=s.x,y=s.y; Events e{s};
        for (int frame=0; frame<300; ++frame) {
            const bool stopped=frame%11==0; s.grounded=frame%7<3;
            if (!stopped) { if (!s.grounded) x+=vx; y+=vy; }
            check(s.step(stopped,false,false,vx<0?-1:vx>0?1:0,e));
            const auto nx=number(x), ny=number(y);
            check(s.xx.high==nx.high && s.xx.low==nx.low && s.yy.high==ny.high && s.yy.low==ny.low);
            check(s.x==std::nearbyint(x) && s.y==std::nearbyint(y));
        }
    }
    for (bool stopped : {false,true}) for (bool grounded : {false,true})
    for (bool dead : {false,true}) {
        downwell::SkeletonActor s; s.create(100,200);
        s.grounded=grounded; s.xsp=downwell::MotionBinary64::fromInteger(2);
        s.ysp=downwell::MotionBinary64::fromInteger(3);
        s.health=dead?0:60; s.hitStun=true; s.takenImpact=number(9);
        Events e{s}; check(s.step(stopped,false,false,1,e));
        check(s.x==96+(!stopped && !grounded?2:0));
        check(s.y==192+(!stopped?3:0));
        check(s.alive==!dead && s.destroyRequested==dead && e.order==(dead?10u:0u));
        check(s.takenImpact.zero() && s.sprite==205 && s.imageIndex.zero());
    }
    downwell::SkeletonActor s;
    Contact ordinary; s.collidePlayer(ordinary);
    check(ordinary.stomps==1 && ordinary.damageCalls==0);
    Contact mutated; mutated.mutate=true; s.collidePlayer(mutated);
    check(mutated.stomps==1 && mutated.damageCalls==1);
    std::puts("Skeleton actor movement, time-stop, death order and collision mutation checks passed");
}

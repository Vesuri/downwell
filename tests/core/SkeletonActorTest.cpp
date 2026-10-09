#include "core/SkeletonActor.h"
#include "core/PlayerMotion.h"
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
    unsigned failAt=0;
    void hurtSound(int n) { check(n==133); }
    bool throwBone(int) { return true; }
    bool enemyDeath() { check(++order==1); return order!=failAt; }
    bool blood(int a,int b) { check(++order==2 && a==10 && b==3); return order!=failAt; }
    bool smoke(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int a,int b) {
        check(++order==3 && x.high==actor.xx.high && y.high==actor.yy.high && a==5 && b==1);
        return order!=failAt;
    }
    bool flash(downwell::MotionBinary64,downwell::MotionBinary64,int a,int b,int c) {
        check(++order==4 && a==6 && b==2 && c==10);
        return order!=failAt;
    }
    bool currency(int n) { check(++order==5 && n==10); return order!=failAt; }
    void deathSound(int n) { check(++order==6 && n==132); }
    bool deadBody(int sprite,downwell::MotionBinary64 impact,int debris) { check(++order==7 && sprite==206 && impact.high==expectedImpact.high && impact.low==expectedImpact.low && debris==102); return order!=failAt; }
    bool debris(int x,int y) { ++order; check((order==8 || order==9) && x==actor.x && y==actor.y-5); return order!=failAt; }
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
struct CheckedBulletEvents {
    downwell::SkeletonActor& actor;unsigned fail=0,calls=0;bool hitIgnore=true;
    bool accept(){return ++calls!=fail;}
    bool clearHitIgnore(){hitIgnore=false;return accept();}
    bool destroyBullet(){check(!hitIgnore&&!actor.hitPending);return accept();}
    bool impactAngle(downwell::MotionBinary64& result){
        check(actor.hitPending&&actor.hitDirection.high==number(270).high);
        if(!accept())return false;result=number(42);return true;
    }
    bool momentDelay(){check(actor.damageStun);return accept();}
    bool bulletHitSound(int id,int priority,bool loop,bool cut){
        check(id==29&&priority==90&&!loop&&cut&&actor.damageStun);return accept();
    }
    bool impactEffect(int sprite,downwell::MotionBinary64 speed,downwell::MotionBinary64 direction,int depth){
        check(sprite==605&&speed.high==number(0.4).high&&speed.low==number(0.4).low&&
            direction.high==number(270).high&&depth==0&&!actor.takenImpact.zero());return accept();
    }
};
struct PreciseContact {
    double y,speed;
    bool onGround;
    int mutation;
    unsigned stomps=0,damages=0;
    downwell::MotionBinary64 globalY(){return number(y);}
    downwell::MotionBinary64 verticalSpeed(){return number(speed);}
    bool grounded(){return onGround;}
    void stomp(){++stomps;if(mutation==1) onGround=true;if(mutation==2) y=193;}
    void damage(int a,int b,int c){check(a==1 && b==3 && c==2);++damages;}
};
struct StompEvents {
    downwell::SkeletonActor& actor;
    bool dead,knees,capacity;
    int style;
    unsigned choice,smokes=0;
    std::vector<int> calls;
    PlayerMotion player{true,1000};
    unsigned failAt=0;
    bool playerDead(){return dead;}
    int playStyle(){return style;}
    bool explodingKnees(){return knees;}
    bool blood(int n,int speed){check(n==3 && speed==0 && actor.health==60);calls.push_back(1);return failAt!=1;}
    void forcedJump(int x,int y){
        check(x==0 && y==(style==2?15:25));
        player.stompBounce(style);calls.push_back(2);
    }
    void shake(int n,int time){check(n==2 && time==10 && actor.health==60);calls.push_back(3);}
    void stompSound(int id){check(id==122 && actor.health==-40);calls.push_back(4);}
    void recharge(){check(!actor.hitPending && !actor.stomped);calls.push_back(5);}
    bool createBlast(int object,int x,int y){
        check(actor.hitPending && actor.stomped && object==276 && x==96 && y==192);
        calls.push_back(6);return capacity;
    }
    unsigned choose(unsigned count){check(count==3 && calls.back()==6);calls.push_back(7);return choice;}
    void explosionSound(int id){check(id==91+int(choice));calls.push_back(8);}
    downwell::MotionBinary64 playerX(){return number(10.25+smokes);}
    downwell::MotionBinary64 playerY(){return number(20.5+smokes);}
    bool emitSmoke(downwell::MotionBinary64 x,downwell::MotionBinary64 y,int direction,int speed){
        const int directions[]={0,180,15,165};
        auto expectedX=number(10.25+smokes),expectedY=number(24.5+smokes);
        check(x.high==expectedX.high && x.low==expectedX.low && y.high==expectedY.high && y.low==expectedY.low);
        check(direction==directions[smokes] && speed==(smokes<2?4:3));
        calls.push_back(9+smokes++);
        return calls.back()!=int(failAt);
    }
};
int main() {
    for(bool stopped:{false,true}) for(unsigned failure:{1u,2u,3u,4u,5u,7u,8u,9u}){
        downwell::SkeletonActor actor;check(actor.create(96,192));
        actor.health=0;actor.takenImpact=number(9);
        Events events{actor};events.failAt=failure;
        check(!actor.step(stopped,false,false,0,events));
        check(events.order==failure&&!actor.destroyRequested&&!actor.alive);
        check(actor.takenImpact.high==number(9).high); // No event tail or retry.
    }
    for(unsigned failure:{1u,9u,10u,11u,12u}){
        downwell::SkeletonActor actor;check(actor.create(96,192));
        StompEvents events{actor,false,true,true,0,0,0,{}};events.failAt=failure;
        check(!actor.stompPlayer(events));
        check(events.calls.size()==failure&&events.calls.back()==int(failure));
        check(actor.health==(failure==1?60:-40)&&!actor.destroyRequested);
        check(events.smokes==(failure==1?0:failure-8));
    }
    for(bool dead:{false,true}) for(bool knees:{false,true}) for(bool capacity:{false,true})
    for(int style:{0,1,2,3,4}) for(unsigned choice:{0u,1u,2u}) {
        downwell::SkeletonActor actor;check(actor.create(96,192));
        StompEvents events{actor,dead,knees,capacity,style,choice,0,{}};
        check(actor.stompPlayer(events)==(dead || !knees || capacity));
        std::vector<int> expected;
        if(!dead) {
            expected={1,2,3,4,5};
            if(knees) {expected.push_back(6);if(capacity) for(int i=7;i<=12;++i) expected.push_back(i);}
        }
        check(events.calls==expected);
        check(actor.health==(dead?60:-40) && actor.hitPending==!dead && actor.stomped==!dead);
        check(actor.alive && !actor.destroyRequested && !actor.damageStun);
        check(events.player.grounded()==dead && events.player.spinJumping()==!dead);
        check(events.player.speedYFixed()==(dead?0:style==2?-384:-640));
    }
    for(int pose:{205,207,208}) for(int facing:{-1,1})
    for(int dx=-20;dx<=20;++dx) for(int dy=-20;dy<=20;++dy) {
        downwell::SkeletonActor skeleton;check(skeleton.create(96,192));
        skeleton.sprite=pose;skeleton.facing=facing;
        skeleton.xx=number(1000);skeleton.yy=number(1000);
        check(downwell::enemyContactMask(120,pose)==208);
        const downwell::ContactActor player{(96+dx)*256L,(192+dy)*256L,1,1,31};
        // Metadata: skeleton [-10,10)x[-7,8), player [-4,4)x[-4,8).
        const bool overlap=dx-4<10 && dx+4> -10 && dy-4<8 && dy+8> -7;
        check(skeleton.meetsPlayer(player)==overlap);
    }
    for(double offset:{-0.00002,-0.000001,0.0,0.000001,0.00002})
    for(double speed:{-0.00002,-0.000001,0.0,0.000001,0.00002})
    for(bool grounded:{false,true}) for(int mutation:{0,1,2}) {
        downwell::SkeletonActor skeleton;check(skeleton.create(96,192));
        skeleton.yy=number(1000); // Collision uses y, not precise yy.
        PreciseContact player{192+offset,speed,grounded,mutation};
        const bool stomp=192-player.y>0.00001 && !grounded && speed>0.00001;
        const bool damage=grounded || (stomp && mutation==1) ||
            ((stomp && mutation==2?193:player.y)-192>0.00001);
        check(skeleton.collidePlayerAt(player));
        check(player.stomps==unsigned(stomp) && player.damages==unsigned(damage));
    }
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
    for(int damage:{9,15})for(bool stunned:{false,true})for(unsigned fail=0;fail<=(stunned?6u:7u);++fail){
        downwell::SkeletonActor actor;check(actor.create(96,192));actor.hitStun=stunned;
        CheckedBulletEvents events{actor,fail};bool good=true;
        check(actor.hitByMachineGunChecked(damage,number(270),events,good)==!fail&&good==!fail);
        check(events.calls==(fail?fail:(stunned?6u:7u))&&!events.hitIgnore);
        const bool hit=!fail||fail>=3,damaged=!fail||fail>=4,impact=!fail||fail>=6;
        check(actor.hitPending==hit&&actor.damageStun==damaged&&actor.health==60-(damaged?damage:0)&&
            actor.hitDamage==(damaged?damage:0)&&actor.takenImpact.zero()==!impact&&actor.alive&&!actor.destroyRequested);
        if(fail){const auto calls=events.calls;check(!actor.hitByMachineGunChecked(damage,number(270),events,good)&&events.calls==calls);}
        else {
            downwell::SkeletonActor oracle;check(oracle.create(96,192));oracle.hitStun=stunned;BulletEvents reference{oracle,{}};
            check(oracle.hitByMachineGun(damage,number(270),reference)&&actor.health==oracle.health&&
                actor.hitDamage==oracle.hitDamage&&actor.hitAngle.high==oracle.hitAngle.high&&
                actor.takenImpact.high==oracle.takenImpact.high&&actor.takenImpact.low==oracle.takenImpact.low);
        }
    }
    for(int hp:{-1,0}){
        downwell::SkeletonActor actor;check(actor.create(96,192));actor.health=hp;
        CheckedBulletEvents events{actor};bool good=true;
        check(actor.hitByMachineGunChecked(9,number(270),events,good)&&good&&!events.calls&&events.hitIgnore);
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
    std::puts("Skeleton actor: 8 death failure boundaries with/without time-stop, 5 stomp failure boundaries, movement, death order, 120 stomps, 10086 masks and 150 precise contacts passed");
}

#include "core/SkeletonActor.h"
#include <cstdlib>
#include <cstdio>
#include <initializer_list>
#include <cstring>
#include <cmath>
static downwell::MotionBinary64 number(double value) {
    std::uint64_t raw; std::memcpy(&raw,&value,8);
    return {std::uint32_t(raw>>32),std::uint32_t(raw)};
}
static void check(bool ok) { if (!ok) std::abort(); }
struct Events {
    downwell::SkeletonActor& actor;
    unsigned order=0;
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
    void deadBody(int sprite,int impact,int debris) { check(++order==7 && sprite==206 && impact==9 && debris==102); }
    void debris(int x,int y) { ++order; check((order==8 || order==9) && x==actor.x && y==actor.y-5); }
    void destroy() { check(++order==10 && actor.destroyRequested && actor.takenImpact==9); }
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
int main() {
    for (double vx : {-1.2,-0.8,0.0,0.8,1.2}) for (double vy : {-0.2,0.0,0.2}) {
        downwell::SkeletonActor s; s.create(100,200); s.xsp=number(vx); s.ysp=number(vy);
        double x=100,y=200; Events e{s};
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
        s.health=dead?0:60; s.hitStun=true; s.takenImpact=9;
        Events e{s}; check(s.step(stopped,false,false,1,e));
        check(s.x==100+(!stopped && !grounded?2:0));
        check(s.y==200+(!stopped?3:0));
        check(s.alive==!dead && s.destroyRequested==dead && e.order==(dead?10u:0u));
        check(s.takenImpact==0 && s.sprite==205 && s.imageIndex.zero());
    }
    downwell::SkeletonActor s;
    Contact ordinary; s.collidePlayer(ordinary);
    check(ordinary.stomps==1 && ordinary.damageCalls==0);
    Contact mutated; mutated.mutate=true; s.collidePlayer(mutated);
    check(mutated.stomps==1 && mutated.damageCalls==1);
    std::puts("Skeleton actor movement, time-stop, death order and collision mutation checks passed");
}

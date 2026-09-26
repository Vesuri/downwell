#include "core/SkeletonActor.h"
#include "core/BoneProjectileStore.h"
#include "core/GameMakerRandom.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Joined event-loop regression, not a Windows execution oracle. The scalar
// model uses native float/double rather than the target's arithmetic helpers.
static unsigned tick;
static void checkAt(bool value,int line) {
    if (!value) { std::fprintf(stderr,"skeleton encounter failed at tick %u, line %d\n",tick,line); std::abort(); }
}
#define check(value) checkAt((value),__LINE__)
static double real(downwell::MotionBinary64 value) {
    const std::uint64_t raw=(std::uint64_t(value.high)<<32)|value.low;
    double result; std::memcpy(&result,&raw,8); return result;
}
static float image(downwell::RunnerImageValue value) {
    float result; std::memcpy(&result,&value.bits,4); return result;
}
static float advance(float position,float speed,int count) {
    float next=position+speed;
    if(next<0 || next>=count)
        next=float(std::fmod(double(float(float(std::fmod(double(next),count))+count)),count));
    return next;
}
struct Reference {
    float timeline=0,index=0,speed=0;
    double rate=0;
    int sprite=208,facing=1;
    std::array<int,4> alarms{{-1,-1,-1,-1}};
    bool shot=false,stun=false,active=true;
    void select(int next) {
        if(sprite==next) return;
        int count=next==205?1:next==207?8:6;
        if(int(index)>=count) index=timeline=0;
        sprite=next;
    }
    void animate() {
        const int count=sprite==205?1:sprite==207?8:6;
        if(timeline>=count || std::fabs(float(index-timeline))>0.00001) {
            check(index==0); timeline=0;
        }
        index=timeline=advance(timeline,speed,count);
    }
    void alarmPhase() {
        for(unsigned i=0;i<4;++i) if(alarms[i]>=0 && --alarms[i]==0) {
            if(i==1) stun=false;
            if(i==2) { index=0;shot=false;alarms[3]=15; }
            if(i==3) rate=0.3;
        }
    }
    bool step(bool stopped,bool hit,int playerX,int playerY) {
        bool fire=false;
        if(!stopped) {
            speed=float(rate);
            if(playerY>96) active=true;
            if(hit) { active=true;stun=true;alarms[1]+=5;index=0;rate=0;alarms[3]=30; }
            if(!active) { select(208);rate=0.2; }
            else {
                if(sprite==208) {index=0;rate=0.3;}
                select(207);
                if(double(index)-3 < -0.00001) facing=96>playerX?-1:1;
                if(double(index)-4 > 0.00001 && !shot) { fire=true;shot=true; }
                if(double(index)-6 > 0.00001 && rate!=0) { rate=0;alarms[2]=30; }
            }
        } else { speed=0;if(alarms[0]>0) ++alarms[0]; }
        if(stun) { index=0;select(205); }
        return fire;
    }
};
using Store=downwell::BoneProjectileStore<32>;
struct Effects {
    downwell::SkeletonActor& actor;
    Store& store;
    downwell::GameMakerRandom& random;
    bool separated=true;
    unsigned born=0,throws=0;
    void operator()(int id) {check(id==134);}
    bool throwBone(int facing) {
        downwell::SkeletonBoneCreation<Store,downwell::GameMakerRandom,Effects>
            creation{store,random,*this,actor.shot};
        if(!creation.run(actor.x,actor.y,separated,facing)) return false;
        born=creation.child;++throws;return true;
    }
    void hurtSound(int id) {check(id==133);}
    bool enemyDeath(){check(false);return false;}
    bool blood(int,int){check(false);return false;}
    bool smoke(downwell::MotionBinary64,downwell::MotionBinary64,int,int){check(false);return false;}
    bool flash(downwell::MotionBinary64,downwell::MotionBinary64,int,int,int){check(false);return false;}
    bool currency(int){check(false);return false;}
    void deathSound(int){check(false);}
    bool deadBody(int,downwell::MotionBinary64,int){check(false);return false;}
    bool debris(int,int){check(false);return false;}
    void destroy(){check(false);}
};
struct BoneEffects {
    unsigned destroyed=0;
    void destroy(){++destroyed;}
    void effect(int,int){check(false);}
};
struct ExpectedBone {
    unsigned handle=0,birth=0;
    double x=96,y=182,vx=0,vy=0,rate=0;
    float index=0,speed=1;
};
int main() {
    unsigned totalThrows=0,totalRetired=0;
    for(unsigned seed=0;seed<64;++seed) {
        Store store;downwell::SkeletonActor actor;check(actor.create(96,192));
        Reference reference;
        actor.active=reference.active=(seed%2==0);
        downwell::GameMakerRandom random(seed),oracle(seed);
        Effects events{actor,store,random};
        std::array<ExpectedBone,32> expected{};
        bool pendingHit=false;
        for(tick=0;tick<900;++tick) {
            const bool stopped=(tick%173)>=140;
            const int playerX=(tick/71)%3==0?96:(tick/71)%3==1?64:128;
            const int playerY=tick<60?0:192;
            if(tick==100 || tick==400) actor.hitPending=pendingHit=true;
            bool crossed=false;check(actor.advanceImage(crossed));reference.animate();
            const auto phase=store.beginPhase();
            store.visitPhase(phase,[&](unsigned,downwell::BoneProjectile& bone){check(bone.advanceImage(crossed));});
            for(auto& bone:expected) if(bone.handle) bone.index=advance(bone.index,bone.speed,4);
            for(unsigned i=0;i<4;++i) actor.advanceAlarm(i);
            reference.alarmPhase();
            events.separated=playerX!=96;events.born=0;
            const bool fire=reference.step(stopped,pendingHit,playerX,playerY);
            if(!stopped) pendingHit=false;
            check(actor.stepAtPlayer(stopped,downwell::MotionBinary64::fromInteger(playerX),
                                    downwell::MotionBinary64::fromInteger(playerY),events));
            check(bool(events.born)==fire);
            if(fire) {
                ExpectedBone* bone=nullptr;
                for(auto& slot:expected) if(!slot.handle) {bone=&slot;break;}
                check(bone!=nullptr);*bone={};bone->handle=events.born;bone->birth=tick;
                oracle.random(1.5);oracle.random(3);bone->rate=0.05+oracle.random(0.45);
                const double vertical[]={-3,-3.1,-3.2},horizontal[]={1,0.8,1.2};
                bone->vy=vertical[oracle.nextU32()%3];
                bone->vx=playerX==96?1:reference.facing*horizontal[oracle.nextU32()%3];
                if(bone->vx>0) bone->rate=-bone->rate;
                ++totalThrows;
            }
            BoneEffects effects;
            store.visitPhase(phase,[&](unsigned handle,downwell::BoneProjectile& bone){
                check(bone.step(stopped,[](int x,int y){return x>=-160 && x<=320 && y<=320;},effects));
                if(bone.destroyRequested) {check(store.retire(handle));++totalRetired;}
            });
            for(auto& bone:expected) if(bone.handle) {
                if(bone.birth!=tick) {
                    bone.speed=stopped?0:float(bone.rate);
                    if(!stopped) {bone.vy+=0.1;bone.x+=bone.vx;bone.y+=bone.vy;}
                }
                const int x=int(std::nearbyint(bone.x)),y=int(std::nearbyint(bone.y));
                auto* actual=store.get(bone.handle);
                if(x< -160 || x>320 || y>320) {check(!actual);bone.handle=0;continue;}
                check(actual && actual->motion.x==x && actual->motion.y==y);
                check(real(actual->motion.xx)==bone.x && real(actual->motion.yy)==bone.y);
                check(real(actual->motion.xsp)==bone.vx && real(actual->motion.ysp)==bone.vy);
                check(real(actual->sampledAnimationRate)==bone.rate);
                check(image(actual->imageIndex)==bone.index && image(actual->imageSpeed)==bone.speed);
            }
            unsigned actualCount=0,expectedCount=0;
            for(unsigned i=0;i<32;++i) {
                actualCount+=store.slot(i).handle!=0;
                expectedCount+=expected[i].handle!=0;
            }
            check(actualCount==expectedCount);
            check(actor.sprite==reference.sprite && actor.facing==reference.facing);
            check(real(actor.imageIndex)==reference.index && real(actor.imageSpeed)==reference.speed);
            check(real(actor.rate)==reference.rate && actor.shot==reference.shot && actor.hitStun==reference.stun);
            for(unsigned i=0;i<4;++i) check(actor.alarms[i]==reference.alarms[i]);
            check(random.drawCount()==oracle.drawCount() && random.stateIndex()==oracle.stateIndex());
            for(unsigned i=0;i<16;++i) check(random.stateWord(i)==oracle.stateWord(i));
        }
        check(events.throws>=8);
    }
    check(totalRetired>0);
    std::printf("Skeleton encounter: 57600 updates, %u throws, %u retirements passed\n",totalThrows,totalRetired);
}

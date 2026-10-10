#include "core/ShopKeeper.h"
#include "core/ShopKeeperEvents.h"
#include "core/ShopKeeperAnimation.h"
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::abort(); }
}
int main() {
    using namespace downwell;
    const int sprites[]={485,486,487,496,497};
    for(int sprite:sprites)for(float speed:{0.0f,0.09f,0.4f,0.2f,0.045f}){
        RunnerImageValue timeline{},index{},rate;std::memcpy(&rate.bits,&speed,4);
        float expected=0;const unsigned frames=ShopKeeperAnimation::frames(sprite);
        for(unsigned tick=0;tick<1000;++tick){
            if(tick%71==0){expected=sprite==487&&(tick&1)?1.0f:0.0f;std::memcpy(&index.bits,&expected,4);}
            expected=float(double(expected)+double(speed));
            if(expected>=frames)expected=float(std::fmod(float(std::fmod(expected,float(frames)))+float(frames),float(frames)));
            require(ShopKeeperAnimation::advance(timeline,sprite,index,rate,[&](RunnerImageValue next){index=next;return true;}),
                "registered keeper animation advances");
            unsigned bits;std::memcpy(&bits,&expected,4);
            require(index.bits==bits&&timeline.bits==bits,"keeper animation matches scalar rounding and wraps");
        }
    }
    RunnerImageValue rejectedTimeline{0x3f000000u};
    require(!ShopKeeperAnimation::advance(rejectedTimeline,485,rejectedTimeline,{0x3db851ecu},
        [](RunnerImageValue){return false;})&&rejectedTimeline.bits==0x3f000000u,"failed image publication retains timeline");
    require(!ShopKeeperAnimation::advance(rejectedTimeline,485,{0x3fc00000u},{},
        [](RunnerImageValue){return true;}),"unsupported explicit inverse is not approximated");
    struct Ops {
        int selected=485;RunnerImageValue frame{},rate{};AlarmBank clocks;
        int player=190;unsigned zone=1,rolls=0;bool crime=false,dead=false,spawned=false,tail=false,rejectImage=false;
        MotionBinary64 x(){return MotionBinary64::fromInteger(200);}
        MotionBinary64 y(){return MotionBinary64::fromInteger(700);}
        MotionBinary64 imageIndex(){return frame.promoted();}
        MotionBinary64 imageSpeed(){return rate.promoted();}
        int spriteIndex(){return selected;}
        bool playerX(MotionBinary64& value){value=MotionBinary64::fromInteger(player);return true;}
        bool criminal(bool& value){value=crime;return true;}
        bool area(unsigned& value){tail=true;value=zone;return true;}
        bool sprite(int value){selected=value;return true;}
        bool speed(MotionBinary64 value){return rate.assign(value);}
        bool image(RunnerImageValue value){if(rejectImage)return false;frame=value;return true;}
        bool alarm(unsigned n,int value){clocks.set(n,value);return true;}
        bool range(unsigned lo,unsigned hi,unsigned& value){require(lo==20&&hi==60,"registered keeper range");++rolls;value=37;return true;}
        bool destroy(){dead=true;return true;}
        bool createEnemy(int object,MotionBinary64 px,MotionBinary64 py){
            require(dead&&object==284&&px.high==x().high&&py.high==y().high,"destroy before replacement Create");
            spawned=true;return true;
        }
    } ops;
    ShopChildState state;
    require(ShopKeeperEvents::step(state,ops)&&ops.selected==485&&ops.rate.bits==0x3db851ecu,"source idle setter narrows to binary32");
    ops.player=200;
    require(ShopKeeperEvents::step(state,ops)&&ops.selected==487&&!ops.rate.bits&&ops.rolls==1&&ops.clocks.get(0)==37,"registered angry transition");
    require(ShopKeeperEvents::step(state,ops)&&ops.rolls==1,"retained angry sprite does not resample delay");
    ops={};state={};state.thankful=state.confused=1;ops.frame.bits=0x40900000u;
    require(ShopKeeperEvents::step(state,ops)&&state.thankful==2&&!state.confused&&!ops.rate.bits&&ops.clocks.get(2)==60,"registered thank completion");
    ops={};state={};state.goodbye=1;
    require(ShopKeeperEvents::step(state,ops)&&state.goodbye==2&&ops.selected==496&&ops.rate.bits==0x3e4ccccdu&&ops.clocks.get(2)==240,"registered goodbye branch");
    ops={};state={};state.confused=1;ops.frame.bits=0x3fc00000u;
    require(ShopKeeperEvents::step(state,ops)&&state.confused==2&&ops.selected==497&&!ops.rate.bits&&ops.clocks.get(2)==60,"registered confused completion");
    ops={};state={};ops.zone=3;
    require(ShopKeeperEvents::step(state,ops)&&ops.rate.bits==0x3d3851ecu,"area three halves narrowed speed");
    ops={};state={};ops.crime=true;ops.player=201;
    require(ShopKeeperEvents::step(state,ops)&&ops.dead&&ops.spawned&&ops.tail,"event continues after deferred destruction");
    ops={};state={};ops.player=200;ops.rejectImage=true;
    require(!ShopKeeperEvents::step(state,ops)&&ops.selected==487&&!ops.rolls&&ops.clocks.get(0)==-1&&!ops.tail,"rejected image preserves prefix and stops RNG/tail");
    downwell::ShopKeeper keeper;
    downwell::ShopFeedback feedback;
    unsigned rolls = 0;
    auto random = [&](unsigned lo, unsigned hi) { require(lo == 20 && hi == 60, "source alarm range"); ++rolls; return 20; };
    auto tick = [&](int x, unsigned area = 1) {
        keeper.alarms(feedback, random);
        keeper.step(feedback, x, 200, false, area, random);
        keeper.advanceAnimation();
        feedback.beginStep();
    };
    volatile double reference = 0;
    for (int i = 0; i < 10000; ++i) {
        tick(190); reference = reference + 0.09;
        if (reference >= 4) reference = reference - 4;
        require(keeper.frame() == static_cast<unsigned>(reference), "idle frame matches binary64 runner arithmetic");
    }
    require(rolls == 0, "idle consumes no random draws");
    tick(200);
    require(keeper.sprite == 487 && keeper.frame() == 0 && keeper.alarm0 == 20 && rolls == 1,
        "equal X enters angry state and rolls once");
    for (int i = 0; i < 19; ++i) tick(200);
    require(keeper.frame() == 0, "angry alarm does not fire early");
    tick(200);
    require(keeper.frame() == 1 && keeper.alarm1 == 4, "alarm zero arms a later slot consumed in this phase");
    for (int i = 0; i < 3; ++i) tick(200);
    require(keeper.frame() == 1 && keeper.alarm1 == 1 && rolls == 1, "blink does not end early");
    tick(200);
    require(keeper.frame() == 0 && rolls == 2 && keeper.alarm0 == 20, "earlier-slot random wait is not decremented again");
    feedback.thankful = 1; feedback.resetKeeperImage = true;
    for (int i = 0; i < 12; ++i) tick(190);
    require(keeper.sprite == 486 && keeper.frame() == 4 && feedback.thankful == 2 && keeper.alarm2 == 60,
        "thank animation holds its final frame for sixty updates");
    for (int i = 0; i < 60; ++i) tick(190);
    require(feedback.thankful == 0 && keeper.sprite == 485, "alarm two returns keeper to idle");
    feedback.goodbye = 1; feedback.resetKeeperImage = true;
    tick(190);
    require(feedback.goodbye == 2 && keeper.alarm2 == 240 && keeper.sprite == 496,
        "goodbye starts a 240-update looping animation");
    tick(201);
    require(!feedback.goodbye, "moving right of keeper clears reaction flags at Step tail");
    tick(201);
    require(keeper.sprite == 487, "angry sprite follows on next Step");
    keeper = {}; feedback = {}; feedback.confused = 1; feedback.resetKeeperImage = true;
    for (int i = 0; i < 7; ++i) tick(190);
    require(keeper.sprite == 497 && keeper.frame() == 1 && feedback.confused == 2 && keeper.alarm2 == 60,
        "confused reaction freezes after passing image index one");
    feedback.thankful = feedback.goodbye = feedback.confused = 1;
    feedback.resetKeeperImage = true;
    tick(190);
    require(keeper.sprite == 486 && feedback.confused == 0 && feedback.goodbye == 1 &&
        keeper.speed.equal(downwell::HudReal{0x1999999999999aULL, -3}),
        "thank branch wins, but goodbye tail still overrides its animation speed");
    keeper = {}; feedback = {}; reference = 0;
    for (int i = 0; i < 1000; ++i) {
        tick(190, 3); reference = reference + 0.045;
        if (reference >= 4) reference = reference - 4;
        require(keeper.frame() == static_cast<unsigned>(reference), "area three halves animation rate");
    }
    keeper.step(feedback, 201, 200, true, 1, random);
    require(!keeper.present && keeper.spawnEnemy, "criminal branch requests enemy replacement");
    keeper={};feedback={};keeper.sprite=487;keeper.alarm0=keeper.alarm1=keeper.alarm2=1;
    feedback.thankful=feedback.goodbye=feedback.confused=1;
    const auto before=rolls;
    keeper.alarms(feedback,random);
    require(keeper.frame()==1&&keeper.alarm0==0&&keeper.alarm1==4&&keeper.alarm2==0&&rolls==before,
        "alarm zero overwrites a due later alarm before it is sampled; no spurious RNG");
    require(!feedback.thankful&&!feedback.goodbye&&!feedback.confused,"alarm two still runs after earlier mutations");
    keeper={};keeper.alarm0=keeper.alarm1=0;
    keeper.alarms(feedback,random);
    require(keeper.alarm0==-1&&keeper.alarm1==-1&&rolls==before,"zero counters retire without dispatch");
    std::puts("Recovered merchant Step, alarms and binary64 animation tests passed");
}

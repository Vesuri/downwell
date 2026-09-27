#include "core/GameplayPlayerStep.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iterator>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
struct Fixture {
    PlayerMotion player{true};
    downwell::PlayerDeathSequence death;
    downwell::RunSession run;
    downwell::CurrencyAccount currency;
    downwell::GameMakerRandom random{42};
    int health = 4;
    Fixture() { run.reset(); run.stage.advance(1); }
    void step(const downwell::Input& input = {}, bool locked = false) {
        downwell::stepGameplayPlayer(player, death, run, currency, health,
            random, input, 240, 176, 160, 34, locked);
    }
};
}

int main() {
    for(bool accept:{false,true}){
        Fixture f;downwell::Input right;right.right=true;unsigned transitions=0;bool published=false;
        downwell::stepGameplayPlayer(f.player,f.death,f.run,f.currency,f.health,f.random,right,
            240,176,160,34,false,[&](int,int){published=true;return true;},downwell::NoPlayerScalePublication{},
            downwell::UnindexedPlayerGoalContact{},[&](const auto& motion,int x,int y){
                ++transitions;
                require(published&&f.run.clock.ticks==1&&!f.run.shops.previous.right,
                    "transition follows position/clock but precedes later shop processing");
                require(&motion==&f.player&&x==240+downwell::roundGameMakerQ8(motion.offsetXFixed())&&
                    y==176+downwell::roundGameMakerQ8(motion.offsetYFixed()),"transition sees authoritative current player motion");
                return accept;
            });
        require(transitions==1&&f.run.effectsSucceeded==accept&&f.run.shops.previous.right==accept,
            "transition failure stops subsequent shared player work");
    }
    for(unsigned guard:{0u,1u,2u})for(bool accept:{false,true})for(bool hit:{false,true}){
        Fixture f;f.run.goalStopped=guard==1;f.death.active=guard==2;
        unsigned calls=0;
        downwell::Input right;right.right=true;
        downwell::stepGameplayPlayer(f.player,f.death,f.run,f.currency,f.health,f.random,right,
            240,176,160,34,false,downwell::NoPlayerPositionPublication{},downwell::NoPlayerScalePublication{},
            [&](downwell::RunSession&,downwell::MotionBinary64 x,downwell::MotionBinary64 y,int,bool& found){
                ++calls;
                const auto px=downwell::MotionBinary64::fromInteger(240),py=downwell::MotionBinary64::fromInteger(176);
                require(x.high==px.high&&x.low==px.low&&y.high==py.high&&y.low==py.low,
                    "bound goal query receives old published coordinates");
                found=hit;return accept;
            });
        require(calls==(guard?0u:1u)&&f.run.effectsSucceeded==(guard||accept),"bound query preserves guards and checked failure");
        require(f.run.clock.ticks==unsigned(!guard&&accept&&!hit),"bound collision result controls clock without fallback");
        require(f.run.shops.previous.right==(guard||accept),"query failure stops before shop input");
    }
    for(bool accept:{false,true}){
        Fixture f;downwell::Input right;right.right=true;
        bool called=false;
        downwell::stepGameplayPlayer(f.player,f.death,f.run,f.currency,f.health,f.random,right,
            240,176,160,34,false,[&](int x,int y){
                called=true;
                require(f.run.clock.ticks==1&&!f.run.shops.previous.right,
                        "position publication follows clock but precedes shop input");
                require(x==240+downwell::roundGameMakerQ8(f.player.offsetXFixed())&&
                        y==176+downwell::roundGameMakerQ8(f.player.offsetYFixed()),
                        "publish rounded source instance coordinates, not private fixed-point values");
                return accept;
            });
        require(called&&f.run.effectsSucceeded==accept&&f.run.shops.previous.right==accept,
                "rejected position publication stops later phases without undoing the clock");
    }
    for(unsigned guard:{0u,1u,2u}){
        Fixture f;
        // An unsupported field makes an accidentally evaluated query fail.
        f.run.levelExitPresent=true;f.run.goalField.x=2147483647;
        f.run.goalStopped=guard==1;f.death.active=guard==2;
        downwell::Input right;right.right=true;
        f.step(right);
        require(f.run.effectsSucceeded==(guard!=0),"clock guards skip the field query itself");
        require(f.run.clock.ticks==0,"guarded or failed clock query never increments time");
        require(f.run.shops.previous.right==(guard!=0),
                "failed clock query stops before shop input processing; guarded clock still permits it");
        require(f.random.drawCount()==0,"clock guard and failure do not consume RNG");
    }
    {
        Fixture failed;
        failed.player.configureSolidResolver([](void*,int,int y){return y<=-12;},nullptr,-60,60);
        failed.player.configureBreakBoxResolver([](void*,long,long,bool& found){found=false;return false;},nullptr);
        downwell::Input jump;jump.fire=true;
        failed.step(jump);
        const auto ticks=failed.run.clock.ticks;
        failed.step(jump);
        require(!failed.run.effectsSucceeded&&!failed.player.motionSucceeded(),
                "checked head-box failure reaches shared run owner");
        require(failed.run.clock.ticks==ticks,"failed player step does not run later clock phase");
    }
    std::ifstream maskFile("generated/downwell-goal-mask.mask", std::ios::binary);
    const std::vector<unsigned char> mask((std::istreambuf_iterator<char>(maskFile)), {});
    require(mask.size() == 2048, "original precise mask is available for independent pixel queries");
    downwell::GoalTimeField field;
    field.x = 240; field.y = 4000;
    downwell::RunSession contacts;
    contacts.levelExitPresent=true;contacts.goalField=field;
    for (int dy = -144; dy <= 144; ++dy) for (int dx = -144; dx <= 144; ++dx) {
        bool expected = false;
        for (int py = dy - 4; py < dy + 8 && !expected; ++py)
            for (int px = dx - 4; px < dx + 4; ++px) {
                if (px < -128 || px >= 128 || py < -128 || py >= 128) continue;
                const int x = (px + 128) / 2, y = (py + 128) / 2;
                if (mask[y * 16 + x / 8] & (128 >> (x % 8))) { expected = true; break; }
            }
        require(field.meetsPlayer(240 + dx, 4000 + dy) == expected,
                "compact overlap exactly matches original mask pixels at every boundary position");
        for(int facing:{-1,1}){
            bool actual=!expected;
            require(contacts.playerGoalContact(downwell::MotionBinary64::fromInteger(240+dx),
                downwell::MotionBinary64::fromInteger(4000+dy),facing,actual) && actual==expected,
                "live field adapter preserves integral clock contacts in both directions");
        }
    }
    unsigned differentPositions=0;
    for(int dy=-136;dy<=136;dy+=2)for(int dx=-136;dx<=136;dx+=2){
        auto x=downwell::MotionBinary64::fromInteger(240+dx);
        auto y=downwell::MotionBinary64::fromInteger(4000+dy);
        require(x.add({0x3fd00000u,0}) && y.add({0x3fe80000u,0}),"fractional fixture setup");
        downwell::RunnerImageValue px,py,sx,sy;
        require(px.assign(x) && py.assign(y) && sx.assign(downwell::MotionBinary64::fromInteger(-1)) &&
            sy.assign(downwell::MotionBinary64::fromInteger(1)),"query transform setup");
        bool expected=false,actual=false;
        require(downwell::TimeStopGoalContact::query(downwell::TimeStopGoalContact::Mask::Player,
            px,py,sx,sy,field,expected) && contacts.playerGoalContact(x,y,-1,actual) && expected==actual,
            "fractional adapter uses precise mirrored query rather than rounded publication");
        differentPositions+=actual!=field.meetsPlayer(240+dx,4000+dy);
    }
    require(differentPositions!=0,"old publication and private motion can disagree at a field boundary");
    bool unchanged=true;
    contacts.goalField.x=2147483647;
    require(!contacts.playerGoalContact({}, {},1,unchanged) && !contacts.effectsSucceeded && unchanged,
        "unsupported field coordinates latch failure without publishing a false overlap");
    contacts.reset();
    require(contacts.playerGoalContact({}, {},1,unchanged) && !unchanged && contacts.effectsSucceeded,
        "run reset clears query failure and retires the field");
    Fixture entering;
    entering.player = PlayerMotion(false, 1000);
    entering.run.levelExitPresent = true;
    entering.run.goalField.x = 240; entering.run.goalField.y = 312;
    bool crossed = false;
    for (unsigned tick = 0; tick < 15; ++tick) {
        const bool priorOverlap = entering.run.goalField.meetsPlayer(240,
            176 + downwell::roundGameMakerQ8(entering.player.offsetYFixed()));
        const auto priorClock = entering.run.clock.ticks;
        entering.step();
        const bool currentOverlap = entering.run.goalField.meetsPlayer(240,
            176 + downwell::roundGameMakerQ8(entering.player.offsetYFixed()));
        crossed |= !priorOverlap && currentOverlap;
        require(entering.run.clock.ticks == priorClock + !priorOverlap,
                "exit-field clock checks old published position, not newly integrated movement");
    }
    require(crossed, "clock fixture crosses the actual precise mask boundary");
    entering.run.levelExitPresent = false;
    const auto priorClock = entering.run.clock.ticks;
    entering.step();
    require(entering.run.clock.ticks == priorClock + 1,
            "retired field cannot suppress the next room's clock");

    Fixture dying;
    dying.health = 0;
    dying.currency.gameGem = 37;
    dying.step();
    require(dying.death.active && !dying.death.started && dying.run.clock.ticks == 1 &&
            !dying.run.gems.recorded,
            "health publication follows movement and clock, not the other way around");
    dying.step();
    require(dying.death.started && dying.death.frames == 1 && dying.death.alarm == 29 &&
            dying.run.clock.ticks == 1 && dying.run.gems.lifetime == 37 &&
            dying.run.gems.savedRunGems == 37,
            "first death movement records gems and advances its new alarm in the same update");
    dying.step();
    require(dying.run.gems.lifetime == 37, "later death updates do not record gems twice");

    Fixture combo;
    combo.health = 0;
    combo.run.combo.state.count = 25;
    combo.step();
    require(combo.health == 1 && !combo.death.active && combo.player.ammoCapacity() == 9 &&
            combo.currency.currency == 100 && combo.run.combo.completion.rewardCreated,
            "grounded combo grants precede health publication and survive empty follow-up completion");
    combo.step();
    require(combo.health == 1 && combo.currency.currency == 100 &&
            !combo.run.combo.completion.rewardCreated,
            "next Begin Step resets completion effects without repeating grants");

    // A direct movement after the shared phase must not call expired stack
    // contexts. The still-pending combo is deliberately observable.
    combo.run.combo.state.count = 8;
    combo.player.step({});
    require(combo.run.combo.state.count == 8 && combo.currency.currency == 100,
            "shared phase detaches its temporary callbacks before returning");
    auto copied = combo;
    copied.step();
    require(copied.currency.currency == 200 && combo.currency.currency == 100,
            "copied simulation binds the next phase to its own state");

    downwell::Input right;
    right.right = true;
    Fixture locked, moving;
    locked.step(right, true);
    moving.step(right);
    require(locked.player.offsetXFixed() == 0 && moving.player.offsetXFixed() > 0,
            "scene control lock is preserved independently of shop and exit locks");
    require(locked.random.drawCount() == 0 && moving.random.drawCount() == 0,
            "empty shops and ordinary movement do not consume RNG");
    std::cout << "shared gameplay player phase tests passed\n";
}

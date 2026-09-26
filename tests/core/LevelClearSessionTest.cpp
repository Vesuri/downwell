#include "core/RunSession.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
    using namespace downwell;
    {
        RunSession failed;GameMakerRandom rng(123);
        require(failed.emitBloodBurst(MotionBinary64::fromInteger(240),{},rng),"failure fixture births");
        unsigned calls=0;
        failed.stepBloodBound(failed.bloodEffects.beginPhase(),true,480,
            [&](const auto&,bool&){++calls;return false;});
        require(!failed.effectsSucceeded && calls==1,"unsupported field query latches session failure");
        failed.bloodEffects.visitNewest([&](auto,const auto& effect){
            require(effect.savedSpeed.bits==0xbf800000u,"failed query must not freeze or move an instance");
        });
        const auto draws=rng.drawCount();
        require(!failed.emitBloodBurst({}, {},rng) && rng.drawCount()==draws,"latched failure consumes no further RNG");
        failed.reset();require(failed.effectsSucceeded && failed.bloodEffects.size()==0,"reset clears failed ownership");
    }
    RunSession run;
    PlayerMotion player(false, 10000);
    GameMakerRandom random(17);
    CurrencyAccount currency;
    currency.currency = 123;
    run.stage.area = 1; run.stage.level = 3;
    run.upgrades.levels[0] = 1;
    run.gems.lifetime = 999;
    int health = 4;
    bool heart = false;
    require(!run.enterLevelClear(0, heart, health, player, random), "entry requires exit request");
    run.levelResultRequested = true;
    run.levelExitPresent = run.goalStopped = run.exitControlsLocked = true;
    run.exitDither.create(160, 1000);
    require(run.flashEffects.create(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(176),random)!=0,
            "old room owns a flash effect");
    require(run.smokeEffects.create(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(176),random)!=0,
            "old room owns a smoke effect");
    const auto draws = random.drawCount();
    require(run.emitBloodBurst(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(176),random),
            "old room owns blood effects");
    const auto bloodDraws=random.drawCount()-draws;
    require(bloodDraws==2,"blood script consumes exactly two angle draws");
    const auto bloodPhase=run.bloodEffects.beginPhase();
    run.advanceBloodAnimation();
    unsigned allowedQueries=0;
    run.stepBlood(bloodPhase,[&](auto,const auto&){++allowedQueries;return false;});
    require(allowedQueries==2 && run.effectsSucceeded,"session dispatches per-instance time-stop query");
    unsigned extraFields=0;
    run.stepBloodBound(bloodPhase,false,480,[&](const auto&,bool&){++extraFields;return false;});
    require(run.effectsSucceeded && extraFields==0,"inactive pTimeStop skips field queries");
    run.goalField.x=240;run.goalField.y=176;
    run.stepBloodBound(bloodPhase,true,480,[&](const auto&,bool&){++extraFields;return false;});
    require(run.effectsSucceeded && extraFields==0,"owned goal field allows effects during global stop");
    run.levelExitPresent=false;
    run.stepBloodBound(bloodPhase,true,480,[&](const auto&,bool& hit){++extraFields;hit=false;return true;});
    require(run.effectsSucceeded && extraFields==2,"outside all fields freezes both blood instances");
    run.levelExitPresent=true;
    run.bloodEffects.visitNewest([&](auto,const BloodEffect& effect){
        require(effect.imageIndex.bits==0x3f000000u && effect.imageSpeed.bits==0,
                "earlier animation remains visible when Step freezes movement");
    });
    require(!run.enterLevelClear(9, heart, health, player, random) && run.levelResultRequested &&
        random.drawCount() == draws+bloodDraws, "invalid entry leaves request and RNG intact");
    require(run.enterLevelClear(0, heart, health, player, random) && run.levelClearActive &&
        !run.levelExitPresent && !run.exitDither.active && !run.levelResultRequested,
        "accepted room boundary retires exit state");
    unsigned flashCount=0;
    run.flashEffects.visitNewest([&](const FlashBallEffect&){++flashCount;});
    require(flashCount==0,"room retirement clears flash effects without resetting the whole run");
    require(run.smokeEffects.size()==0,"room retirement clears smoke ownership");
    require(run.bloodEffects.size()==0,"room retirement clears blood ownership");
    require(!run.enterLevelClear(0, heart, health, player, random) && random.drawCount() == draws + bloodDraws + 1,
        "duplicate room entry cannot consume another Create random word");
    unsigned tick = 0, advances = 0, dithers = 0;
    auto emit = [&](LevelClearSelection::Effect event, unsigned) {
        if (event == LevelClearSelection::Effect::dither) {
            require(tick == 121, "source departure dither alarm timing"); ++dithers;
        }
        if (event == LevelClearSelection::Effect::nextLevel) {
            require(tick == 211, "source next-room alarm timing"); ++advances;
        }
    };
    for (tick = 1; tick <= 211; ++tick) {
        require(!run.advanceAfterLevelClear(), "loader cannot advance before departure completes");
        bool confirm = true;
        require(run.stepLevelClear(confirm, false, false, heart, currency, player, health, random, emit),
            "active result room advances shared controller");
        require(run.stage.area == 1 && run.stage.level == 3, "request does not construct next stage prematurely");
        if (tick == 60) require(run.levelClearFrame.circle == 3 && run.levelClearFrame.time == 1 &&
            run.levelClearFrame.choices, "choice reveal resets shadow phase before presentation");
        if (tick == 90) require(!run.levelClearFrame.choices && run.levelClearFrame.time == 30,
            "confirmation hides choices before the Draw clock can advance");
        if (tick == 121) require(run.exitDither.active && run.exitDither.x == 80,
            "result departure anchors its new wipe in the narrow room");
        if (tick == 136) require(run.exitDither.soundRequested, "departure wipe advances its own alarm");
    }
    require(advances == 1 && dithers == 1 && run.nextLevelRequested, "one transition request");
    bool confirm = true;
    require(!run.stepLevelClear(confirm, false, false, heart, currency, player, health, random, emit),
        "pending loader does not simulate additional old-room frames");
    require(run.advanceAfterLevelClear() && run.stage.area == 2 && run.stage.level == 1 &&
        !run.goalStopped && !run.exitControlsLocked && !run.exitDither.active,
        "consuming transition advances stage and releases gameplay locks");
    require(!run.advanceAfterLevelClear(), "stage advancement is one-shot");
    require(run.upgrades.levels[0] == 1 && run.upgrades.count == 1 && run.gems.lifetime == 999 &&
        currency.currency >= 123, "level transition preserves run ownership and records");
    player.increaseAmmoCapacity(4);
    player.face(-1);
    Input shooting; shooting.fire = true; shooting.left = true;
    for (unsigned tick = 0; tick < 40; ++tick) player.step(shooting);
    const int capacity = player.ammoCapacity();
    require(player.shots() && player.ammo() < capacity, "room-entry fixture spent charge and moved");
    player.enterRoom(true);
    require(player.ammoCapacity() == capacity && player.ammo() == capacity && player.facing() == -1 &&
        player.offsetXFixed() == 0 && player.offsetYFixed() == 0 && player.speedXFixed() == 0 &&
        player.speedYFixed() == 0 && !player.grounded() && !player.shots() &&
        player.jumpShootLocked() && player.shotDelayed(),
        "room entry resets instance motion but preserves globals and refills charge");
    run.reset();
    require(!run.levelClearActive && !run.nextLevelRequested && run.levelClear.selection.alarm4 == 60 &&
        run.upgrades.count == 0, "new-run reset clears result lifecycle");
    std::puts("Shared exit/result/next-stage lifecycle and persistent ownership passed");
}

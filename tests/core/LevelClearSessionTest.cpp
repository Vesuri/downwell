#include "core/RunSession.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
    using namespace downwell;
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
    const auto draws = random.drawCount();
    require(!run.enterLevelClear(9, heart, health, player, random) && run.levelResultRequested &&
        random.drawCount() == draws, "invalid entry leaves request and RNG intact");
    require(run.enterLevelClear(0, heart, health, player, random) && run.levelClearActive &&
        !run.levelExitPresent && !run.exitDither.active && !run.levelResultRequested,
        "accepted room boundary retires exit state");
    unsigned flashCount=0;
    run.flashEffects.visitNewest([&](const FlashBallEffect&){++flashCount;});
    require(flashCount==0,"room retirement clears flash effects without resetting the whole run");
    require(!run.enterLevelClear(0, heart, health, player, random) && random.drawCount() == draws + 1,
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

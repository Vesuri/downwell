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
    std::ifstream maskFile("generated/downwell-goal-mask.mask", std::ios::binary);
    const std::vector<unsigned char> mask((std::istreambuf_iterator<char>(maskFile)), {});
    require(mask.size() == 2048, "original precise mask is available for independent pixel queries");
    downwell::GoalTimeField field;
    field.x = 240; field.y = 4000;
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
    }
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

#include "core/GameplayPlayerStep.h"
#include <cstdlib>
#include <iostream>

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

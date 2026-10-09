#include "core/LevelClearController.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
    using namespace downwell;
    {
        LevelClearMotion motion;std::int32_t y=-1;
        motion.cameraY=HudReal::ratio(285,2);
        require(motion.instanceY(y)&&y==142,"Result y rounds an even exact half down");
        motion.cameraY=HudReal::ratio(287,2);
        require(motion.instanceY(y)&&y==144,"Result y rounds an odd exact half up");
        motion.playerY=HudReal::integer(2000);
        require(motion.instanceY(y)&&y==144,"Result instance y follows yy, not global.ply");
    }
    LevelClearController room;
    UpgradeInventory inventory;
    Consumables consumables;
    CurrencyAccount currency;
    HealthReserve reserve;
    PlayerMotion player(false, 10000);
    Input fire; fire.fire = true;
    for (unsigned i = 0; i < 40; ++i) player.step(fire);
    require(player.ammo() < player.ammoCapacity(), "fixture has spent charge");
    GameMakerRandom random(7), oracle(7);
    bool heartGoal = true;
    int health = 1;
    require(room.create(0, heartGoal, health, inventory, player, random) &&
        player.ammo() == player.ammoCapacity() && room.gemSpeedRandomWord == oracle.nextU32(),
        "Result Create refills charge and consumes its initial random draw");
    LevelUpgradeOffers expected;
    expected.randomize(2, UpgradeInventory::highestId, oracle,
        [&](unsigned id) { return inventory.eligible(id); });
    unsigned grants = 0, advances = 0;
    auto effect = [&](LevelClearSelection::Effect event, unsigned cursor) {
        if (event == LevelClearSelection::Effect::grantChoice) {
            require(cursor == 1 && inventory.levels[13] == 1 && health == 4,
                "inventory/healing grant executes synchronously before external effects");
            ++grants;
        }
        if (event == LevelClearSelection::Effect::nextLevel) ++advances;
    };
    for (unsigned tick = 1; tick <= 211; ++tick) {
        room.advanceAlarms(effect);
        bool confirm = true;
        if (tick == 90) room.offers.ids[1] = 13; // deterministic Apple grant fixture
        require(room.step(confirm, false, false, heartGoal, inventory, consumables,
            currency, player, health, reserve, random, effect), "shared controller Step succeeds");
        if (tick == 60) {
            require(room.motion.cameraTarget.floor() == 340 && room.motion.playerTarget.floor() == 302 &&
                room.motion.playerDivisor == 10 && room.motion.viewY() == 20,
                "showChoices switches easing targets before the same update's motion");
            for (unsigned i = 0; i < 3; ++i) require(room.offers.ids[i] == expected.ids[i], "actual inventory drives original offers");
            require(!heartGoal && room.heartReceived && health == 2, "heart-goal snapshot applied before choices");
            require(random.drawCount() == oracle.drawCount(), "Create and selection RNG order matches");
            require(random.stateIndex() == oracle.stateIndex(), "composed controller RNG index matches");
            for (unsigned word = 0; word < 16; ++word)
                require(random.stateWord(word) == oracle.stateWord(word), "composed controller full RNG state matches");
        }
        if (tick == 91) require(health == 2, "source repeats heart snapshot before departure, rather than retaining prior upgrade healing");
        if (tick == 91) require(!room.motion.acceleration.mantissa,
            "entering next=7 does not execute the departure else branch immediately");
        if (tick == 92) require(room.motion.acceleration.mantissa && room.motion.playerTarget.floor() == 302,
            "first departure update adds zero acceleration before incrementing it");
    }
    require(grants == 1 && advances == 1 && inventory.count == 1, "one selected grant and one level request");
    inventory.levels[6] = 1;
    require(room.create(2, false, health, inventory, player, random) && room.selection.lastIndex == 2,
        "Youth and Boulder option-count adjustments combine at room creation");
    require(room.motion.cameraY.floor() == 142 && !room.motion.playerY.mantissa &&
        !room.motion.acceleration.mantissa && room.motion.playerDivisor == 20,
        "room creation resets presentation motion after the previous departure");
    room.selection.next = 4;
    bool confirm = false;
    auto ignore = [](LevelClearSelection::Effect, unsigned) {};
    room.step(confirm, false, true, heartGoal, inventory, consumables, currency,
        player, health, reserve, random, ignore);
    require(room.carousel.target == 4 && room.carousel.position.magnitude.less(HudReal::integer(42)),
        "navigation eases the carousel during confirmation lock");
    auto expectedCarousel = room.carousel;
    expectedCarousel.step(1);
    room.selection.inputWait = false;
    confirm = true;
    room.step(confirm, true, false, heartGoal, inventory, consumables, currency,
        player, health, reserve, random, ignore);
    require(!room.selection.visible && room.carousel.target == 42 &&
        room.carousel.position.magnitude.equal(expectedCarousel.position.magnitude),
        "last confirmation still eases using the post-navigation cursor");
    room.step(confirm, false, false, heartGoal, inventory, consumables, currency,
        player, health, reserve, random, ignore);
    require(room.carousel.position.magnitude.equal(expectedCarousel.position.magnitude),
        "departure leaves the hidden carousel stationary");
    require(room.create(4, false, health, inventory, player, random) && room.selection.lastIndex == 3 &&
        room.upgradesRemaining == 0, "Handstand retains source layout setup but offers no upgrade");
    std::puts("Shared level-clear creation, offers, grants and heart-goal ordering passed");
}

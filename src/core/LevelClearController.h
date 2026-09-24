#ifndef DOWNWELL_CORE_LEVEL_CLEAR_CONTROLLER_H
#define DOWNWELL_CORE_LEVEL_CLEAR_CONTROLLER_H
#include "LevelClearSelection.h"
#include "LevelUpgradeOffers.h"
#include "UpgradeInventory.h"
#include "LevelClearMotion.h"
#include "LevelClearCarousel.h"
namespace downwell {
// Shared non-rendering Result-room controller. Room loading, presentation and
// audio playback stay outside; nextLevel is a request, not a direct stage jump.
struct LevelClearController {
    LevelClearSelection selection;
    LevelUpgradeOffers offers;
    LevelClearMotion motion;
    LevelClearCarousel carousel;
    unsigned upgradesRemaining = 1;
    int healthAtEntry = 0;
    bool wasHeartGoal = false, heartReceived = true;
    std::uint32_t gemSpeedRandomWord = 0; // gemSp = 1 + 3*word/2^32
    bool create(unsigned style, bool heartGoal, int health, const UpgradeInventory& inventory,
                PlayerMotion& player, GameMakerRandom& random) {
        const unsigned last = 2 + inventory.levels[6] - (style == 2 ? 1 : 0);
        if (style > 4 || last >= 4) return false;
        *this = LevelClearController{};
        selection.create(last);
        carousel.create(selection.cursor);
        healthAtEntry = health;
        wasHeartGoal = heartGoal;
        heartReceived = !heartGoal;
        upgradesRemaining = style == 4 ? 0 : 1;
        player.increaseAmmoCapacity(0); // Result Create refills stammo, not capacity.
        gemSpeedRandomWord = random.nextU32(); // even though gemTotal starts at zero
        return true;
    }
    template<class Emit> void advanceAlarms(Emit emit) { selection.advanceAlarms(emit); }
    template<class Emit>
    bool step(bool& confirm, bool leftPressed, bool rightPressed, bool& heartGoal,
              UpgradeInventory& inventory, Consumables& consumables,
              CurrencyAccount& currency, PlayerMotion& player, int& health,
              HealthReserve& reserve, GameMakerRandom& random, Emit emit) {
        // Source repeats the snapshot assignment on EVERY next=4 Step, even
        // after an upgrade healed the player on the preceding update.
        if (selection.next == 4 && wasHeartGoal) {
            health = healthAtEntry + 1;
            heartReceived = true;
            heartGoal = false;
        }
        bool ok = true;
        const bool departureBranch = selection.next >= 7 && heartReceived;
        // The source enters the visible branch before confirmation can hide
        // it, so the final grant still performs one carousel easing update.
        const bool carouselBranch = selection.next == 4 && upgradesRemaining != 0;
        selection.step(confirm, leftPressed, rightPressed, upgradesRemaining,
            [&](LevelClearSelection::Effect effect, unsigned value) {
                if (effect == LevelClearSelection::Effect::showChoices) motion.showChoices();
                if (effect == LevelClearSelection::Effect::rollOffers)
                    ok = offers.randomize(selection.lastIndex, UpgradeInventory::highestId, random,
                        [&](unsigned id) { return inventory.eligible(id); }) && ok;
                if (effect == LevelClearSelection::Effect::grantChoice)
                    ok = inventory.grant(offers.ids[value], consumables, currency, player, health, reserve) && ok;
                emit(effect, value);
            });
        if (carouselBranch) carousel.step(selection.cursor);
        motion.step(departureBranch);
        return ok;
    }
};
}
#endif

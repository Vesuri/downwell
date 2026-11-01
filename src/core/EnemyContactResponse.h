#ifndef DOWNWELL_CORE_ENEMY_CONTACT_RESPONSE_H
#define DOWNWELL_CORE_ENEMY_CONTACT_RESPONSE_H
#include "EnemyContact.h"
#include "PlayerLifeRules.h"

namespace downwell {
struct NoContactRetirement {
    template<class... Args> bool operator()(Args&&...) const { return true; }
};
// External owners remove membership after reward creation and before the
// current immediate-retirement writes. False is terminal; the caller must
// latch failure, not retry a partially rewarded contact. This boundary does
// not change the still-approximate source death-event scheduling below.
// Shared response for the currently implemented default-style area-one
// contact pass. Storage/FX adapters supply reward/damage emission; player movement,
// health, immunity and defeat accounting are decided here, not by a backend.
// Immediate enemy retirement/reward is still an approximation of the original
// collision -> later enemy Step/death effects, and must be migrated together.
template<class Player, class Immunity, class Count, class Reward, class Damage,
         class Retire = NoContactRetirement>
EnemyContact applyEnemyContact(int object, const ContactActor& enemy,
    int publishedX, int publishedY, bool& active, int& enemyHealth, Player& player, int& playerHealth,
    Immunity& invulnerability, Count& defeated, Reward reward, bool candle, Damage damage,
    Retire retire = {}, long stompY = 0x7fffffffL) {
    if (!active || enemyHealth <= 0 || !playerCanAct(playerHealth)) return EnemyContact::none;
    const ContactActor playerContact{player.offsetXFixed(), player.offsetYFixed(),
                                    player.facing(), 1, 31};
    const auto contact = classifyEnemyContact(playerContact, enemy, object,
        player.frameStartYFixed(), stompY == 0x7fffffffL ? enemy.y : stompY);
    if (contact == EnemyContact::stomp) {
        if (!reward()) return EnemyContact::none;
        if (!retire()) return EnemyContact::none;
        enemyHealth = 0;
        active = false;
        ++defeated;
        player.stompBounce();
        return contact;
    }
    if (contact == EnemyContact::damage && invulnerability == 0 && playerHealth > 0) {
        --playerHealth;
        // scrPDamage creates the number after subtraction, before the later
        // Alarm 1 assignment and scrTypicalDamage's forced jump.
        if (!damage(publishedX, publishedY)) return EnemyContact::none;
        // scrPDamage VM blocks 1–3: owned ug[7][1] selects Alarm 1.
        invulnerability = candle ? 240 : 90;
        // scrTypicalDamage uses instance x/y, not the precise xx/yy state.
        player.contactKnockback(publishedX, publishedY);
        return contact;
    }
    return EnemyContact::none;
}
struct EnemyContactLocation {
    ContactActor actor;
    int publishedX, publishedY;
};

// Storage adapters retain their coordinate representation and presentation
// hooks; eligibility, immunity expiry and ordered contact effects are shared.
// Failed reward/damage callbacks stop traversal without rolling back earlier
// combo, child-creation or health changes. They must latch world failure.
template<class Player, class Immunity, class Count, class Combo,
         class Get, class Locate, class Reward, class Publish, class Damage,
         class Retire = NoContactRetirement>
bool stepEnemyContacts(unsigned count, Player& player, int& health,
    Immunity& immunity, Count& defeated, Combo& combo, bool candle,
    Get get, Locate locate, Reward reward, Publish publish, Damage damage, Retire retire = {}) {
    if (immunity > 0) --immunity;
    for (unsigned i = 0; i < count; ++i) {
        auto& enemy = get(i);
        const int object = enemy.objectId;
        if (!enemy.active || enemy.health <= 0 ||
            (object != 111 && object != 112 && object != 113 &&
             object != 116 && object != 117 && object != 119)) continue;
        const auto location = locate(enemy);
        bool succeeded = true;
        const auto contact = applyEnemyContact(object, location.actor,
            location.publishedX, location.publishedY, enemy.active, enemy.health,
            player, health, immunity, defeated, [&] {
                combo.enemyDefeated();
                return succeeded = reward(enemy);
            }, candle, [&](int x, int y) {
                return succeeded = damage(x, y);
            }, [&] {
                return succeeded = retire(enemy);
            });
        if (!succeeded) return false;
        if (contact != EnemyContact::none) publish(contact);
    }
    return true;
}
} // namespace downwell
#endif

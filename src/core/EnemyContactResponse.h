#ifndef DOWNWELL_CORE_ENEMY_CONTACT_RESPONSE_H
#define DOWNWELL_CORE_ENEMY_CONTACT_RESPONSE_H
#include "EnemyContact.h"
#include "PlayerLifeRules.h"

namespace downwell {
// Shared response for the currently implemented default-style area-one
// contact pass. Storage/FX adapters supply reward emission; player movement,
// health, immunity and defeat accounting are decided here, not by a backend.
// Immediate enemy retirement/reward is still an approximation of the original
// collision -> later enemy Step/death effects, and must be migrated together.
template<class Player, class Immunity, class Count, class Reward>
EnemyContact applyEnemyContact(int object, const ContactActor& enemy,
    int publishedX, int publishedY, bool& active, int& enemyHealth, Player& player, int& playerHealth,
    Immunity& invulnerability, Count& defeated, Reward reward, bool candle) {
    if (!active || enemyHealth <= 0 || !playerCanAct(playerHealth)) return EnemyContact::none;
    const ContactActor playerContact{player.offsetXFixed(), player.offsetYFixed(),
                                    player.facing(), 1, 31};
    const auto contact = classifyEnemyContact(playerContact, enemy, object,
                                             player.frameStartYFixed());
    if (contact == EnemyContact::stomp) {
        reward();
        enemyHealth = 0;
        active = false;
        ++defeated;
        player.stompBounce();
        return contact;
    }
    if (contact == EnemyContact::damage && invulnerability == 0 && playerHealth > 0) {
        --playerHealth;
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
template<class Player, class Immunity, class Count, class Combo,
         class Get, class Locate, class Reward, class Publish>
void stepEnemyContacts(unsigned count, Player& player, int& health,
    Immunity& immunity, Count& defeated, Combo& combo, bool candle,
    Get get, Locate locate, Reward reward, Publish publish) {
    if (immunity > 0) --immunity;
    for (unsigned i = 0; i < count; ++i) {
        auto& enemy = get(i);
        const int object = enemy.objectId;
        if (!enemy.active || enemy.health <= 0 ||
            (object != 111 && object != 112 && object != 113 &&
             object != 116 && object != 117 && object != 119)) continue;
        const auto location = locate(enemy);
        const auto contact = applyEnemyContact(object, location.actor,
            location.publishedX, location.publishedY, enemy.active, enemy.health,
            player, health, immunity, defeated, [&] {
                combo.enemyDefeated();
                reward(enemy);
            }, candle);
        if (contact != EnemyContact::none) publish(contact);
    }
}
} // namespace downwell
#endif

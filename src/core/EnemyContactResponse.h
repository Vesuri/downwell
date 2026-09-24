#ifndef DOWNWELL_CORE_ENEMY_CONTACT_RESPONSE_H
#define DOWNWELL_CORE_ENEMY_CONTACT_RESPONSE_H
#include "EnemyContact.h"

namespace downwell {
// Shared response for the currently implemented default-style area-one
// contact pass. Storage/FX adapters supply reward emission; player movement,
// health, immunity and defeat accounting are decided here, not by a backend.
// Immediate enemy retirement/reward is still an approximation of the original
// collision -> later enemy Step/death effects, and must be migrated together.
template<class Player, class Immunity, class Count, class Reward>
EnemyContact applyEnemyContact(int object, const ContactActor& enemy,
    int publishedX, int publishedY, bool& active, int& enemyHealth, Player& player, int& playerHealth,
    Immunity& invulnerability, Count& defeated, Reward reward) {
    if (!active || enemyHealth <= 0) return EnemyContact::none;
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
        invulnerability = 90; // scrPDamage without the immunity upgrade.
        // scrTypicalDamage uses instance x/y, not the precise xx/yy state.
        player.contactKnockback(publishedX, publishedY);
        return contact;
    }
    return EnemyContact::none;
}
} // namespace downwell
#endif

#ifndef DOWNWELL_CORE_CURRENCY_STEP_H
#define DOWNWELL_CORE_CURRENCY_STEP_H
#include "CurrencyAccount.h"
#include "GemMotion.h"
namespace downwell {
enum class CurrencyStepResult { alive, expired, collected };

// The implemented currency phase uses the player's frame-start position for
// homing, but its current position for collection. Expiry must short-circuit
// movement and collection. Backends only publish surviving presentation state
// or retire storage and handle a collected notification.
// This preserves the existing phase; full runner collision ordering and
// GemMotion's provisional terrain response are separate fidelity work.
template<class Solid, class Random>
CurrencyStepResult stepCurrency(GemMotion& motion, bool large, unsigned amount,
    long homingX, long homingY, long collectionX, long collectionY,
    CurrencyAccount& account, Solid solid, Random& random, bool lessResist = false) {
    if (!motion.step(homingX, homingY, solid, random)) return CurrencyStepResult::expired;
    if (!motion.collectible(collectionX, collectionY, large)) return CurrencyStepResult::alive;
    account.collect(amount, 300, lessResist);
    return CurrencyStepResult::collected;
}
} // namespace downwell
#endif

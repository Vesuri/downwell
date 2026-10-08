#ifndef DOWNWELL_CORE_CURRENCY_REWARD_FIELDS_H
#define DOWNWELL_CORE_CURRENCY_REWARD_FIELDS_H
#include "MotionBinary64.h"
namespace downwell {
// Dynamic scrCurrencySpawn fields on the reward caller. Keep these cold
// fields out of the per-Step box state snapshot. myGem is a source numeric ID.
struct CurrencyRewardFields {
    MotionBinary64 moniAmount{};
    std::uint32_t myGem=0;
    bool atrState=false;
};
}
#endif

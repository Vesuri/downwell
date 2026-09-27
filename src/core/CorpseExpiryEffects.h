#ifndef DOWNWELL_CORE_CORPSE_EXPIRY_EFFECTS_H
#define DOWNWELL_CORE_CORPSE_EXPIRY_EFFECTS_H
#include "CorpseExpiryEffect.h"
#include "NonLoopEffects.h"
namespace downwell {
template<unsigned Capacity> using CorpseExpiryEffects=NonLoopEffects<CorpseExpiryEffect,Capacity>;
}
#endif

#ifndef DOWNWELL_CORE_STORED_WHITE_JELLY_STATES_H
#define DOWNWELL_CORE_STORED_WHITE_JELLY_STATES_H
#include "StoredSteeringEnemyStates.h"
#include "WhiteJellyStep.h"
namespace downwell {
template<unsigned WorldCapacity,unsigned Capacity>
using StoredWhiteJellyStates=StoredSteeringEnemyStates<WorldCapacity,Capacity,WhiteJellyStep,115>;
}
#endif

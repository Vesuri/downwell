#ifndef DOWNWELL_CORE_STORED_RED_BALL_STATES_H
#define DOWNWELL_CORE_STORED_RED_BALL_STATES_H
#include "StoredSteeringEnemyStates.h"
#include "RedBallStep.h"
namespace downwell {
template<unsigned WorldCapacity,unsigned Capacity>
using StoredRedBallStates=StoredSteeringEnemyStates<WorldCapacity,Capacity,RedBallStep,118>;
}
#endif

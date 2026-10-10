#ifndef DOWNWELL_CORE_GAMEPLAY_RED_BALLS_H
#define DOWNWELL_CORE_GAMEPLAY_RED_BALLS_H
#include "RedBallStep.h"
#include "GameplaySteeringEnemies.h"
namespace downwell {
// Object 118: source metadata selects sprRedBall/sprRedballStun and mask 153.
using GameplayRedBalls=GameplaySteeringEnemies<RedBallStep,702,703,153>;
}
#endif

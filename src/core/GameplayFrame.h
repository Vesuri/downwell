#ifndef DOWNWELL_CORE_GAMEPLAY_FRAME_H
#define DOWNWELL_CORE_GAMEPLAY_FRAME_H
#include "PlayerMotion.h"
#include "GameplayLimits.h"

// A value result. Only [0, destroyedBlockCount) is a published repaint
// payload (destroyed boxes, then changed scenery cells); copying a result must not read or initialize the unused capacity.
struct GameplayFrame {
    PlayerMotionDelta playerMotion;
    bool ammoChanged,healthChanged,currencyChanged,streakChanged,bulletChanged,scrolled;
    int scrollDelta;
    bool cameraXChanged;
    int cameraXDelta;
    unsigned destroyedBlockCount;
    int blockX[downwell::gameplayRepaintCapacity];
    int blockY[downwell::gameplayRepaintCapacity];
};
#endif

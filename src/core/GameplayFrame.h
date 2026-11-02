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
#ifndef DOWNWELL_REFERENCE_FRAME_WORK
    GameplayFrame():playerMotion{},ammoChanged(false),healthChanged(false),currencyChanged(false),
        streakChanged(false),bulletChanged(false),scrolled(false),scrollDelta(0),
        cameraXChanged(false),cameraXDelta(0),destroyedBlockCount(0){}
    GameplayFrame(const GameplayFrame& other){*this=other;}
    GameplayFrame& operator=(const GameplayFrame& other){
        playerMotion=other.playerMotion;
        ammoChanged=other.ammoChanged;healthChanged=other.healthChanged;
        currencyChanged=other.currencyChanged;streakChanged=other.streakChanged;
        bulletChanged=other.bulletChanged;scrolled=other.scrolled;scrollDelta=other.scrollDelta;
        cameraXChanged=other.cameraXChanged;cameraXDelta=other.cameraXDelta;
        destroyedBlockCount=other.destroyedBlockCount;
        for(unsigned i=0;i<destroyedBlockCount;++i){blockX[i]=other.blockX[i];blockY[i]=other.blockY[i];}
        return *this;
    }
#endif
};
#endif

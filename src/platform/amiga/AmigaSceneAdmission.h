#ifndef DOWNWELL_AMIGA_SCENE_ADMISSION_H
#define DOWNWELL_AMIGA_SCENE_ADMISSION_H

#include "AmigaSceneRuntime.h"

// Check ownership at acquisition/dispatch boundaries, never by walking the
// resource or actor catalogue. Explicit pixel/replay fixtures may stage art
// before presentation; ordinary builds must not inherit that exception.
struct AmigaSceneAdmission {
    static constexpr bool acquireGameplay(AmigaScene scene, bool referenceOracle) {
        return scene == AmigaScene::gameplay || referenceOracle;
    }
    static constexpr bool stepGameplay(AmigaScene scene, bool cpuArt,
                                      bool dmaArt, bool entryMask,
                                      bool terrainRaster) {
        return scene == AmigaScene::gameplay && cpuArt && dmaArt &&
               entryMask && terrainRaster;
    }
};

#endif

#ifndef DOWNWELL_AMIGA_WALL_COLLISION_H
#define DOWNWELL_AMIGA_WALL_COLLISION_H
#include "AmigaTerrain.h"
// Cache contributions only. The resource-backed owner retains the identity
// and marked record until the runner's physical cleanup boundary.
struct AmigaWallCollision {
    AmigaTerrainIndex& index;
    bool changed(int x,int y,bool enabled){
        return enabled ? amigaAddTerrainObject(index,60,x,y) : amigaRemoveTerrainWall(index,x,y);
    }
};
#endif

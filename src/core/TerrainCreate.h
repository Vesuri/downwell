#ifndef DOWNWELL_CORE_TERRAIN_CREATE_H
#define DOWNWELL_CORE_TERRAIN_CREATE_H
#include "GameMakerRandom.h"
namespace downwell {
// scrInitialize levelTile table, selected by scrNextLevel before room entry.
// Temporary controller/boxFlash wallTile overrides must be supplied by the caller.
inline int terrainTileForArea(int area){
    constexpr int tiles[]={76,51,74,72,70,55};
    return area>=0&&area<6?tiles[area]:-1;
}
// Collision-relevant Create property writes from code 985 (wall) and 1090
// (thin platform). Material and wall End Step bookkeeping live separately.
// Callbacks commit in source order; a failure preserves earlier effects/RNG.
template<class Sprite,class Index,class Speed>
bool terrainCreateProperties(int object,int wallTile,bool glitch,GameMakerRandom& random,
    Sprite sprite,Index index,Speed speed){
    if(object!=60&&object!=93)return false;
    if(!sprite(wallTile))return false;
    if(object==60&&glitch){
        constexpr int choices[]={51,52,53,55,57};
        if(!sprite(choices[random.chooseIndexSmall(5)]))return false;
    }
    if(object==93&&!index(23))return false;
    return speed(0);
}
}
#endif

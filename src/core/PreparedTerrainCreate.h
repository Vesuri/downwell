#ifndef DOWNWELL_PREPARED_TERRAIN_CREATE_H
#define DOWNWELL_PREPARED_TERRAIN_CREATE_H
#include "PreparedObjectDefaults.h"
#include "RunnerSpriteChange.h"
#include "TerrainCreate.h"
#include "Profile.h"
namespace downwell {
// Ordinary generated wall/platform Create has no RNG or gameplay-dependent
// branch. Compile its fixed setter effects before birth; glitch Create and
// non-area sprite overrides keep the script path. This is NOT current bounds:
// birth publishes default bounds before these appearance writes.
class PreparedTerrainCreate {
public:
    struct Plan {
        int sprite=-1,index=0;
        RunnerImageValue imageIndex{};
        bool changedSprite=false,writeIndex=false,ready=false;
    };
private:
    Plan plans_[6][2]{};
public:
    template<unsigned N> bool prepare(const PreparedObjectDefaults<N>& defaults,
        const SpriteCollisionPack& sprites){
        DW_TERRAIN_PREPARATION_SCOPE;
        for(auto& area:plans_)for(auto& p:area)p=Plan{};
        if(!sprites.valid())return false;
        for(unsigned area=0;area<6;++area)for(unsigned type=0;type<2;++type){
            const int object=type?93:60;
            const auto* d=defaults.get(object);if(!d)continue;
            Plan p;GameMakerRandom unused;
            int current=d->spriteId,speed=1;
            const bool okay=terrainCreateProperties(object,terrainTileForArea(area),false,unused,
                [&](int sprite){
                    std::uint32_t frames=0;
                    if(sprite>=0)sprites.frameCount(unsigned(sprite),frames);
                    RunnerSpriteChange::Result change;
                    if(!RunnerSpriteChange::planReference(current,sprite,p.imageIndex,frames,change))return false;
                    p.changedSprite|=change.changed;
                    if(change.resetIndex){p.index=0;p.imageIndex={};}
                    current=sprite;return true;
                },[&](int index){p.writeIndex=true;p.index=index;return p.imageIndex.assignInteger(index);},
                [&](int value){speed=value;return true;});
            if(!okay||unused.drawCount()||speed!=0)continue;
            p.sprite=current;p.ready=true;plans_[area][type]=p;
        }
        return true;
    }
    const Plan* get(int object,int tile)const{
        if(object!=60&&object!=93)return nullptr;
        for(unsigned area=0;area<6;++area)if(terrainTileForArea(area)==tile){
            const auto& p=plans_[area][object==93];return p.ready?&p:nullptr;
        }
        return nullptr;
    }
};
}
#endif

#ifndef DOWNWELL_AMIGA_GAMEPLAY_GENERATION_H
#define DOWNWELL_AMIGA_GAMEPLAY_GENERATION_H
#include "AmigaTerrain.h"
#include "core/ObjectPack.h"

// Backend registration of synchronous recovered Create commands. Game rules
// and their RNG effects remain in GameplayWorld; display publication stays
// with the scene owner. No allocations or delayed command queue.
class AmigaGameplayGeneration {
public:
    static constexpr int worldRows = AmigaTerrainIndex::rows * 16;
    AmigaGameplayGeneration(AmigaAreaOneTerrainBuilder& builder,
        const downwell::BuilderPatternPack& patterns, const downwell::ObjectPack& objects,
        AmigaTerrainIndex& index, AmigaTerrainTiles& tiles)
        : builder_(builder), patterns_(patterns), objects_(objects), index_(index), tiles_(tiles) {}
    void bind(AmigaPortRuntime& runtime,bool terrainRuns=true);
    bool loadRoom(AmigaPortRuntime& runtime,const downwell::RoomPack& rooms,const downwell::PackedRoom& room);
    bool step(downwell::GameMakerRandom& random, int viewBottom);
    unsigned fragmentCount() const { return builder_.fragmentCount(); }
    int dirtyFirst = worldRows, dirtyEnd = 0;
private:
    static bool create(void*, const downwell::DeferredCreateInstance&);
    static bool createBreakable(void*,int,int,const downwell::GeneratedBreakableCreate&);
    static bool createRun(void*,const downwell::AreaOneBuilder::TerrainRun&);
    static bool createPrepared(void*,const downwell::AreaOneBuilder::PreparedTerrain&,downwell::AreaOneBuilder::EmitFunction);
    AmigaAreaOneTerrainBuilder& builder_;
    const downwell::BuilderPatternPack& patterns_;
    const downwell::ObjectPack& objects_;
    AmigaTerrainIndex& index_;
    AmigaTerrainTiles& tiles_;
    AmigaPortRuntime* runtime_ = nullptr;
};
#endif

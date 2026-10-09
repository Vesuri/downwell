#ifndef DOWNWELL_AMIGA_GAMEPLAY_GENERATION_H
#define DOWNWELL_AMIGA_GAMEPLAY_GENERATION_H
#include "AmigaTerrain.h"
#include "core/ObjectPack.h"
#include "core/PreparedRoomTerrain.h"
#include "AmigaRoomTerrainPlan.h"
#include "core/GameplayDitherAbove.h"
#include "core/NativeTerrainLifetimes.h"

// Backend registration of synchronous recovered Create commands. Game rules
// and their RNG effects remain in GameplayWorld; display publication stays
// with the scene owner. No allocations or delayed command queue.
class AmigaGameplayGeneration {
public:
    using TerrainLifetimes=downwell::NativeTerrainLifetimes<2048>;
    static constexpr int worldRows = AmigaTerrainIndex::rows * 16;
    AmigaGameplayGeneration(AmigaAreaOneTerrainBuilder& builder,
        const downwell::BuilderPatternPack& patterns, const downwell::ObjectPack& objects,
        AmigaTerrainIndex& index, AmigaTerrainTiles& tiles,TerrainLifetimes* lifetimes=nullptr)
        : builder_(builder), patterns_(patterns), objects_(objects), index_(index), tiles_(tiles),lifetimes_(lifetimes) {}
    void bind(AmigaPortRuntime& runtime,bool terrainRuns=true);
    bool loadRoom(AmigaPortRuntime& runtime,const downwell::RoomPack& rooms,const downwell::PackedRoom& room,
        const downwell::PreparedRoomTerrain* prepared=nullptr,AmigaRoomTerrainPlan* backend=nullptr);
    bool adoptRoom(AmigaPortRuntime& runtime,const downwell::RoomPack& rooms,const downwell::PackedRoom& room,
        const downwell::GameplayDitherAbove::Snapshot& dither,
        const downwell::PreparedRoomTerrain* prepared=nullptr,AmigaRoomTerrainPlan* backend=nullptr);
    bool loadRoomWithStartField(AmigaPortRuntime& runtime,const downwell::RoomPack& rooms,const downwell::PackedRoom& room,
        const downwell::PreparedRoomTerrain* prepared=nullptr,AmigaRoomTerrainPlan* backend=nullptr,
        bool (*builderCreate)(AmigaPortRuntime&,AmigaAreaOneTerrainBuilder&,const downwell::PackedRoomInstance&,void*)=nullptr,
        void* builderContext=nullptr);
    bool step(downwell::GameMakerRandom& random, int viewBottom);
    unsigned fragmentCount() const { return builder_.fragmentCount(); }
    int dirtyFirst = worldRows, dirtyEnd = 0;
private:
    bool roomBirth(const downwell::PackedRoomInstance& instance);
    bool birth(int object,int x,int y,bool active=true);
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
    TerrainLifetimes* lifetimes_ = nullptr;
};
#endif

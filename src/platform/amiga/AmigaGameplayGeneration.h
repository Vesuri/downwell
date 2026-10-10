#ifndef DOWNWELL_AMIGA_GAMEPLAY_GENERATION_H
#define DOWNWELL_AMIGA_GAMEPLAY_GENERATION_H
#include "AmigaTerrain.h"
#include "AmigaPortRuntime.h"
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
    // Borrow the complete room allocator, never seed a second backend cursor.
    // All optimized publishers must re-enter the scalar Create boundary in
    // this scope: a nested birth or failure can observe each prefix.
    template<class Allocate,class Shop,class Tick>
    bool withSourceCreation(Allocate& allocate,Shop& shop,Tick tick){
        if(!runtime_||sourceCreate_||!runtime_->effectsSucceeded()||
            (!lifetimes_&&!runtime_->terrainInstances()))return false;
        auto dispatch=[&](const downwell::DeferredCreateInstance& command){
            return runtime_->createGeneratedInstanceWithSourceIds(command,objects_,allocate,
                [&](const auto& c,unsigned id){return publish(c,id);},shop);
        };
        sourceContext_=&dispatch;
        sourceCreate_=[](void* p,const downwell::DeferredCreateInstance& c){return (*static_cast<decltype(dispatch)*>(p))(c);};
        struct Reset {AmigaGameplayGeneration& owner;~Reset(){owner.sourceCreate_=nullptr;owner.sourceContext_=nullptr;}} reset{*this};
        return tick()&&runtime_->effectsSucceeded();
    }
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
    bool birth(int object,int x,int y,bool active=true,unsigned source=0);
    bool publish(const downwell::DeferredCreateInstance&,unsigned source=0);
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
    void* sourceContext_=nullptr;
    bool (*sourceCreate_)(void*,const downwell::DeferredCreateInstance&)=nullptr;
};
#endif

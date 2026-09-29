#include "AmigaGameplayGeneration.h"
#include "AmigaPortRuntime.h"
#include "AmigaProfile.h"

bool AmigaGameplayGeneration::loadRoom(AmigaPortRuntime& runtime,
    const downwell::RoomPack& rooms,const downwell::PackedRoom& room){
    return runtime.loadRoomTerrain(rooms,room,[this](const downwell::PackedRoomInstance& instance){
        return amigaAddTerrainObject(index_,instance.objectId,instance.x,instance.y,true);
    });
}

void AmigaGameplayGeneration::bind(AmigaPortRuntime& runtime,bool terrainRuns) {
    runtime_ = &runtime;
    dirtyFirst = worldRows; dirtyEnd = 0;
    builder_.setCreateObserver(create, this);
    if(terrainRuns)builder_.setTerrainRunObserver(createRun);
}

bool AmigaGameplayGeneration::createRun(void* opaque,const downwell::AreaOneBuilder::TerrainRun& run){
    auto& self=*static_cast<AmigaGameplayGeneration*>(opaque);
    AmigaTerrainRunIndex index(self.index_,run.objectId,run.x,run.y,run.stepX,run.count);
    // Unsupported geometry retains scalar partial publication/failure timing.
    if(!index.ready())return run.execute(create,opaque);
    struct Context{AmigaGameplayGeneration& self;AmigaTerrainRunIndex& index;bool dirty=false;};
    Context context{self,index};
    return run.execute([](void* opaque,const downwell::DeferredCreateInstance& command){
        DW_PROFILE_SCOPE(profileGeneratedRegistration);
        auto& c=*static_cast<Context*>(opaque);
        return c.self.runtime_->createGeneratedTerrain(command,[&](const auto& command){
            {
                DW_PROFILE_SCOPE(profileGeneratedIndex);
                if(!c.index.append()||!c.self.tiles_.append(command))return false;
            }
            // Same row throughout; publish on the first successful backend
            // cell, before its core Create (also preserving failure prefixes).
            if(!c.dirty){
                if(command.y-24<c.self.dirtyFirst)c.self.dirtyFirst=command.y-24;
                if(command.y+24>c.self.dirtyEnd)c.self.dirtyEnd=command.y+24;
                c.dirty=true;
            }
            return true;
        });
    },&context);
}

bool AmigaGameplayGeneration::create(void* opaque,
    const downwell::DeferredCreateInstance& command) {
    DW_PROFILE_SCOPE(profileGeneratedRegistration);
    auto& self = *static_cast<AmigaGameplayGeneration*>(opaque);
    return self.runtime_->createGeneratedInstance(command, self.objects_,
        [&self](const downwell::DeferredCreateInstance& command) {
            {
                DW_PROFILE_SCOPE(profileGeneratedIndex);
                if ((!(command.objectId==60&&!command.active) &&
                     !amigaAddTerrainObject(self.index_, command.objectId, command.x, command.y)) ||
                    !self.tiles_.append(command)) return false;
            }
            if (command.objectId == 60 || command.objectId == 92 || command.objectId == 93 ||
                command.objectId == 289 || command.objectId == 293 || command.objectId == 296 ||
                command.objectId == 109) {
                if (command.y - 24 < self.dirtyFirst) self.dirtyFirst = command.y - 24;
                if (command.y + 24 > self.dirtyEnd) self.dirtyEnd = command.y + 24;
            }
            return true;
        });
}

bool AmigaGameplayGeneration::step(downwell::GameMakerRandom& random, int viewBottom) {
    dirtyFirst = worldRows; dirtyEnd = 0;
    if (!runtime_) return false;
    builder_.setPlayerHealth(runtime_->health());
    downwell::AreaOneBuilderStep result;
    const bool ok = builder_.stepVisible(patterns_, random, viewBottom, result);
    if (dirtyFirst < 0) dirtyFirst = 0;
    if (dirtyEnd > worldRows) dirtyEnd = worldRows;
    return ok;
}

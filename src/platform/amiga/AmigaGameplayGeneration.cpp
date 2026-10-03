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
    // With no world terrain state, the run's Creates are this backend's
    // publication alone: draw every cell, then publish the admitted span.
    if(self.runtime_->terrainCreatesBackendOnly()&&index.admitSpan(self.tiles_)){
        DW_PROFILE_SCOPE(profileGeneratedRegistration);
        if(run.y-24<self.dirtyFirst)self.dirtyFirst=run.y-24;
        if(run.y+24>self.dirtyEnd)self.dirtyEnd=run.y+24;
        return run.executeSpan([&]{return index.publishSpan(self.tiles_);});
    }
    bool dirty=false;
    return self.runtime_->createGeneratedTerrainRun(run,[&](const downwell::DeferredCreateInstance& command){
        DW_PROFILE_SCOPE(profileGeneratedRegistration);
        {
            DW_PROFILE_SCOPE(profileGeneratedIndex);
            if(!index.append()||!self.tiles_.append(command))return false;
        }
        // Same row throughout; publish on the first successful backend
        // cell, before its core Create (also preserving failure prefixes).
        if(!dirty){
            if(command.y-24<self.dirtyFirst)self.dirtyFirst=command.y-24;
            if(command.y+24>self.dirtyEnd)self.dirtyEnd=command.y+24;
            dirty=true;
        }
        return true;
    });
}

bool AmigaGameplayGeneration::create(void* opaque,
    const downwell::DeferredCreateInstance& command) {
    DW_PROFILE_SCOPE(profileGeneratedRegistration);
    auto& self = *static_cast<AmigaGameplayGeneration*>(opaque);
    // Border, pillar and isolated wall/platform Creates: the same backend
    // publication createGeneratedTerrain would make, without the world hop.
    if ((command.objectId == 60 || command.objectId == 93) && self.runtime_->terrainCreatesBackendOnly()) {
        if ((!(command.objectId==60&&!command.active) &&
             !amigaAddTerrainObject(self.index_, command.objectId, command.x, command.y)) ||
            !self.tiles_.append(command)) return false;
        if (command.y - 24 < self.dirtyFirst) self.dirtyFirst = command.y - 24;
        if (command.y + 24 > self.dirtyEnd) self.dirtyEnd = command.y + 24;
        return true;
    }
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

#include "AmigaGameplayGeneration.h"
#include "AmigaPortRuntime.h"
#include "AmigaProfile.h"

bool AmigaGameplayGeneration::loadRoom(AmigaPortRuntime& runtime,
    const downwell::RoomPack& rooms,const downwell::PackedRoom& room,
    const downwell::PreparedRoomTerrain* prepared){
    return runtime.loadRoomTerrain(rooms,room,[this](const downwell::PackedRoomInstance& instance){
        return amigaAddTerrainObject(index_,instance.objectId,instance.x,instance.y,true);
    },prepared);
}

void AmigaGameplayGeneration::bind(AmigaPortRuntime& runtime,bool terrainRuns) {
    runtime_ = &runtime;
    dirtyFirst = worldRows; dirtyEnd = 0;
    builder_.setCreateObserver(create, this);
#ifndef DOWNWELL_REFERENCE_GENERATED_BOXES
    if(runtime.terrainCreatesBackendOnly())builder_.setBreakablePublisher(createBreakable);
#endif
    if(terrainRuns)builder_.setTerrainRunObserver(createRun);
#ifndef DOWNWELL_REFERENCE_PATTERNS
    if(terrainRuns)builder_.setPreparedTerrainPublisher(createPrepared);
#endif
}

bool AmigaGameplayGeneration::createPrepared(void* opaque,const downwell::AreaOneBuilder::PreparedTerrain& batch,
    downwell::AreaOneBuilder::EmitFunction){
    auto& self=*static_cast<AmigaGameplayGeneration*>(opaque);
    if(!self.runtime_->terrainCreatesBackendOnly() || !amigaAdmitPreparedTerrain(self.index_,self.tiles_,batch))
        return batch.executeReference(create,opaque);
    if(!batch.beginPublication())return false;
    if(batch.count()){
        if(batch.y-24<self.dirtyFirst)self.dirtyFirst=batch.y-24;
        if(batch.y+24>self.dirtyEnd)self.dirtyEnd=batch.y+24;
    }
    // Occupancy and landing candidates are one publication, in the source
    // segment's order relative to event Creates. No later segment is exposed.
    const unsigned row=unsigned(batch.y)>>4;
    auto* destination=self.index_.cells+row*AmigaTerrainIndex::columns;
    auto* landing=self.index_.nextLanding.publicationRow(row);
    const std::uint16_t bit=std::uint16_t(0x8000u>>(row&15));
    const auto* word=batch.words();
    for(unsigned i=0;i<batch.wordCount();++i,word+=4){
        auto* target=destination+word[0];
        if(!(target[0]|target[1])){
            target[0]=word[2];target[1]=word[3];
        }else for(unsigned half=0;half<2;++half){
            const unsigned old=target[half],prepared=word[2+half];
            unsigned walls=(old>>6)+(prepared>>6);
            while(walls>3){
                if(!self.index_.extraContacts.append(60,(int(word[0]+half)-32)*16,batch.y))return false;
                --walls;
            }
            target[half]=((old|prepared)&63)|(walls<<6);
        }
        auto* candidate=landing+unsigned(word[0])*32;
        if(word[1]&1)*candidate|=bit;
        if(word[1]&2)candidate[32]|=bit;
    }
    if(!self.tiles_.appendPrepared(batch))return false;
    batch.completePublication();
    return true;
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

bool AmigaGameplayGeneration::createBreakable(void* opaque,int x,int y,
    const downwell::GeneratedBreakableCreate& creation){
    auto& self=*static_cast<AmigaGameplayGeneration*>(opaque);
    if(!self.runtime_->effectsSucceeded())return false;
    // Preserve the original partial-publication sequence: occupancy, tile,
    // dirty extent, then the family's lifetime/registration and collision grid.
    if(!amigaAddTerrainObject(self.index_,92,x,y)||
       !self.tiles_.appendCreatedTile(92,x,y,creation.frame))return false;
    if(y-24<self.dirtyFirst)self.dirtyFirst=y-24;
    if(y+24>self.dirtyEnd)self.dirtyEnd=y+24;
    return self.runtime_->appendGeneratedBreakable(x-240,y-176,creation);
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

#ifndef DOWNWELL_AMIGA_ENCOUNTER_RESOURCES_H
#define DOWNWELL_AMIGA_ENCOUNTER_RESOURCES_H
#include "AmigaRunSoundAudio.h"
#include "framework/AmigaPlatform.h"
#include "core/GameplayEncounterEnvironment.h"
#include "core/GameplayEncounterSession.h"
#include "core/PlacementNew.h"
#include "../../../generated/amiga-encounter-masks.h"
#include <exec/memory.h>
#include <proto/exec.h>
#if defined(DOWNWELL_RESULTS_RESTART_VERIFY) || defined(DOWNWELL_CLEAR_VERIFY)
extern "C" {
volatile unsigned gDownwellEncounterSceneLoads=0,gDownwellEncounterSceneReleases=0;
volatile unsigned gDownwellEncounterRunLoads=0;
}
#endif

// Run-owned collision history is acquired before rmMain's first birth. The
// immutable masks and update environment are gameplay scene resources.
// Neither is embedded in the executable or leased by the intro screens.
// Sound goes to the run's request ring (AmigaRunSoundAudio), which the main
// loop drains into the Paula mixer.
class AmigaEncounterResources {
    struct Room {
        GameplayWorld* world;
        downwell::RunnerCollisionEventContext::RoomState operator()()const{
            return {world->runStage().area!=0&&!world->levelClearActive(),
                    world->levelResultRequested()||world->nextLevelRequested()};
        }
    };
    using Owner=downwell::GameplayCollisionOwner;
    using Environment=downwell::GameplayEncounterEnvironment<downwell::AmigaRunSoundAudio,Room>;
public:
    AmigaEncounterResources(GameplayWorld& world,Owner& owner):world_(world),owner_(owner),audio_(world){}
    ~AmigaEncounterResources(){
        releaseScene();
        if(collision_){collision_->~NativeCollisionStorage();FreeMem(collision_,sizeof(*collision_));}
    }
    bool ready()const{return environment_!=nullptr;}
    bool acquire(AmigaPlatform& platform){
        if(ready())return true;
        if(!collision_){
            auto* allocation=AllocMem(sizeof(Owner::NativeCollisionStorage),MEMF_PUBLIC);
            if(!allocation)return false;
            collision_=new(allocation) Owner::NativeCollisionStorage;
            if(!owner_.bindNativeCollisionStorage(*collision_))return false;
#if defined(DOWNWELL_RESULTS_RESTART_VERIFY) || defined(DOWNWELL_CLEAR_VERIFY)
            ++gDownwellEncounterRunLoads;
#endif
        }
        masks_=static_cast<UBYTE*>(AllocMem(DOWNWELL_ENCOUNTER_MASK_BYTES,MEMF_PUBLIC));
        if(!masks_||platform.loadData("Downwell.encountermasks",masks_,
                DOWNWELL_ENCOUNTER_MASK_BYTES)!=DOWNWELL_ENCOUNTER_MASK_BYTES){releaseScene();return false;}
        maskView_=downwell::SpriteMaskPack(masks_,DOWNWELL_ENCOUNTER_MASK_BYTES);
        if(!maskView_.valid()){releaseScene();return false;}
        auto* allocation=AllocMem(sizeof(Environment),MEMF_PUBLIC);
        if(!allocation){releaseScene();return false;}
        environment_=new(allocation) Environment(world_,owner_,maskView_,audio_,Room{&world_});
#if defined(DOWNWELL_RESULTS_RESTART_VERIFY) || defined(DOWNWELL_CLEAR_VERIFY)
        ++gDownwellEncounterSceneLoads;
#endif
        return true;
    }
    bool bind(downwell::GameplayEncounterSession::Frame& frame){
        return ready()&&(world_.boundEncounterUpdate(environment_)||frame.bindUpdateEnvironment(*environment_));
    }
    void releaseScene(){
#if defined(DOWNWELL_RESULTS_RESTART_VERIFY) || defined(DOWNWELL_CLEAR_VERIFY)
        if(environment_)++gDownwellEncounterSceneReleases;
#endif
        if(environment_){environment_->~Environment();FreeMem(environment_,sizeof(Environment));environment_=nullptr;}
        maskView_=downwell::SpriteMaskPack(nullptr,0);
        if(masks_){FreeMem(masks_,DOWNWELL_ENCOUNTER_MASK_BYTES);masks_=nullptr;}
    }
private:
    GameplayWorld& world_;
    Owner& owner_;
    Owner::NativeCollisionStorage* collision_=nullptr;
    UBYTE* masks_=nullptr;
    downwell::SpriteMaskPack maskView_{nullptr,0};
    downwell::AmigaRunSoundAudio audio_;
    Environment* environment_=nullptr;
};
#endif

#ifndef DOWNWELL_CORE_GAMEPLAY_ENCOUNTER_ENVIRONMENT_H
#define DOWNWELL_CORE_GAMEPLAY_ENCOUNTER_ENVIRONMENT_H
#include "GameplayDitherCollisionPass.h"
#include "EncounterSettings.h"
namespace downwell {
// Production update services for the currently admitted dither/wind pairs.
// The frontend owns the scene lease; the frame borrows this scope until Draw.
template<class Audio,class RoomState,unsigned Pairs=16384>
class GameplayEncounterEnvironment {
public:
    GameplayEncounterEnvironment(GameplayWorld& world,GameplayCollisionOwner& owner,
        const SpriteMaskPack& masks,Audio& audio,RoomState roomState)
        :world_(world),owner_(owner),audio_(audio),roomState_(roomState),contacts_(owner,masks){}
    GameplayEncounterEnvironment(const GameplayEncounterEnvironment&)=delete;
    GameplayEncounterEnvironment& operator=(const GameplayEncounterEnvironment&)=delete;
    ~GameplayEncounterEnvironment(){if(bound_)leaveUpdate();}
    EncounterSettings& settings(){return settings_;}
    bool enterUpdate(){
        if(bound_||!owner_.boundTo(world_)||!audio_.ready()||settings_.gameVolume.v<0)return false;
        if(!prepared_){
            for(int id=TimeFieldWind::firstSound;id<TimeFieldWind::firstSound+TimeFieldWind::soundCount;++id)
                if(!audio_.hasResource(id))return false;
            prepared_=true;
        }
        if(!audio_.setMasterGain(settings_.masterGain))return false;
        audio_.update();
        world_.configureDitherAboveEnvironment(settings_.lowSpec,settings_.ending);
        bound_=world_.bindEncounterUpdate(this,RunAudioBinding::bind(audio_),this,
            [](void* p){auto& self=*static_cast<GameplayEncounterEnvironment*>(p);
                return self.contacts_.run(self.world_,self.settings_.gameVolume,self.audio_,self.roomState_);});
        return bound_;
    }
    bool leaveUpdate(){
        if(!bound_)return false;
        const bool okay=world_.unbindEncounterUpdate(this);bound_=false;return okay;
    }
private:
    GameplayWorld& world_;
    GameplayCollisionOwner& owner_;
    Audio& audio_;
    RoomState roomState_;
    GameplayDitherCollisionPass<Pairs> contacts_;
    EncounterSettings settings_;
    bool bound_=false;
    bool prepared_=false; // Immutable scene dependencies outlive this borrower.
};
}
#endif

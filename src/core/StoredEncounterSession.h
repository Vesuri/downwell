#ifndef DOWNWELL_CORE_STORED_ENCOUNTER_SESSION_H
#define DOWNWELL_CORE_STORED_ENCOUNTER_SESSION_H
#include "EncounterBoundary.h"
#include "EncounterSettings.h"
#include "Game.h"
#include "StoredDitherCollisionPass.h"
#include "TimeFieldWind.h"

namespace downwell {
// Production owner for the stored-world frontend's admitted event bindings.
// Devices and decoded assets outlive this object; no SDL calls or game rules
// belong here. New event families must install their real scopes here before
// they are admitted, not add a second collision pass to the frontend.
template<class Audio> class StoredEncounterSession {
public:
    StoredEncounterSession(Game& game,StoredWorldCollision& collision,Audio& audio)
        :game_(game),collision_(collision),audio_(audio),contacts_(collision){}
    StoredEncounterSession(const StoredEncounterSession&)=delete;
    StoredEncounterSession& operator=(const StoredEncounterSession&)=delete;
    EncounterSettings& settings(){return settings_;}
    const EncounterSettings& settings()const{return settings_;}
    bool succeeded()const{return boundary_.succeeded();}
    bool finished()const{return !succeeded()||game_.finished();}
    bool start(){
        return boundary_.start([&]{
            if(game_.startupShortcutApplied()||!collision_.boundTo(game_.startup())||!audio_.ready())return false;
            // TimeFieldWind chooses any one of these seven sounds. Check the
            // complete dependency before enabling that collision event.
            for(int id=TimeFieldWind::firstSound;id<TimeFieldWind::firstSound+TimeFieldWind::soundCount;++id)
                if(!audio_.hasResource(id))return false;
            return withUpdateEnvironment([&]{
                game_.applyStartupShortcut();return worldSucceeded();
            });
        });
    }
    bool update(const Input& input,std::uint32_t seed=2){
        return boundary_.update([&]{
            audio_.update();
            return withUpdateEnvironment([&]{
                game_.update(input,seed);return game_.simulationSucceeded()&&worldSucceeded();
            });
        });
    }
    bool draw(Renderer& renderer){
        return boundary_.draw([&]{
            if(!worldSucceeded())return false;
            game_.render(renderer);return game_.renderingSucceeded()&&worldSucceeded();
        });
    }
    bool retire(){return boundary_.retire();}
private:
    bool worldSucceeded()const{
        const auto& world=game_.startup();
        return collision_.valid()&&world.effectsSucceeded()&&world.roomLoadSucceeded()&&world.deferredCommandsSucceeded();
    }
    template<class Body> bool withUpdateEnvironment(Body body){
        if(!settings_.gameVolume.valid()||!audio_.setMasterGain(settings_.masterGain))return false;
        return game_.withCollisionPass([&](StartupRuntime& world){
            return contacts_.run(world,settings_.gameVolume,audio_);
        },[&]{
            // The mutable runtime is supplied at the core-owned binding seam;
            // backend adapters do not reach through const inspection APIs.
            game_.configureDitherAboveEnvironment(settings_.lowSpec,settings_.ending);
            return body();
        });
    }
    Game& game_;
    StoredWorldCollision& collision_;
    Audio& audio_;
    StoredDitherCollisionPass contacts_;
    EncounterSettings settings_;
    EncounterBoundary boundary_;
};
}
#endif

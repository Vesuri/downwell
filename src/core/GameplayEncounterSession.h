#ifndef DOWNWELL_CORE_GAMEPLAY_ENCOUNTER_SESSION_H
#define DOWNWELL_CORE_GAMEPLAY_ENCOUNTER_SESSION_H
#include "EncounterBoundary.h"
#include "GameplayWorld.h"
#include "GameplayCollisionOwner.h"

namespace downwell {
// Session lifetime is independent of rooms, Retry and display banks. The
// world's attached collision/lifetime owner remains the authority; this does
// not create another dispatch stream or admit unbound registered families.
class GameplayEncounterSession {
public:
    GameplayEncounterSession(GameplayWorld& world,GameplayCollisionOwner& owner)
        :world_(world),owner_(owner){}
    GameplayEncounterSession(const GameplayEncounterSession&)=delete;
    GameplayEncounterSession& operator=(const GameplayEncounterSession&)=delete;
    bool start(){return boundary_.start([&]{return healthy();});}
    bool succeeded()const{return boundary_.succeeded()&&healthy();}
    bool retire(){if(!healthy())boundary_.fail();return boundary_.retire();}
    class Frame {
    public:
        explicit Frame(GameplayEncounterSession& session):session_(session),frame_(session.boundary_){
            if(!session_.healthy())frame_.fail();
        }
        bool valid()const{return frame_.valid()&&session_.healthy();}
        bool beginDraw(bool gameplay=false){
            if(!valid()||!frame_.beginDraw())return frame_.fail();
            // Source deferred destruction is swept at gameplay Draw entry,
            // before collecting render commands, not at publication time.
            if(gameplay&&!session_.owner_.prepareDraw())return frame_.fail();
            return true;
        }
        bool publishable(){
            if(!frame_.drawing()||!session_.healthy())return frame_.fail();
            return true;
        }
        bool finish(){
            if(!session_.healthy())frame_.fail();
            return frame_.finish();
        }
    private:
        GameplayEncounterSession& session_;
        EncounterBoundary::Frame frame_;
    };
private:
    bool healthy()const{
        return owner_.boundTo(world_)&&world_.effectsSucceeded()&&world_.generationSucceeded();
    }
    GameplayWorld& world_;
    GameplayCollisionOwner& owner_;
    EncounterBoundary boundary_;
};
}
#endif

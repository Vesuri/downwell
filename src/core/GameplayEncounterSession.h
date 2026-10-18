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
    ~GameplayEncounterSession(){if(inputBound_)world_.bindEncounterInput(nullptr);}
    bool start(){return boundary_.start([&]{
        if(!healthy()||!world_.bindEncounterInput(&input_))return false;
        inputBound_=true;return true;
    });}
    bool succeeded()const{return boundary_.succeeded()&&healthy();}
    const ShopInteraction::Input& inputPublication()const{return input_.publication();}
    bool retire(){
        if(!healthy())boundary_.fail();
        if(inputBound_){world_.bindEncounterInput(nullptr);inputBound_=false;}
        return boundary_.retire();
    }
    class Frame {
    public:
        explicit Frame(GameplayEncounterSession& session):session_(session),frame_(session.boundary_){
            if(!session_.healthy())frame_.fail();
        }
        bool valid()const{return frame_.valid()&&session_.healthy();}
        bool sampleInput(const Input& input){
            if(!valid()||sampled_||frame_.drawing())return frame_.fail();
            session_.input_.sample(input);sampled_=true;return true;
        }
        bool beginDraw(bool gameplay=false){
            if(gameplay&&!sampled_)return frame_.fail();
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
        bool sampled_=false;
    };
private:
    bool healthy()const{
        return (!inputBound_||world_.boundEncounterInput(&input_))&&
            owner_.boundTo(world_)&&world_.effectsSucceeded()&&world_.generationSucceeded();
    }
    GameplayWorld& world_;
    GameplayCollisionOwner& owner_;
    EncounterBoundary boundary_;
    EncounterInput input_;
    bool inputBound_=false;
};
}
#endif

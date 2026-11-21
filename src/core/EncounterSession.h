#ifndef DOWNWELL_CORE_ENCOUNTER_SESSION_H
#define DOWNWELL_CORE_ENCOUNTER_SESSION_H
#include "EncounterBoundary.h"
#include "GameplayWorld.h"

namespace downwell {
// Session lifetime is independent of rooms, Retry and display banks. The
// world's attached collision/lifetime owner, if any, remains the authority;
// this does not create another dispatch stream or admit unbound registered
// families. Owner needs boundTo(world) and prepareDraw().
template<class Owner> class BasicEncounterSession {
public:
    BasicEncounterSession(GameplayWorld& world,Owner& owner)
        :world_(world),owner_(owner){}
    BasicEncounterSession(const BasicEncounterSession&)=delete;
    BasicEncounterSession& operator=(const BasicEncounterSession&)=delete;
    ~BasicEncounterSession(){if(inputBound_)world_.bindEncounterInput(nullptr);}
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
        explicit Frame(BasicEncounterSession& session):session_(session),frame_(session.boundary_){
            if(!session_.healthy())frame_.fail();
        }
        ~Frame(){leaveEnvironment();}
        template<class Environment> bool bindUpdateEnvironment(Environment& environment){
            if(!valid()||!sampled_||frame_.drawing()||environment_)return frame_.fail();
            if(!environment.enterUpdate())return frame_.fail();
            environment_=&environment;
            leave_=[](void* p){return static_cast<Environment*>(p)->leaveUpdate();};
            return true;
        }
        bool valid()const{return frame_.valid()&&session_.healthy();}
        bool sampleInput(const Input& input){
            if(!valid()||sampled_||frame_.drawing())return frame_.fail();
            session_.input_.sample(input);sampled_=true;return true;
        }
        bool beginDraw(bool gameplay=false){
            if(gameplay&&!sampled_)return frame_.fail();
            if(!leaveEnvironment())return frame_.fail();
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
            if(!leaveEnvironment())frame_.fail();
            if(!session_.healthy())frame_.fail();
            return frame_.finish();
        }
    private:
        bool leaveEnvironment(){
            if(!environment_)return true;
            auto* context=environment_;environment_=nullptr;
            const bool okay=leave_(context);leave_=nullptr;
            return okay||frame_.fail();
        }
        BasicEncounterSession& session_;
        EncounterBoundary::Frame frame_;
        void* environment_=nullptr;
        bool (*leave_)(void*)=nullptr;
        bool sampled_=false;
    };
private:
    bool healthy()const{
        return (!inputBound_||world_.boundEncounterInput(&input_))&&
            owner_.boundTo(world_)&&world_.effectsSucceeded()&&world_.generationSucceeded();
    }
    GameplayWorld& world_;
    Owner& owner_;
    EncounterBoundary boundary_;
    EncounterInput input_;
    bool inputBound_=false;
};
// The shipping world-1 build has no collision owner: the compact world owns
// every family it draws and sweeps nothing at Draw entry.
struct NoCollisionOwner {
    bool boundTo(const GameplayWorld&)const{return true;}
    bool prepareDraw(){return true;}
};
using UnownedEncounterSession=BasicEncounterSession<NoCollisionOwner>;
}
#endif

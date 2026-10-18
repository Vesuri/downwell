#include "core/GameplayEncounterSession.h"
#include "CollisionOwnerFixture.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <memory>
#include <vector>
using namespace downwell;
static std::vector<std::uint8_t> read(const char* path){
    std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};
}
int main(){
    const auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc"),
        rb=read("generated/downwell-rooms.dwrm");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    RoomPack rooms(rb.data(),rb.size());assert(objects.valid()&&sprites.valid()&&rooms.valid());
    for(unsigned failure=0;failure<3;++failure){
        auto owner=makeCollisionOwner(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},
            &rooms,GameplayCollisionOwner::Ownership::native);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),nullptr,&rooms);
        assert(owner->attach(*world));
        GameplayEncounterSession session(*world,*owner);
        assert(session.start());
        for(unsigned update=0;update<164;++update){
            GameplayEncounterSession::Frame frame(session);assert(frame.valid());
            world->step(0);
            assert(frame.beginDraw(true)&&frame.publishable()&&frame.finish());
        }
        {
            GameplayEncounterSession::Frame handoff(session);assert(handoff.valid());
            assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay);
            assert(handoff.finish());
        }
        unsigned publications=0;
        {
            GameplayEncounterSession::Frame frame(session);assert(frame.valid());
            if(failure==1){
                // Real failed world callback, before Draw: no presentation.
                assert(!world->withShopStepState([](auto&,auto&){return false;}));
                assert(!frame.beginDraw(true));
            }else{
                assert(frame.beginDraw(true));
                if(failure==2)assert(!world->withShopStepState([](auto&,auto&){return false;}));
                if(frame.publishable())++publications;
            }
            assert(frame.finish()==(failure==0));
        }
        assert(publications==(failure==0)&&session.succeeded()==(failure==0));
        assert(session.retire()==(failure==0));
        GameplayEncounterSession::Frame retired(session);assert(!retired.valid());
    }
    std::puts("Native encounter session: updates, Draw, Retry, failure publication and retirement passed");
}

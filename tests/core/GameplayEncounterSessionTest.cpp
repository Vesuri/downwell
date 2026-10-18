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
    for(unsigned failure=0;failure<7;++failure){
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
            Input input;input.cancel=update<2;assert(frame.sampleInput(input));
            world->step(update<2?16:0);
            assert(world->shops().inputState().cancel==(update==0));
            assert(frame.beginDraw(true)&&frame.publishable()&&frame.finish());
        }
        {
            GameplayEncounterSession::Frame handoff(session);assert(handoff.valid());
            Input held;held.cancel=held.fire=held.right=true;assert(handoff.sampleInput(held));
            struct Audio {unsigned stops=0;bool stopAll(){++stops;return true;}} audio;
            assert(world->withRunAudio(audio,[&]{
                assert(!world->withRunAudio(audio,[]{return true;}));
                for(unsigned retry=0;retry<2;++retry)
                    assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay);
                return true;
            })&&audio.stops==2); // Binding survives the in-place world reconstruction.
            assert(world->withRunAudio(audio,[]{return true;})); // Lexical teardown.
            assert(handoff.finish());
        }
        // Menus publish input even when the gameplay world does not Step.
        for(unsigned menu=0;menu<2;++menu){
            GameplayEncounterSession::Frame frame(session);Input held;held.cancel=held.fire=held.right=true;
            assert(frame.sampleInput(held)&&frame.beginDraw()&&frame.finish());
        }
        {
            GameplayEncounterSession::Frame frame(session);Input held;held.cancel=held.fire=held.right=true;
            assert(frame.sampleInput(held));world->step(22);
            assert(!world->shops().inputState().cancel&&!world->shops().inputState().confirm&&
                   !world->shops().inputState().right);
            assert(frame.beginDraw(true)&&frame.finish());
        }
        unsigned publications=0;
        {
            GameplayEncounterSession::Frame frame(session);assert(frame.valid());
            if(failure!=5)assert(frame.sampleInput({}));
            if(failure==4){
                assert(!frame.sampleInput({})&&!frame.beginDraw(true));
            }else if(failure==5){
                assert(!frame.beginDraw(true)); // No gameplay publication without a sample.
            }else if(failure==6){
                assert(world->bindEncounterInput(nullptr)&&!frame.beginDraw(true));
            }else if(failure==3){
                struct RejectAudio {bool stopAll(){return false;}} audio;
                assert(world->withRunAudio(audio,[&]{
                    assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)==ResultsDestination::none);
                    return true; // Ignoring the callback failure cannot permit publication.
                }));
                assert(!world->effectsSucceeded()&&!frame.beginDraw(true));
            }else if(failure==1){
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
        assert(!world->shops().borrowedInput);
        GameplayEncounterSession::Frame retired(session);assert(!retired.valid());
    }
    std::puts("Native encounter session: updates, Draw, Retry, failure publication and retirement passed");
}

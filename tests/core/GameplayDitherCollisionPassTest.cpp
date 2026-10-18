#include "core/GameplayDitherCollisionPass.h"
#include "core/GameplayEncounterEnvironment.h"
#include "core/GameplayEncounterSession.h"
#include "CollisionOwnerFixture.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <cstdio>
using namespace downwell;
static std::vector<std::uint8_t> read(const char* path){
    std::ifstream in(path,std::ios::binary);assert(in);return {std::istreambuf_iterator<char>(in),{}};
}
struct Audio {
    bool playing=false,fail=false;std::vector<int> calls;
    unsigned stops=0,updates=0;int missing=-1;bool masterOkay=true;
    bool ready()const{return true;}
    bool hasResource(int id)const{return id!=missing;}
    bool stopAll(){++stops;playing=false;return !fail;}
    void update(){++updates;}
    bool setMasterGain(MotionBinary64 gain){return masterOkay&&gain.high==0x3fe33333u&&gain.low==0x33333333u;}
    bool isPlaying(int id,bool& out){calls.push_back(id);out=id==900&&playing;return !fail;}
    bool stop(int id){calls.push_back(10000+id);return true;}
    bool play(int id,int priority,bool loop,int& voice){
        assert(priority==90&&!loop);calls.push_back(20000+id);voice=900;playing=true;return true;
    }
    bool getGain(int,MotionBinary64& gain){gain=MotionBinary64::fromInteger(1);return true;}
    bool setGain(int,MotionBinary64,int){return true;}
    bool setPitch(int,MotionBinary64){return true;}
};
int main(int argc,char** argv){
    const auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc"),
        rb=read("generated/downwell-rooms.dwrm"),mb=read(argc>1?argv[1]:"generated/downwell-sprite-masks.dwsm");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    RoomPack rooms(rb.data(),rb.size());SpriteMaskPack masks(mb.data(),mb.size());
    assert(objects.valid()&&sprites.valid()&&rooms.valid()&&masks.valid());
    std::vector<int> reference;
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::nativeScenery}){
        for(unsigned failure=0;failure<2;++failure){
            auto owner=makeCollisionOwner(objects,sprites,{}, {},&rooms,mode);
            const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
                owner->breakableBinding(),owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
            auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
            assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain))&&world->createAuthoredDitherAbove());
            GameMakerRandom random;
            for(unsigned i=0;i<8;++i){
                if(native){assert(terrain->append(60,496+int(i)*16,-128));
                    assert(terrain->createProperties(terrain->handle(i),51));}
                else {assert(owner->terrain().append(60,496+int(i)*16,-128,true));
                    assert(owner->terrain().createProperties(owner->terrain().handle(i),51,false,random));}
            }
            auto& fields=const_cast<GameplayGoalFields&>(world->goalFieldInstances());
            // Retained overlapping entry fields; normal first-level Create
            // destroys its entry field. This is an explicit ordering fixture.
            assert(fields.create(240,176,371)&&fields.create(240,176,371));
            auto pass=std::make_unique<GameplayDitherCollisionPass<256>>(*owner,masks);
            Audio audio;audio.fail=failure;
            const auto before=world->randomDrawCount();
            bool pending=true;
            auto room=[&]{return RunnerCollisionEventContext::RoomState{true,pending};};
            assert(pass->run(*world,MotionBinary64::fromInteger(1),audio,room));
            assert(audio.calls.empty()&&world->randomDrawCount()==before);
            pending=false;
            // Source dispatcher retains dirty collision entries for next pass.
            const bool okay=pass->run(*world,MotionBinary64::fromInteger(1),audio,room);
            assert(okay==!failure&&!audio.calls.empty());
            if(failure){
                assert(!owner->valid()&&!world->effectsSucceeded());
                const auto count=audio.calls.size();
                assert(!pass->run(*world,MotionBinary64::fromInteger(1),audio,room)&&audio.calls.size()==count);
            }else{
                assert(world->randomDrawCount()==before+1&&world->timeFieldWindVoice()==900);
                // Which overlapping lifetime owns the RNG-selected sound is
                // observable even though both contacts share the same voice.
                for(unsigned i=0;i<2;++i)audio.calls.push_back(fields.find(fields.handle(i))->windSound);
                if(mode==GameplayCollisionOwner::Ownership::reference)reference=audio.calls;
                else assert(audio.calls==reference);
                unsigned destroyed=0;
                for(unsigned i=0;i<8;++i){
                    if(native){GameplayWorld::NativeTerrain::Identity id;bool active,dead;
                        assert(terrain->read(terrain->handle(i),id,active,dead));destroyed+=dead;}
                    else destroyed+=owner->terrain().find(owner->terrain().handle(i))->destroyRequested;
                }
                assert(destroyed>0); // Same sealed stream also executed code 991.
                const auto queries=audio.calls.size();unsigned passes=0;
                assert(world->withCollisionPass([&]{++passes;return pass->run(*world,
                    MotionBinary64::fromInteger(1),audio,room);},[&]{world->step(0);return world->effectsSucceeded();}));
                assert(passes==1&&audio.calls.size()>queries);
                assert(owner->prepareDraw()); // Physical retirement only at Draw.
                GameplayEncounterSession session(*world,*owner);assert(session.start());
                using Environment=GameplayEncounterEnvironment<Audio,decltype(room),256>;
                auto environment=std::make_unique<Environment>(*world,*owner,masks,audio,room);
                {
                    GameplayEncounterSession::Frame frame(session);
                    assert(frame.sampleInput({})&&frame.bindUpdateEnvironment(*environment));
                    assert(world->boundEncounterUpdate(environment.get()));
                    world->step(0);assert(world->effectsSucceeded());
                    assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay);
                    assert(audio.stops==1&&world->boundEncounterUpdate(environment.get()));
                    assert(frame.beginDraw(true)&&!world->boundEncounterUpdate(environment.get()));
                    assert(frame.publishable()&&frame.finish());
                }
                {
                    GameplayEncounterSession::Frame frame(session);
                    assert(frame.sampleInput({})&&frame.bindUpdateEnvironment(*environment));
                    assert(frame.finish()&&!world->boundEncounterUpdate(environment.get()));
                }
                assert(audio.updates==2&&session.retire());
                // An abandoned frame is terminal, but must still retire its
                // borrowed callbacks before the environment/device disappears.
                GameplayEncounterSession abandoned(*world,*owner);assert(abandoned.start());
                {
                    GameplayEncounterSession::Frame frame(abandoned);
                    assert(frame.sampleInput({})&&frame.bindUpdateEnvironment(*environment));
                }
                assert(!abandoned.succeeded()&&!world->boundEncounterUpdate(environment.get()));
            }
        }
    }
    for(unsigned failure=0;failure<4;++failure){
        auto owner=makeCollisionOwner(objects,sprites,{}, {},&rooms,GameplayCollisionOwner::Ownership::nativeScenery);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),nullptr,&rooms);
        assert(owner->attach(*world));
        GameplayEncounterSession session(*world,*owner);assert(session.start());
        Audio audio;
        auto room=[]{return RunnerCollisionEventContext::RoomState{true,false};};
        using Environment=GameplayEncounterEnvironment<Audio,decltype(room),256>;
        auto environment=std::make_unique<Environment>(*world,*owner,masks,audio,room);
        {
            GameplayEncounterSession::Frame frame(session);assert(frame.sampleInput({}));
            if(failure==0)audio.missing=355;
            if(failure==1)audio.masterOkay=false;
            if(failure<2){assert(!frame.bindUpdateEnvironment(*environment));}
            else {
                assert(frame.bindUpdateEnvironment(*environment));
                if(failure==2)assert(!frame.bindUpdateEnvironment(*environment));
                else {
                    audio.fail=true;
                    assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::none);
                }
            }
            assert(!frame.beginDraw(true)&&!frame.publishable()&&!frame.finish());
        }
        assert(!session.succeeded()&&!world->boundEncounterUpdate(environment.get()));
        // Even a failed owner releases the reference binding slots.
        audio.fail=false;assert(world->withRunAudio(audio,[]{return true;}));
    }
    std::puts("Dither/wind admission: reference/native sealed order, room suppression and terminal audio failure passed");
}

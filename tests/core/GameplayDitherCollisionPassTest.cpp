#include "core/GameplayDitherCollisionPass.h"
#include "core/GameplayEncounterEnvironment.h"
#include "core/GameplayEncounterSession.h"
#include "CollisionOwnerFixture.h"
#include <cassert>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
using namespace downwell;
static std::vector<std::uint8_t> read(const char* path){
    std::ifstream in(path,std::ios::binary);assert(in);return {std::istreambuf_iterator<char>(in),{}};
}
struct Audio {
    bool playing=false,fail=false;std::vector<int> calls;
    std::vector<MotionBinary64> gains;
    MotionBinary64 expectedMaster{0x3fe33333u,0x33333333u};
    unsigned stops=0,updates=0;int missing=-1;bool masterOkay=true;
    bool ready()const{return true;}
    bool hasResource(int id)const{return id!=missing;}
    bool stopAll(){++stops;playing=false;return !fail;}
    void update(){++updates;}
    bool setMasterGain(MotionBinary64 gain){return masterOkay&&gain.high==expectedMaster.high&&gain.low==expectedMaster.low;}
    bool isPlaying(int id,bool& out){calls.push_back(id);out=id==900&&playing;return !fail;}
    bool stop(int id){calls.push_back(10000+id);return true;}
    bool play(int id,int priority,bool loop,int& voice){
        assert(priority==90&&!loop);calls.push_back(20000+id);voice=900;playing=true;return true;
    }
    bool getGain(int,MotionBinary64& gain){gain=MotionBinary64::fromInteger(1);return true;}
    bool setGain(int,MotionBinary64 gain,int){gains.push_back(gain);return true;}
    bool setPitch(int,MotionBinary64){return true;}
};
template<bool Prepared> static void exercise(int argc,char** argv,unsigned history=0){
    const auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc"),
        rb=read("generated/downwell-rooms.dwrm"),mb=read(argc>1?argv[1]:"generated/downwell-sprite-masks.dwsm");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    RoomPack rooms(rb.data(),rb.size());SpriteMaskPack masks(mb.data(),mb.size());
    assert(objects.valid()&&sprites.valid()&&rooms.valid()&&masks.valid());
    std::vector<int> reference,referenceDestroyed;
    for(auto mode:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::nativeScenery}){
        for(unsigned failure=0;failure<10;++failure){
            struct Presentation {
                unsigned rejectAt=0;std::vector<int> destroyed;
                bool changed(int x,int y,int before,int after){
                    if(before>=0&&after<0){destroyed.push_back(x);destroyed.push_back(y);return destroyed.size()/2!=rejectAt;}
                    return true;
                }
            } presentation;
            auto owner=makeCollisionOwner(objects,sprites,GameplayTerrainInstances::Presentation::bind(presentation), {},&rooms,mode);
            const bool native=mode!=GameplayCollisionOwner::Ownership::reference;
            auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
                owner->breakableBinding(),owner->playerBinding(),native?nullptr:&owner->terrain(),&rooms);
            auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
            assert(owner->attach(*world)&&(!native||world->bindNativeTerrain(*terrain))&&world->createAuthoredDitherAbove());
            GameMakerRandom random;
            // Remote, non-contacting births still alter the reference spatial
            // tree. A replacement must not infer ordering from one tidy fixture.
            auto remoteWalls=[&]{
                for(unsigned i=0;i<48;++i){
                    const int x=2048+int(i%8)*16,y=1024+int(i/8)*16;
                    if(native)assert(terrain->append(60,x,y));
                    else assert(owner->terrain().append(60,x,y,true));
                }
            };
            if(history==1||history==3)remoteWalls();
            const unsigned wallBase=history==1||history==3?48:0;
            for(unsigned i=0;i<8;++i){
                if(native){assert(terrain->append(60,496+int(i)*16,-128));
                    assert(terrain->createProperties(terrain->handle(wallBase+i),51));}
                else {assert(owner->terrain().append(60,496+int(i)*16,-128,true));
                    assert(owner->terrain().createProperties(owner->terrain().handle(wallBase+i),51,false,random));}
            }
            if(history==2||history==3)remoteWalls();
            auto& fields=const_cast<GameplayGoalFields&>(world->goalFieldInstances());
            // Retained overlapping entry fields; normal first-level Create
            // destroys its entry field. This is an explicit ordering fixture.
            assert(fields.create(240,176,371)&&fields.create(240,176,371));
            auto pass=std::make_unique<GameplayDitherCollisionPass<256,Prepared>>(*owner,masks);
            Audio audio;audio.fail=failure==1;
            const auto before=world->randomDrawCount();
            bool pending=true;
            auto room=[&]{return RunnerCollisionEventContext::RoomState{true,pending};};
            assert(pass->run(*world,MotionBinary64::fromInteger(1),audio,room));
            assert(audio.calls.empty()&&world->randomDrawCount()==before);
            pending=false;
            presentation.rejectAt=failure>=2?failure-1:0;
            // Source dispatcher retains dirty collision entries for next pass.
            const bool okay=pass->run(*world,MotionBinary64::fromInteger(1),audio,room);
            assert(okay==!failure);
            // Pin the runner oracle's ordered notifications, including every
            // possible failing prefix. Remote non-contacting births change it.
            const std::vector<int> expected=history==1?
                std::vector<int>{496,-128,528,-128,544,-128,512,-128,560,-128,576,-128,608,-128,592,-128}:
                std::vector<int>{560,-128,576,-128,608,-128,592,-128,496,-128,528,-128,544,-128,512,-128};
            if(failure<2)assert(!audio.calls.empty());
            else assert(presentation.destroyed.size()==2*(failure-1));
            assert(presentation.destroyed.size()<=expected.size());
            for(unsigned i=0;i<presentation.destroyed.size();++i)assert(presentation.destroyed[i]==expected[i]);
            if(!failure)assert(presentation.destroyed==expected);
            if(failure){
                assert(!owner->valid());
                // The scalar terrain observer latches its owner, whereas
                // native destruction also passes through world.effects.
                if(failure<2||native)assert(!world->effectsSucceeded());
                const auto count=audio.calls.size();
                assert(!pass->run(*world,MotionBinary64::fromInteger(1),audio,room)&&audio.calls.size()==count);
            }else{
                assert(world->randomDrawCount()==before+1&&world->timeFieldWindVoice()==900);
                // Which overlapping lifetime owns the RNG-selected sound is
                // observable even though both contacts share the same voice.
                for(unsigned i=0;i<2;++i)audio.calls.push_back(fields.find(fields.handle(i))->windSound);
                if(std::getenv("DOWNWELL_CONTACT_ORDER_WITNESS")){
                    std::printf("ORDER prepared=%u native=%u history=%u walls=",unsigned(Prepared),unsigned(native),history);
                    for(auto coordinate:presentation.destroyed)std::printf("%d,",coordinate);
                    std::printf(" audio=");for(auto call:audio.calls)std::printf("%d,",call);
                    std::puts("");
                }
                if(mode==GameplayCollisionOwner::Ownership::reference){reference=audio.calls;referenceDestroyed=presentation.destroyed;}
                else assert(audio.calls==reference&&presentation.destroyed==referenceDestroyed);
                unsigned destroyed=0;
                for(unsigned i=0;i<8;++i){
                    if(native){GameplayWorld::NativeTerrain::Identity id;bool active,dead;
                        assert(terrain->read(terrain->handle(wallBase+i),id,active,dead));destroyed+=dead;}
                    else destroyed+=owner->terrain().find(owner->terrain().handle(wallBase+i))->destroyRequested;
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
static void sessionInputsAndSettings(int argc,char** argv){
    const auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc"),
        rb=read("generated/downwell-rooms.dwrm"),mb=read(argc>1?argv[1]:"generated/downwell-sprite-masks.dwsm");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    RoomPack rooms(rb.data(),rb.size());SpriteMaskPack masks(mb.data(),mb.size());
    PackedRoom room;assert(rooms.findRoom(11,room));unsigned sourceId=0;
    for(unsigned i=0;i<room.instanceCount;++i){PackedRoomInstance r;
        assert(rooms.roomInstance(room,i,r));if(r.objectId==61)sourceId=r.instanceId;
    }
    assert(sourceId);
    for(bool lowSpec:{false,true}){
        auto owner=makeCollisionOwner(objects,sprites,{}, {},&rooms,GameplayCollisionOwner::Ownership::nativeScenery);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),nullptr,&rooms);
        auto terrain=std::make_unique<GameplayWorld::NativeTerrain>();
        assert(owner->attach(*world)&&world->bindNativeTerrain(*terrain));
        GameplayDitherAbove::Snapshot imported;imported.sourceId=sourceId;
        assert(imported.y.assignInteger(-64)&&world->adoptDitherAbove(imported));
        const auto dither=world->ditherAbove()->handle();
        auto& fields=const_cast<GameplayGoalFields&>(world->goalFieldInstances());
        assert(fields.create(240,176,371));
        Audio audio;audio.expectedMaster={0x3fe00000u,0};
        auto roomState=[]{return RunnerCollisionEventContext::RoomState{true,false};};
        using Environment=GameplayEncounterEnvironment<Audio,decltype(roomState),256>;
        auto environment=std::make_unique<Environment>(*world,*owner,masks,audio,roomState);
        environment->settings().masterGain=audio.expectedMaster;
        environment->settings().gameVolume={0x3fd00000u,0}; // A quarter, before the explicit wind override.
        environment->settings().lowSpec=lowSpec;
        GameplayEncounterSession session(*world,*owner);assert(session.start());
        Input held;held.right=held.left=held.fire=held.cancel=true;
        {
            GameplayEncounterSession::Frame frame(session);
            assert(frame.sampleInput(held)&&frame.bindUpdateEnvironment(*environment));
            world->step(0);assert(world->effectsSucceeded());
            const auto* actor=world->ditherAbove()->find(dither);assert(actor&&!actor->destroyRequested);
            RunnerImageValue expected;assert(expected.assignInteger(lowSpec?-48:-64));
            assert(actor->y.bits==expected.bits); // Camera y=176: only the 208-pixel branch advances.
            assert(audio.gains.size()==2&&audio.gains[0].high==0x3fd00000u&&!audio.gains[0].low&&
                audio.gains[1].high==0x3fe00000u&&!audio.gains[1].low);
            const auto& edges=session.inputPublication();
            assert(edges.right&&edges.left&&edges.confirm&&edges.cancel); // Neither admitted event consumes controls.
            assert(frame.beginDraw(true)&&frame.publishable()&&frame.finish());
            assert(!world->boundEncounterUpdate(environment.get()));
        }
        environment->settings().ending=true; // Must be reread on the next production binding.
        {
            GameplayEncounterSession::Frame frame(session);
            assert(frame.sampleInput(held)&&frame.bindUpdateEnvironment(*environment));
            world->step(0);assert(world->effectsSucceeded());
            const auto* actor=world->ditherAbove()->find(dither);assert(actor&&actor->destroyRequested);
            const auto& edges=session.inputPublication();
            assert(!edges.right&&!edges.left&&!edges.confirm&&!edges.cancel);
            assert(frame.beginDraw(true)&&frame.publishable()&&frame.finish());
        }
        assert(session.retire());
    }
    std::puts("Encounter bindings: live lowSpec/ending, separate master/game/wind gain and unconsumed input passed");
}
int main(int argc,char** argv){
    for(unsigned history=0;history<4;++history){exercise<false>(argc,argv,history);exercise<true>(argc,argv,history);}
    sessionInputsAndSettings(argc,argv);
}

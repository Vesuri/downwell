#include "core/GameplayDitherCollisionPass.h"
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
    bool isPlaying(int id,bool& out){calls.push_back(id);out=id==900&&playing;return !fail;}
    bool stop(int id){calls.push_back(10000+id);return true;}
    bool play(int id,int priority,bool loop,int& voice){
        assert(priority==90&&!loop);calls.push_back(20000+id);voice=900;playing=true;return true;
    }
    bool getGain(int,MotionBinary64& gain){gain=MotionBinary64::fromInteger(1);return true;}
    bool setGain(int,MotionBinary64,int){return true;}
    bool setPitch(int,MotionBinary64){return true;}
};
int main(){
    const auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc"),
        rb=read("generated/downwell-rooms.dwrm"),mb=read("generated/downwell-sprite-masks.dwsm");
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
            }
        }
    }
    std::puts("Dither/wind admission: reference/native sealed order, room suppression and terminal audio failure passed");
}

// Differential gate for unregistered native terrain. The registered
// collision owner is the reference: both worlds run the same deterministic
// tape, and every frame must agree on RNG, simulation state and the actor
// orders that the owner supplies (Draw, and through it Step/alarm dispatch).
#include "platform/amiga/AmigaPortRuntime.h"
#include "platform/amiga/AmigaTerrain.h"
#include "platform/amiga/AmigaGameplayGeneration.h"
#include "core/BuilderPatternPack.h"
#include "core/GameplayCollisionOwner.h"
#include "core/GameplayContactProfileInput.h"
#include "core/ObjectPack.h"
#include "core/Replay.h"
#include "core/RoomPack.h"
#include "core/SpriteCollisionPack.h"
#include "core/StartupRuntime.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <vector>

namespace {
unsigned frame=0;
void require(bool condition,const char* message){
    if(condition)return;
    std::cerr<<"FAILED: "<<message<<" (frame "<<frame<<")\n";
    std::exit(1);
}
std::vector<std::uint8_t> readFile(const char* path){
    std::ifstream input(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
using Ownership=downwell::GameplayCollisionOwner::TerrainOwnership;

struct Scene {
    AmigaTerrainFragment fragments[19];
    AmigaTerrainIndex index;
    AmigaAreaOneTerrainBuilder builder{fragments,19};
    AmigaTerrainCollisionContext collision;
    AmigaTerrainTiles tiles;
    AmigaGameplayGeneration generation;
    std::unique_ptr<downwell::GameplayCollisionOwner> owner;
    std::unique_ptr<AmigaPortRuntime> world;
    Scene(const downwell::BuilderPatternPack& patterns,const downwell::ObjectPack& objects,
          const downwell::SpriteCollisionPack& sprites,const downwell::RoomPack& rooms,
          const downwell::PackedRoom& room,const downwell::StartupRuntime& entry,Ownership ownership)
        :collision{nullptr,nullptr,&patterns,nullptr,0,nullptr},
         generation(builder,patterns,objects,index,tiles){
        owner=std::make_unique<downwell::GameplayCollisionOwner>(objects,sprites,
            downwell::GameplayTerrainInstances::Presentation{},downwell::GameplayTerrainInstances::WallCollision{},
            &rooms,ownership);
        require(owner->valid(),"owner prepares");
        world=std::make_unique<AmigaPortRuntime>(300,false,1201-176,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        require(world->effectsSucceeded()&&owner->attach(*world),"owner attaches");
        world->initializeEntryInput(false);
        tiles.clear();
        require(amigaBuildTerrainIndex(collision,index),"index initializes");
        require(generation.loadRoom(*world,rooms,room),"room terrain loads");
        collision.index=&index;collision.runtime=world.get();
        const int none=0;world->configureBreakables(&none,&none,0);
        require(builder.resetFromRoomEntry(entry.mainBuilderState()),"builder imports entry");
        world->configureEnemyRandom(entry.randomState());
        generation.bind(*world);
        world->configureGenerationStep([](void* p,AmigaPortRuntime&,downwell::GameMakerRandom& random,int bottom){
            return static_cast<Scene*>(p)->generation.step(random,bottom);
        },this);
        world->configureLandingResolver(amigaResolveTerrainLanding,&collision);
        world->configureSolidResolver(amigaResolveTerrainSolid,&collision);
    }
    std::uint64_t digest(){
        downwell::StateDigest d;
        const auto& w=*world;
        d.addU64(w.checkpoint());d.addU64(w.randomDrawCount());
        auto random=w.randomState();for(unsigned i=0;i<4;++i)d.addU32(random.nextU32());
        d.addI32(w.health());d.addI32(w.player().offsetXFixed());d.addI32(w.player().offsetYFixed());
        d.addU32(w.enemyCount());
        for(unsigned i=0;i<w.enemyCount();++i){const auto& e=w.enemyAt(i);
            d.addI32(e.objectId);d.addI32(e.x);d.addI32(e.y);d.addI32(e.health);d.addBool(e.active);
            d.addI32(e.alarm0);d.addI32(e.alarm1);d.addU32(e.imagePhase);d.addU32(e.lifetimeId);}
        for(unsigned i=0;i<w.gemCapacity;++i){const auto& g=w.gemAt(i);if(!g.active)continue;
            d.addI32(g.x);d.addI32(g.y);d.addU32(g.age);d.addU32(g.lifetimeId);}
        d.addU32(w.damageNumbers().size());
        unsigned drawn=0;
        require(w.visitRoomActorsReadOnly([&](GameplayWorld::DrawActor kind,downwell::InstanceHandle key,
            downwell::RunnerImageValue depth){
            d.addU32(unsigned(kind));d.addU32(key.slot);d.addU32(key.generation);d.addU32(depth.bits);
            ++drawn;return true;
        }),"draw order visits");
        d.addU32(drawn);
        return d.value();
    }
};
}

int main(int argc,char** argv){
    if(argc<5){std::cerr<<"usage: rooms objects patterns sprites\n";return 2;}
    const auto roomBytes=readFile(argv[1]),objectBytes=readFile(argv[2]),
        patternBytes=readFile(argv[3]),spriteBytes=readFile(argv[4]);
    downwell::RoomPack rooms(roomBytes.data(),roomBytes.size());
    downwell::ObjectPack objects(objectBytes.data(),objectBytes.size());
    downwell::BuilderPatternPack patterns(patternBytes.data(),patternBytes.size());
    downwell::SpriteCollisionPack sprites(spriteBytes.data(),spriteBytes.size());
    downwell::PackedRoom room;
    require(rooms.valid()&&objects.valid()&&patterns.valid()&&sprites.valid()&&rooms.findRoom(11,room),"resources");
    unsigned tapes=0,frames=0,maxTerrain=0;
    for(unsigned phase=0;phase<10;++phase){
        auto host=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::nonDesktop,
            &rooms,&objects,2,&patterns);
        host->skipToGameForTesting();
        auto reference=std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::registered);
        auto native=std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::unregistered);
        for(frame=0;frame<downwell::gameplayContactProfileFrames;++frame){
            const auto& player=reference->world->player();
            const auto input=downwell::packReplayInput(downwell::gameplayContactProfileInput(
                reference->world->tick(),240+player.offsetX(),176+player.offsetY(),phase));
            reference->world->step(input);native->world->step(input);
            const bool referenceOk=reference->world->effectsSucceeded()&&reference->world->generationSucceeded()&&
                reference->owner->valid()&&reference->owner->prepareDraw();
            const bool nativeOk=native->world->effectsSucceeded()&&native->world->generationSucceeded()&&
                native->owner->valid()&&native->owner->prepareDraw();
            require(referenceOk==nativeOk,"both ownership modes succeed or fail together");
            if(!referenceOk)break;
            require(reference->digest()==native->digest(),"simulation state and actor order agree");
            require(reference->owner->terrain().size()==native->owner->terrain().size(),"terrain identities agree");
            if(native->owner->terrain().size()>maxTerrain)maxTerrain=native->owner->terrain().size();
            ++frames;
        }
        ++tapes;
    }
    std::cout<<"terrain ownership: "<<tapes<<" tapes, "<<frames<<" frames, up to "<<maxTerrain
             <<" terrain records agree\n";
    return 0;
}

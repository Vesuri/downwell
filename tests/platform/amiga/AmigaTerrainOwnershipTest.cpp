// Differential gate for native ownership. The complete registered collision
// owner is the reference: every world runs the same deterministic tape, and
// every frame must agree on RNG, simulation state, actor Draw order, and the
// registration order and flag words that order Step and alarm dispatch.
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
#include <algorithm>
#include <cstdlib>
#include <cstring>
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
using Ownership=downwell::GameplayCollisionOwner::Ownership;
using View=downwell::GameplayCollisionView;

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
        // Native ownership keeps no terrain identity records: the occupancy
        // grid is the only terrain representation, as on the Amiga.
        world=std::make_unique<AmigaPortRuntime>(300,false,1201-176,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),
            ownership==Ownership::native?nullptr:&owner->terrain(),&rooms);
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
        d.addU32(w.breakableSlots());
        for(unsigned i=0;i<w.breakableSlots();++i){const auto& b=w.breakableSlot(i);
            d.addU32(b.lifetimeId);d.addBool(b.active);d.addBool(b.destroyRequested);d.addI32(b.alarm0);}
        unsigned drawn=0;
        require(w.visitRoomActorsReadOnly([&](GameplayWorld::DrawActor kind,downwell::InstanceHandle key,
            downwell::RunnerImageValue depth){
            d.addU32(unsigned(kind));d.addU32(key.slot);d.addU32(key.generation);d.addU32(depth.bits);
            ++drawn;return true;
        }),"draw order visits");
        d.addU32(drawn);
        unsigned registered=0;
        require(owner->visitRegistrationOrder([&](downwell::InstanceHandle h,int object,std::uint32_t flags){
            if(h.slot>=View::terrainBase&&h.slot<View::timeFieldBase)return true;
            d.addU32(h.slot);d.addU32(h.generation);d.addI32(object);d.addU32(flags&0x190013u); // Fast-draw 0x4000 is not an order input.
            ++registered;return true;
        }),"registration order visits");
        d.addU32(registered);
        return d.value();
    }
};
}

// Walls are painted as sprite 51 at each visible wall position, with a frame
// derived from the grid. The native painter reads the grid alone, so its
// output matches the record-driven reference when the sets of positions do.
void requireWallsMatchGrid(Scene& reference,Scene& native){
    const auto& records=reference.owner->terrain();
    unsigned referenceWalls=0;
    std::vector<std::uint8_t> seen(AmigaTerrainIndex::columns*AmigaTerrainIndex::rows);
    for(unsigned i=0;i<records.size();++i){
        const auto* r=records.find(records.handle(i));
        require(r!=nullptr,"reference terrain record resolves");
        if(r->objectId!=60||!r->active||r->destroyRequested)continue;
        require(!r->wall.checked,"native walls never run End Step");
        require(amigaIndexedWall(native.index,r->x,r->y),"visible reference wall is a native grid wall");
        const unsigned cell=unsigned((r->y/16)*AmigaTerrainIndex::columns+(r->x-AmigaTerrainIndex::minX)/16);
        if(!seen[cell]){seen[cell]=1;++referenceWalls;}
    }
    unsigned gridWalls=0;
    for(unsigned row=0;row<AmigaTerrainIndex::rows;++row)
        for(unsigned column=0;column<AmigaTerrainIndex::columns;++column)
            if(amigaIndexedWall(native.index,AmigaTerrainIndex::minX+int(column)*16,int(row)*16))++gridWalls;
    require(gridWalls==referenceWalls,"native grid has no wall without a visible reference record");
}

// Backend terrain must not depend on the Create path: native runs publish
// spans directly, while the reference goes through every world Create.
void requireSameBackendTerrain(const Scene& reference,const Scene& other){
    require(std::equal(std::begin(reference.index.cells),std::end(reference.index.cells),std::begin(other.index.cells)),
        "occupancy cells and contact counts agree");
    require(!std::memcmp(&reference.index.nextLanding,&other.index.nextLanding,sizeof(AmigaLandingIndex)),
        "landing candidates agree");
    require(reference.index.extraContacts.size()==other.index.extraContacts.size(),"overflow contacts agree");
    require(reference.tiles.size()==other.tiles.size(),"tile counts agree");
    for(unsigned id=1;id<=reference.tiles.size();++id){
        const auto &a=reference.tiles.at(id),&b=other.tiles.at(id);
        require(a.x==b.x&&a.objectId==b.objectId&&a.frame==b.frame&&a.next==b.next,"tile records agree");
    }
    for(int row=0;row<AmigaTerrainTiles::rows;++row)
        require(reference.tiles.first(row*16)==other.tiles.first(row*16),"tile rows agree");
    require(reference.generation.dirtyFirst==other.generation.dirtyFirst&&
            reference.generation.dirtyEnd==other.generation.dirtyEnd,"dirty generation rows agree");
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
        auto reference=std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::reference);
        std::unique_ptr<Scene> others[2]={
            std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::nativeTerrain),
            std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::native)};
        for(frame=0;frame<downwell::gameplayContactProfileFrames;++frame){
            const auto& player=reference->world->player();
            const auto input=downwell::packReplayInput(downwell::gameplayContactProfileInput(
                reference->world->tick(),240+player.offsetX(),176+player.offsetY(),phase));
            reference->world->step(input);
            const bool referenceOk=reference->world->effectsSucceeded()&&reference->world->generationSucceeded()&&
                reference->owner->valid()&&reference->owner->prepareDraw();
            const auto expected=referenceOk?reference->digest():0;
            for(auto& other:others){
                other->world->step(input);
                const bool ok=other->world->effectsSucceeded()&&other->world->generationSucceeded()&&
                    other->owner->valid()&&other->owner->prepareDraw();
                require(referenceOk==ok,"every ownership mode succeeds or fails together");
                if(!ok)continue;
                require(other->digest()==expected,"simulation state and actor order agree");
                requireSameBackendTerrain(*reference,*other);
                if(other->world->terrainInstances())
                    require(other->owner->terrain().size()==reference->owner->terrain().size(),"terrain identities agree");
                else if(frame%30==0)requireWallsMatchGrid(*reference,*other);
            }
            if(!referenceOk)break;
            if(reference->owner->terrain().size()>maxTerrain)maxTerrain=reference->owner->terrain().size();
            ++frames;
        }
        ++tapes;
    }
    std::cout<<"native ownership: "<<tapes<<" tapes, "<<frames<<" frames, up to "<<maxTerrain
             <<" terrain records agree\n";
    return 0;
}

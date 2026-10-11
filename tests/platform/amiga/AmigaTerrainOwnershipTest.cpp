// Differential gate for native ownership. The complete registered collision
// owner is the reference: every world runs the same deterministic tape, and
// every frame must agree on RNG, simulation state, actor Draw order, and the
// registration order and flag words that order Step and alarm dispatch.
#include "platform/amiga/AmigaPortRuntime.h"
#include "platform/amiga/AmigaTerrain.h"
#include "platform/amiga/AmigaGameplayGeneration.h"
#include "core/BuilderPatternPack.h"
#include "core/GameplayCollisionOwner.h"
#include "../../core/CollisionOwnerFixture.h"
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

// Exercise room cleanup directly: a final-state digest alone cannot detect
// reordered callbacks when all marked records ultimately disappear.
std::vector<unsigned> cleanupOrder(const downwell::ObjectPack& objects,
    const downwell::SpriteCollisionPack& sprites,Ownership ownership,unsigned scenario,
    const downwell::RoomPack* rooms=nullptr,bool externalCollision=false){
    auto owner=downwell::makeCollisionOwner(objects,sprites,
        downwell::GameplayTerrainInstances::Presentation{},downwell::GameplayTerrainInstances::WallCollision{},
        rooms,ownership,true,true,!externalCollision);
    require(owner->hasNativeCollisionStorage()==(!externalCollision&&
        (ownership==Ownership::native||ownership==Ownership::nativeScenery)),"scene collision backing admission");
    require(owner->hasPreparedCollisionGeometry()==!externalCollision,"geometry follows collision admission");
    require(owner->terrain().hasStorage()==(ownership==Ownership::reference||ownership==Ownership::nativeTerrain),
        "scalar terrain backing belongs only to reference terrain owners");
    if(externalCollision)require(owner->declareExternalCollisionBackend(),"cleanup external collision admission");
    auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
        owner->breakableBinding(),owner->playerBinding());
    require(owner->attach(*world),"cleanup fixture attaches");
    for(int object:{112,82,92,112,92,82,112,92})
        require(world->createGeneratedInstance({object,240,512},objects,[](const auto&){return true;}),"mixed births");
    downwell::InstanceHandle enemies[3];
    for(unsigned i=0;i<3;++i)enemies[i]=world->enemyHandle(world->enemyAt(i).lifetimeId);
    if(scenario==1||scenario==2){
        require(owner->enemyBinding().activation(enemies[0],world->enemyAt(0),false)&&
            owner->reconcileActivation(),"fixture deactivation");
        if(scenario==2)require(owner->enemyBinding().activation(enemies[0],world->enemyAt(0),true)&&
            owner->reconcileActivation(),"fixture reactivation");
    }
    if(scenario==3){
        // Registration append is NOT room reactivation. Cleanup retains the
        // old room position, even though the alarm registration moves later.
        require(owner->enemyBinding().membership(enemies[0],world->enemyAt(0),false)&&
            owner->enemyBinding().membership(enemies[0],world->enemyAt(0),true),"fixture re-registration");
    }
    if(scenario==4){
        const downwell::InstanceHandle old{0,world->breakableSlot(0).lifetimeId};
        auto& used=const_cast<GameplayWorld::Breakable&>(world->breakableSlot(0));
        used.alarm0=17;used.myFx={9,12};used.sound.sndsnd=42;
        used.state.hit=true;used.state.scriptIndex=7;
        require(world->markBreakableDestroyed(old),"old slot marked");
        const int x=0,y=336;world->configureBreakables(&x,&y,1);
        require(world->effectsSucceeded(),"box slots reset before pending cleanup");
        const auto& fresh=world->breakableSlot(0);
        require(!world->findBreakable(old)&&fresh.alarm0==-1&&!fresh.myFx.generation&&
            fresh.sound.sndsnd==-1&&!fresh.state.hit&&fresh.state.scriptIndex==0&&
            fresh.state.breakSound==171&&fresh.state.wallHp.high==0x3ff00000u&&
            fresh.state.wallHp.low==0&&!fresh.destroyRequested,
            "prepared defaults replace all previous-lifetime script state");
    }
    for(unsigned i=world->breakableSlots();i;--i)
        require(world->markBreakableDestroyed({i-1,world->breakableSlot(i-1).lifetimeId}),"reverse box marking");
    for(unsigned i=3;i;--i)require(world->markEnemyDestroyed(enemies[i-1]),"reverse enemy marking");
    std::vector<unsigned> order;
    require(owner->cleanupDestroyed([&](downwell::InstanceHandle shared,const View::Record&){
        View::Kind kind;downwell::InstanceHandle local;
        if(!View::decode(shared,kind,local))return false;
        order.push_back(shared.slot);order.push_back(shared.generation);
        return kind==View::Kind::enemy?world->retireMarkedEnemy(local):
            kind==View::Kind::breakable&&world->retireMarkedBreakable(local);
    }),"ordered cleanup succeeds");
    require(owner->prepareDraw(),"second cleanup is empty");
    auto registrations=[&](){
        order.push_back(0xffffffffu);
        require(owner->visitRegistrationOrder([&](downwell::InstanceHandle h,int object,std::uint32_t flags){
            order.push_back(h.slot);order.push_back(h.generation);order.push_back(unsigned(object));
            order.push_back(flags&0x190013u);return true;
        }),"native field registration order survives cleanup");
    };
    registrations();
    const auto previousField=world->timeFields().identityAt(0);
    require(world->restartFromResults(downwell::DeathMenuSequence::Action::Retry,42,300)!=
        downwell::ResultsDestination::none,"native fields retire during Retry");
    require(owner->valid()&&world->timeFields().size()==0,"Retry clears all fields");
    require(world->createGeneratedInstance({82,256,600},objects,[](const auto&){return false;}),"field recreated after Retry");
    require(world->timeFields().identityAt(0)>previousField,"field identity cursor survives Retry");
    registrations();
    return order;
}

struct Scene {
    AmigaTerrainFragment fragments[19];
    AmigaTerrainIndex index;
    AmigaAreaOneTerrainBuilder builder{fragments,19};
    AmigaTerrainCollisionContext collision;
    AmigaTerrainTiles tiles;
    std::unique_ptr<AmigaGameplayGeneration::TerrainLifetimes> lifetimes;
    AmigaGameplayGeneration generation;
    downwell::CollisionOwnerFixture owner;
    std::unique_ptr<AmigaPortRuntime> world;
    Scene(const downwell::BuilderPatternPack& patterns,const downwell::ObjectPack& objects,
          const downwell::SpriteCollisionPack& sprites,const downwell::RoomPack& rooms,
          const downwell::PackedRoom& room,const downwell::StartupRuntime& entry,Ownership ownership,
          bool nativeBoxes=true,bool nativeFields=true,bool nativeLifetimes=false,bool noQueries=false)
        :collision{nullptr,nullptr,&patterns,nullptr,0,nullptr},
         lifetimes(nativeLifetimes?std::make_unique<AmigaGameplayGeneration::TerrainLifetimes>():nullptr),
         generation(builder,patterns,objects,index,tiles,lifetimes.get()){
        owner=downwell::makeCollisionOwner(objects,sprites,
            downwell::GameplayTerrainInstances::Presentation{},downwell::GameplayTerrainInstances::WallCollision{},
            &rooms,ownership,nativeBoxes,nativeFields,!noQueries);
        require(owner->hasNativeCollisionStorage()==(!noQueries&&
            (ownership==Ownership::native||ownership==Ownership::nativeScenery)),"replay collision backing admission");
        require(owner->valid(),"owner prepares");
        if(noQueries)require(owner->declareExternalCollisionBackend(),"scene declares external collision backend before births");
        // Exercise both the older occupancy-only backend and the shipping
        // native lifetime store; the latter is bound during loadRoom below.
        world=std::make_unique<AmigaPortRuntime>(300,false,1201-176,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),
            (ownership==Ownership::native||ownership==Ownership::nativeScenery)?nullptr:&owner->terrain(),&rooms);
        require(world->effectsSucceeded()&&owner->attach(*world),"owner attaches");
        world->initializeEntryInput(false);
        tiles.clear();
        require(amigaBuildTerrainIndex(collision,index),"index initializes");
        auto prepared=std::make_unique<downwell::PreparedRoomTerrain>();
        require(prepared->prepare(rooms,room),"room terrain resource admission");
        require(generation.loadRoom(*world,rooms,room,
            ownership==Ownership::reference?nullptr:prepared.get()),"room terrain loads");
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
        const auto& random=w.randomState();
        d.addU32(random.seed());d.addU32(unsigned(random.stateIndex()));
        for(unsigned i=0;i<16;++i)d.addU32(random.stateWord(i));
        d.addI32(w.health());d.addI32(w.player().offsetXFixed());d.addI32(w.player().offsetYFixed());
        d.addU32(w.enemyCount());
        for(unsigned i=0;i<w.enemyCount();++i){const auto& e=w.enemyAt(i);
            d.addI32(e.objectId);d.addI32(e.x);d.addI32(e.y);d.addI32(e.health);d.addBool(e.active);
            d.addI32(e.alarm0);d.addI32(e.alarm1);d.addU32(e.imagePhase);d.addU32(e.lifetimeId);}
        for(unsigned i=0;i<w.gemCapacity;++i){const auto& g=w.gemAt(i);if(!g.active)continue;
            d.addI32(g.x);d.addI32(g.y);d.addU32(g.age);d.addU32(g.lifetimeId);}
        d.addU32(w.damageNumbers().size());
        d.addU32(w.timeFields().size());
        for(unsigned i=0;i<w.timeFields().size();++i){
            const auto* f=w.timeFields().at(i);require(f!=nullptr,"field record resolves");
            d.addU32(w.timeFields().identityAt(i));d.addI32(f->x);d.addI32(f->y);
            d.addU64(f->radius.mantissa);d.addI32(f->radius.exponent);
        }
        d.addU32(w.breakableSlots());
        for(unsigned i=0;i<w.breakableSlots();++i){const auto& b=w.breakableSlot(i);
            d.addU32(b.lifetimeId);d.addBool(b.active);d.addBool(b.destroyRequested);d.addI32(b.alarm0);
            d.addI32(b.x);d.addI32(b.y);d.addU32(b.state.wallHp.high);d.addU32(b.state.wallHp.low);
            d.addU32(b.state.breakSound);d.addU32(b.state.haveGem);d.addI32(b.state.scriptIndex);
            d.addBool(b.state.hit);d.addBool(b.state.drilled);d.addBool(b.state.autoTile);d.addBool(b.state.checked);}
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
    {
        auto actual=std::make_unique<AmigaTerrainIndex>();
        auto oracle=std::make_unique<AmigaTerrainIndex>();
        for(auto& cell:actual->cells)cell=255;
        for(unsigned column=0;column<128;++column)
            for(unsigned row=0;row<512;++row)actual->nextLanding.insert(column,row);
        require(actual->extraContacts.append(60,160,32),"dirty reset contact");
        actual->wallContactsComplete=false;actual->mutableWallOccupancy=false;
        *oracle=*actual;
        const AmigaTerrainCollisionContext empty{};
        require(amigaBuildTerrainIndex(empty,*actual)&&amigaReconstructTerrainIndex(empty,*oracle),
            "empty terrain reset and reconstruction succeed");
        require(std::memcmp(actual->cells,oracle->cells,sizeof(actual->cells))==0&&
            actual->extraContacts.size()==oracle->extraContacts.size()&&
            actual->wallContactsComplete==oracle->wallContactsComplete&&
            actual->mutableWallOccupancy==oracle->mutableWallOccupancy,"empty terrain ownership agrees");
        for(unsigned column=0;column<128;++column)for(unsigned row=0;row<512;++row)
            require(actual->nextLanding.next(column,row)==oracle->nextLanding.next(column,row),
                "empty terrain landing queries agree after dirty reset");
        require(amigaAddTerrainObject(*actual,60,160,32)&&amigaAddTerrainObject(*oracle,60,160,32)&&
            amigaAddTerrainObject(*actual,93,176,48)&&amigaAddTerrainObject(*oracle,93,176,48),
            "source Creates publish after reset");
        require(std::memcmp(actual->cells,oracle->cells,sizeof(actual->cells))==0&&
            actual->nextLanding.next(42,0)==oracle->nextLanding.next(42,0),"reset preserves later publication");
    }
    if(argc<5){std::cerr<<"usage: rooms objects patterns sprites\n";return 2;}
    const auto roomBytes=readFile(argv[1]),objectBytes=readFile(argv[2]),
        patternBytes=readFile(argv[3]),spriteBytes=readFile(argv[4]);
    downwell::RoomPack rooms(roomBytes.data(),roomBytes.size());
    downwell::ObjectPack objects(objectBytes.data(),objectBytes.size());
    downwell::BuilderPatternPack patterns(patternBytes.data(),patternBytes.size());
    downwell::SpriteCollisionPack sprites(spriteBytes.data(),spriteBytes.size());
    downwell::PackedRoom room;
    require(rooms.valid()&&objects.valid()&&patterns.valid()&&sprites.valid()&&rooms.findRoom(11,room),"resources");
    {
        auto prepared=std::make_unique<downwell::PreparedRoomTerrain>();
        require(prepared->prepare(rooms,room)&&prepared->matches(rooms,room),"room terrain plan admits resources");
        unsigned selected=0;int extent=0;
        for(unsigned i=0;i<room.instanceCount;++i){
            downwell::PackedRoomInstance instance;
            require(rooms.roomInstance(room,i,instance),"room plan scalar record");
            if(instance.objectId==60)extent=std::max(extent,instance.y+32);
            require(selected<prepared->count()&&std::memcmp(&instance,&prepared->at(selected),sizeof(instance))==0,
                "room plan retains every field and serialized order");++selected;
        }
        require(selected==prepared->count()&&extent==prepared->wallEnd(),"room terrain extent and count");
        downwell::RoomPack twin(roomBytes.data(),roomBytes.size());
        require(!prepared->matches(twin,room),"room plan belongs to the admitted immutable pack");
        auto other=room;++other.roomId;
        require(!prepared->matches(rooms,other),"room plan rejects different room identity");
        other=room;other.firstInstance=0xffffffffu;
        require(!prepared->prepare(rooms,other)&&!prepared->matches(rooms,room),"failed admission invalidates old room plan");
        // Deliberately span the global record bank to exceed the bounded plan;
        // this is an admission fixture, not a room or gameplay-state injection.
        other=room;other.firstInstance=0;other.instanceCount=rooms.instanceCount();
        require(!prepared->prepare(rooms,other)&&prepared->count()==256&&
            !prepared->matches(rooms,room),"oversized admission stays bounded and invalidates old plan");
        {
            auto fallback=std::make_unique<AmigaPortRuntime>(300,true,0,false);
            auto scalar=std::make_unique<AmigaPortRuntime>(300,true,0,false);
            unsigned a=0,b=0;
            require(fallback->loadRoomTerrain(rooms,room,[&](const auto&){++a;return true;},prepared.get())&&
                scalar->loadRoomTerrain(rooms,room,[&](const auto&){++b;return true;}),"rejected plan uses generic loader");
            require(a==b&&a==240&&fallback->checkpoint()==scalar->checkpoint(),"rejected plan fallback state");
        }
        require(prepared->prepare(rooms,room),"room plan can readmit immutable resources");
        for(unsigned stop:{1u,3u,240u,241u}){
            auto scalar=std::make_unique<AmigaPortRuntime>(300,true,0,false);
            auto retained=std::make_unique<AmigaPortRuntime>(300,true,0,false);
            unsigned a=0,b=0;
            const bool first=scalar->loadRoomTerrain(rooms,room,[&](const auto&){return ++a<stop;});
            const bool second=retained->loadRoomTerrain(rooms,room,[&](const auto&){return ++b<stop;},prepared.get());
            require(first==second&&a==b&&scalar->checkpoint()==retained->checkpoint(),
                "prepared room retains callback failure prefix and source state");
            require(scalar->randomDrawCount()==retained->randomDrawCount()&&
                scalar->randomState().stateIndex()==retained->randomState().stateIndex(),"prepared room RNG cursor");
            for(unsigned i=0;i<16;++i)require(scalar->randomState().stateWord(i)==retained->randomState().stateWord(i),
                "prepared room RNG words");
            require(!retained->loadRoomTerrain(rooms,room,[&](const auto&){++b;return true;},prepared.get())&&a==b,
                "prepared partial room cannot retry");
        }
    }
    for(unsigned scenario=0;scenario<5;++scenario){
        const auto expected=cleanupOrder(objects,sprites,Ownership::reference,scenario);
        require(cleanupOrder(objects,sprites,Ownership::nativeScenery,scenario)==expected,
            "native family destruction callback order matches the registered room");
        const auto roomExpected=cleanupOrder(objects,sprites,Ownership::reference,scenario,&rooms);
        require(cleanupOrder(objects,sprites,Ownership::nativeScenery,scenario,&rooms)==roomExpected,
            "native scenery lease preserves room-backed cleanup, reuse and Retry");
        require(cleanupOrder(objects,sprites,Ownership::nativeScenery,scenario,&rooms,true)==roomExpected,
            "external collision backend preserves cleanup, reuse and Retry");
    }
    {
        downwell::StartupRuntime entry(downwell::PlatformProfile::nonDesktop,&rooms,&objects,2,&patterns);
        entry.skipToGameForTesting();
        auto reference=std::make_unique<Scene>(patterns,objects,sprites,rooms,room,entry,Ownership::nativeScenery);
        auto direct=std::make_unique<Scene>(patterns,objects,sprites,rooms,room,entry,Ownership::nativeScenery);
        for(unsigned round=0;round<2;++round){
            const int none=0;
            reference->world->configureBreakables(&none,&none,0);
            direct->world->configureBreakables(&none,&none,0);
            for(unsigned i=0;i<=GameplayWorld::breakableCapacity;++i){
                // Include duplicate and unaligned positions, which disable
                // the grid accelerator without changing first-hit ownership.
                const int x=i%3?0:1,y=int(i%128)*16;
                const downwell::GeneratedBreakableCreate create{16+i%2,171+i%4,i%7?0u:2u};
                downwell::BreakableState state;state.breakSound=create.sound;state.haveGem=create.gems;
                const bool expected=reference->world->appendBreakables(&x,&y,1,&state);
                require(direct->world->appendGeneratedBreakable(x,y,create)==expected,
                    "direct box capacity prefix equals the retained allocator");
                require(direct->digest()==reference->digest(),
                    "direct box state, registration and slot reuse equal command publication");
                require(expected==(i<GameplayWorld::breakableCapacity),"box fixture reaches capacity failure");
            }
        }
    }
    // Admission is fail-closed, including direct consumers which never query
    // the tree. Each violation poisons a fresh owner, not the replay below.
    for(unsigned consumer=0;consumer<6;++consumer){
        auto guarded=downwell::makeCollisionOwner(objects,sprites,{}, {},&rooms,Ownership::nativeScenery,true,true,false);
        require(guarded->declareExternalCollisionBackend(),"external collision admission");
        bool accepted=false;
        if(consumer==0){
            downwell::GameplayCollisionOwner::Owner::Box bounds;
            accepted=guarded->nativeActorBounds({},bounds);
        }else if(consumer==1)accepted=guarded->flushNativeSpatial();
        else if(consumer==2){
            auto pairs=std::make_unique<downwell::RunnerCollisionPairs<downwell::GameplayCollisionView::capacity,16>>();
            accepted=guarded->collectCollisionPairs(*pairs,false);
        }else if(consumer==3)accepted=guarded->queueCollision({});
        else if(consumer==4){downwell::RunnerImageValue x,y;accepted=guarded->nativePublishedPosition({0,1},x,y);}
        else {downwell::GameplayCollisionOwner::Owner::Sprite sprite;std::uint32_t flags;
            accepted=guarded->nativePublishedSprite({View::enemyBase,1},sprite,flags);}
        require(!accepted&&!guarded->valid(),"unexpected collision consumer fails terminally");
    }
    // End Step must consume the bullet's published binary32 state, including
    // writes during Collision, not stale birth coordinates or integer artwork.
    for(std::uint32_t yBits:{0x80000000u,0x00000000u,0x44000001u,0x461c4000u,0xc61c4000u}){
        std::uint64_t referenceDigest=0;bool referenceDestroyed=false;
        for(bool external:{false,true}){
            auto owner=downwell::makeCollisionOwner(objects,sprites,{}, {},nullptr,
                Ownership::nativeScenery,true,true,!external);
            if(external)require(owner->declareExternalCollisionBackend(),"position owner scene admission");
            auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
                owner->breakableBinding(),owner->playerBinding(),nullptr);
            require(owner->attach(*world),"position owner world attaches");
            downwell::InstanceHandle shot;
            require(world->withCollisionPass([&]{
                require(world->bulletCount()==1,"position fixture fires once");
                const auto& bullet=world->bulletAt(0);shot={bullet.identitySlot,bullet.lifetimeId};
                return world->publishBulletCollisionPosition(shot,{0x43700001u},{yBits});
            },[&]{world->step(4);return world->effectsSucceeded();}),"collision-position publication");
            const auto* bullet=world->resolveBulletLifetime(shot);
            require(bullet&&bullet->sourceMotion.x.bits==0x43700001u&&bullet->sourceMotion.y.bits==yBits,
                "record retains exact published bullet coordinates");
            if(!external){referenceDigest=world->checkpoint();referenceDestroyed=bullet->destroyRequested;}
            else require(world->checkpoint()==referenceDigest&&bullet->destroyRequested==referenceDestroyed,
                "record-owned position preserves End Step retirement and state");
        }
    }
    {
        auto unneeded=downwell::makeCollisionOwner(objects,sprites,{}, {},&rooms,Ownership::nativeScenery);
        require(!unneeded->declareExternalCollisionBackend()&&!unneeded->valid(),
            "external backend rejects unnecessary collision backing");
        auto missing=downwell::makeCollisionOwner(objects,sprites,{}, {},&rooms,Ownership::nativeScenery,true,true,false);
        require(!missing->hasPreparedCollisionGeometry(),"no collision backing means no geometry preparation");
        require(!missing->flushNativeSpatial()&&!missing->valid(),
            "full collision service fails without explicit backing");
        std::cout<<"native collision backing: "<<sizeof(downwell::GameplayCollisionOwner::NativeCollisionStorage)
            <<" bytes; external scenes retain only a pointer\n";
        std::cout<<"optional collision geometry: "<<sizeof(downwell::GameplayCollisionOwner::CollisionGeometryStorage)<<" bytes\n";
    }
    unsigned tapes=0,frames=0,maxTerrain=0;
    for(unsigned phase=0;phase<10;++phase){
        auto host=std::make_unique<downwell::StartupRuntime>(downwell::PlatformProfile::nonDesktop,
            &rooms,&objects,2,&patterns);
        host->skipToGameForTesting();
        auto reference=std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::reference);
        std::unique_ptr<Scene> others[]={
            std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::nativeTerrain),
            std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::native),
            std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::nativeScenery),
            std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::nativeScenery,false,false),
            std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::nativeScenery,true,false),
            std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::nativeScenery,false,true),
            std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::nativeScenery,true,true,true),
            std::make_unique<Scene>(patterns,objects,sprites,rooms,room,*host,Ownership::nativeScenery,true,true,true,true)};
        for(frame=0;frame<downwell::gameplayContactProfileFrames;++frame){
            const auto& player=reference->world->player();
            const auto input=downwell::packReplayInput(downwell::gameplayContactProfileInput(
                reference->world->tick(),240+player.offsetX(),176+player.offsetY(),phase));
            reference->world->step(input);
            require(reference->world->effectsSucceeded()&&reference->world->generationSucceeded()&&
                reference->owner->valid(),"reference Step succeeds for the complete tape");
            // Marked lifetimes still exist before Draw cleanup. Comparing
            // only afterwards misses early retirement and stale flag words.
            const auto beforeCleanup=reference->digest();
            for(auto& other:others){
                other->world->step(input);
                require(other->world->effectsSucceeded()&&other->world->generationSucceeded()&&
                    other->owner->valid(),"native Step succeeds for the complete tape");
                require(other->digest()==beforeCleanup,"pre-cleanup state, flags and order agree");
                if(other->world->terrainInstances()){
                    const auto& expectedTerrain=reference->owner->terrain();
                    const auto& actual=other->owner->terrain();
                    require(actual.size()==expectedTerrain.size(),"terrain identities survive until Draw cleanup");
                    for(unsigned i=0;i<expectedTerrain.size();++i){
                        const auto h=expectedTerrain.handle(i);const auto* a=expectedTerrain.find(h);
                        if(a->objectId!=93)continue;
                        const auto* b=actual.find(h);
                        require(b&&b->objectId==93&&b->x==a->x&&b->y==a->y&&
                            b->active==a->active&&b->destroyRequested==a->destroyRequested,
                            "thin-platform identities and End Step marks agree before Draw");
                    }
                }
            }
            require(reference->owner->prepareDraw(),"reference Draw cleanup succeeds");
            const auto expected=reference->digest();
            for(auto& other:others){
                require(other->owner->prepareDraw(),"native Draw cleanup succeeds");
                require(other->digest()==expected,"simulation state and actor order agree");
                requireSameBackendTerrain(*reference,*other);
                if(other->world->terrainInstances()){
                    if(other->owner->terrain().size()!=reference->owner->terrain().size()){
                        std::cerr<<"phase="<<phase<<" owner="<<(&other-&others[0])<<" terrain="
                            <<other->owner->terrain().size()<<" reference="<<reference->owner->terrain().size()<<"\n";
                        const auto& actual=other->owner->terrain();const auto& expectedTerrain=reference->owner->terrain();
                        for(unsigned i=0;i<actual.size();++i){const auto h=actual.handle(i);const auto* a=actual.find(h);
                            const auto* b=expectedTerrain.find(h);
                            if(!b||a->objectId!=b->objectId||a->x!=b->x||a->y!=b->y||a->destroyRequested!=b->destroyRequested)
                                std::cerr<<"extra/different handle="<<h.slot<<":"<<h.generation<<" object="<<a->objectId
                                    <<" xy="<<a->x<<","<<a->y<<" active="<<a->active<<" destroyed="<<a->destroyRequested<<"\n";
                        }
                    }
                    require(other->owner->terrain().size()==reference->owner->terrain().size(),"terrain identities agree");
                }
                else if(frame%30==0)requireWallsMatchGrid(*reference,*other);
            }
            if(reference->owner->terrain().size()>maxTerrain)maxTerrain=reference->owner->terrain().size();
            ++frames;
        }
        ++tapes;
    }
    std::cout<<"native ownership: "<<tapes<<" tapes, "<<frames<<" frames, up to "<<maxTerrain
             <<" terrain records agree\n";
    return 0;
}

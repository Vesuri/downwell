#include "core/GameplayCollisionOwner.h"
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>
#include <cassert>
#include <cstdio>
using namespace downwell;
static std::vector<std::uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
struct TerrainAudit {
    GameplayTerrainInstances* store=nullptr;
    unsigned births=0,retires=0,failBirth=0,failRetire=0;
    bool birth(InstanceHandle h,const GameplayTerrainInstances::Record& r){
        assert(store->find(h)==&r);return ++births!=failBirth;
    }
    bool retire(InstanceHandle h,const GameplayTerrainInstances::Record& r){
        assert(store->find(h)==&r);return ++retires!=failRetire;
    }
    bool activation(InstanceHandle,const GameplayTerrainInstances::Record&,bool){return true;}
    bool membership(InstanceHandle,const GameplayTerrainInstances::Record&,bool){return true;}
};
int main(){
    auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    auto rb=read("generated/downwell-rooms.dwrm");RoomPack rooms(rb.data(),rb.size());
    PackedRoom room;assert(rooms.valid()&&rooms.findRoom(11,room));
    {
        auto roomOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto roomWorld=std::make_unique<GameplayWorld>(300,true,0,false,roomOwner->enemyBinding(),
            roomOwner->breakableBinding(),roomOwner->playerBinding(),&roomOwner->terrain());
        assert(roomOwner->attach(*roomWorld));
        unsigned calls=0,codes=0;
        assert(roomWorld->loadRoomTerrain(rooms,room,[&](const PackedRoomInstance& r){++calls;codes+=r.creationCodeId!=-1;return true;}));
        assert(calls==240&&codes==3&&roomOwner->terrain().size()==240);
        unsigned ordinal=0;
        for(unsigned i=0;i<room.instanceCount;++i){
            PackedRoomInstance r;assert(rooms.roomInstance(room,i,r));
            if(r.objectId!=60&&r.objectId!=93)continue;
            const auto local=roomOwner->terrain().handle(ordinal++);
            const auto* stored=roomOwner->terrain().find(local);InstanceHandle shared;
            assert(stored&&stored->sourceInstanceId==r.instanceId&&stored->creationCodeId==r.creationCodeId&&
                stored->x==r.x&&stored->y==r.y&&stored->objectId==r.objectId&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared));
            const auto* state=roomOwner->state(shared);std::int32_t x,y;
            assert(state&&state->x.promoted().roundPixel(x)&&state->y.promoted().roundPixel(y)&&x==r.x&&y==r.y);
            assert(stored->spriteId==51&&stored->imageSpeed==0&&roomOwner->properties(shared)->sprite.spriteId==51);
            assert(stored->wall.pillar==(r.creationCodeId==677?1:r.creationCodeId==676||r.creationCodeId==678?-1:0));
            assert(!stored->wall.checked&&!stored->wall.surrounded&&!stored->wall.deactivate&&!stored->wall.lights);
        }
        const auto old=roomOwner->terrain().handle(0);
        assert(roomWorld->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(roomOwner->terrain().size()==0&&!roomOwner->terrain().find(old));
        assert(roomWorld->loadRoomTerrain(rooms,room,[](const auto&){return true;}));
        assert(roomOwner->terrain().size()==240&&roomOwner->terrain().handle(0).generation>old.generation);
        assert(!roomWorld->loadRoomTerrain(rooms,room,[&](const auto&){++calls;return true;})&&calls==240);
        assert(!roomWorld->effectsSucceeded()&&roomOwner->terrain().size()==240);
    }
    {
        auto terrain=std::make_unique<GameplayTerrainInstances>();
        auto partial=std::make_unique<GameplayWorld>(300,true,0,false,
            GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
            GameplayWorld::PlayerLifecycleBinding{},terrain.get());
        unsigned calls=0;
        assert(!partial->loadRoomTerrain(rooms,room,[&](const auto&){return ++calls<3;}));
        assert(calls==3&&terrain->size()==2&&!partial->effectsSucceeded());
        assert(!partial->loadRoomTerrain(rooms,room,[&](const auto&){++calls;return true;})&&calls==3);
    }
    {
        auto terrain=std::make_unique<GameplayTerrainInstances>();PackedRoomInstance r;
        assert(rooms.roomInstance(room,0,r));r.scaleX=2;
        assert(!terrain->appendRoom(r)&&!terrain->valid()&&terrain->size()==0);
    }
    auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
    {
        auto terrain=std::make_unique<GameplayTerrainInstances>();GameMakerRandom random;
        assert(terrain->append(60,128,0,true,123,999));
        const auto h=terrain->handle(0);
        assert(terrain->createProperties(h,51,false,random));
        assert(!terrain->roomCreation(h)&&!terrain->valid());
        assert(terrain->size()==1&&terrain->find(h)->spriteId==51&&terrain->find(h)->wall.pillar==0);
    }
    assert(owner->valid());
    auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
    auto unrelated=std::make_unique<GameplayWorld>(300,true,0,false);
    assert(!owner->attach(*unrelated)&&owner->valid());
    assert(owner->attach(*world)&&world->effectsSucceeded());
    world->configureSolidResolver([](void*,int,int){return false;},nullptr);
    int x=80,y=40;world->configureBreakables(&x,&y,1);
    assert(world->configureEnemy(0,111,163,100,-2000,0,0,5,6));
    assert(world->configureEnemy(1,116,153,40,500,0,0,15,12,0,0,-1,-1,180,320,false));
    const auto local=world->enemyHandle(world->enemyAt(1).lifetimeId);
    InstanceHandle key,player;
    assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::enemy,local,key)&&
        GameplayCollisionView::encode(GameplayCollisionView::Kind::player,world->playerHandle(),player));
    auto state=owner->state(key);std::int32_t px,py;
    assert(state&&(state->flags&2)&&state->x.promoted().roundPixel(px)&&px==280&&state->y.promoted().roundPixel(py)&&py==676);
    assert(owner->properties(key)->sprite.spriteId==153&&owner->properties(key)->sprite.maskId==-1);
    unsigned count=0;
    assert(owner->population(35,count)&&count==1&&owner->population(92,count)&&count==1);
    unsigned terrainCalls=0;
    auto terrain=[&](const auto&){++terrainCalls;return true;};
    assert(world->createGeneratedInstance({60,240,512},objects,terrain));
    assert(world->createGeneratedInstance({60,240,512},objects,terrain)); // Coincident, distinct births.
    DeferredCreateInstance platform(93,256,528);platform.active=false;
    assert(world->createGeneratedInstance(platform,objects,terrain));
    assert(terrainCalls==3&&owner->terrain().size()==3);
    InstanceHandle wall,otherWall,ledge;
    const auto wallLocal=owner->terrain().handle(0);
    assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,wallLocal,wall)&&
        GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,owner->terrain().handle(1),otherWall)&&
        GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,owner->terrain().handle(2),ledge));
    assert(wall.slot!=otherWall.slot&&owner->state(wall)&&owner->state(otherWall));
    assert(owner->properties(ledge)->sprite.spriteId==51&&
        owner->properties(ledge)->sprite.imageIndex.bits==0x41b80000u&&
        owner->terrain().find(owner->terrain().handle(2))->imageIndex==23&&
        owner->terrain().find(owner->terrain().handle(2))->imageSpeed==0);
    assert(owner->state(wall)->x.promoted().roundPixel(px)&&px==240&&
        owner->state(wall)->y.promoted().roundPixel(py)&&py==512&&(owner->state(ledge)->flags&2));
    assert(owner->population(60,count)&&count==2);
    GameplayCollisionView::Record terrainRecord;
    assert(GameplayCollisionView(*world).resolve(wall,terrainRecord)&&terrainRecord.terrain==owner->terrain().find(wallLocal));
    // A backend rejection must not invent an identity for a cell it rejected.
    assert(!world->createGeneratedInstance({60,1,2},objects,[](const auto&){return false;})&&owner->terrain().size()==3);
    world->step(0);
    assert(world->effectsSucceeded()&&owner->valid()&&world->enemyCount()==1&&owner->state(key));
    const auto* enemy=world->findEnemy(local);assert(enemy);
    state=owner->state(key);
    assert(state->x.promoted().roundPixel(px)&&px==enemy->x+240&&state->y.promoted().roundPixel(py)&&py==enemy->y+176);
    assert(owner->properties(key)->sprite.spriteId==enemy->spriteId);
    assert(owner->population(111,count)&&count==0);
    assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
    assert(owner->valid()&&!owner->state(key)&&!owner->state(player)&&owner->population(35,count)&&count==1);
    assert(owner->population(92,count)&&count==0&&owner->population(116,count)&&count==0);
    assert(!owner->state(wall)&&!owner->state(otherWall)&&!owner->state(ledge)&&
        !owner->terrain().find(wallLocal)&&owner->terrain().size()==0&&owner->population(60,count)&&count==0);
    assert(world->createGeneratedInstance({60,240,512},objects,terrain));
    assert(owner->terrain().handle(0).generation>wallLocal.generation&&!GameplayCollisionView(*world).resolve(wall,terrainRecord));
    // A copied record must not mutate the installed lifetime's properties.
    auto copy=*world->findPlayer(world->playerHandle());
    assert(!owner->playerBinding().integerPosition(world->playerHandle(),copy,1,2)&&!owner->valid());
    world->step(0);assert(!world->effectsSucceeded());
    {
        auto masks=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto batWorld=std::make_unique<GameplayWorld>(300,false,1000,false,
            masks->enemyBinding(),masks->breakableBinding(),masks->playerBinding());
        assert(masks->attach(*batWorld));
        batWorld->configureSolidResolver([](void*,int,int){return false;},nullptr);
        assert(batWorld->configureEnemy(0,116,153,0,12,0,0,15,12,0,0,-1,-1,270,256,true,false,true));
        const auto bat=batWorld->enemyHandle(batWorld->enemyAt(0).lifetimeId);
        InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::enemy,bat,shared));
        assert(masks->properties(shared)->sprite.maskId==-1);
        batWorld->step(4); // Real surviving bullet hit, not a forced sprite write.
        assert(batWorld->effectsSucceeded()&&batWorld->enemyAt(0).health==6&&batWorld->enemyAt(0).batHit.pending);
        for(unsigned frame=0;frame<3;++frame){
            batWorld->step(0);
            const auto* p=masks->properties(shared);
            assert(batWorld->effectsSucceeded()&&p&&p->sprite.spriteId==154&&p->sprite.maskId==154&&
                batWorld->enemyAt(0).maskId==154);
        }
        batWorld->step(0);
        assert(batWorld->effectsSucceeded()&&masks->properties(shared)->sprite.spriteId==153&&
            masks->properties(shared)->sprite.maskId==153&&batWorld->enemyAt(0).maskId==153);
        auto copied=batWorld->enemyAt(0);
        assert(!masks->enemyBinding().mask(bat,copied,999)&&!masks->valid()&&batWorld->enemyAt(0).maskId==153);
        batWorld->step(0);assert(!batWorld->effectsSucceeded());
    }
    {
        TerrainAudit audit;
        auto store=std::make_unique<GameplayTerrainInstances>(GameplayTerrainInstances::Binding::bind(audit));
        audit.store=store.get();
        assert(store->append(60,1,2,true)&&store->append(93,3,4,true));
        const auto first=store->handle(0),second=store->handle(1);
        audit.failRetire=2;
        assert(!store->clear()&&!store->valid()&&audit.retires==2);
        assert(!store->find(first)&&store->find(second)); // Successful prefix is not rolled back.
        assert(!store->clear()&&audit.retires==2&&!store->append(60,5,6,true));
    }
    {
        TerrainAudit audit;
        auto store=std::make_unique<GameplayTerrainInstances>(GameplayTerrainInstances::Binding::bind(audit));
        audit.store=store.get();audit.failBirth=1;
        auto failure=std::make_unique<GameplayWorld>(300,true,0,false,
            GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
            GameplayWorld::PlayerLifecycleBinding{},store.get());
        assert(!failure->createGeneratedInstance({60,1,2},objects,[](const auto&){return true;}));
        assert(!failure->effectsSucceeded()&&!store->valid()&&store->find(store->handle(0))&&audit.births==1);
        failure->step(0);assert(!failure->effectsSucceeded());
    }
    {
        auto terrain=std::make_unique<GameplayTerrainInstances>();
        auto full=std::make_unique<GameplayWorld>(300,true,0,false,
            GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
            GameplayWorld::PlayerLifecycleBinding{},terrain.get());
        unsigned calls=0;
        const auto backend=[&](const auto&){++calls;return true;};
        for(unsigned i=0;i<GameplayTerrainInstances::capacity;++i)
            assert(full->createGeneratedInstance({60,240,512},objects,backend));
        assert(!full->createGeneratedInstance({93,240,528},objects,backend)&&!full->effectsSucceeded());
        assert(terrain->size()==GameplayTerrainInstances::capacity&&calls==GameplayTerrainInstances::capacity+1);
        assert(!full->createGeneratedInstance({60,0,0},objects,backend)&&calls==GameplayTerrainInstances::capacity+1);
        full->step(0);assert(!full->effectsSucceeded());
    }
    std::puts("Native collision owner: actor/terrain births, dormant flags, movement, compaction, retirement, Retry and terminal failure passed");
}

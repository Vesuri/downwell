#include "core/GameplayCollisionOwner.h"
#include "core/WallPresentation.h"
#include "platform/amiga/AmigaWallDamage.h"
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
struct TerrainDestructionAudit:TerrainAudit {
    unsigned destroys=0,activations=0;bool rejectDestroy=false,rejectActivation=false;
    bool move(InstanceHandle,const GameplayTerrainInstances::Record&,RunnerImageValue,RunnerImageValue){return true;}
    bool sprite(InstanceHandle,const GameplayTerrainInstances::Record&,int){return true;}
    bool imageIndex(InstanceHandle,const GameplayTerrainInstances::Record&,RunnerImageValue){return true;}
    bool markDestroyed(InstanceHandle h,const GameplayTerrainInstances::Record& r){
        assert(store->find(h)==&r&&!r.destroyRequested);++destroys;return !rejectDestroy;
    }
    bool activation(InstanceHandle h,const GameplayTerrainInstances::Record& r,bool){
        assert(store->find(h)==&r&&r.destroyRequested&&r.active);++activations;return !rejectActivation;
    }
};
int main(){
    {
        TerrainDestructionAudit lifecycle;
        struct RejectDamage {unsigned calls=0;bool changed(int x,int y,int before,int after){assert(x==32&&y==48&&before==51&&after==-1);++calls;return false;}} damage;
        auto store=std::make_unique<GameplayTerrainInstances>(GameplayTerrainInstances::Binding::bindDestruction(lifecycle),
            GameplayTerrainInstances::Presentation::bind(damage));lifecycle.store=store.get();
        assert(store->append(60,32,48,true));const auto h=store->handle(0);
        assert(!store->requestDestroy(h)&&store->find(h)->destroyRequested&&!store->valid()&&damage.calls==1&&lifecycle.destroys==1);
        assert(!store->activation(h,false)&&!store->retireMarked(h)&&lifecycle.activations==0&&lifecycle.retires==0);
    }
    for(unsigned failure=0;failure<3;++failure){
        TerrainDestructionAudit audit;audit.rejectDestroy=failure==1;audit.rejectActivation=failure==2;
        auto terrain=std::make_unique<GameplayTerrainInstances>(GameplayTerrainInstances::Binding::bindDestruction(audit));
        audit.store=terrain.get();assert(terrain->append(60,32,48,true));
        const auto h=terrain->handle(0);const auto* record=terrain->find(h);
        assert(terrain->requestDestroy(h)==(failure!=1)&&audit.destroys==1);
        if(failure!=1){
            assert(terrain->requestDestroy(h)&&audit.destroys==1);
            assert(terrain->activation(h,false)==(failure==0)&&audit.activations==1);
        }
        assert(terrain->find(h)==record&&terrain->size()==1&&record->destroyRequested==(failure!=1));
        assert(record->active==(failure!=0)&&terrain->valid()==(failure==0));
        if(failure){
            assert(!terrain->requestDestroy(h)&&!terrain->activation(h,false)&&audit.destroys==1);
        }else{
            assert(terrain->clear()&&audit.retires==1&&!terrain->find(h));
        }
    }
    {
        auto terrain=std::make_unique<GameplayTerrainInstances>();assert(terrain->append(60,0,0,true));
        const auto h=terrain->handle(0);
        assert(!terrain->requestDestroy(h)&&!terrain->find(h)->destroyRequested&&!terrain->valid());
    }
    {
        TerrainDestructionAudit audit;
        auto store=std::make_unique<GameplayTerrainInstances>(GameplayTerrainInstances::Binding::bindDestruction(audit));
        audit.store=store.get();
        for(unsigned i=0;i<32;++i)assert(store->append(60,int(i),0,true));
        for(unsigned i=0;i<4096;++i){
            const unsigned index=(i*13u)%32u;const auto old=store->handle(index);
            InstanceHandle survivors[31];const GameplayTerrainInstances::Record* pointers[31];unsigned n=0;
            for(unsigned j=0;j<32;++j)if(j!=index){survivors[n]=store->handle(j);pointers[n]=store->find(survivors[n]);++n;}
            assert(store->requestDestroy(old)&&store->retireMarked(old)&&!store->find(old)&&store->size()==31);
            assert(store->append(60,int(i),0,true));const auto fresh=store->handle(31);
            assert(fresh.slot==old.slot&&fresh.generation>old.generation&&!store->find(old));
            for(unsigned j=0;j<31;++j)assert(store->handle(j).slot==survivors[j].slot&&store->find(survivors[j])==pointers[j]);
        }
        const auto rejected=store->handle(7);assert(store->requestDestroy(rejected));audit.failRetire=audit.retires+1;
        assert(!store->retireMarked(rejected)&&!store->valid()&&store->size()==32&&store->find(rejected));
        assert(!store->retireMarked(rejected)&&audit.retires==audit.failRetire);
    }
    auto ob=read("generated/downwell-objects.dwob"),sb=read("generated/downwell-sprites.dwsc");
    ObjectPack objects(ob.data(),ob.size());SpriteCollisionPack sprites(sb.data(),sb.size());
    const auto mb=read("generated/downwell-sprite-masks.dwsm");SpriteMaskPack masks(mb.data(),mb.size());
    assert(masks.valid());
    for(bool rejectTile:{false,true}){
        struct Damage:AmigaWallDamage<64> {std::vector<std::pair<int,int>> cells;
            bool changed(int x,int y,int before,int after){assert(before==51&&(after==51||after==-1));cells.push_back({x,y});return AmigaWallDamage<64>::changed(x,y,before,after);}
        } damage;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation::bind(damage));
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*scene));
        for(int y=0;y<3;++y)for(int x=0;x<3;++x)
            assert(scene->createGeneratedInstance({60,240+x*16,512+y*16},objects,[](const auto&){return true;}));
        struct Effects {
            GameplayWorld& world;bool reject;unsigned tiles=0;
            bool tile(MotionBinary64 x,MotionBinary64 y){++tiles;return !reject&&world.addWallBackground(x,y);}
            bool outOfView(InstanceHandle){assert(false);return false;}
            bool create(int,MotionBinary64,MotionBinary64){assert(false);return false;}
        } effects{*scene,rejectTile};
        GameMakerRandom random(123);const auto draws=random.drawCount();
        for(unsigned i=0;i<9;++i)assert(owner->wallEndStepEvent(owner->terrain().handle(i),1,random,masks,effects));
        assert(damage.cells.size()==9&&damage.size()==1);damage.cells.clear();damage.clear();
        const auto middle=owner->terrain().handle(4);const auto* record=owner->terrain().find(middle);
        assert(record->wall.surrounded&&record->wall.checked==1&&record->wall.lights==0&&record->imageIndex==15&&
            !record->destroyRequested&&effects.tiles==0&&scene->wallBackgroundTiles().size()==0);
        unsigned drawn=0;
        assert(drawWallPresentation(*record,[]{assert(false);return 0u;},[&](int sprite,unsigned frame){
            assert(sprite==record->spriteId&&frame==15);++drawn;
        })&&drawn==1);
        assert(owner->wallEndStepEvent(middle,1,random,masks,effects)==!rejectTile&&effects.tiles==1);
        assert(damage.cells.size()==(rejectTile?0u:1u));
        if(!rejectTile){
            assert(damage.cells[0].first==256&&damage.cells[0].second==528&&damage.size()==1);
            assert(damage.visit([](int first,int end){return first==520&&end==536;}));
        }
        assert(random.drawCount()==draws&&record->destroyRequested==!rejectTile&&owner->terrain().find(middle)==record);
        if(!rejectTile)assert(drawWallPresentation(*record,[]{assert(false);return 0u;},
            [](int,unsigned){assert(false);}));
        if(rejectTile){
            assert(!owner->valid()&&scene->wallBackgroundTiles().size()==0);
            assert(!owner->wallEndStepEvent(middle,1,random,masks,effects)&&effects.tiles==1);
        }else{
            assert(scene->wallBackgroundTiles().size()==1);
            std::int32_t x,y;const auto* tile=scene->wallBackgroundTiles().at(0);
            assert(tile->x.roundPixel(x)&&tile->y.roundPixel(y)&&x==248&&y==520);
            assert(owner->wallEndStepEvent(middle,1,random,masks,effects)&&effects.tiles==1);
            assert(owner->terrain().activation(middle,false)&&damage.cells.size()==1);
            assert(owner->cleanupDestroyed([&](InstanceHandle shared,const auto&){
                GameplayCollisionView::Kind kind;InstanceHandle local;
                return GameplayCollisionView::decode(shared,kind,local)&&kind==GameplayCollisionView::Kind::terrain&&
                    owner->terrain().retireMarked(local);
            }));
            assert(!owner->terrain().find(middle)&&owner->terrain().size()==8&&scene->wallBackgroundTiles().size()==1);
            assert(damage.cells.size()==1); // Hiding, deactivating, cleanup do not repaint the same wall three times.
        }
    }
    {
        auto queries=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,queries->enemyBinding(),
            queries->breakableBinding(),queries->playerBinding(),&queries->terrain());
        assert(queries->attach(*scene));
        for(auto p:{DeferredCreateInstance{60,240,512},DeferredCreateInstance{60,256,512},DeferredCreateInstance{60,240,528}})
            assert(scene->createGeneratedInstance(p,objects,[](const auto&){return true;}));
        const auto self=queries->terrain().handle(0),right=queries->terrain().handle(1);
        InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,self,shared));
        const auto before=*queries->state(shared);
        for(int y=492;y<=548;++y)for(int x=220;x<=276;++x){
            bool expected=false;
            for(unsigned i=0;i<3;++i){
                const auto local=queries->terrain().handle(i);const auto* r=queries->terrain().find(local);
                InstanceHandle key;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,key));
                const auto& p=queries->properties(key)->sprite;PackedSpriteCollision resource;
                assert(sprites.collisionSprite(p.spriteId,p.maskId,resource));
                const int px=x-r->x+resource.originX,py=y-r->y+resource.originY;
                if(px<resource.marginLeft||px>resource.marginRight||py<resource.marginTop||py>resource.marginBottom)continue;
                if(!resource.collisionMode||!resource.collisionMaskCount){expected=true;break;}
                SpriteMaskPack::Mask mask;assert(masks.mask(unsigned(p.maskId<0?p.spriteId:p.maskId),0,mask));
                if(mask.contains(px,py)){expected=true;break;}
            }
            bool hit=!expected;
            const bool ok=queries->terrainPositionMeeting(60,MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y),masks,hit);
            if(!ok||hit!=expected)std::fprintf(stderr,"native point %d,%d: ok=%d hit=%d expected=%d\n",x,y,ok,hit,expected);
            assert(ok&&hit==expected);
        }
        for(auto p:{DeferredCreateInstance{0,256,512},DeferredCreateInstance{0,240,528},DeferredCreateInstance{1,240,512},DeferredCreateInstance{1,224,512}}){
            bool hit=false;
            assert(queries->terrainPlaceMeeting(self,60,MotionBinary64::fromInteger(p.x),MotionBinary64::fromInteger(p.y),masks,hit)&&hit==(p.objectId==0));
        }
        const auto after=*queries->state(shared);
        assert(before.x.bits==after.x.bits&&before.y.bits==after.y.bits&&scene->effectsSucceeded());
        bool hit=true;assert(queries->terrain().activation(right,false));
        assert(queries->terrainPositionMeeting(60,MotionBinary64::fromInteger(256),MotionBinary64::fromInteger(512),masks,hit)&&!hit);
        assert(queries->reconcileActivation()&&queries->terrain().activation(right,true)&&queries->reconcileActivation());
        assert(queries->terrainPositionMeeting(60,MotionBinary64::fromInteger(256),MotionBinary64::fromInteger(512),masks,hit)&&hit);
        assert(queries->terrain().requestDestroy(right));
        assert(queries->terrainPositionMeeting(60,MotionBinary64::fromInteger(256),MotionBinary64::fromInteger(512),masks,hit)&&!hit);
        assert(scene->createGeneratedInstance({99,400,800},objects,[](const auto&){return true;}));
        const auto marker=queries->terrain().handle(3);
        assert(queries->terrain().find(marker)->spriteId==46&&queries->terrain().find(marker)->imageSpeed==1);
        assert(queries->terrainPositionMeeting(60,MotionBinary64::fromInteger(400),MotionBinary64::fromInteger(800),masks,hit)&&hit);
        hit=true;assert(!queries->terrainPositionMeeting(82,{},{},masks,hit)&&hit&&!queries->valid());
    }
    {
        auto phaseOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto phaseWorld=std::make_unique<GameplayWorld>(300,true,0,false,phaseOwner->enemyBinding(),
            phaseOwner->breakableBinding(),phaseOwner->playerBinding(),&phaseOwner->terrain());
        assert(phaseOwner->attach(*phaseWorld));
        assert(phaseWorld->createGeneratedInstance({60,240,512},objects,[](const auto&){return true;}));
        auto local=phaseOwner->terrain().handle(0);InstanceHandle shared;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared));
        assert(phaseOwner->terrain().activation(local,false));
        assert(!(phaseOwner->state(shared)->flags&0x80000u)); // Deferred until phase boundary.
        phaseWorld->step(0);assert(phaseWorld->effectsSucceeded());
        assert(phaseOwner->state(shared)->flags&0x80000u);
        assert(phaseOwner->terrain().activation(local,true));
        assert(phaseOwner->state(shared)->flags&0x80000u);
        phaseWorld->step(0);assert(phaseWorld->effectsSucceeded());
        assert(!(phaseOwner->state(shared)->flags&0x80000u));
        phaseWorld->configureGenerationStep([](void* p,GameplayWorld&,GameMakerRandom&,int){
            auto& terrain=static_cast<GameplayCollisionOwner*>(p)->terrain();
            return terrain.activation(terrain.handle(0),false);
        },phaseOwner.get());
        phaseWorld->step(0);assert(phaseWorld->effectsSucceeded());
        assert(phaseOwner->state(shared)->flags&0x80000u); // Step request is not delayed to next frame.
        assert(phaseWorld->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(phaseWorld->createGeneratedInstance({60,240,512},objects,[](const auto&){return true;}));
        local=phaseOwner->terrain().handle(0);
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared));
        assert(phaseOwner->terrain().activation(local,false));
        phaseWorld->step(0);assert(phaseWorld->effectsSucceeded());
        assert(phaseOwner->state(shared)->flags&0x80000u); // Callback survives Retry.
    }
    {
        auto phaseWorld=std::make_unique<GameplayWorld>(300);
        struct Audit{unsigned boundaries=0,generations=0;} audit;
        phaseWorld->configurePhaseBoundary([](void* p,GameplayWorld::PhaseBoundary phase){
            assert(phase==GameplayWorld::PhaseBoundary::beginStep);
            ++static_cast<Audit*>(p)->boundaries;return false;
        },&audit);
        phaseWorld->configureGenerationStep([](void* p,GameplayWorld&,GameMakerRandom&,int){
            ++static_cast<Audit*>(p)->generations;return true;
        },&audit);
        phaseWorld->step(0);
        assert(!phaseWorld->effectsSucceeded()&&audit.boundaries==1&&!audit.generations);
        phaseWorld->step(0);assert(audit.boundaries==1&&!audit.generations);
    }
    for(unsigned reject=0;reject<=5;++reject){
        auto phaseWorld=std::make_unique<GameplayWorld>(300);
        struct Audit{unsigned reject,calls=0,generations=0;} audit{reject};
        phaseWorld->configurePhaseBoundary([](void* p,GameplayWorld::PhaseBoundary phase){
            auto& a=*static_cast<Audit*>(p);
            assert(unsigned(phase)==a.calls);
            assert(a.generations==(a.calls<2?0u:1u));
            return a.calls++!=a.reject;
        },&audit);
        phaseWorld->configureGenerationStep([](void* p,GameplayWorld&,GameMakerRandom&,int){
            auto& a=*static_cast<Audit*>(p);assert(a.calls==2);++a.generations;return true;
        },&audit);
        phaseWorld->step(0);
        assert(phaseWorld->effectsSucceeded()==(reject==5));
        assert(audit.calls==(reject<5?reject+1:5));
        if(reject<5){phaseWorld->step(0);assert(audit.calls==reject+1);}
    }
    for(bool reject:{false,true}){
        auto ordered=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,ordered->enemyBinding(),
            ordered->breakableBinding(),ordered->playerBinding(),&ordered->terrain());
        assert(ordered->attach(*scene));
        InstanceHandle h[3];
        for(unsigned i=0;i<3;++i){
            assert(scene->createGeneratedInstance({60,240,int(512+i*16)},objects,[](const auto&){return true;}));
            h[i]=ordered->terrain().handle(i);
        }
        assert(ordered->terrain().activation(h[1],false)&&ordered->reconcileActivation());
        unsigned population=0;assert(ordered->population(60,population)&&population==2);
        for(auto key:h)assert(ordered->terrain().requestDestroy(key));
        std::vector<unsigned> order;
        const bool cleaned=ordered->cleanupDestroyed([&](InstanceHandle shared,const GameplayCollisionView::Record& record){
            GameplayCollisionView::Kind kind;InstanceHandle local;
            assert(GameplayCollisionView::decode(shared,kind,local)&&kind==GameplayCollisionView::Kind::terrain&&
                record.terrain==ordered->terrain().find(local));
            order.push_back(local.slot);
            if(reject&&order.size()==2)return true; // A lying success must not acknowledge cleanup.
            return ordered->terrain().retireMarked(local);
        });
        assert(cleaned==!reject&&order[0]==h[0].slot&&order[1]==h[2].slot);
        if(reject){
            assert(!ordered->valid()&&order.size()==2&&ordered->terrain().size()==2&&
                !ordered->terrain().find(h[0])&&ordered->terrain().find(h[2]));
            assert(!ordered->cleanupDestroyed([](auto,const auto&){assert(false);return true;}));
        }else{
            assert(order.size()==3&&order[2]==h[1].slot&&ordered->terrain().size()==0&&ordered->valid());
            assert(ordered->population(60,population)&&population==0);
        }
    }
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
    const auto* dying=terrainRecord.terrain;
    assert(owner->terrain().requestDestroy(wallLocal)&&dying->destroyRequested&&
        owner->terrain().size()==3&&owner->terrain().find(wallLocal)==dying&&
        (owner->state(wall)->flags&1u)&&!(owner->state(otherWall)->flags&1u));
    assert(owner->terrain().requestDestroy(wallLocal)); // repeated request retains the same lifetime
    assert(owner->terrain().activation(wallLocal,false)&&!dying->active&&
        (owner->state(wall)->flags&3u)==3u&&owner->terrain().find(wallLocal)==dying);
    const auto* survivor=owner->terrain().find(owner->terrain().handle(1));
    const auto survivorHandle=owner->terrain().handle(1);
    assert(owner->terrain().retireMarked(wallLocal)&&!owner->terrain().find(wallLocal)&&!owner->state(wall));
    assert(owner->terrain().size()==2&&owner->terrain().handle(0).slot==survivorHandle.slot&&
        owner->terrain().find(survivorHandle)==survivor&&owner->state(otherWall));
    assert(world->createGeneratedInstance({60,240,512},objects,terrain));
    const auto replacement=owner->terrain().handle(2);
    assert(replacement.slot==wallLocal.slot&&replacement.generation!=wallLocal.generation&&
        !owner->terrain().find(wallLocal)&&owner->terrain().find(survivorHandle)==survivor);
    assert(owner->terrain().find(replacement)->spriteId==51&&!owner->terrain().find(replacement)->destroyRequested);
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
        assert(store->size()==1&&store->handle(0).slot==second.slot);
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

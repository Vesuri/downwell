#include "core/GameplayCollisionOwner.h"
#include "core/GameplayFieldReentryVerification.h"
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
struct GoalAudit {
    GameplayWorld* world=nullptr;
    unsigned reject=0;
    std::uint64_t draws=0;
    unsigned stage=0;
    bool roomBoundary=false;
    using Record=GameplayGoalFields::Record;
    bool birth(InstanceHandle h,const Record& r){
        assert(world->goalFieldInstances().find(h)==&r&&r.scaleX==1&&r.scaleY==1&&world->randomDrawCount()==draws);
        if(r.objectId==255){
            const auto& exits=world->exitControllers();
            assert(exits.find(exits.handle(0))->objectId==306&&exits.find(exits.handle(1))->objectId==74);
        }
        stage=1;return reject!=stage;
    }
    bool transform(InstanceHandle h,const Record& r,RunnerTransformField field,RunnerImageValue value,RunnerImageValue,unsigned){
        assert(world->goalFieldInstances().find(h)==&r&&world->randomDrawCount()==draws&&value.bits==0x40000000u);
        assert(r.scaleY==1&&((stage==1&&field==RunnerTransformField::scaleX&&r.scaleX==1)||
            (stage==2&&field==RunnerTransformField::scaleY&&r.scaleX==2)));
        return ++stage!=reject;
    }
    bool retire(InstanceHandle h,const Record& r){
        assert(world->goalFieldInstances().find(h)==&r&&r.scaleX==2&&r.scaleY==2);
        if(r.objectId==371)assert(roomBoundary&&r.destroyRequested);
        stage=4;return reject!=stage;
    }
    bool markDestroyed(InstanceHandle h,const Record& r){
        assert(world->goalFieldInstances().find(h)==&r&&r.objectId==371&&r.scaleX==2&&r.scaleY==2&&
            !r.destroyRequested&&world->randomDrawCount()==draws+1);
        stage=5;return reject!=stage;
    }
    bool mask(InstanceHandle,const Record&,int){return true;}
    bool activation(InstanceHandle,const Record&,bool){return true;}
    bool membership(InstanceHandle,const Record&,bool){return true;}
    bool move(InstanceHandle,const Record&,RunnerImageValue,RunnerImageValue){return true;}
    bool sprite(InstanceHandle,const Record&,int){return true;}
    bool imageIndex(InstanceHandle,const Record&,RunnerImageValue){return true;}
};
struct ExitAudit {
    GameplayWorld* world;
    unsigned reject=0,births=0,retires=0;
    std::uint64_t draws;
    bool birth(InstanceHandle h,const GameplayExitControllers::Record& r){
        assert(world->exitControllers().find(h)==&r&&r.active&&r.x==240&&r.y==700);
        assert(world->randomDrawCount()==draws&&!world->goalFieldInstances().find({0,1}));
        assert(r.objectId==(births==0?306:74));
        return ++births!=reject;
    }
    bool retire(InstanceHandle h,const GameplayExitControllers::Record& r){
        assert(world->exitControllers().find(h)==&r);++retires;return reject!=3;
    }
    bool activation(InstanceHandle,const GameplayExitControllers::Record&,bool){return true;}
    bool membership(InstanceHandle,const GameplayExitControllers::Record&,bool){return true;}
};
int main(){
    for(unsigned failure=0;failure<4;++failure){
        auto world=std::make_unique<GameplayWorld>(300,true,0,false);
        ExitAudit audit{world.get(),failure,0,0,world->randomDrawCount()};
        assert(world->bindExitControllerLifecycle(GameplayExitControllers::Binding::bind(audit)));
        const bool created=world->registerLevelExit(700);
        assert(created==(failure==0||failure==3));
        if(!created){
            assert(audit.births==failure&&world->randomDrawCount()==audit.draws&&!world->effectsSucceeded());
            assert(!world->registerLevelExit(700)&&audit.births==failure);
            continue;
        }
        assert(audit.births==2&&world->randomDrawCount()==audit.draws+1);
        const auto old=world->exitControllers().handle(0);
        const auto result=world->restartFromResults(DeathMenuSequence::Action::Retry,2,300);
        if(failure==3){assert(result==ResultsDestination::none&&!world->effectsSucceeded()&&audit.retires==1);continue;}
        assert(result==ResultsDestination::gameplay&&audit.retires==2&&!world->exitControllers().find(old));
        assert(world->exitControllers().birthSequence()>old.generation);
    }
    for(unsigned failure=0;failure<=6;++failure){
        auto world=std::make_unique<GameplayWorld>(300,true,0,false);
        GoalAudit audit{world.get(),failure,world->randomDrawCount(),0};
        assert(world->bindGoalFieldLifecycle(GameplayGoalFields::Binding::bindMaskDestruction(audit)));
        world->configurePhaseBoundary([](void* p,GameplayWorld::PhaseBoundary phase){
            auto& a=*static_cast<GoalAudit*>(p);
            const auto h=a.world->goalFieldInstances().handle(0);
            assert(phase==GameplayWorld::PhaseBoundary::roomEntry&&a.world->goalFieldInstances().find(h)->destroyRequested);
            a.roomBoundary=true;return a.reject!=6;
        },&audit);
        assert(world->createRoomStartField()==(failure==0));
        assert(world->randomDrawCount()==audit.draws+(failure==0||failure>=4?1:0));
        const auto* record=world->goalFieldInstances().find({0,1});
        if(!failure)assert(!record&&audit.roomBoundary);
        else{
            assert(record&&!world->effectsSucceeded()&&record->destroyRequested==(failure==4||failure==6));
            assert(!world->createRoomStartField());
            struct BuilderProbe {
                bool called=false;
                void resetRoom(int,int,GameMakerRandom&,const AreaOneBuilder::Setup&){called=true;}
            } builder;
            const auto draws=world->randomDrawCount();
            assert(!world->initializeAreaOneBuilder(builder,0,0)&&!builder.called&&world->randomDrawCount()==draws);
        }
    }
    for(unsigned failure=1;failure<=4;++failure){
        auto world=std::make_unique<GameplayWorld>(300,true,0,false);
        GoalAudit audit{world.get(),failure,world->randomDrawCount(),0};
        assert(world->bindGoalFieldLifecycle(GameplayGoalFields::Binding::bindTransforms(audit)));
        const bool created=world->registerLevelExit(700);
        assert(created==(failure==4)&&audit.stage==(failure==4?3:failure));
        const auto key=world->goalFieldInstances().handle(0);
        const auto* record=world->goalFieldInstances().find(key);
        assert(record&&record->scaleX==(failure<=2?1:2)&&record->scaleY==(failure==4?2:1));
        assert(world->randomDrawCount()==audit.draws+(failure==4?1:0));
        if(failure==4)assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::none&&audit.stage==4);
        const auto draws=world->randomDrawCount();
        assert(!world->effectsSucceeded()&&!world->registerLevelExit(800)&&world->randomDrawCount()==draws&&
            world->goalFieldInstances().find(key)==record);
    }
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
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        for(int object:{112,92,60,112})
            assert(world->createGeneratedInstance({object,240,512},objects,[](const auto&){return true;}));
        const auto first=world->enemyHandle(world->enemyAt(0).lifetimeId);
        const auto survivor=world->enemyHandle(world->enemyAt(1).lifetimeId);
        const InstanceHandle box{0,1};assert(world->findBreakable(box));
        const auto wall=owner->terrain().handle(0);
        assert(world->markEnemyDestroyed(first)&&world->markEnemyDestroyed(first));
        assert(world->markBreakableDestroyed(box)&&world->markBreakableDestroyed(box));
        assert(owner->terrain().requestDestroy(wall));
        assert(world->resolveEnemyLifetime(first)->active&&world->resolveEnemyLifetime(first)->destroyRequested);
        assert(world->findBreakable(box)->active&&world->findBreakable(box)->destroyRequested);
        unsigned count=99;
        assert(owner->population(112,count)&&count==1);
        assert(owner->population(92,count)&&count==0);
        assert(owner->reconcileActivation()&&world->resolveEnemyLifetime(first)&&world->findBreakable(box));
        RunnerStepSnapshot<16> snapshot;unsigned enemies=0,boxes=0;
        assert(owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle,const PackedObjectEvent& event){
            enemies+=event.codeId==1200;boxes+=event.codeId==1087;return true;
        }));
        assert(enemies==1&&boxes==0);
        assert(owner->prepareDraw()&&owner->prepareDraw());
        assert(!world->resolveEnemyLifetime(first)&&!world->findBreakable(box)&&!owner->terrain().find(wall));
        assert(world->enemyCount()==1&&world->resolveEnemyLifetime(survivor)==&world->enemyAt(0));
        assert(world->createGeneratedInstance({112,272,512},objects,[](const auto&){return true;}));
        const auto replacement=world->enemyHandle(world->enemyAt(1).lifetimeId);
        assert(replacement.slot==first.slot&&replacement.generation!=first.generation);
        assert(!world->resolveEnemyLifetime(replacement)->destroyRequested);
        assert(world->markEnemyDestroyed(replacement)&&owner->prepareDraw());
        assert(world->resolveEnemyLifetime(survivor)&&!world->resolveEnemyLifetime(replacement));
        assert(!world->markEnemyDestroyed(first)&&!world->effectsSucceeded());
    }
    {
        auto world=std::make_unique<GameplayWorld>(300,true,0,false);
        assert(world->createGeneratedInstance({112,240,512},objects,[](const auto&){return true;}));
        const auto h=world->enemyHandle(world->enemyAt(0).lifetimeId);
        assert(!world->markEnemyDestroyed(h)&&!world->effectsSucceeded());
        assert(world->resolveEnemyLifetime(h)&&!world->resolveEnemyLifetime(h)->destroyRequested);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        // Deliberately interleave registration/storage order across families.
        for(int object:{99,60,112,93,60})
            assert(world->createGeneratedInstance({object,240,512},objects,[](const auto&){return true;}));
        const auto first=owner->terrain().handle(1),second=owner->terrain().handle(3);
        assert(owner->terrain().activation(first,false)&&owner->reconcileActivation());
        assert(owner->terrain().activation(first,true)&&owner->reconcileActivation());
        RunnerStepSnapshot<16> snapshot;
        std::vector<int> codes;std::vector<InstanceHandle> walls;
        assert(owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle h,const PackedObjectEvent& event){
            assert(event.eventType==3&&event.eventSubtype==2);
            codes.push_back(event.codeId);
            if(event.codeId==986){
                GameplayCollisionView::Kind kind;InstanceHandle local;
                assert(GameplayCollisionView::decode(h,kind,local)&&kind==GameplayCollisionView::Kind::terrain);
                walls.push_back(local);
            }
            return true;
        }));
        assert((codes==std::vector<int>{848,986,986,1091,1112,1200}));
        assert(walls.size()==2&&walls[0].slot==second.slot&&walls[1].slot==first.slot);
        // Object 99's empty own End Step (1112) must NOT inherit wall 986.
        codes.clear();bool changed=false;InstanceHandle newborn;
        assert(owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle,const PackedObjectEvent& event){
            codes.push_back(event.codeId);
            if(event.codeId==986&&!changed){
                changed=true;
                assert(owner->terrain().requestDestroy(first));
                assert(owner->prepareDraw()); // Exercise physical slot reuse too.
                assert(world->createGeneratedInstance({60,272,512},objects,[](const auto&){return true;}));
                newborn=owner->terrain().handle(owner->terrain().size()-1);
                assert(newborn.slot==first.slot&&newborn.generation!=first.generation);
            }
            return true;
        }));
        assert(changed&&(codes==std::vector<int>{848,986,1091,1112,1200}));
        codes.clear();
        assert(owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle,const PackedObjectEvent& event){
            codes.push_back(event.codeId);return true;
        }));
        assert((codes==std::vector<int>{848,986,986,1091,1112,1200}));
        unsigned calls=0;
        assert(!owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle,const PackedObjectEvent&){return ++calls<2;}));
        assert(calls==2&&!owner->valid());
        assert(!owner->dispatchStepPhase(2,snapshot,[&](InstanceHandle,const PackedObjectEvent&){++calls;return true;})&&calls==2);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        assert(world->createGeneratedInstance({60,240,512},objects,[](const auto&){return true;}));
        RunnerStepSnapshot<1> tooSmall;unsigned calls=0;
        assert(!owner->dispatchStepPhase(2,tooSmall,[&](InstanceHandle,const PackedObjectEvent&){++calls;return true;}));
        assert(calls==0&&!owner->valid());
    }
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
        RunnerStepSnapshot<16> phase;unsigned wallEvents=0;
        assert(owner->dispatchStepPhase(2,phase,[&](InstanceHandle shared,const PackedObjectEvent& event){
            if(event.codeId!=986){assert(event.codeId==848);return true;}
            GameplayCollisionView::Kind kind;InstanceHandle local;
            assert(GameplayCollisionView::decode(shared,kind,local)&&kind==GameplayCollisionView::Kind::terrain);
            ++wallEvents;return owner->wallEndStepEvent(local,1,random,masks,effects);
        }));
        assert(wallEvents==9);
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
    {
        auto ordered=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,ordered->enemyBinding(),
            ordered->breakableBinding(),ordered->playerBinding(),&ordered->terrain());
        assert(ordered->attach(*scene));
        for(unsigned i=0;i<3;++i)
            assert(scene->configureEnemy(i,112,163,int(i)*32,128,0,0,5,14,0,1,-1,1));
        const auto first=scene->enemyHandle(scene->enemyAt(0).lifetimeId);
        auto& record=const_cast<GameplayWorld::EnemyState&>(*scene->findEnemy(first));
        auto binding=ordered->enemyBinding();
        assert(binding.activation(first,record,false));record.active=false;
        assert(ordered->reconcileActivation());
        assert(binding.activation(first,record,true));record.active=true;
        assert(ordered->reconcileActivation());
        GameMakerRandom expected=scene->randomState();
        const int second=120+int(expected.irandomSmall(180));
        const int third=120+int(expected.irandomSmall(180));
        const int reactivated=120+int(expected.irandomSmall(180));
        scene->step(0);
        assert(scene->effectsSucceeded()&&scene->enemyAt(1).alarm1==second&&
            scene->enemyAt(2).alarm1==third&&scene->findEnemy(first)->alarm1==reactivated);
        // A failed external capture is terminal before any actor alarm runs.
        scene->configureEnemyStepOrder([](void*,GameplayWorld::EnemyStepOrder&){return false;},nullptr);
        const int before=scene->enemyAt(1).alarm1;
        scene->step(0);
        assert(!scene->effectsSucceeded()&&scene->enemyAt(1).alarm1==before);
    }
    {
        auto fieldsOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto fieldWorld=std::make_unique<GameplayWorld>(300,true,0,false,fieldsOwner->enemyBinding(),
            fieldsOwner->breakableBinding(),fieldsOwner->playerBinding(),&fieldsOwner->terrain());
        assert(fieldsOwner->attach(*fieldWorld));
        const auto draws=fieldWorld->randomDrawCount();
        assert(fieldWorld->createGeneratedInstance({82,240,512},objects,[](const auto&){return false;}));
        const auto identity=fieldWorld->timeFields().identityAt(0);
        InstanceHandle fieldKey;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::timeField,{0,identity},fieldKey));
        GameplayCollisionView::Record record;
        assert(GameplayCollisionView(*fieldWorld).resolve(fieldKey,record)&&record.timeField==fieldWorld->timeFields().at(0));
        const auto* state=fieldsOwner->state(fieldKey);std::int32_t x,y;
        assert(state&&state->x.promoted().roundPixel(x)&&x==240&&state->y.promoted().roundPixel(y)&&y==512);
        unsigned population=0;
        assert(fieldsOwner->population(82,population)&&population==1&&
            fieldsOwner->properties(fieldKey)->sprite.spriteId==597&&fieldWorld->randomDrawCount()==draws);
        assert(fieldWorld->registerLevelExit(900));
        const auto exitLocal=fieldWorld->exitControllers().handle(0);
        InstanceHandle exitKey,setterKey;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::exitController,exitLocal,exitKey)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::exitController,fieldWorld->exitControllers().handle(1),setterKey));
        for(auto key:{exitKey,setterKey}){
            const auto* p=fieldsOwner->properties(key);const auto* s=fieldsOwner->state(key);
            PackedObject definition;
            assert(objects.findObject(key.slot==exitKey.slot?306:74,definition)&&p&&s&&!(s->flags&0x100003u)&&
                p->sprite.spriteId==definition.spriteId&&p->sprite.maskId==definition.textureMaskId&&
                p->sprite.scaleX.bits==0x3f800000u&&p->sprite.scaleY.bits==0x3f800000u);
        }
        const auto goalLocal=fieldWorld->goalFieldInstances().handle(0);
        InstanceHandle goalKey;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::goalField,goalLocal,goalKey));
        const auto* goal=fieldsOwner->state(goalKey);
        const auto* goalProperties=fieldsOwner->properties(goalKey);
        assert(goal&&goalProperties&&goal->x.promoted().roundPixel(x)&&x==240&&
            goal->y.promoted().roundPixel(y)&&y==932&&goalProperties->sprite.scaleX.bits==0x40000000u&&
            goalProperties->sprite.scaleY.bits==0x40000000u&&fieldWorld->randomDrawCount()==draws+1);
        assert(fieldWorld->registerLevelExit(910)&&fieldsOwner->population(255,population)&&population==2&&
            fieldsOwner->state(goalKey)&&fieldWorld->randomDrawCount()==draws+2);
        assert(fieldWorld->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(fieldsOwner->valid()&&!fieldsOwner->state(fieldKey)&&
            !GameplayCollisionView(*fieldWorld).resolve(fieldKey,record)&&fieldsOwner->population(82,population)&&population==0);
        assert(!fieldsOwner->state(goalKey)&&fieldsOwner->population(255,population)&&population==0);
        assert(!fieldsOwner->state(exitKey)&&!fieldsOwner->state(setterKey)&&
            fieldsOwner->population(306,population)&&population==0&&fieldsOwner->population(74,population)&&population==0);
        assert(fieldWorld->registerLevelExit(920));
        const auto nextGoal=fieldWorld->goalFieldInstances().handle(0);
        assert(nextGoal.generation>goalLocal.generation&&fieldWorld->goalFieldInstances().find(nextGoal));
        assert(fieldWorld->createGeneratedInstance({82,256,600},objects,[](const auto&){return false;}));
        const auto next=fieldWorld->timeFields().identityAt(0);
        InstanceHandle nextKey;
        assert(next>identity&&GameplayCollisionView::encode(GameplayCollisionView::Kind::timeField,{0,next},nextKey)&&
            fieldsOwner->state(nextKey)&&!fieldsOwner->state(fieldKey));
        assert(!fieldWorld->enterLevelClear(0,false)&&fieldsOwner->population(255,population)&&population==1);
        assert(fieldWorld->registerLevelExit(176+fieldWorld->player().offsetY()));
        for(unsigned i=0;i<100;++i)fieldWorld->step(0);
        assert(fieldWorld->levelResultRequested()&&fieldWorld->enterLevelClear(0,false)&&
            fieldsOwner->population(255,population)&&population==0);
        bool heart=false;
        for(unsigned i=0;i<400&&!fieldWorld->nextLevelRequested();++i){
            bool confirm=true;
            assert(fieldWorld->stepLevelClear(confirm,false,false,heart,[](auto,unsigned){}));
        }
        assert(fieldWorld->prepareNextAreaOneLevel({},300)&&fieldWorld->runStage().level==2);
        const auto startDraws=fieldWorld->randomDrawCount();
        assert(fieldWorld->createRoomStartField()&&fieldWorld->startTimeField()&&
            fieldsOwner->population(371,population)&&population==1&&fieldWorld->randomDrawCount()==startDraws+1);
        const auto startLocal=fieldWorld->goalFieldInstances().handle(0);
        InstanceHandle startKey;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::goalField,startLocal,startKey));
        assert(fieldsOwner->state(startKey)&&fieldsOwner->properties(startKey)->sprite.scaleX.bits==0x40000000u&&
            fieldsOwner->properties(startKey)->sprite.scaleY.bits==0x40000000u);
        assert(fieldWorld->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        fieldWorld->confirmFirstStage();
        const auto firstIdentity=fieldWorld->goalFieldInstances().birthSequence();
        assert(fieldWorld->createRoomStartField()&&!fieldWorld->startTimeField()&&!fieldsOwner->state(startKey)&&
            fieldsOwner->population(371,population)&&population==0&&
            !fieldWorld->goalFieldInstances().find({0,firstIdentity})&&fieldsOwner->valid());
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1201-176,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        auto tape=std::make_unique<GameplayFieldReentryVerification>();
        assert(tape->run(*world,*owner,objects,300)&&tape->checks==255&&tape->gameSteps==108&&
            tape->clearSteps>0&&tape->clearSteps<400&&tape->firstGeneration<tape->laterGeneration&&
            tape->laterGeneration<tape->retryGeneration);
        std::printf("Field reentry tape: checks=%u gameplay=%u upgrade=%u generations=%u/%u/%u\n",
            tape->checks,tape->gameSteps,tape->clearSteps,tape->firstGeneration,tape->laterGeneration,tape->retryGeneration);
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
    assert(owner->reconcileActivation()&&owner->terrain().find(wallLocal)==dying);
    assert(owner->prepareDraw()&&!owner->terrain().find(wallLocal)&&!owner->state(wall));
    assert(owner->prepareDraw()); // An unchanged frame does not sweep again.
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

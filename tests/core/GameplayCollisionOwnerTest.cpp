#include "core/GameplayCollisionOwner.h"
#ifdef DOWNWELL_LOCAL_DRAW_CAPTURE
namespace downwell { volatile unsigned localDrawCaptureMode=DOWNWELL_LOCAL_DRAW_CAPTURE; }
#endif
#include "core/RunnerBindingRevisionVerification.h"
#include "core/MachineGunSourceStep.h"
#ifdef DOWNWELL_NATIVE_QUERY_PROBE
bool nativeRegisteredTimeChange(downwell::GameplayCollisionOwner& owner,
    downwell::TimeChangeEffects<8>& effects,int& speed,std::uint32_t sourceId){
    return owner.createTimeChangeWithSourceId(effects,sourceId,
        downwell::RunnerImageValue{0x43704000u}.promoted(),
        downwell::RunnerImageValue{0x43308000u}.promoted(),speed,[](unsigned ms){return ms==30;});
}
bool nativePlayerTimeField(downwell::GameplayCollisionOwner& owner,downwell::MotionBinary64 x,
    downwell::MotionBinary64 y,const downwell::SpriteMaskPack& masks,bool& hit){
    return owner.playerTimeFieldMeeting(x,y,masks,hit);
}
bool nativeBoxAlarmStop(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle box,
    const downwell::SpriteMaskPack& masks,bool stopped,int width){
    return owner.stopBoxAlarm0(box,stopped,width,masks);
}
bool nativePlayerTimeFieldSelection(downwell::GameplayCollisionOwner& owner,downwell::MotionBinary64 x,
    downwell::MotionBinary64 y,const downwell::SpriteMaskPack& masks,
    downwell::GameplayCollisionOwner::RegisteredFieldContact& result){
    return owner.playerTimeFieldSelection(x,y,masks,result);
}
bool nativeOwnedDebrisTick(GameplayWorld& world){
    return world.stepWithOwnedDebris(0,[](auto,const auto&,bool& frozen){frozen=false;return true;},
        [](auto,const auto&,auto,auto,bool& hit){hit=false;return true;});
}
bool nativeAlarmDispatch(downwell::GameplayCollisionOwner& owner,downwell::RunnerStepSnapshot<16>& phase,
    bool (*dispatch)(downwell::InstanceHandle,unsigned,const downwell::PackedObjectEvent&)){
    return owner.dispatchAlarmPhase(phase,dispatch);
}
bool nativeDebrisOwnership(GameplayWorld& world,downwell::DebrisEffects<2>& effects,
    downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle& key){
    return world.spawnDebris(effects,{},{},1,key)&&owner.prepareDraw();
}
bool nativeDebrisQuery(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle local,
    downwell::MotionBinary64 x,downwell::MotionBinary64 y,const downwell::SpriteMaskPack& masks,bool& hit){
    return owner.debrisPlaceMeeting(local,x,y,masks,hit);
}
bool nativeDebrisDispatch(GameplayWorld& world,downwell::GameplayCollisionOwner& owner,
    downwell::DebrisEffects<2>& effects,downwell::InstanceHandle local,downwell::InstanceHandle shared,
    const downwell::SpriteMaskPack& masks){
    const auto phase=effects.beginPhase();
    return world.advanceDebrisRoomAlarm(effects,local,phase,0)&&owner.stepDebris(effects,shared,phase,masks,
        [](auto,const auto&,bool& frozen){frozen=false;return true;});
}
bool nativeDebrisFreeze(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle local,
    const downwell::SpriteMaskPack& masks,bool stopped,bool& frozen){
    return owner.debrisFrozen(local,stopped,480,masks,frozen);
}
bool nativeScriptSpawn(GameplayWorld& world,downwell::ScriptSpawnEffects<2>& effects,
    const downwell::SpriteCollisionPack& sprites,downwell::InstanceHandle& caller){
    return world.spawnScriptEffect(effects,{},{},603,{0x3fe00000u,0},{},{},
        [&](int id,unsigned& frames){downwell::PackedSpriteCollision sprite;
            if(!sprites.sprite(id,sprite))return false;frames=sprite.frameCount;return true;},caller)&&
        world.advanceScriptEffectAnimation(effects,effects.beginPhase());
}
bool nativeScriptCleanup(downwell::GameplayCollisionOwner& owner,downwell::ScriptSpawnEffects<2>& effects){
    return owner.prepareDraw(effects);
}
bool nativeScriptTick(GameplayWorld& world,downwell::ScriptSpawnEffects<2>& effects){
    return world.stepWithScriptEffects(effects,0);
}
bool nativeOwnedScript(GameplayWorld& world,const downwell::SpriteCollisionPack& sprites,downwell::InstanceHandle& caller){
    return world.spawnScriptEffect({},{},603,{0x3fe00000u,0},{},{},
        [&](int id,unsigned& frames){downwell::PackedSpriteCollision r;if(!sprites.sprite(id,r))return false;
            frames=r.frameCount;return true;},caller);
}
bool nativeBoxScript(GameplayWorld& world,const downwell::SpriteCollisionPack& sprites,downwell::InstanceHandle box){
    return world.spawnBoxScriptEffect(box,{},{},603,{0x3fe00000u,0},
        downwell::MotionBinary64::fromInteger(270),downwell::MotionBinary64::fromInteger(-60000),
        [&](int id,unsigned& frames){downwell::PackedSpriteCollision r;if(!sprites.sprite(id,r))return false;
            frames=r.frameCount;return true;});
}
bool nativeActorDraw(const GameplayWorld& world){
    return world.visitRoomActors([](auto,downwell::InstanceHandle key,auto){return key.generation!=0;});
}
bool nativeBoxWrites(downwell::GameplayCollisionOwner& owner,downwell::InstanceHandle h,
    downwell::MotionBinary64 health,int& parent){
    return owner.projectileWallParent(h,parent)&&owner.projectileBoxHealth(h,health)&&
        owner.projectileBoxHealthWrite(h,health)&&owner.projectileBoxHit(h);
}
downwell::ResultsDestination nativeScriptRetry(GameplayWorld& world,downwell::ScriptSpawnEffects<2>& effects){
    return world.restartFromResultsWithScriptEffects(downwell::DeathMenuSequence::Action::Retry,43,300,effects);
}
// Compile-only A500 gate: instantiate the public selectors without the host
// fixture loader, filesystem, heap-backed assertions or C++ runtime headers.
bool nativeCollisionQueries(downwell::GameplayCollisionOwner& owner,
    const downwell::SpriteMaskPack& masks,downwell::InstanceHandle self,
    downwell::MotionBinary64 x,downwell::MotionBinary64 y,
    downwell::GameplayCollisionOwner::Owner::Result& result){
    return owner.terrainCollisionPoint(self,60,x,y,true,true,masks,result)&&
        owner.terrainNonPreciseLine(self,60,x,y,x,y,true,result)&&
        owner.projectileNonPreciseLine(self,59,x,y,x,y,false,result)&&
        owner.projectileNonPreciseLine(self,83,x,y,x,y,false,result)&&
        owner.projectileSolidPoint(self,x,y,false,masks,result);
}
#else
#include "BoxBurstWorldCheck.h"
#include "core/GameplayFieldReentryVerification.h"
#include "core/ProjectilePublication.h"
#include "core/WallPresentation.h"
#include "platform/amiga/AmigaWallDamage.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
using namespace downwell;
static void checkReadOnlyDraw(const GameplayWorld& world){
    std::vector<std::array<std::uint32_t,4>> expected;
    assert(world.visitRoomActors([&](auto kind,InstanceHandle h,auto depth){
        expected.push_back({unsigned(kind),h.slot,h.generation,depth.bits});return true;
    }));
    // Fail before completing a possible cold fill; no later callback may run.
    if(!expected.empty()){
        unsigned calls=0;
        assert(!world.visitRoomActorsReadOnly([&](auto,auto,auto){++calls;return false;}));
        assert(calls==1);
    }
    for(unsigned pass=0;pass<2;++pass){
        std::vector<std::array<std::uint32_t,4>> actual;
        assert(world.visitRoomActorsReadOnly([&](auto kind,InstanceHandle h,auto depth){
            actual.push_back({unsigned(kind),h.slot,h.generation,depth.bits});return true;
        }));
        assert(actual==expected);
    }
}
static std::vector<std::uint8_t> read(const char* path){std::ifstream f(path,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
struct TerrainAudit {
    GameplayTerrainInstances* store=nullptr;
    unsigned births=0,retires=0,failBirth=0,failRetire=0;
    bool birth(InstanceHandle h,const GameplayTerrainInstances::Record& r){
        assert(store->ownsIdentity(h,r));return ++births!=failBirth;
    }
    bool retire(InstanceHandle h,const GameplayTerrainInstances::Record& r){
        assert(store->ownsIdentity(h,r));return ++retires!=failRetire;
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
        assert(store->ownsIdentity(h,r)&&!r.destroyRequested);++destroys;return !rejectDestroy;
    }
    bool activation(InstanceHandle h,const GameplayTerrainInstances::Record& r,bool){
        assert(store->ownsIdentity(h,r)&&r.destroyRequested&&r.active);++activations;return !rejectActivation;
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
struct BulletAudit {
    GameplayWorld* world;unsigned reject=0,last=0;
    using Record=GameplayWorld::BulletState;
    bool check(InstanceHandle h,const Record& r,unsigned event){
        assert(world->resolveBulletLifetime(h)==&r);last=event;return event!=reject;
    }
    bool birth(InstanceHandle h,const Record& r){assert(!r.trajectory.initialized);return check(h,r,1);}
    bool sprite(InstanceHandle h,const Record& r,int sprite){assert(sprite==459);return check(h,r,2);}
    bool transform(InstanceHandle h,const Record& r,RunnerTransformField,RunnerImageValue,RunnerImageValue,unsigned){
        assert(r.trajectory.initialized);return check(h,r,3);
    }
    bool move(InstanceHandle h,const Record& r,RunnerImageValue,RunnerImageValue){return check(h,r,4);}
    bool imageIndex(InstanceHandle h,const Record& r,RunnerImageValue){return check(h,r,5);}
    bool markDestroyed(InstanceHandle h,const Record& r){assert(!r.destroyRequested);return check(h,r,6);}
    bool retire(InstanceHandle h,const Record& r){return check(h,r,7);}
    bool activation(InstanceHandle,const Record&,bool){return true;}
    bool membership(InstanceHandle,const Record&,bool){return true;}
    bool mask(InstanceHandle,const Record&,int){return true;}
};
static void checkTypedCollisionView(const GameplayCollisionView& view,GameplayCollisionView::Kind kind,InstanceHandle local){
    using K=GameplayCollisionView::Kind;
    InstanceHandle shared;GameplayCollisionView::Record reference;
    if(GameplayCollisionView::encode(kind,local,shared))view.resolve(shared,reference);
    switch(kind){
        case K::player:assert(view.resolveLocal<K::player>(local)==reference.player);break;
        case K::enemy:assert(view.resolveLocal<K::enemy>(local)==reference.enemy);break;
        case K::breakable:assert(view.resolveLocal<K::breakable>(local)==reference.breakable);break;
        case K::terrain:assert(view.resolveLocal<K::terrain>(local)==reference.terrain);break;
        case K::timeField:assert(view.resolveLocal<K::timeField>(local)==reference.timeField);break;
        case K::goalField:assert(view.resolveLocal<K::goalField>(local)==reference.goalField);break;
        case K::exitController:assert(view.resolveLocal<K::exitController>(local)==reference.exitController);break;
        case K::bullet:assert(view.resolveLocal<K::bullet>(local)==reference.bullet);break;
        case K::scriptEffect:assert(view.resolveLocal<K::scriptEffect>(local)==reference.scriptEffect);break;
        case K::debris:assert(view.resolveLocal<K::debris>(local)==reference.debris);break;
        case K::timeChange:assert(view.resolveLocal<K::timeChange>(local)==reference.timeChange);break;
        case K::entryDither:assert(view.resolveLocal<K::entryDither>(local)==reference.entryDither);break;
        case K::drone:assert(view.resolveLocal<K::drone>(local)==reference.drone);break;
    }
}
static void checkCollisionViewMembership(const GameplayWorld& world){
    const GameplayCollisionView view(world);
    for(unsigned family=0;family<=unsigned(GameplayCollisionView::Kind::drone);++family)
        for(auto local:{InstanceHandle{0,0},InstanceHandle{0xffffffffu,1},InstanceHandle{GameplayCollisionView::capacity,1}})
            checkTypedCollisionView(view,GameplayCollisionView::Kind(family),local);
    for(unsigned slot=0;slot<=GameplayCollisionView::capacity;++slot){
        for(unsigned generation:{0u,1u,2u,3u,0xffffffffu}){
            GameplayCollisionView::Record record;
            const InstanceHandle h{slot,generation};
            assert(view.contains(h)==view.resolve(h,record));
            auto kind=GameplayCollisionView::Kind::debris;
            InstanceHandle local{0xffffffffu,0xffffffffu};
            const bool decoded=GameplayCollisionView::decode(h,kind,local);
            assert(decoded==(slot<GameplayCollisionView::capacity&&generation!=0));
            if(decoded){
                InstanceHandle encoded;
                assert(GameplayCollisionView::encode(kind,local,encoded));
                assert(encoded.slot==h.slot&&encoded.generation==h.generation);
                checkTypedCollisionView(view,kind,local);
            }else assert(kind==GameplayCollisionView::Kind::debris&&local.slot==0xffffffffu&&local.generation==0xffffffffu);
        }
    }
}
static void checkEnemySlotDirectory(const ObjectPack& objects,const SpriteCollisionPack& sprites){
    auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
    auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
        owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
    assert(owner->attach(*world));
    InstanceHandle handles[24]{};bool retired[24]{};
    for(unsigned i=0;i<24;++i){
        assert(world->createGeneratedInstance({112,240+int(i)*16,512},objects,[](const auto&){return true;}));
        const auto& e=world->enemyAt(i);handles[i]={e.identitySlot,e.lifetimeId};
    }
    const auto check=[&](){
        checkCollisionViewMembership(*world);
        for(unsigned i=0;i<24;++i){
            const GameplayWorld::EnemyState* expected=nullptr;
            for(unsigned j=0;j<world->enemyCount();++j){
                const auto& e=world->enemyAt(j);
                if(e.identitySlot==handles[i].slot&&e.lifetimeId==handles[i].generation)expected=&e;
            }
            assert((expected==nullptr)==retired[i]);
            assert(world->resolveEnemyLifetime(handles[i])==expected);
            assert(world->findEnemy(handles[i])==expected);
            InstanceHandle shared;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::enemy,handles[i],shared));
            assert(GameplayCollisionView(*world).contains(shared)==(expected!=nullptr));
        }
    };
    check();
    // Middle, head and tail removals keep source traversal stable while the
    // borrowed record addresses change. Compare every indexed lookup to scan.
    for(unsigned parity=0;parity<2;++parity)for(unsigned i=parity;i<24;i+=2){
        assert(world->markEnemyDestroyed(handles[i])&&owner->prepareDraw());
        retired[i]=true;check();
    }
    assert(world->enemyCount()==0);
    assert(world->createGeneratedInstance({112,240,512},objects,[](const auto&){return true;}));
    const auto& next=world->enemyAt(0);const InstanceHandle replacement{next.identitySlot,next.lifetimeId};
    assert(replacement.slot==handles[0].slot&&replacement.generation!=handles[0].generation);
    assert(!world->resolveEnemyLifetime(handles[0])&&world->resolveEnemyLifetime(replacement)==&next);
    assert(world->configureEnemy(0,112,163,0,336,0,0,5,14,0,0,-1,-1,0,0,false));
    const auto& overwritten=world->enemyAt(0);const InstanceHandle dormant{overwritten.identitySlot,overwritten.lifetimeId};
    assert(!world->resolveEnemyLifetime(replacement)&&world->resolveEnemyLifetime(dormant)==&overwritten);
    auto& flags=const_cast<GameplayWorld::EnemyState&>(overwritten);
    flags.activationPending=false;
    assert(!world->findEnemy(dormant)&&world->resolveEnemyLifetime(dormant)==&overwritten);
}
static void checkNativeAnimationBatches(const ObjectPack& objects,const SpriteCollisionPack& sprites){
    struct Cursor {
        std::vector<unsigned> slots;unsigned at=0;
        unsigned take(){return slots.at(at++);}
        bool within(unsigned first,unsigned last)const{return at<slots.size()&&slots[at]>=first&&slots[at]<last;}
    };
    using View=GameplayCollisionView;
    for(unsigned scenario=0;scenario<3;++scenario){
        unsigned phases[2][3]{},remainders[2][3]{};bool results[2]{},healthy[2]{};
        for(unsigned side=0;side<2;++side){
            auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
                GameplayTerrainInstances::WallCollision{},nullptr,GameplayCollisionOwner::Ownership::native);
            auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),owner->breakableBinding(),
                owner->playerBinding(),&owner->terrain());
            assert(owner->attach(*world));
            InstanceHandle keys[3];Cursor cursor;
            for(unsigned i=0;i<3;++i){
                assert(world->createGeneratedInstance({112,240+int(i)*16,512},objects,[](const auto&){return true;}));
                auto& enemy=const_cast<GameplayWorld::EnemyState&>(world->enemyAt(i));
                keys[i]={enemy.identitySlot,enemy.lifetimeId};cursor.slots.push_back(View::enemyBase+keys[i].slot);
                enemy.imagePhase=scenario==1&&i==1?0x7fffffffu:17*i;enemy.imageRemainder=i;enemy.imageSpeed=7+i;
            }
            if(scenario==2)++keys[1].generation;
            auto identity=[&](unsigned slot){for(const auto key:keys)if(key.slot+View::enemyBase==slot)return key.generation;return 0u;};
            if(side)results[side]=world->advanceNativeEnemyAnimations(cursor,identity,View::enemyBase,View::breakableBase);
            else {
                results[side]=true;
                for(const auto key:keys){
                    // The retained adapter checks the lifetime before entering
                    // the world method; a stale handle must not alter run state.
                    if(!world->resolveEnemyLifetime(key)||!world->advanceEnemyRoomAnimation(key)){results[side]=false;break;}
                }
            }
            healthy[side]=world->effectsSucceeded();
            for(unsigned i=0;i<3;++i){phases[side][i]=world->enemyAt(i).imagePhase;remainders[side][i]=world->enemyAt(i).imageRemainder;}
        }
        assert(results[0]==results[1]&&healthy[0]==healthy[1]);
        for(unsigned i=0;i<3;++i)assert(phases[0][i]==phases[1][i]&&remainders[0][i]==remainders[1][i]);
    }
}
int main(){
    assert(downwell::RunnerBindingRevisionVerification::run());
    {
        bool owner=true,otherOwner=true;
        FailureSignal<> signal;signal.observe(owner);
        assert(signal&&owner);signal=false;assert(!signal&&!owner);
        signal=true;assert(signal&&!owner); // Child recovery cannot recover its owner.
        FailureSignal<> copy(signal);copy=false;assert(signal&&!copy);
        FailureSignal<> destination;destination.observe(otherOwner);
        destination=copy;assert(!destination&&!otherOwner);
        owner=true;FailureSignal<false> inverted(false);inverted.observe(owner);
        inverted=true;assert(inverted&&!owner);
        owner=true;FailureSignal<> failed(false);failed.observe(owner);assert(!owner);
        owner=otherOwner=true;FailureSignal<> bound;bound.observe(owner);bound.observe(otherOwner);
        assert(!owner&&!otherOwner);
        owner=true;RunnerDepthChanges<1> depths;depths.observeFailure(owner);
        RunnerImageValue a{},b{};
        assert(depths.request(0,a,MotionBinary64::fromInteger(1))&&owner);
        assert(!depths.request(1,b,MotionBinary64::fromInteger(2))&&!owner&&!depths.succeeded());
    }
    for(bool failSmoke:{false,true}){
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false);
        checkBoxBurstWorld(*world,failSmoke);
    }
    {
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false);
        BulletAudit audit{world.get()};
        assert(world->bindBulletLifecycle(GameplayWorld::BulletLifecycleBinding::bindMaskDestruction(audit)));
        world->step(4);assert(world->effectsSucceeded()&&world->bulletCount()==1);
        const auto& shot=world->bulletAt(0);const InstanceHandle key{shot.identitySlot,shot.lifetimeId};
        assert(world->markBulletDestroyed(key)&&audit.last==6);
        const auto* retained=world->resolveBulletLifetime(key);
        assert(retained&&retained->destroyRequested&&!retained->active);
        audit.last=0;assert(world->markBulletDestroyed(key)&&audit.last==0);
        assert(world->retireMarkedBullet(key)&&audit.last==7&&!world->resolveBulletLifetime(key));
        assert(!world->markBulletDestroyed(key)&&!world->effectsSucceeded()&&audit.last==7);
    }
    for(unsigned failure=1;failure<=7;++failure){
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false);
        BulletAudit audit{world.get(),failure};
        assert(world->bindBulletLifecycle(GameplayWorld::BulletLifecycleBinding::bindMaskDestruction(audit)));
        if(failure>=6)world->configureSolidResolver([](void*,int,int y){return y==8;},nullptr);
        world->step(4);
        if(failure==5)world->step(0);
        if(failure==7){
            const auto& bullet=world->bulletAt(0);
            assert(bullet.destroyRequested&&!world->retireMarkedBullet({bullet.identitySlot,bullet.lifetimeId}));
        }
        assert(!world->effectsSucceeded()&&audit.last==failure);
        const auto draws=world->randomDrawCount();world->step(4);
        assert(audit.last==failure&&world->randomDrawCount()==draws);
    }
    for(unsigned i=0;i<10000;++i){
        const std::uint32_t turns=i*2654435761u;
        RunnerImageValue actual;
        assert(ProjectilePublication::angle(turns,actual));
        float expected=static_cast<float>(double(turns)*360.0/4294967296.0);
        std::uint32_t bits;std::memcpy(&bits,&expected,4);assert(actual.bits==bits);
        const MachineGunTrajectory::Coordinate position{int(i)-5000,turns&1048575u};
        assert(ProjectilePublication::coordinate(position,176,actual));
        expected=static_cast<float>(position.pixel+176+double(position.fraction)/1048576.0);
        std::memcpy(&bits,&expected,4);assert(actual.bits==bits);
    }
    {
        std::uint32_t seed=0x97463251u;
        auto next=[&](){seed=seed*1664525u+1013904223u;return seed;};
        for(unsigned i=0;i<100000;++i){
            const auto raw=next();
            const std::int32_t pixel=raw<=0x7fffffffu?std::int32_t(raw):-1-std::int32_t(~raw);
            const unsigned fraction=next()&1048575u;
            const int origin=int(next()&511u);
            RunnerImageValue actual{0xdeadbeefu},reference=actual;
            const MachineGunTrajectory::Coordinate position{pixel,fraction};
            const bool valid=ProjectilePublication::coordinateBinary64(position,origin,reference);
            assert(ProjectilePublication::coordinate(position,origin,actual)==valid&&actual.bits==reference.bits);
            if(valid){
                const float native=float(double(pixel)+origin+double(fraction)/1048576.0);
                std::uint32_t bits;std::memcpy(&bits,&native,4);assert(actual.bits==bits);
            }
        }
        assert(ProjectilePublicationVerification::run());
    }
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
        const auto h=terrain->handle(0);const auto record=terrain->find(h);
        assert(terrain->requestDestroy(h)==(failure!=1)&&audit.destroys==1);
        if(failure!=1){
            assert(terrain->requestDestroy(h)&&audit.destroys==1);
            assert(terrain->activation(h,false)==(failure==0)&&audit.activations==1);
        }
        assert(terrain->find(h)==record&&terrain->size()==1&&terrain->find(h)->destroyRequested==(failure!=1));
        assert(terrain->find(h)->active==(failure!=0)&&terrain->valid()==(failure==0));
        if(failure){
            assert(!terrain->requestDestroy(h)&&!terrain->activation(h,false)&&audit.destroys==1);
        }else{
            assert(terrain->clear()&&audit.retires==1&&!terrain->find(h));
        }
    }
    {
        auto terrain=std::make_unique<GameplayTerrainInstances>();assert(terrain->append(60,0,0,true));
        const auto h=terrain->handle(0);
        GameplayTerrainInstances::Record snapshot;
        assert(terrain->read(h,snapshot)&&terrain->ownsIdentity(h,snapshot));
        ++snapshot.x;assert(!terrain->ownsIdentity(h,snapshot));--snapshot.x;
        ++snapshot.lifetimeId;assert(!terrain->ownsIdentity(h,snapshot));--snapshot.lifetimeId;
        const auto stale=InstanceHandle{h.slot,h.generation+1};
        snapshot.imageIndex=123;
        assert(!terrain->read(stale,snapshot)&&snapshot.imageIndex==123&&
            !terrain->ownsIdentity(stale,snapshot));
        assert(!terrain->requestDestroy(h)&&!terrain->find(h)->destroyRequested&&!terrain->valid());
        assert(terrain->read(h,snapshot)&&terrain->ownsIdentity(h,snapshot));
    }
    {
        TerrainDestructionAudit audit;
        auto store=std::make_unique<GameplayTerrainInstances>(GameplayTerrainInstances::Binding::bindDestruction(audit));
        audit.store=store.get();
        for(unsigned i=0;i<32;++i)assert(store->append(60,int(i),0,true));
        for(unsigned i=0;i<4096;++i){
            const unsigned index=(i*13u)%32u;const auto old=store->handle(index);
            InstanceHandle survivors[31];decltype(store->find(InstanceHandle{})) pointers[31];unsigned n=0;
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
    for(bool native:{false,true})for(unsigned family=0;family<6;++family){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},nullptr,
            native?GameplayCollisionOwner::Ownership::native:GameplayCollisionOwner::Ownership::reference);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world)&&owner->valid());
        // Fail before any family binding callback: enclosing validation must
        // observe this immediately, not only at the next simulation boundary.
        switch(family){
            case 0:assert(!owner->terrain().append(-1,0,0,true));break;
            case 1:{ScriptSpawnEffect effect;assert(!owner->scriptEffects().birth(0,effect));break;}
            case 2:{DebrisAnimation effect;assert(!owner->debris().birth(0,effect));break;}
            case 3:{std::uint32_t id=0;assert(!owner->timeChanges().birth(0,{},{},id));break;}
            case 4:{InstanceHandle key;auto camera=[](auto&){assert(false);return false;};
                assert(!owner->entryDithers().create(0,camera,camera,key));break;}
            case 5:assert(!owner->droneBullets().advanceAnimation({}));break;
        }
        assert(!owner->valid());
        unsigned callbacks=0;
        assert(!owner->visitRegistrationOrder([&](auto,auto,auto){++callbacks;return true;})&&callbacks==0);
        world->step(0);assert(!world->effectsSucceeded());
    }
    {
        ObjectAlarmMetadata<386> metadata;unsigned mask=0;
        assert(!metadata.mask(0,mask)&&metadata.bind(objects));
        ObjectAlarmMetadata<386,388> tooSmall;
        assert(!tooSmall.bind(objects)&&!tooSmall.mask(0,mask));
        ObjectAlarmMetadata<386,389> exact;
        assert(exact.bind(objects));
        unsigned inherited=0;
        for(unsigned pass=0;pass<2;++pass)for(unsigned id=0;id<objects.objectCount();++id){
            assert(metadata.mask(int(id),mask));
            for(unsigned alarm=0;alarm<12;++alarm){
                PackedObjectEvent event;
                const bool found=objects.findEvent(int(id),2,int(alarm),event);
                assert(bool(mask&(1u<<alarm))==found);
                for(unsigned repeat=0;repeat<2;++repeat){
                    PackedObjectEvent cached{};
                    assert(metadata.findEvent(int(id),alarm,cached)==found);
                    if(found)assert(cached.ownerObjectId==event.ownerObjectId&&
                        cached.eventType==event.eventType&&cached.eventSubtype==event.eventSubtype&&cached.codeId==event.codeId);
                    PackedObjectEvent prepared{};
                    assert(exact.findEvent(int(id),alarm,prepared)==found);
                    if(found)assert(prepared.ownerObjectId==event.ownerObjectId&&prepared.codeId==event.codeId);
                }
                if(found&&event.ownerObjectId!=int(id))++inherited;
            }
        }
        assert(inherited&&!metadata.mask(-1,mask)&&!metadata.mask(int(objects.objectCount()),mask));
        ObjectPack invalid(nullptr,0);
        assert(!metadata.bind(invalid)&&!metadata.mask(0,mask));
        assert(metadata.bind(objects)&&metadata.mask(266,mask));
        assert(mask&1u);
        PackedObjectEvent cached{};
        assert(metadata.findEvent(266,0,cached)&&!metadata.findEvent(-1,0,cached)&&
            !metadata.findEvent(386,0,cached)&&!metadata.findEvent(266,12,cached));
        auto noAlarms=ob;
        for(unsigned i=0;i<objects.bindingCount();++i){
            PackedObjectEvent event;assert(objects.binding(i,event));
            if(event.eventType==2)noAlarms[ObjectPack::headerSize+
                objects.objectCount()*ObjectPack::objectRecordSize+i*ObjectPack::bindingRecordSize]=3;
        }
        ObjectPack replacement(noAlarms.data(),noAlarms.size());
        assert(metadata.bind(replacement)&&metadata.mask(266,mask)&&mask==0&&!metadata.findEvent(266,0,cached));
        assert(!metadata.bind(invalid)&&!metadata.findEvent(266,0,cached));
        assert(metadata.bind(objects)&&metadata.findEvent(266,0,cached));
    }
    {
        TimeChangeEffects<8> effects;int speed=60;unsigned pauses=0;
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        const auto draws=world->randomDrawCount();
        auto create=[&](std::uint32_t id){return owner->createTimeChangeWithSourceId(effects,id,
            RunnerImageValue{0x43704000u}.promoted(),RunnerImageValue{0x43308000u}.promoted(),speed,
            [&](unsigned ms){
                ++pauses;const auto key=owner->timeChanges().handle(pauses);
                const auto* record=owner->timeChanges().find(key);InstanceHandle shared;
                if(!record||!GameplayCollisionView::encode(GameplayCollisionView::Kind::timeChange,key,shared))return false;
                const auto published=owner->state(shared);
                return ms==30&&speed==30&&record->sourceId==id&&record->alarms.get(0)==2&&
                    record->alarms.get(1)==2&&published&&published->x.bits==0x43704000u&&published->y.bits==0x43308000u;
            });};
        // Warm an empty combat capture/mask before admitting new alarm
        // columns. Later inactive/active transitions and Retry must rebuild
        // that plan, while unchanged captures retain immediate countdowns.
        for(unsigned tick=0;tick<3;++tick){world->step(0);assert(world->effectsSucceeded()&&owner->valid());}
        assert(create(105187));
        const auto old=owner->timeChanges().handle(1);InstanceHandle shared;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::timeChange,old,shared));
        assert(owner->timeChanges().activation(old,false)&&!owner->timeChanges().find(old)->active&&
            (owner->state(shared)->flags&2u));
        for(unsigned tick=0;tick<7;++tick){
            world->step(0);
            const auto* record=owner->timeChanges().find(old);
            assert(world->effectsSucceeded()&&owner->valid()&&record&&record->alarms.get(0)==2&&
                record->alarms.get(1)==2&&speed==30);
        }
        assert(owner->timeChanges().activation(old,true)&&owner->timeChanges().find(old)->active&&
            !(owner->state(shared)->flags&2u));
        for(unsigned tick=1;tick<=14;++tick){
            world->step(0);
            assert(world->effectsSucceeded()&&owner->valid());
            assert(speed==int(tick<12?30+5*(tick/2):60));
        }
        assert(effects.size()==0&&owner->timeChanges().size()==1&&owner->state(shared)&&
            (owner->state(shared)->flags&1u)&&world->randomDrawCount()==draws);
        assert(owner->prepareDraw()&&!owner->state(shared)&&!owner->timeChanges().find(old));
        assert(create(105188));const auto beforeRetry=owner->timeChanges().handle(2);
        assert(beforeRetry.slot==old.slot&&beforeRetry.generation!=old.generation);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)==ResultsDestination::gameplay);
        assert(world->effectsSucceeded()&&owner->valid()&&effects.size()==0&&owner->timeChanges().size()==0);
        assert(!owner->timeChanges().find(beforeRetry)&&create(105189));
        for(unsigned tick=0;tick<14;++tick)world->step(0);
        assert(world->effectsSucceeded()&&owner->prepareDraw()&&effects.size()==0&&owner->timeChanges().size()==0);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        InstanceHandle shared[3],local[3];
        const int kinds[]={60,93,99};
        for(unsigned i=0;i<3;++i){
            assert(owner->terrain().append(kinds[i],32+int(i)*16,240,true));
            local[i]=owner->terrain().handle(i);
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local[i],shared[i]));
            const auto state=owner->state(shared[i]);
            assert(state&&state->previous.x.bits==state->x.bits);
            // Seed only the snapshot with a sentinel to make the animation
            // boundary's write observable without moving indexed geometry.
            const_cast<GameplayCollisionOwner::Owner::State*>(state)->previous={};
        }
        assert(owner->terrain().activation(local[1],false)&&owner->reconcileActivation());
        world->step(0);assert(world->effectsSucceeded()&&owner->valid());
        for(unsigned i:{0u,2u}){
            const auto state=owner->state(shared[i]);
            assert(state&&state->previous.x.bits==state->x.bits&&state->previous.y.bits==state->y.bits);
        }
        assert(owner->state(shared[1])->previous.x.bits==0&&owner->state(shared[1])->previous.y.bits==0);
        assert(owner->terrain().activation(local[1],true)&&owner->reconcileActivation());
        world->step(0);assert(world->effectsSucceeded()&&owner->valid());
        const auto reactivated=owner->state(shared[1]);
        assert(reactivated->previous.x.bits==reactivated->x.bits&&reactivated->previous.y.bits==reactivated->y.bits);
    }
    checkEnemySlotDirectory(objects,sprites);
    checkNativeAnimationBatches(objects,sprites);
    const auto mb=read("generated/downwell-sprite-masks.dwsm");SpriteMaskPack masks(mb.data(),mb.size());
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        assert(scene->createGeneratedInstance({82,240,512},objects,[](const auto&){return true;}));
        DebrisEffects<1> effects;InstanceHandle local;
        assert(scene->spawnDebris(effects,{},{},1,local));auto& particle=*effects.get(local.generation);
        SpriteMaskPack::Mask shardMask,fieldMask;
        assert(masks.mask(598,0,shardMask)&&masks.mask(597,0,fieldMask));
        auto move=[&](MotionBinary64 x,MotionBinary64 y){RunnerImageValue px,py;
            assert(px.assign(x)&&py.assign(y)&&collision->debris().move(local.generation,particle,px,py));
            particle.x=px;particle.y=py;
        };
        unsigned edgeHits=0;
        for(int y=480;y<=544;++y)for(int x=208;x<=272;++x){
            move(MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y));
            bool overlap=false;
            for(int py=y-5;py<=y+4&&!overlap;++py)for(int px=x-5;px<=x+4;++px)
                if(shardMask.contains(px-x+8,py-y+8)&&fieldMask.contains(px-240+24,py-512+24)){overlap=true;break;}
            bool frozen=overlap;
            assert(collision->debrisFrozen(local,true,480,masks,frozen)&&frozen==!overlap);
            if(overlap&&!fieldMask.contains(x-240+24,y-512+24))++edgeHits;
        }
        assert(edgeHits); // A centre-only circle/point query would miss these.
        const SpriteMaskPack unavailable(nullptr,0);
        for(int qx:{639,640,960,1280,1281}){
            auto x=MotionBinary64::fromInteger(qx);x.high-=2u<<20;
            move(x,MotionBinary64::fromInteger(20000));bool frozen=true;
            assert(collision->debrisFrozen(local,false,480,unavailable,frozen)&&!frozen);
            assert(collision->debrisFrozen(local,true,480,masks,frozen)&&frozen==(qx>=640&&qx<=1280));
        }
        move(MotionBinary64::fromInteger(159),MotionBinary64::fromInteger(20000));bool unchanged=true;
        assert(!collision->debrisFrozen(local,true,480,unavailable,unchanged)&&unchanged);
        // Even side-exempt particles query the field first; missing mask data
        // is a terminal query failure, not a successful outside-area result.
        assert(!collision->valid());
    }
    for(unsigned seed=0;seed<16;++seed){
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));scene->configureEnemyRandom(GameMakerRandom(seed));
        for(int x:{224,240,256})assert(scene->createGeneratedInstance({60,x,512},objects,[](const auto&){return true;}));
        DebrisEffects<3> effects;InstanceHandle local[3],shared[3];DebrisAnimation expected[3];
        for(unsigned i=0;i<3;++i){
            assert(scene->spawnDebris(effects,MotionBinary64::fromInteger(232+int(i)*8),
                MotionBinary64::fromInteger(500+int(i)*4),1,local[i]));
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,local[i],shared[i]));
            expected[i]=*effects.get(local[i].generation);
        }
        auto random=scene->randomState();PackedSpriteCollision shape;
        assert(sprites.sprite(598,shape)&&!shape.collisionMode);
        auto real=[](MotionBinary64 v){const std::uint64_t bits=(std::uint64_t(v.high)<<32)|v.low;
            double d;std::memcpy(&d,&bits,sizeof d);return float(d);};
        auto contact=[&](MotionBinary64 px,MotionBinary64 py,bool& hit){
            const float x=real(px),y=real(py);
            const int left=int(std::nearbyint(x+float(shape.marginLeft-shape.originX)));
            const int right=int(std::nearbyint(x+float(shape.marginRight-shape.originX+1)))-1;
            const int top=int(std::nearbyint(y+float(shape.marginTop-shape.originY)));
            const int bottom=int(std::nearbyint(y+float(shape.marginBottom-shape.originY+1)))-1;
            hit=left<=263&&right>=216&&top<=519&&bottom>=504;return true;
        };
        for(unsigned tick=0;tick<40;++tick){
            const auto phase=effects.beginPhase();
            for(unsigned alarm=0;alarm<3;++alarm)for(unsigned i=0;i<3;++i){
                if(!effects.retained(local[i].generation))continue;
                expected[i].advanceAlarm(alarm);
                assert(scene->advanceDebrisRoomAlarm(effects,local[i],phase,alarm));
            }
            RunnerStepSnapshot<8> snapshot;unsigned next=0,stops=0;
            assert(collision->dispatchStepPhase(0,snapshot,[&](InstanceHandle h,const PackedObjectEvent&){
                GameplayCollisionView::Kind kind;InstanceHandle selected;
                assert(GameplayCollisionView::decode(h,kind,selected));
                if(kind!=GameplayCollisionView::Kind::debris)return true;
                while(next<3&&expected[next].destroyRequested)++next;
                assert(next<3&&selected.generation==local[next].generation);
                const auto i=next++;const bool frozen=i==1&&(tick%3)!=0;
                assert(expected[i].step(frozen,random,contact));
                assert(collision->stepDebris(effects,h,phase,masks,
                    [&](InstanceHandle key,const DebrisAnimation&,bool& stopped){
                        assert(key.generation==local[i].generation);++stops;stopped=frozen;return true;
                    }));
                return true;
            }));
            assert(stops<=3);
            for(unsigned i=0;i<3;++i){
                const auto* actual=effects.retained(local[i].generation);if(!actual)continue;
                const auto& e=expected[i];
                assert(actual->x.bits==e.x.bits&&actual->y.bits==e.y.bits&&
                    actual->xsp.high==e.xsp.high&&actual->xsp.low==e.xsp.low&&
                    actual->ysp.high==e.ysp.high&&actual->ysp.low==e.ysp.low&&
                    actual->imageSpeed.bits==e.imageSpeed.bits&&actual->flash==e.flash&&
                    actual->noTouch==e.noTouch&&actual->disappearing==e.disappearing&&actual->destroyRequested==e.destroyRequested);
                for(unsigned a=0;a<3;++a)assert(actual->alarms.get(a)==e.alarms.get(a));
                assert(collision->state(shared[i])->x.bits==e.x.bits&&collision->state(shared[i])->y.bits==e.y.bits);
            }
            assert(scene->randomState().drawCount()==random.drawCount()&&scene->randomState().stateIndex()==random.stateIndex());
            for(unsigned w=0;w<16;++w)assert(scene->randomState().stateWord(w)==random.stateWord(w));
            assert(collision->prepareDraw());
            for(unsigned i=0;i<3;++i)if(expected[i].destroyRequested)
                assert(!effects.retained(local[i].generation)&&!collision->state(shared[i]));
        }
        assert(effects.clear(collision->debris()));
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));DebrisEffects<1> effects;
        const auto oldPhase=effects.beginPhase();InstanceHandle local,shared;
        assert(scene->spawnDebris(effects,{},{},1,local)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,local,shared));
        unsigned stops=0;const auto draws=scene->randomDrawCount();
        auto reject=[&](auto,const auto&,bool&){++stops;return false;};
        assert(scene->advanceDebrisRoomAlarm(effects,local,oldPhase,0)&&effects.get(local.generation)->alarms.get(0)==30);
        assert(collision->stepDebris(effects,shared,oldPhase,masks,reject)&&!stops);
        assert(!collision->stepDebris(effects,shared,effects.beginPhase(),masks,reject)&&stops==1&&
            !scene->effectsSucceeded()&&!effects.valid()&&!collision->valid());
        assert(!collision->stepDebris(effects,shared,effects.beginPhase(),masks,reject)&&stops==1&&scene->randomDrawCount()==draws);
    }
    // Family 56 is broader than either wall selector: thin platforms and
    // shootable boxes must participate. These resource masks are rectangular;
    // independently round their quarter-pixel edges, not the particle anchor.
    for(int area:{1,2,4})for(int object:{60,93,92}){
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        assert(scene->createGeneratedInstance({object,240,512},objects,[](const auto&){return true;}));
        DebrisEffects<1> effects;InstanceHandle local,shared;
        assert(scene->spawnDebris(effects,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(504),area,local));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,local,shared));
        PackedSpriteCollision particle,target;PackedObject definition;
        const auto p=collision->properties(shared);
        assert(p&&sprites.collisionSprite(p->sprite.spriteId,p->sprite.maskId,particle)&&!particle.collisionMode);
        assert(objects.findObject(object,definition)&&
            sprites.collisionSprite(object==92?definition.spriteId:terrainTileForArea(1),definition.textureMaskId,target)&&!target.collisionMode);
        auto roundQuarter=[](int n){const int whole=n/4,tail=n%4;
            return whole+int(tail>2||(tail==2&&(whole&1)));};
        auto coordinate=[](int n){auto value=MotionBinary64::fromInteger(n);value.high-=2u<<20;return value;};
        const auto before=*collision->state(shared);
        unsigned hits=0,misses=0;
        for(int y=492;y<=532;++y)for(int x=220;x<=260;++x)for(int fraction=0;fraction<4;++fraction){
            const int qx=x*4+fraction,qy=y*4+fraction;
            const int left=roundQuarter(qx+4*(particle.marginLeft-particle.originX));
            const int right=roundQuarter(qx+4*(particle.marginRight-particle.originX+1))-1;
            const int top=roundQuarter(qy+4*(particle.marginTop-particle.originY));
            const int bottom=roundQuarter(qy+4*(particle.marginBottom-particle.originY+1))-1;
            const bool expected=left<=240+target.marginRight-target.originX&&
                right>=240+target.marginLeft-target.originX&&top<=512+target.marginBottom-target.originY&&
                bottom>=512+target.marginTop-target.originY;
            bool hit=!expected;
            assert(collision->debrisPlaceMeeting(local,coordinate(qx),coordinate(qy),masks,hit)&&hit==expected);
            hit?++hits:++misses;
            assert(collision->state(shared)->x.bits==before.x.bits&&collision->state(shared)->y.bits==before.y.bits);
        }
        assert(hits&&misses);
        if(object!=92){
            const auto targetKey=collision->terrain().handle(0);bool hit=true;
            const auto hitX=MotionBinary64::fromInteger(240+target.marginLeft-target.originX-particle.marginLeft+particle.originX);
            const auto hitY=MotionBinary64::fromInteger(512+target.marginTop-target.originY-particle.marginTop+particle.originY);
            assert(collision->terrain().activation(targetKey,false)&&collision->reconcileActivation());
            assert(collision->debrisPlaceMeeting(local,hitX,hitY,masks,hit)&&!hit);
            assert(collision->terrain().activation(targetKey,true)&&collision->reconcileActivation());
            assert(collision->debrisPlaceMeeting(local,hitX,hitY,masks,hit)&&hit);
            assert(collision->terrain().requestDestroy(targetKey));
            assert(collision->debrisPlaceMeeting(local,hitX,hitY,masks,hit)&&!hit);
        }
        assert(effects.clear(collision->debris()));
        bool unchanged=true;
        assert(!collision->debrisPlaceMeeting(local,{},{},masks,unchanged)&&unchanged&&!collision->valid());
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        InstanceHandle first,second;
        assert(scene->spawnDebris(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(300),1,first));
        assert(scene->debrisEffects().size()==1&&collision->debris().find(first));
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(!scene->debrisEffects().size()&&!collision->debris().find(first));
        assert(scene->spawnDebris(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(300),1,second));
        assert(second.generation>first.generation&&scene->debrisEffects().retained(second.generation));
        auto expected=*scene->debrisEffects().retained(second.generation);auto random=scene->randomState();
        for(unsigned tick=0;tick<35;++tick){
            const auto phase=scene->debrisEffects().beginPhase();
            for(unsigned a=0;a<3;++a){
                expected.advanceAlarm(a);
                assert(scene->advanceOwnedDebrisRoomAlarm(second,phase,a));
            }
            if(expected.destroyRequested){
                assert(collision->prepareDraw()&&!scene->debrisEffects().retained(second.generation));
                break;
            }
            const bool frozen=tick<5;
            assert(expected.step(frozen,random,[](auto,auto,bool& hit){hit=false;return true;}));
            assert(scene->stepOwnedDebrisRoomEffect(second,phase,
                [&](InstanceHandle,const auto&,bool& stop){stop=frozen;return true;},
                [](InstanceHandle,const auto&,auto,auto,bool& hit){hit=false;return true;}));
            const auto* actual=scene->debrisEffects().retained(second.generation);assert(actual);
            assert(actual->x.bits==expected.x.bits&&actual->y.bits==expected.y.bits&&
                actual->imageSpeed.bits==expected.imageSpeed.bits&&actual->flash==expected.flash);
            for(unsigned a=0;a<3;++a)assert(actual->alarms.get(a)==expected.alarms.get(a));
        }
        assert(expected.destroyRequested&&!scene->debrisEffects().size());
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        // Deliberately create higher object IDs first; registration order is
        // retained within object 370, not used to order different objects.
        DebrisEffects<3> effects;InstanceHandle first,second,newborn;
        assert(scene->spawnDebris(effects,{},{},1,first));
        assert(scene->createGeneratedInstance({112,240,512},objects,[](const auto&){return true;}));
        assert(scene->spawnDebris(effects,{},{},1,second));
        assert(scene->createGeneratedInstance({92,240,512},objects,[](const auto&){return true;}));
        effects.get(first.generation)->alarms.set(1,1);
        effects.get(second.generation)->alarms.set(1,1);
        const auto cutoff=effects.beginPhase();RunnerStepSnapshot<16> snapshot;
        std::vector<unsigned> order,debrisOrder;unsigned bornVisits=0;
        assert(collision->dispatchAlarmPhase(snapshot,[&](InstanceHandle h,unsigned alarm,const PackedObjectEvent& event){
            GameplayCollisionView::Kind kind;InstanceHandle local;
            assert(GameplayCollisionView::decode(h,kind,local));
            if(kind==GameplayCollisionView::Kind::breakable||kind==GameplayCollisionView::Kind::enemy||
                kind==GameplayCollisionView::Kind::debris){
                const int object=kind==GameplayCollisionView::Kind::breakable?92:
                    kind==GameplayCollisionView::Kind::enemy?112:370;
                PackedObjectEvent expected;
                assert(objects.findEvent(object,2,int(alarm),expected));
                assert(event.ownerObjectId==expected.ownerObjectId&&event.codeId==expected.codeId&&
                    event.eventType==expected.eventType&&event.eventSubtype==expected.eventSubtype);
            }
            if(kind==GameplayCollisionView::Kind::breakable){
                order.push_back(alarm*1000+92);
                assert(scene->spawnDebris(effects,{},{},1,newborn));
            }else if(kind==GameplayCollisionView::Kind::enemy){
                order.push_back(alarm*1000+112);
                // This earlier object's Alarm 1 changes a later object's
                // countdown before it is touched in the same alarm index.
                effects.get(second.generation)->alarms.set(1,2);
            }else if(kind==GameplayCollisionView::Kind::debris){
                order.push_back(alarm*1000+370);
                bornVisits+=local.generation==newborn.generation;
                debrisOrder.push_back(local.generation);
                return scene->advanceDebrisRoomAlarm(effects,local,cutoff,alarm);
            }
            return true;
        }));
        assert((order==std::vector<unsigned>{92,370,370,1112,1370,1370,2370,2370}));
        assert((debrisOrder==std::vector<unsigned>{first.generation,second.generation,
            first.generation,second.generation,first.generation,second.generation}));
        assert(!bornVisits&&effects.get(first.generation)->alarms.get(1)==0&&
            effects.get(second.generation)->alarms.get(1)==1&&effects.get(newborn.generation)->alarms.get(0)==30);
        // A previous callback may destroy a later captured lifetime. Recheck
        // flags before every countdown, including later alarm indices.
        unsigned secondVisits=0;const auto nextCutoff=effects.beginPhase();
        assert(collision->dispatchAlarmPhase(snapshot,[&](InstanceHandle h,unsigned alarm,const PackedObjectEvent&){
            GameplayCollisionView::Kind kind;InstanceHandle local;
            assert(GameplayCollisionView::decode(h,kind,local));
            if(kind!=GameplayCollisionView::Kind::debris)return true;
            if(local.generation==first.generation&&alarm==0){
                effects.get(second.generation)->alarms.set(0,1);
                assert(scene->advanceDebrisRoomAlarm(effects,second,nextCutoff,0));
            }
            secondVisits+=local.generation==second.generation;
            return scene->advanceDebrisRoomAlarm(effects,local,nextCutoff,alarm);
        }));
        assert(!secondVisits&&effects.retained(second.generation)->destroyRequested);
        assert(collision->prepareDraw()&&!effects.retained(second.generation));
        assert(effects.clear(collision->debris()));
        unsigned calls=0;
        assert(!collision->dispatchAlarmPhase(snapshot,[&](InstanceHandle,unsigned,const PackedObjectEvent&){++calls;return false;}));
        assert(calls==1&&!collision->valid());
        assert(!collision->dispatchAlarmPhase(snapshot,[&](InstanceHandle,unsigned,const PackedObjectEvent&){++calls;return true;}));
        assert(calls==1);
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));InstanceHandle key;
        for(unsigned retry=0;retry<2;++retry){
            assert(scene->spawnDebris(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(300),1,key));
            for(unsigned tick=1;tick<=30;++tick){
                scene->step(0);assert(scene->effectsSucceeded());
                const auto* actual=scene->debrisEffects().retained(key.generation);assert(actual);
                assert(actual->alarms.get(0)==int(30-tick));
                assert(actual->destroyRequested==(tick==30));
                assert(collision->prepareDraw());
            }
            assert(!scene->debrisEffects().retained(key.generation)&&!collision->debris().size());
            assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        }
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));InstanceHandle key;
        assert(scene->spawnDebris(MotionBinary64::fromInteger(20000),MotionBinary64::fromInteger(20000),1,key));
        auto expected=*scene->debrisEffects().retained(key.generation);auto random=scene->randomState();unsigned stops=0;
        for(unsigned tick=0;tick<40;++tick){
            const bool frozen=tick<5;
            assert(expected.advanceImage());for(unsigned a=0;a<3;++a)expected.advanceAlarm(a);
            assert(expected.step(frozen,random,[](auto,auto,bool& hit){hit=false;return true;}));
            assert(scene->stepWithOwnedDebris(0,[&](auto,const auto&,bool& stop){++stops;stop=frozen;return true;},
                [&](InstanceHandle local,const auto&,auto x,auto y,bool& hit){return collision->debrisPlaceMeeting(local,x,y,masks,hit);}));
            assert(collision->prepareDraw());
            const auto* actual=scene->debrisEffects().retained(key.generation);
            if(expected.destroyRequested){assert(!actual);continue;}
            assert(actual&&actual->x.bits==expected.x.bits&&actual->y.bits==expected.y.bits&&
                actual->imageIndex.bits==expected.imageIndex.bits&&actual->imageSpeed.bits==expected.imageSpeed.bits&&
                actual->noTouch==expected.noTouch&&actual->disappearing==expected.disappearing&&actual->flash==expected.flash);
            for(unsigned a=0;a<3;++a)assert(actual->alarms.get(a)==expected.alarms.get(a));
        }
        assert(stops==34);
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(scene->spawnDebris(MotionBinary64::fromInteger(20000),MotionBinary64::fromInteger(20000),1,key));
        // A failed source time-stop query is terminal, not interpreted as an
        // unfrozen effect; no collision callbacks may follow it.
        unsigned queries=0;
        assert(!scene->stepWithOwnedDebris(0,[](auto,const auto&,bool&){return false;},
            [&](auto,const auto&,auto,auto,bool&){++queries;return true;}));
        assert(!queries&&!scene->effectsSucceeded());
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        assert(scene->createGeneratedInstance({82,240,512},objects,[](const auto&){return false;}));
        InstanceHandle player;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::player,scene->playerHandle(),player));
        const auto before=*collision->state(player);
        unsigned hits=0,misses=0;
        for(int facing:{-1,1}){
            auto binding=collision->playerBinding();
            const auto* record=scene->findPlayer(scene->playerHandle());assert(record);
            assert(binding.integerTransform(scene->playerHandle(),*record,RunnerTransformField::scaleX,facing,{0x3f800000u},0xffffffu));
            for(int qx=640;qx<=1280;qx+=7){
                auto x=MotionBinary64::fromInteger(qx);x.high-=2u<<20; // quarter-pixel inputs
                bool hit=false;
                assert(collision->playerTimeFieldMeeting(x,MotionBinary64::fromInteger(512),masks,hit));
                hit?++hits:++misses;
                const auto state=collision->state(player);assert(state);
                assert(state->x.bits==before.x.bits&&state->y.bits==before.y.bits&&
                    state->previous.x.bits==before.previous.x.bits&&state->previous.y.bits==before.previous.y.bits);
            }
            bool hit=false;
            assert(collision->playerTimeFieldMeeting(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),masks,hit)&&hit);
            assert(collision->playerTimeFieldMeeting(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(10000),masks,hit)&&!hit);
        }
        assert(hits&&misses);
        GameplayCollisionOwner::RegisteredFieldContact selected;
        assert(collision->playerTimeFieldSelection(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),masks,selected));
        assert(selected.found&&selected.x.bits==0x43700000u&&selected.y.bits==0x44000000u);
        const auto first=selected.handle;
        assert(scene->createGeneratedInstance({82,241,512},objects,[](const auto&){return false;}));
        assert(collision->playerTimeFieldSelection(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),masks,selected));
        assert(selected.found&&selected.handle.slot==first.slot&&selected.handle.generation==first.generation);
        const auto fieldBinding=scene->timeFields().lifecycle();
        const auto* firstField=scene->timeFields().at(0);assert(firstField);
        assert(fieldBinding.activation({0,scene->timeFields().identityAt(0)},*firstField,false)&&collision->reconcileActivation());
        assert(collision->playerTimeFieldSelection(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512),masks,selected));
        assert(selected.found&&selected.handle.slot!=first.slot&&selected.x.bits==0x43710000u);
        assert(collision->playerTimeFieldSelection(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(10000),masks,selected));
        assert(!selected.found&&!selected.handle.generation&&selected.x.bits==0&&selected.y.bits==0);
        SpriteMaskPack absent(nullptr,0);bool sentinel=true;
        assert(!collision->playerTimeFieldMeeting({},{},absent,sentinel)&&sentinel&&!collision->valid());
    }
    for(int area=1;area<=5;++area){
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        DebrisEffects<2> effects;auto& registry=collision->debris();InstanceHandle local,key;
        const MotionBinary64 x{0x406e0800u,0},y{0x40660100u,0};
        auto expectedRandom=scene->randomState();DebrisAnimation expected;
        assert(expected.create(x,y,area,expectedRandom));
        assert(scene->spawnDebris(effects,x,y,area,local));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,local,key));
        GameplayCollisionView::Kind kind;InstanceHandle decoded;
        assert(GameplayCollisionView::decode(key,kind,decoded)&&kind==GameplayCollisionView::Kind::debris&&
            decoded.slot==local.slot&&decoded.generation==local.generation);
        const auto* record=registry.find(local);assert(record);
        const auto state=collision->state(key);const auto properties=collision->properties(key);
        RunnerImageValue depth;assert(depth.assign(MotionBinary64::fromInteger(-1000)));
        assert(state&&state->x.bits==expected.x.bits&&state->y.bits==expected.y.bits&&
            properties&&properties->depth.bits==depth.bits&&properties->sprite.spriteId==expected.spriteId&&
            properties->sprite.imageIndex.bits==expected.imageIndex.bits);
        assert(scene->randomState().drawCount()==expectedRandom.drawCount());
        assert(scene->randomState().stateIndex()==expectedRandom.stateIndex());
        for(unsigned word=0;word<16;++word)assert(scene->randomState().stateWord(word)==expectedRandom.stateWord(word));
        unsigned count=0;assert(collision->population(370,count)&&count==1);
        assert(expected.advanceImage());scene->step(0);
        assert(scene->effectsSucceeded()&&record->effect->imageIndex.bits==expected.imageIndex.bits&&
            collision->properties(key)->sprite.imageIndex.bits==expected.imageIndex.bits);
        assert(collision->state(key)->previous.x.bits==expected.x.bits);
        unsigned writes=0;
        assert(effects.stepRegisteredOne(local.generation,effects.beginPhase(),expectedRandom,
            [](auto,const auto&,bool& stopped){stopped=false;return true;},
            [&](auto,const auto& effect,auto,auto,bool& hit){
                assert(collision->state(key)->x.bits==effect.x.bits&&collision->state(key)->y.bits==effect.y.bits);
                hit=false;return true;
            },[&](auto h,const auto& effect,auto px,auto py){++writes;return registry.move(h,effect,px,py);},registry));
        assert(writes==2&&collision->state(key)->x.bits==record->effect->x.bits&&
            collision->state(key)->y.bits==record->effect->y.bits);
        effects.get(local.generation)->alarms.set(0,1);
        assert(effects.advanceRegisteredAlarmOne(local.generation,effects.beginPhase(),0,registry));
        assert(collision->state(key)->flags&1u);
        assert(registry.find(local)==record&&effects.retained(local.generation)==record->effect);
        assert(collision->prepareDraw()&&!collision->state(key)&&!registry.find(local)&&registry.size()==0);
        assert(collision->population(370,count)&&count==0);
        InstanceHandle next;assert(scene->spawnDebris(effects,x,y,area,next)&&next.generation>local.generation);
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none&&
            !registry.size()&&!effects.size()&&scene->debrisInstances()==&registry&&collision->valid());
        InstanceHandle renewed;assert(scene->spawnDebris(effects,x,y,area,renewed)&&renewed.generation>next.generation);
        scene->step(0);assert(scene->effectsSucceeded()&&effects.clear(registry));
    }
    {
        auto collision=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,collision->enemyBinding(),
            collision->breakableBinding(),collision->playerBinding(),&collision->terrain());
        assert(collision->attach(*scene));
        ScriptSpawnEffects<2> effects;auto& registration=collision->scriptEffects();
        const auto phase=effects.beginPhase();
        InstanceHandle caller;
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision resource;
            if(!sprites.sprite(id,resource))return false;count=resource.frameCount;return true;};
        assert(scene->spawnScriptEffect(effects,{0x406e0800u,0},{0x40afff00u,0},603,
            {0x3fe00000u,0},MotionBinary64::fromInteger(270),{},frames,caller));
        const auto first=caller.generation;
        assert(first&&registration.size()==1);
        InstanceHandle key;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::scriptEffect,registration.handle(first),key));
        const auto* effect=effects.get(first);const auto state=collision->state(key);
        assert(effect&&state&&state->x.bits==effect->x.bits&&state->y.bits==effect->y.bits);
        assert(collision->properties(key)->sprite.spriteId==603&&collision->properties(key)->sprite.angle.bits==0x43870000u);
        unsigned count=0;assert(collision->population(219,count)&&count==1);
        assert(scene->advanceScriptEffectAnimation(effects,phase)&&effects.get(first)->imageIndex.bits==0);
        assert(scene->stepWithScriptEffects(effects,0));
        assert(collision->properties(key)->sprite.imageIndex.bits==0x3f000000u);
        assert(collision->state(key)->previous.x.bits==effect->x.bits&&collision->state(key)->previous.y.bits==effect->y.bits);
        const auto second=effects.create({}, {},registration);assert(second>first);
        assert(effects.setSprite(second,602,1,registration));
        assert(effects.mutate(second,[](auto& e){e.imageSpeed={0x3f000000u};return true;}));
        assert(scene->stepWithScriptEffects(effects,0)&&collision->state(key)&&
            (collision->state(key)->flags&1u)&&registration.size()==2&&collision->properties(key)->sprite.imageIndex.bits==0);
        assert(effects.get(second)&&effects.get(second)->imageIndex.bits==0x3f000000u);
        assert(scene->stepWithScriptEffects(effects,0)&&!effects.get(second)&&registration.size()==2);
        assert(collision->prepareDraw(effects));
        assert(!collision->state(key)&&registration.size()==0);
        assert(collision->population(219,count)&&count==0);
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none&&
            scene->scriptSpawnInstances()==&registration);
        const auto next=effects.create({}, {},registration);assert(next>first&&!collision->state(key));
        assert(effects.setSprite(next,602,1,registration));
        const auto oldHandle=registration.handle(next);
        assert(scene->restartFromResultsWithScriptEffects(DeathMenuSequence::Action::None,43,300,effects)==ResultsDestination::none&&
            registration.find(oldHandle)&&effects.size()==1&&scene->effectsSucceeded());
        assert(scene->restartFromResultsWithScriptEffects(DeathMenuSequence::Action::Retry,43,300,effects)!=ResultsDestination::none&&
            effects.size()==0&&registration.size()==0&&!registration.find(oldHandle)&&collision->valid());
        const auto renewed=effects.create({}, {},registration);
        assert(renewed>next&&!registration.find(oldHandle)&&effects.clear(registration));
        assert(scene->spawnScriptEffect(effects,{},{},603,{0x3fe00000u,0},{},{},frames,caller));
        const auto automatic=caller;
        scene->step(0);
        assert(scene->effectsSucceeded()&&effects.get(automatic.generation)->imageIndex.bits==0x3f000000u);
        assert(collision->prepareDraw());
        scene->step(0);
        assert(scene->effectsSucceeded()&&collision->prepareDraw()&&effects.size()==0&&!registration.find(automatic));
        assert(scene->spawnScriptEffect(effects,{},{},603,{0x3fe00000u,0},{},{},frames,caller));
        const auto beforeRetry=caller;
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,44,300)!=ResultsDestination::none&&
            effects.size()==0&&!registration.find(beforeRetry)&&collision->valid());
        // Failure after Create must poison the actual runtime, retain its
        // completed birth/caller publication and never repeat the event.
        const auto draws=scene->randomDrawCount();unsigned lookups=0;
        auto rejectFrames=[&](int,unsigned&){++lookups;return false;};
        assert(!scene->spawnScriptEffect(effects,{},{},603,{0x3fe00000u,0},{},{},rejectFrames,caller));
        assert(!scene->effectsSucceeded()&&registration.find(caller)&&effects.size()==1&&lookups==1);
        assert(!scene->spawnScriptEffect(effects,{},{},603,{},{},{},rejectFrames,caller)&&lookups==1&&
            scene->randomDrawCount()==draws);
        assert(!scene->advanceScriptEffectAnimation(effects,effects.beginPhase())&&effects.size()==1);
        assert(scene->restartFromResultsWithScriptEffects(DeathMenuSequence::Action::Retry,43,300,effects)==ResultsDestination::none&&
            effects.size()==1&&registration.find(caller)&&scene->randomDrawCount()==draws);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        InstanceHandle previous{};
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision r;if(!sprites.sprite(id,r))return false;
            count=r.frameCount;return true;};
        for(unsigned cycle=0;cycle<3;++cycle){
            InstanceHandle caller;
            assert(world->spawnScriptEffect({},{},603,{0x3fe00000u,0},{},{},frames,caller)&&
                caller.generation>previous.generation&&!owner->scriptEffects().find(previous));
            world->step(0);
            assert(world->effectsSucceeded()&&world->scriptEffects().size()==1&&
                world->scriptEffects().atNewest(0)->imageIndex.bits==0x3f000000u&&owner->prepareDraw());
            world->step(0);
            assert(world->effectsSucceeded()&&owner->prepareDraw()&&world->scriptEffects().size()==0);
            assert(world->spawnScriptEffect({},{},602,{0x3fe00000u,0},{},{},frames,caller));previous=caller;
            assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43+cycle,300)!=ResultsDestination::none&&
                world->scriptEffects().size()==0&&!owner->scriptEffects().find(previous)&&world->hasRoomActorDraw());
            assert(world->visitRoomActors([](auto kind,auto,auto){return kind!=GameplayWorld::DrawActor::scriptEffect;}));
        }
    }
    assert(masks.valid());
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));world->step(4);
        assert(world->effectsSucceeded()&&world->bulletCount()==1);
        const auto& shot=world->bulletAt(0);const InstanceHandle key{shot.identitySlot,shot.lifetimeId};
        InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,key,shared));
        const auto original=*owner->state(shared);const auto trajectory=shot.trajectory;
        MachineGunSourceMotion motion;GameMakerRandom random(2);
        assert(motion.reset(original.x.promoted(),original.y.promoted())&&motion.initialize(random));
        assert(world->markBulletDestroyed(key));unsigned writes=0;
        assert(motion.commitPosition([&](RunnerImageValue x,RunnerImageValue y){
            assert(writes==0?y.bits==original.y.bits:x.bits==motion.x.bits);
            assert(world->publishBulletCollisionPosition(key,x,y));
            const auto state=owner->state(shared);
            assert(state&&state->x.bits==x.bits&&state->y.bits==y.bits&&(state->flags&1u));
            assert(state->previous.x.bits==original.previous.x.bits&&state->previous.y.bits==original.previous.y.bits);
            ++writes;return true;
        }));
        assert(writes==2&&shot.destroyRequested&&shot.trajectory.x.pixel==trajectory.x.pixel&&
            shot.trajectory.x.fraction==trajectory.x.fraction&&shot.trajectory.y.pixel==trajectory.y.pixel&&
            shot.trajectory.y.fraction==trajectory.y.fraction);
        const auto last=*owner->state(shared);
        assert(!world->publishBulletCollisionPosition(key,{0x7f800000u},{})&&!world->effectsSucceeded());
        assert(owner->state(shared)->x.bits==last.x.bits&&owner->state(shared)->y.bits==last.y.bits);
    }
    for(bool high:{false,true})for(bool laser:{false,true}){
        // Exercise the recovered Step against real resource-backed selectors
        // and box writes, using a real fired lifetime for setter/destruction
        // callbacks. The supplied profile and isolated position are fixture
        // inputs, not a claim about natural gem-high activation/scheduling.
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));world->step(4);
        assert(world->effectsSucceeded()&&world->bulletCount()==1);
        const auto& shot=world->bulletAt(0);const InstanceHandle shotKey{shot.identitySlot,shot.lifetimeId};
        InstanceHandle shotShared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,shotKey,shotShared));
        assert(world->createGeneratedInstance({92,240,512},objects,[](const auto&){return true;}));
        MachineGunSourceMotion motion;GameMakerRandom random(2);
        assert(motion.reset(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(500),laser));
        assert(world->publishBulletCollisionPosition(shotKey,motion.x,motion.y));
        ScriptSpawnEffects<4> effects;bool succeeded=true;unsigned child=~0u;
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision art;
            if(!sprites.sprite(id,art))return false;count=art.frameCount;return true;};
        auto muzzle=scriptSpawnOperations(effects,owner->scriptEffects(),frames,
            [&](unsigned id){child=id;return true;},succeeded);
        struct Ops {
            GameplayCollisionOwner& owner;MachineGunSourceMotion& motion;const SpriteMaskPack& masks;
            GameplayWorld& world;InstanceHandle shotKey,shotShared;
            InstanceHandle selected{};unsigned lines=0,points=0,writes=0,approachWrites=0;
            bool destroyed=false,approaching=false;
            bool publishAngle(RunnerImageValue angle){
                return angle.bits==motion.imageAngle.bits&&world.publishBulletAngle(shotKey,angle)&&
                    owner.properties(shotShared)->sprite.angle.bits==angle.bits;
            }
            bool publishPosition(RunnerImageValue x,RunnerImageValue y){
                if(approaching){
                    assert((approachWrites%2==0?x.bits==motion.x.bits:y.bits==motion.y.bits));
                    ++approachWrites;
                }else{
                    assert(destroyed&&(writes%2==0?y.bits==motion.y.bits:x.bits==motion.x.bits));
                    ++writes;
                }
                if(!world.publishBulletCollisionPosition(shotKey,x,y))return false;
                const auto state=owner.state(shotShared);
                return state&&state->x.bits==x.bits&&state->y.bits==y.bits;
            }
            bool query(int object,GameplayCollisionOwner::Owner::Result& result){
                MotionBinary64 x,y;if(!motion.endpoint(x,y))return false;++lines;
                return owner.projectileNonPreciseLine(shotShared,object,motion.x.promoted(),motion.y.promoted(),x,y,false,result);
            }
            bool line(int object,bool& hit){GameplayCollisionOwner::Owner::Result result;
                if(!query(object,result))return false;hit=result.found;return true;}
            bool selectWall(){GameplayCollisionOwner::Owner::Result result;
                if(!query(59,result)||!result.found)return false;selected=result.handle;return true;}
            bool approachSolid(){approaching=true;unsigned iterations=0;
                const bool ok=motion.approachSolid([&](auto x,auto y,bool& hit){
                    ++points;GameplayCollisionOwner::Owner::Result result;
                    if(!owner.projectileSolidPoint(shotShared,x,y,false,masks,result))return false;
                    hit=result.found;return true;
                },[&](auto x,auto y){return publishPosition(x,y);},iterations);
                approaching=false;return ok;
            }
            bool wallParent(int& parent){return owner.projectileWallParent(selected,parent);}
            bool wallHealth(MotionBinary64& hp){return owner.projectileBoxHealth(selected,hp);}
            bool wallHealthWrite(MotionBinary64 hp){return owner.projectileBoxHealthWrite(selected,hp);}
            bool wallHit(){return owner.projectileBoxHit(selected);}
            bool destroySelf(){destroyed=world.markBulletDestroyed(shotKey);return destroyed;}
            bool effect(int,MotionBinary64,MotionBinary64){return false;}
            bool soundWall(){return false;}bool explosion(int){return false;}
            bool placeShootable(int,bool&){return false;}bool selectShootable(int){return false;}
            bool thingHealth(MotionBinary64&){return false;}bool thingHealthWrite(MotionBinary64){return false;}
            bool thingHit(){return false;}bool thingHitEffect(bool&){return false;}
        } ops{*owner,motion,masks,*world,shotKey,shotShared};
        assert(MachineGunSourceStep::run(motion,random,high,muzzle,ops,succeeded));
        assert(succeeded&&ops.destroyed&&ops.lines==3&&ops.points>1&&ops.approachWrites==2*(ops.points-1)&&ops.writes==2);
        assert(random.drawCount()==1&&child!=~0u&&owner->scriptEffects().size()==1);
        const auto* box=world->findBreakable({0,1});assert(box);
        const auto expected=MotionBinary64::fromInteger(1-machineGunProfile(high).damage);
        assert(box->state.wallHp.high==expected.high&&box->state.wallHp.low==expected.low&&box->state.hit);
        assert(box->active&&!box->destroyRequested); // Its Step, not bullet damage, owns destruction effects.
        GameplayCollisionOwner::Owner::Result selected;
        const auto x=MotionBinary64::fromInteger(240),y=MotionBinary64::fromInteger(512);
        assert(owner->projectileSolidPoint({},x,y,false,masks,selected)&&selected.found&&
            selected.handle.slot==ops.selected.slot&&selected.handle.generation==ops.selected.generation);
        assert(world->resolveBulletLifetime(shotKey)&&world->resolveBulletLifetime(shotKey)->destroyRequested);
        std::printf("SOURCE_SHOT %u %u %08x %08x %08x %08x %08x %u %u %u %u\n",
            unsigned(high),unsigned(laser),unsigned(motion.x.bits),unsigned(motion.y.bits),unsigned(motion.imageAngle.bits),
            unsigned(box->state.wallHp.high),unsigned(box->state.wallHp.low),ops.lines,ops.points,ops.approachWrites,ops.writes);
        assert(owner->prepareDraw(effects)&&!world->resolveBulletLifetime(shotKey)&&!owner->state(shotShared));
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        DeferredCreateInstance box{92,240,512};box.setBoxCreate(174,2);
        box.imageSpeed=0;box.imageIndex=22;box.integerImageIndex=22;
        const auto draws=world->randomDrawCount();
        assert(world->createGeneratedInstance(box,objects,[](const auto&){return true;})&&world->randomDrawCount()==draws);
        InstanceStore<1> stored;DeferredInstanceQueue<1> queue;std::uint32_t next=100000;
        assert(queue.enqueueCreate(box)&&queue.applyChecked(stored,objects,11,next,[](auto&){return true;})==ApplyInstanceCommandsResult::success);
        const auto* native=world->findBreakable({0,1});assert(native);
        const auto& a=native->state;const auto& b=stored.at(0).box;
        assert(a.wallHp.high==0x3ff00000u&&!a.wallHp.low&&a.wallHp.high==b.wallHp.high&&
            a.breakSound==174&&a.breakSound==b.breakSound&&a.haveGem==2&&a.haveGem==b.haveGem&&
            !a.hit&&!b.hit&&!a.drilled&&!b.drilled&&!a.autoTile&&!b.autoTile&&!a.checked&&!b.checked);
        InstanceHandle selected;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,{0,1},selected));
        GameplayCollisionOwner::Owner::Result line;
        const auto xroom=MotionBinary64::fromInteger(240),yroom=MotionBinary64::fromInteger(512);
        assert(owner->projectileNonPreciseLine({},59,xroom,yroom,xroom,yroom,false,line)&&line.found&&
            line.handle.slot==selected.slot&&line.handle.generation==selected.generation);
        selected=line.handle;
        int parent=-123;MotionBinary64 hp;
        assert(owner->projectileWallParent(selected,parent)&&parent==84&&owner->projectileBoxHealth(selected,hp));
        const auto flags=owner->state(selected)->flags;
        const MotionBinary64 fractional{0xbfe00000u,0}; // -0.5, never clamped to zero.
        assert(owner->projectileBoxHealthWrite(selected,fractional)&&owner->projectileBoxHit(selected)&&
            owner->projectileBoxHealth(selected,hp)&&hp.high==fractional.high&&!hp.low&&native->state.hit&&
            native->active&&!native->destroyRequested&&owner->state(selected)->flags==flags&&
            world->randomDrawCount()==draws);
        BoxDestructionScript::Caller caller;
        RunnerImageValue preciseX{0x43704000u},preciseY{0x44000800u};
        assert(owner->breakableBinding().publishedPosition({0,1},*native,preciseX,preciseY)&&
            owner->readBoxDestruction(selected,caller)&&caller.x.high==preciseX.promoted().high&&
            caller.x.low==preciseX.promoted().low&&caller.y.high==preciseY.promoted().high&&
            caller.y.low==preciseY.promoted().low&&caller.state.wallHp.high==fractional.high&&caller.state.hit);
        assert(world->markBreakableDestroyed({0,1})&&owner->projectileBoxHealthWrite(selected,{})&&
            owner->projectileBoxHit(selected)&&owner->readBoxDestruction(selected,caller)&&caller.state.wallHp.zero());
        assert(owner->prepareDraw()&&!owner->readBoxDestruction(selected,caller)&&caller.state.wallHp.zero()&&
            caller.x.high==preciseX.promoted().high&&caller.x.low==preciseX.promoted().low);
        hp={0x3ff00000u,0};parent=-123;
        assert(!owner->projectileBoxHealth(selected,hp)&&hp.high==0x3ff00000u&&
            !owner->projectileWallParent(selected,parent)&&parent==-123&&
            !owner->projectileBoxHealthWrite(selected,fractional)&&!owner->projectileBoxHit(selected));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)!=ResultsDestination::none);
        const int x=0,y=336;assert(world->appendBreakables(&x,&y,1));
        const auto* fresh=world->findBreakable({0,2});
        assert(fresh&&fresh->state.breakSound==171&&!fresh->state.haveGem&&!fresh->state.hit&&
            fresh->state.wallHp.high==0x3ff00000u);
        assert(!owner->projectileBoxHealthWrite(selected,fractional)&&!fresh->state.hit);
        InstanceHandle replacement;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::breakable,{0,2},replacement));
        assert(!owner->projectileBoxHealthWrite(replacement,{0x7ff00000u,0})&&
            !owner->projectileBoxHit(replacement)&&!fresh->state.hit&&fresh->state.wallHp.high==0x3ff00000u);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*scene));
        assert(scene->createGeneratedInstance({60,240,512},objects,[](const auto&){return true;}));
        assert(scene->createGeneratedInstance({92,240,512},objects,[](const auto&){return true;}));
        assert(scene->configureEnemy(0,112,163,0,336,0,0,5,14,0,0,-1,-1,0,0,true,true,true));
        const auto enemy=scene->enemyHandle(scene->enemyAt(0).lifetimeId);
        InstanceHandle enemyShared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::enemy,enemy,enemyShared));
        auto n=[](int v){return MotionBinary64::fromInteger(v);};
        auto query=[&](int object,InstanceHandle self={},bool notMe=false){
            GameplayCollisionOwner::Owner::Result selected;
            assert(owner->projectileNonPreciseLine(self,object,n(240),n(496),n(240),n(528),notMe,selected));
            return selected;
        };
        const auto wall=query(59),actor=query(83);
        GameplayCollisionOwner::Owner::Result point;
        assert(owner->projectileSolidPoint({},n(240),n(512),false,masks,point)&&point.found);
        const auto firstPoint=point.handle;
        assert(owner->projectileSolidPoint(firstPoint,n(240),n(512),true,masks,point)&&
            point.found&&point.handle.slot!=firstPoint.slot);
        assert(wall.found&&actor.found&&actor.handle.slot==enemyShared.slot&&actor.handle.generation==enemyShared.generation);
        const auto excluded=query(83,enemyShared,true);assert(!excluded.found);
        assert(query(83,enemyShared,false).found); // VM uses notme=false.
        const auto* liveEnemy=scene->resolveEnemyLifetime(enemy);assert(liveEnemy);
        assert(owner->enemyBinding().activation(enemy,*liveEnemy,false)&&!query(83).found);
        assert(owner->reconcileActivation()&&!query(83).found);
        assert(owner->enemyBinding().activation(enemy,*liveEnemy,true)&&owner->reconcileActivation()&&query(83).found);
        const auto otherSolid=query(59,wall.handle,true);
        assert(otherSolid.found&&otherSolid.handle.slot!=wall.handle.slot);
        GameplayCollisionView::Kind firstKind,otherKind;InstanceHandle firstLocal,otherLocal;
        assert(GameplayCollisionView::decode(wall.handle,firstKind,firstLocal)&&
            GameplayCollisionView::decode(otherSolid.handle,otherKind,otherLocal)&&
            ((firstKind==GameplayCollisionView::Kind::terrain&&otherKind==GameplayCollisionView::Kind::breakable)||
             (otherKind==GameplayCollisionView::Kind::terrain&&firstKind==GameplayCollisionView::Kind::breakable)));
        // The enemy guard must not accidentally return the overlapping solid.
        assert(scene->markEnemyDestroyed(enemy)&&!query(83).found&&query(59).found);
        assert(owner->prepareDraw()&&!scene->resolveEnemyLifetime(enemy));
        assert(scene->configureEnemy(0,112,163,0,336,0,0,5,14));
        const auto replacement=query(83);
        assert(replacement.found&&replacement.handle.generation!=enemyShared.generation);
        assert(scene->restartFromResults(DeathMenuSequence::Action::Retry,43,300)!=ResultsDestination::none);
        assert(!query(59).found&&!query(83).found&&owner->valid());
        GameplayCollisionOwner::Owner::Result sentinel;sentinel.found=true;sentinel.handle=enemyShared;
        assert(!owner->projectileNonPreciseLine({},90,n(240),n(496),n(240),n(528),false,sentinel)&&
            !owner->valid()&&sentinel.found&&sentinel.handle.generation==enemyShared.generation);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        for(int object:{112,92,60,112})
            assert(world->createGeneratedInstance({object,240,512},objects,[](const auto&){return true;}));
        const auto first=world->enemyHandle(world->enemyAt(0).lifetimeId);
        const auto survivor=world->enemyHandle(world->enemyAt(1).lifetimeId);
        assert(!world->resolveEnemyLifetime({GameplayWorld::enemyCapacity,1})&&
            !world->resolveEnemyLifetime({first.slot,0})&&
            !world->resolveEnemyLifetime({first.slot,first.generation+100}));
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
        const auto middle=owner->terrain().handle(4);const auto record=owner->terrain().find(middle);
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
        assert(random.drawCount()==draws&&owner->terrain().find(middle)->destroyRequested==!rejectTile&&owner->terrain().find(middle)==record);
        if(!rejectTile)assert(drawWallPresentation(*owner->terrain().find(middle),[]{assert(false);return 0u;},
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
            bool expected=false,boxHit=false;
            for(unsigned i=0;i<3;++i){
                const auto local=queries->terrain().handle(i);const auto r=queries->terrain().find(local);
                InstanceHandle key;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,key));
                const auto appearance=queries->properties(key);
                const auto& p=appearance->sprite;PackedSpriteCollision resource;
                assert(sprites.collisionSprite(p.spriteId,p.maskId,resource));
                const int px=x-r->x+resource.originX,py=y-r->y+resource.originY;
                if(px<resource.marginLeft||px>resource.marginRight||py<resource.marginTop||py>resource.marginBottom)continue;
                boxHit=true;
                if(!resource.collisionMode||!resource.collisionMaskCount){expected=true;break;}
                SpriteMaskPack::Mask mask;assert(masks.mask(unsigned(p.maskId<0?p.spriteId:p.maskId),0,mask));
                if(mask.contains(px,py)){expected=true;break;}
            }
            bool hit=!expected;
            const bool ok=queries->terrainPositionMeeting(60,MotionBinary64::fromInteger(x),MotionBinary64::fromInteger(y),masks,hit);
            if(!ok||hit!=expected)std::fprintf(stderr,"native point %d,%d: ok=%d hit=%d expected=%d\n",x,y,ok,hit,expected);
            assert(ok&&hit==expected);
            const auto px=MotionBinary64::fromInteger(x),py=MotionBinary64::fromInteger(y);
            GameplayCollisionOwner::Owner::Result selected;
            assert(queries->terrainCollisionPoint({},60,px,py,true,false,masks,selected)&&selected.found==expected);
            assert(queries->terrainCollisionPoint({},60,px,py,false,false,masks,selected)&&selected.found==boxHit);
            assert(queries->terrainNonPreciseLine({},60,px,py,px,py,false,selected)&&selected.found==boxHit);
            if(selected.found){
                GameplayCollisionView::Kind kind;InstanceHandle local;
                assert(GameplayCollisionView::decode(selected.handle,kind,local)&&
                    kind==GameplayCollisionView::Kind::terrain&&queries->terrain().find(local));
                const auto excluded=selected.handle;
                assert(queries->terrainNonPreciseLine(excluded,60,px,py,px,py,true,selected));
                assert(!selected.found||selected.handle.slot!=excluded.slot||selected.handle.generation!=excluded.generation);
            }
        }
        for(auto p:{DeferredCreateInstance{0,256,512},DeferredCreateInstance{0,240,528},DeferredCreateInstance{1,240,512},DeferredCreateInstance{1,224,512}}){
            bool hit=false;
            assert(queries->terrainPlaceMeeting(self,60,MotionBinary64::fromInteger(p.x),MotionBinary64::fromInteger(p.y),masks,hit)&&hit==(p.objectId==0));
            const auto restored=queries->state(shared);
            assert(restored&&restored->x.bits==before.x.bits&&restored->y.bits==before.y.bits);
            assert(restored->previous.x.bits==before.previous.x.bits&&restored->previous.y.bits==before.previous.y.bits);
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
        assert(scene->createGeneratedInstance({93,432,800},objects,[](const auto&){return true;}));
        // All supported terrain identities can be the temporary query caller.
        // A fractional query must neither move its authoritative record nor
        // overwrite the pre-animation snapshot, on hits or misses. Repeated
        // queries also exercise flushing the preceding restoration's queues.
        for(auto caller:{self,marker,queries->terrain().handle(4)}){
            InstanceHandle callerKey;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,caller,callerKey));
            const auto original=*queries->state(callerKey);
            const auto record=*queries->terrain().find(caller);
            for(int repeat=0;repeat<3;++repeat)for(int targetX:{240,256,400,432,1000}){
                auto x=MotionBinary64::fromInteger(targetX);
                auto y=MotionBinary64::fromInteger(targetX<400?512:800);
                assert(x.add({0x3fd00000u,0})&&y.add({0xbfd00000u,0})); // +1/4, -1/4
                bool found=false;
                assert(queries->terrainPlaceMeeting(caller,60,x,y,masks,found));
                const auto restored=queries->state(callerKey);
                const auto terrain=queries->terrain().find(caller);
                assert(restored&&terrain&&restored->x.bits==original.x.bits&&restored->y.bits==original.y.bits);
                assert(restored->previous.x.bits==original.previous.x.bits&&restored->previous.y.bits==original.previous.y.bits);
                assert(terrain->x==record.x&&terrain->y==record.y&&terrain->lifetimeId==record.lifetimeId);
            }
        }
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
    {
        auto layers=std::make_unique<GameplayRoomLayers<8>>(&rooms);bool healthy=true;
        layers->observeFailure(healthy);assert(healthy&&layers->prepare());
        // The nested lifetime owner, not GameplayRoomLayers' own flag, fails.
        assert(!layers->writeDepth({0,1},MotionBinary64::fromInteger(2)));
        assert(!healthy&&!layers->valid());
    }
    if(const char* path=std::getenv("DOWNWELL_OWNERSHIP_WITNESS")){
        // Compare the whole owner, not just a membership container. Emit full
        // field values (not a hash) so a divergent publication is inspectable.
        std::ofstream trace(path);assert(trace);
        // Feed the real builder's borrowed spans through the core transaction.
        // The backend can fail at any cell, but may not mutate core ownership
        // inside this trusted run. Queries follow the run, never prewarm it.
        auto pb=read("generated/downwell-builder-patterns.dwbp");
        BuilderPatternPack patterns(pb.data(),pb.size());assert(patterns.valid());
        for(unsigned seed:{1u,5u,9u})for(unsigned reject:{0u,1u,3u,9u,41u}){
            auto runOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
                GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
            auto runWorld=std::make_unique<GameplayWorld>(300,true,0,false,runOwner->enemyBinding(),
                runOwner->breakableBinding(),runOwner->playerBinding(),&runOwner->terrain());
            assert(runOwner->attach(*runWorld));
            GameMakerRandom random(seed);AreaOneBuilder builder;
            builder.reset(240,704,random,{1,0,true,false});builder.enableTerrainRuns(true);
            struct RunContext{GameplayWorld& world;const ObjectPack& objects;unsigned calls=0,reject=0,runs=0;};
            RunContext context{*runWorld,objects,0,reject};
            const auto emit=[](void* opaque,const DeferredCreateInstance& command){
                auto& c=*static_cast<RunContext*>(opaque);
                if(command.objectId!=60&&command.objectId!=93&&command.objectId!=99)return true;
                return c.world.createGeneratedTerrain(command,[&](const auto&){return ++c.calls!=c.reject;});
            };
            builder.setTerrainRunEmitter([](void* opaque,const AreaOneBuilder::TerrainRun& run,AreaOneBuilder::EmitFunction){
                auto& c=*static_cast<RunContext*>(opaque);++c.runs;
                return c.world.createGeneratedTerrainRun(run,[&](const auto&){return ++c.calls!=c.reject;});
            });
            AreaOneBuilderStep step;
            const bool okay=builder.stepVisible(patterns,random,100000,emit,&context,step);
            trace<<"builder-run "<<seed<<' '<<reject<<' '<<okay<<' '<<context.calls<<' '<<context.runs
                <<' '<<runOwner->valid()<<' '<<runWorld->effectsSucceeded()<<' '<<random.drawCount()<<' '<<random.stateIndex()<<'\n';
            for(unsigned i=0;i<16;++i)trace<<random.stateWord(i)<<' ';
            trace<<'\n';
            for(unsigned i=0;i<runOwner->terrain().size();++i){
                const auto h=runOwner->terrain().handle(i);const auto r=runOwner->terrain().find(h);
                InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,h,shared));
                const auto s=runOwner->state(shared);const auto p=runOwner->properties(shared);assert(s&&p);
                trace<<"run-cell "<<h.slot<<' '<<h.generation<<' '<<r->objectId<<' '<<r->x<<' '<<r->y
                    <<' '<<r->spriteId<<' '<<r->imageIndex<<' '<<r->imageSpeed<<' '<<s->x.bits<<' '<<s->y.bits
                    <<' '<<s->previous.x.bits<<' '<<s->previous.y.bits<<' '<<s->flags<<' '<<s->kind<<' '<<s->special
                    <<' '<<s->bounds.left.bits<<' '<<s->bounds.top.bits<<' '<<s->bounds.right.bits<<' '<<s->bounds.bottom.bits
                    <<' '<<p->sprite.spriteId<<' '<<p->sprite.maskId<<' '<<p->sprite.scaleX.bits<<' '<<p->sprite.scaleY.bits
                    <<' '<<p->sprite.angle.bits<<' '<<p->sprite.imageIndex.bits<<' '<<p->depth.bits<<' '<<p->timeline.bits
                    <<' '<<p->cachedFrame.bits<<'\n';
            }
            for(int object:{60,93}){unsigned count;assert(runOwner->population(object,count));
                trace<<"run-count "<<object<<' '<<count<<'\n';}
            for(unsigned i=0;i<runOwner->terrain().size();i+=7){
                const auto r=runOwner->terrain().find(runOwner->terrain().handle(i));
                const auto x=MotionBinary64::fromInteger(r->x),y=MotionBinary64::fromInteger(r->y);
                GameplayCollisionOwner::Owner::Result result;
                assert(runOwner->terrainNonPreciseLine({},60,x,y,x,y,false,result));
                trace<<"run-query "<<result.found<<' '<<result.handle.slot<<' '<<result.handle.generation<<'\n';
            }
            assert(runOwner->prepareDraw());
            assert(runWorld->visitRoomActors([&](auto kind,InstanceHandle h,auto depth){
                trace<<"run-draw "<<unsigned(kind)<<' '<<h.slot<<' '<<h.generation<<' '<<depth.bits<<'\n';return true;
            }));
        }
        // Do not warm the representation by resolving every birth. A compact
        // terrain owner must survive a cold burst before its first observer,
        // including interleaved actors, coincident geometry and removed cells.
        for(unsigned mode=0;mode<4;++mode){
            auto cold=std::make_unique<GameplayCollisionOwner>(objects,sprites,
                GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
            auto scene=std::make_unique<GameplayWorld>(300,true,0,false,cold->enemyBinding(),
                cold->breakableBinding(),cold->playerBinding(),&cold->terrain());
            assert(cold->attach(*scene));
            for(unsigned i=0;i<128;++i){
                // Each group contains a coincident wall pair. Preserve first
                // identity, not merely the set of occupied cells.
                const int objectsByCell[]={60,60,93,99};
                const int x=240+int((i/4)%8)*16,y=512+int(i/32)*16;
                assert(scene->createGeneratedInstance({objectsByCell[i%4],x,y},objects,
                    [](const auto&){return true;}));
                if(i%17==0)assert(scene->createGeneratedInstance({112,x,y-32},objects,
                    [](const auto&){return true;}));
                auto random=scene->randomState();random.nextU32();scene->configureEnemyRandom(random);
            }
            for(unsigned i=0;i<128;i+=8){
                const auto h=cold->terrain().handle(i);
                if(mode==1||mode==2)assert(cold->terrain().activation(h,false));
                if(mode==3)assert(cold->terrain().requestDestroy(h));
            }
            if(mode==2)assert(cold->reconcileActivation());
            const auto random=scene->randomState();
            trace<<"cold "<<mode<<' '<<random.drawCount()<<' '<<random.stateIndex()<<'\n';
            for(unsigned probe=0;probe<32;++probe){
                const auto x=MotionBinary64::fromInteger(240+int(probe%8)*16);
                const auto y=MotionBinary64::fromInteger(512+int(probe/8)*16);
                GameplayCollisionOwner::Owner::Result hit;
                auto record=[&](const char* kind){
                    trace<<kind<<' '<<probe<<' '<<hit.found<<' '<<hit.handle.slot<<' '<<hit.handle.generation<<'\n';
                    if(hit.found){
                        GameplayCollisionView::Kind family;InstanceHandle local;
                        assert(GameplayCollisionView::decode(hit.handle,family,local)&&
                            family==GameplayCollisionView::Kind::terrain&&cold->terrain().find(local));
                    }
                };
                assert(cold->terrainCollisionPoint({},60,x,y,false,false,masks,hit));record("cold-point");
                const auto first=hit.handle;
                assert(cold->terrainCollisionPoint(first,60,x,y,false,true,masks,hit));record("cold-excluded");
                assert(!hit.found||hit.handle.slot!=first.slot||hit.handle.generation!=first.generation);
                assert(cold->terrainNonPreciseLine({},60,x,y,x,y,false,hit));record("cold-line");
            }
            for(unsigned i=0;i<128;++i){
                const auto h=cold->terrain().handle(i);const auto r=cold->terrain().find(h);assert(r);
                InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,h,shared));
                const auto before=*cold->state(shared);
                bool hit=false;
                assert(cold->terrainPlaceMeeting(h,60,MotionBinary64::fromInteger(r->x+16),
                    MotionBinary64::fromInteger(r->y),masks,hit));
                const auto after=cold->state(shared);assert(after);
                assert(before.x.bits==after->x.bits&&before.y.bits==after->y.bits&&
                    before.previous.x.bits==after->previous.x.bits&&before.previous.y.bits==after->previous.y.bits);
                trace<<"cold-restore "<<i<<' '<<hit<<' '<<after->x.bits<<' '<<after->y.bits
                    <<' '<<after->flags<<' '<<after->bounds.left.bits<<' '<<after->bounds.top.bits
                    <<' '<<after->bounds.right.bits<<' '<<after->bounds.bottom.bits<<'\n';
            }
            assert(scene->randomState().drawCount()==random.drawCount()&&
                scene->randomState().stateIndex()==random.stateIndex());
            for(unsigned i=0;i<16;++i){
                assert(scene->randomState().stateWord(i)==random.stateWord(i));
                trace<<"cold-rng "<<i<<' '<<random.stateWord(i)<<'\n';
            }
        }
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        unsigned checkpoint=0;
        auto capture=[&](){
            checkReadOnlyDraw(*world);
            trace<<"checkpoint "<<checkpoint++<<' '<<owner->valid()<<' '<<world->effectsSucceeded()<<'\n';
            const auto& random=world->randomState();
            trace<<"rng "<<random.drawCount()<<' '<<random.stateIndex();
            for(unsigned i=0;i<16;++i)trace<<' '<<random.stateWord(i);
            trace<<'\n';
            for(unsigned i=0;i<owner->terrain().size();++i){
                const auto h=owner->terrain().handle(i);const auto r=owner->terrain().find(h);assert(r);
                InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,h,shared));
                const auto s=owner->state(shared);const auto p=owner->properties(shared);assert(s&&p);
                trace<<"terrain "<<h.slot<<' '<<h.generation<<' '<<r->objectId<<' '<<r->x<<' '<<r->y
                    <<' '<<r->active<<' '<<r->sourceInstanceId<<' '<<r->creationCodeId<<' '<<r->spriteId
                    <<' '<<r->imageIndex<<' '<<r->imageSpeed<<' '<<r->destroyRequested<<'\n';
                trace<<"state "<<s->x.bits<<' '<<s->y.bits<<' '<<s->previous.x.bits<<' '<<s->previous.y.bits
                    <<' '<<s->bounds.left.bits<<' '<<s->bounds.top.bits<<' '<<s->bounds.right.bits
                    <<' '<<s->bounds.bottom.bits<<' '<<s->flags<<' '<<s->kind<<' '<<s->special<<'\n';
                trace<<"properties "<<p->sprite.spriteId<<' '<<p->sprite.maskId<<' '<<p->sprite.scaleX.bits
                    <<' '<<p->sprite.scaleY.bits<<' '<<p->sprite.angle.bits<<' '<<p->sprite.imageIndex.bits
                    <<' '<<p->depth.bits<<' '<<p->timeline.bits<<' '<<p->cachedFrame.bits<<'\n';
            }
            for(int object:{35,60,93,99,112}){unsigned count;assert(owner->population(object,count));
                trace<<"population "<<object<<' '<<count<<'\n';}
            RunnerStepSnapshot<256> phase;
            assert(owner->dispatchStepPhase(2,phase,[&](InstanceHandle h,const PackedObjectEvent& event){
                trace<<"event "<<h.slot<<' '<<h.generation<<' '<<event.ownerObjectId<<' '<<event.codeId<<'\n';
                return true;
            }));
            assert(world->visitRoomActors([&](auto kind,InstanceHandle h,auto depth){
                trace<<"draw "<<unsigned(kind)<<' '<<h.slot<<' '<<h.generation<<' '<<depth.bits<<'\n';return true;
            }));
        };
        capture();
        for(unsigned i=0;i<96;++i){
            const int kinds[]={60,60,93,99,112,60};
            DeferredCreateInstance next(kinds[i%6],240+int(i%8)*16,512+int(i/8)*16);
            next.active=i%7!=0;
            // The terrain sink precedes native ownership and Create. Capture
            // there as well as after the builder-facing call returns.
            assert(world->createGeneratedInstance(next,objects,[&](const auto&){capture();return true;}));
            capture();
            if(i%5==0&&owner->terrain().size()>2){
                const auto h=owner->terrain().handle(1);
                assert(owner->terrain().activation(h,false));capture();
                assert(owner->reconcileActivation());capture();
                assert(owner->terrain().activation(h,true));capture();
                assert(owner->reconcileActivation());capture();
            }
            if(i%3==0&&owner->terrain().size()>4){
                const auto h=owner->terrain().handle(2);
                assert(owner->terrain().requestDestroy(h));capture();
                assert(owner->prepareDraw());capture();
                assert(!owner->terrain().find(h));
            }
        }
        // Observe the birth/Create boundary separately. The complete builder
        // above cannot expose it through its terrain sink (which runs first).
        for(unsigned i=0;i<16;++i){
            assert(owner->terrain().append(60,240+int(i%8)*16,768+int(i/8)*16,true));
            capture();
            auto random=world->randomState();
            assert(owner->terrain().createProperties(owner->terrain().handle(owner->terrain().size()-1),
                terrainTileForArea(1),false,random));
            world->configureEnemyRandom(random);capture();
            // An unrelated gameplay draw must remain between these Creates.
            random.nextU32();world->configureEnemyRandom(random);capture();
        }
        capture();
        assert(!world->createGeneratedInstance({60,256,832},objects,[](const auto&){return false;}));
        capture(); // Sink rejection publishes neither lifetime nor RNG draw.
        assert(trace.good());
    }
    {
        // Ownership-replacement gate: typed terrain appearance, runner
        // appearance and published bounds are three different observations.
        // A compact owner must not derive all three from the current tile.
        auto boundary=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto scene=std::make_unique<GameplayWorld>(300,true,0,false,boundary->enemyBinding(),
            boundary->breakableBinding(),boundary->playerBinding(),&boundary->terrain());
        assert(boundary->attach(*scene));
        GameMakerRandom random;const auto draws=random.drawCount();
        for(int object:{60,93,99}){
            assert(boundary->terrain().append(object,240,512,true));
            const auto local=boundary->terrain().handle(boundary->terrain().size()-1);
            InstanceHandle shared;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared));
            const auto initial=*boundary->properties(shared);
            const auto birth=*boundary->state(shared);
            auto unchangedDefaults=[&]{
                const auto p=*boundary->properties(shared);
                assert(p.sprite.maskId==initial.sprite.maskId&&p.depth.bits==initial.depth.bits&&
                    p.sprite.scaleX.bits==0x3f800000u&&p.sprite.scaleY.bits==0x3f800000u&&
                    p.sprite.angle.bits==0&&p.timeline.bits==0&&p.cachedFrame.bits==0xbf800000u);
                const auto stateRead=boundary->state(shared);const auto& s=*stateRead;
                assert(s.x.bits==birth.x.bits&&s.y.bits==birth.y.bits&&
                    s.previous.x.bits==birth.previous.x.bits&&s.previous.y.bits==birth.previous.y.bits&&
                    s.kind==birth.kind&&s.special==birth.special);
            };
            auto unchangedBounds=[&]{
                const auto stateRead=boundary->state(shared);const auto& b=stateRead->bounds;
                assert(b.left.bits==birth.bounds.left.bits&&b.top.bits==birth.bounds.top.bits&&
                    b.right.bits==birth.bounds.right.bits&&b.bottom.bits==birth.bounds.bottom.bits);
            };
            unchangedDefaults();
            // Marker Create is a no-op, even with an invalid requested tile.
            if(object==99){
                assert(boundary->terrain().createProperties(local,-1,false,random));
                assert(boundary->properties(shared)->sprite.spriteId==initial.sprite.spriteId);
                unchangedDefaults();unchangedBounds();continue;
            }
            for(int area=0;area<6;++area){
                assert(boundary->terrain().createProperties(local,terrainTileForArea(area),false,random));
                assert(boundary->terrain().imageIndex(local,15));
                assert(boundary->properties(shared)->sprite.imageIndex.bits==0x41700000u);
                // Missing sprite: the original ordinary sprite setter resets
                // the nonnegative index, but wall Create does not subsequently
                // write the typed presentation index. Platform Create does.
                assert(boundary->terrain().createProperties(local,-1,false,random));
                const auto p=*boundary->properties(shared);
                const auto typed=boundary->terrain().find(local);
                assert(p.sprite.spriteId==-1&&typed->spriteId==-1);
                assert(p.sprite.imageIndex.bits==(object==60?0u:0x41b80000u));
                assert(typed->imageIndex==(object==60?15:23));
                unchangedDefaults();unchangedBounds();
                // A same-sprite write is a no-op, including its index. It
                // must not perform another inferred constructor/reset step.
                const auto savedIndex=p.sprite.imageIndex.bits;
                assert(boundary->terrain().createProperties(local,-1,false,random));
                assert(boundary->properties(shared)->sprite.imageIndex.bits==savedIndex);
                unchangedDefaults();unchangedBounds();
            }
        }
        assert(random.drawCount()==draws&&boundary->valid());
    }
    PackedRoom room;assert(rooms.valid()&&rooms.findRoom(11,room));
    {
        // Prepared Draw changes only at ownership mutations. Compare the
        // layer adapter against its live oracle after every intermediate
        // state, including inactive cutoffs before activation reconciliation.
        auto layers=std::make_unique<GameplayRoomLayers<16>>(&rooms);
        layers->enablePreparedDraw();assert(layers->prepare());
        RunnerLayerDraw::Element states[16]{};
        unsigned generations[16]{};
        auto publish=[&](unsigned slot){
            assert(layers->publishPreparedDraw({slot,generations[slot]},slot,
                RunnerLayerDraw::action(states[slot])));
        };
        auto compare=[&](){
            std::vector<std::pair<unsigned,std::uint32_t>> expected,actual;
            assert(layers->visit([&](InstanceHandle h,auto& out){
                assert(h.generation==generations[h.slot]);out=states[h.slot];return true;
            },[&](InstanceHandle h,int depth){
                RunnerImageValue value;assert(value.assignInteger(depth));
                expected.push_back({h.slot,value.bits});return true;
            }));
            assert(layers->visitPreparedDraw([&](unsigned slot,RunnerImageValue depth){
                actual.push_back({slot,depth.bits});return true;
            }));
            assert(actual==expected);
            if(!expected.empty()){
                unsigned calls=0;
                assert(!layers->visitPreparedDraw([&](unsigned,auto){++calls;return false;}));
                assert(calls==1);
            }
        };
        auto birth=[&](unsigned slot,int depth,bool projected){
            ++generations[slot];auto& state=states[slot];state={};
            state.type=2;state.instance=true;state.flags=0x10;
            state.object=projected;state.sprite=true;
            RunnerImageValue value;assert(value.assignInteger(depth));
            assert(layers->birth({slot,generations[slot]},0,value,false,projected));
            publish(slot);compare();
        };
        for(unsigned i=0;i<12;++i)birth(i,i<8?-321:-500,i%3!=0);
        for(unsigned i=0;i<12;++i){
            const InstanceHandle h{i,generations[i]};
            states[i].flags|=2;assert(layers->activation(h,false));publish(i);compare();
            assert(layers->reconcile(h));compare();
            states[i].flags&=~2u;assert(layers->activation(h,true));publish(i);compare();
            assert(layers->reconcile(h));compare();
            states[i].sprite=false;publish(i);compare();
            states[i].drawEvent=states[i].object;publish(i);compare();
            states[i].flags|=1;publish(i);compare();
            states[i].flags&=~1u;publish(i);compare();
            assert(layers->writeDepth(h,MotionBinary64::fromInteger(i%2?-600:-200)));
            compare(); // Pending depth requests have not moved any elements.
            assert(layers->serviceDepths());compare();
        }
        for(unsigned i=0;i<12;++i){
            assert(layers->retire({i,generations[i]}));compare();
            birth(i,-321,i%2==0); // Element/identity reuse, different eligibility.
        }
        for(unsigned i=0;i<12;++i)assert(layers->retire({i,generations[i]}));
        compare();assert(layers->prepare());compare();
    }
    {
        // Cached retained scenery must have the same live termination behavior
        // as the uncached instance lookup, before and after relinking/reuse.
        auto cached=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        auto oracle=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        assert(cached->prepare()&&oracle->prepare());
        RunnerImageValue depth;assert(depth.assign(MotionBinary64::fromInteger(-321)));
        bool inactive=false;unsigned terrainGeneration=1;
        auto birth=[&](InstanceHandle h){
            assert(cached->birth(h,0,depth,false,h.slot!=0));
            assert(oracle->birth(h,0,depth,false,true));
        };
        auto compare=[&](){
            std::vector<unsigned> actual,expected;unsigned cachedTerrain=0,oracleTerrain=0;
            auto walk=[&](auto& layers,auto& order,unsigned& terrainCalls){
                return layers.visit([&](InstanceHandle h,RunnerLayerDraw::Element& out){
                    out.instance=true;out.flags=0x10;out.object=h.slot!=0;out.drawEvent=true;
                    if(h.slot==0){++terrainCalls;assert(h.generation==terrainGeneration);if(inactive)out.flags|=2;}
                    return true;
                },[&](InstanceHandle h,int){order.push_back(h.slot);return true;});
            };
            assert(walk(*cached,actual,cachedTerrain)&&walk(*oracle,expected,oracleTerrain));
            assert(actual==expected&&cachedTerrain==0);
            return actual;
        };
        birth({1,1});birth({0,1});birth({2,1});
        assert((compare()==std::vector<unsigned>{2,1}));
        inactive=true;assert(cached->activation({0,1},false)&&oracle->activation({0,1},false));
        assert((compare()==std::vector<unsigned>{2}));
        assert(cached->reconcile({0,1})&&oracle->reconcile({0,1}));
        assert((compare()==std::vector<unsigned>{2,1}));
        inactive=false;assert(cached->activation({0,1},true)&&oracle->activation({0,1},true));compare();
        assert(cached->reconcile({0,1})&&oracle->reconcile({0,1}));compare();
        assert(cached->writeDepth({0,1},MotionBinary64::fromInteger(-400))&&
            oracle->writeDepth({0,1},MotionBinary64::fromInteger(-400)));
        assert(cached->serviceDepths()&&oracle->serviceDepths());compare();
        assert(cached->retire({0,1})&&oracle->retire({0,1}));compare();
        terrainGeneration=2;birth({0,2});compare();
        inactive=true;assert(cached->activation({0,2},false)&&oracle->activation({0,2},false));compare();
        for(auto h:{InstanceHandle{0,2},InstanceHandle{1,1},InstanceHandle{2,1}})
            assert(cached->retire(h)&&oracle->retire(h));
        assert(cached->prepare()&&oracle->prepare());compare();
    }
    for(unsigned removed=0;removed<8;++removed)for(unsigned mode=0;mode<4;++mode){
        // Interior retirement repairs only the preceding skipped run. Cover
        // actor/scenery/head/tail removal, inactive termination, cold/warm
        // projections and reuse of the freed element by a different kind.
        auto cached=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        auto oracle=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        assert(cached->prepare()&&oracle->prepare());
        RunnerImageValue depth;assert(depth.assignInteger(-321));
        bool scenery[8]={false,false,true,true,false,true,true,false};
        bool live[8]={true,true,true,true,true,true,true,true};
        bool inactive[8]={false,false,bool(mode&2),false,false,false,false,false};
        unsigned generation[8]={1,1,1,1,1,1,1,1};
        auto birth=[&](unsigned slot){
            assert(cached->birth({slot,generation[slot]},0,depth,inactive[slot],!scenery[slot]));
            assert(oracle->birth({slot,generation[slot]},0,depth,inactive[slot],true));
        };
        std::vector<unsigned> rawOrder;
        for(unsigned slot=0;slot<8;++slot){birth(slot);rawOrder.insert(rawOrder.begin(),slot);}
        auto compare=[&](){
            std::vector<unsigned> expected;
            for(auto slot:rawOrder)if(live[slot]){
                if(inactive[slot])break;
                if(!scenery[slot])expected.push_back(slot);
            }
            for(unsigned side=0;side<2;++side){
                std::vector<unsigned> actual;
                assert((side?oracle:cached)->visit([&](InstanceHandle h,RunnerLayerDraw::Element& out){
                    assert(live[h.slot]&&h.generation==generation[h.slot]);
                    assert(side||!scenery[h.slot]);
                    out.instance=true;out.flags=0x10|(inactive[h.slot]?2:0);
                    out.object=!scenery[h.slot];out.drawEvent=true;return true;
                },[&](InstanceHandle h,int){actual.push_back(h.slot);return true;}));
                assert(actual==expected);
            }
        };
        if(mode&1)compare();
        assert(cached->retire({removed,1})&&oracle->retire({removed,1}));live[removed]=false;
        compare();
        rawOrder.erase(std::find(rawOrder.begin(),rawOrder.end(),removed));
        scenery[removed]=!scenery[removed];inactive[removed]=false;
        generation[removed]=2;live[removed]=true;birth(removed);
        rawOrder.insert(rawOrder.begin(),removed);compare();
        for(auto slot:rawOrder){
            assert(cached->retire({slot,generation[slot]})&&oracle->retire({slot,generation[slot]}));
            live[slot]=false;compare();
        }
        assert(cached->prepare()&&oracle->prepare());compare();
    }
    for(unsigned mode=0;mode<5;++mode){
        // Warm skip links, then change a later scenery element or insert at
        // the head during Draw. The uncached/all-projected owner walks every
        // raw element and is the ordering oracle for this optimization.
        auto cached=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        auto oracle=std::make_unique<GameplayRoomLayers<8>>(&rooms);
        assert(cached->prepare()&&oracle->prepare());
        RunnerImageValue depth;assert(depth.assign(MotionBinary64::fromInteger(-321)));
        bool inactive[2]={mode==1,mode==1};
        for(unsigned slot:{1u,3u,0u,2u}){
            const bool scenery=slot==0||slot==3;
            assert(cached->birth({slot,1},0,depth,false,!scenery));
            assert(oracle->birth({slot,1},0,depth,false,true));
        }
        for(unsigned side=0;side<2;++side)if(inactive[side])
            assert((side?oracle:cached)->activation({3,1},false));
        auto draw=[&](unsigned side,bool mutate){
            auto& layers=*(side?oracle:cached);std::vector<unsigned> trace;
            assert(layers.visit([&](InstanceHandle h,RunnerLayerDraw::Element& out){
                out.instance=true;out.flags=0x10;out.object=h.slot!=0&&h.slot!=3;out.drawEvent=true;
                if(h.slot==4&&mode>=3){out.object=false;if(mode==4)out.flags|=2;}
                if(h.slot==3&&inactive[side])out.flags|=2;
                return true;
            },[&](InstanceHandle h,int){
                trace.push_back(h.slot);
                if(mutate&&h.slot==2){
                    if(mode>=2)assert(layers.birth({4,1},0,depth,mode==4,side||mode==2));
                    else{inactive[side]=mode==0;assert(layers.activation({3,1},!inactive[side]));}
                }
                return true;
            }));return trace;
        };
        assert(draw(0,false)==draw(1,false));
        const auto actual=draw(0,true),expected=draw(1,true);
        assert(actual==expected&&actual==(mode==0?std::vector<unsigned>{2}:std::vector<unsigned>{2,1}));
        assert(draw(0,false)==draw(1,false));
        if(mode>=2){
            assert(cached->retire({4,1})&&oracle->retire({4,1}));
            assert(draw(0,false)==draw(1,false));
        }
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world));auto& entries=owner->entryDithers();
        InstanceHandle key,shared;unsigned reads=0;
        auto camera=[&](MotionBinary64& value){++reads;value=RunnerImageValue{0x43708000u}.promoted();return true;};
        // Explicit fixture ID, not an assertion about the unfinished allocator.
        assert(entries.create(400001,camera,[&](MotionBinary64& value){
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::entryDither,key,shared));
            assert(owner->state(shared)&&owner->state(shared)->x.bits==0x43708000u&&owner->state(shared)->y.bits==0);
            value=world->roomEntryView()->y.promoted();return true;
        },key));
        assert(entries.find(key)->sourceId==400001&&entries.find(key)->effect.alarm0==120&&reads==1);
        assert(owner->properties(shared)->sprite.spriteId==129);
        auto phase=std::make_unique<RunnerStepSnapshot<GameplayCollisionView::capacity>>();
        auto tick=[&](){
            assert(owner->dispatchAlarmPhase(*phase,[&](InstanceHandle h,unsigned alarm,const auto& event){
                if(h.slot!=shared.slot)return true;
                assert(event.codeId==1032&&alarm==0);return entries.advanceAlarm(key,alarm);
            }));
            assert(owner->dispatchStepPhase(0,*phase,[&](InstanceHandle h,const auto& event){
                if(h.slot!=shared.slot)return true;
                assert(event.codeId==1033);return entries.step(key,camera);
            }));
        };
        assert(entries.activate(key,false)&&owner->reconcileActivation());
        tick();assert(reads==1&&entries.find(key)->effect.alarm0==120);
        assert(entries.activate(key,true)&&owner->reconcileActivation());
        for(unsigned frame=1;frame<=120;++frame){
            tick();const auto* entry=entries.find(key);assert(entry);
            assert(entry->effect.alarm0==120-int(frame)&&entry->destroyRequested==(frame==120));
            const unsigned steps=frame<120?frame:119;
            RunnerImageValue expectedY;assert(expectedY.assign(MotionBinary64::fromInteger(16*int(steps))));
            assert(reads==1+steps&&entry->y.bits==expectedY.bits);
            unsigned drawn=0;
            checkReadOnlyDraw(*world);
            assert(world->visitRoomActors([&](auto kind,auto local,auto depth){
                if(kind!=GameplayWorld::DrawActor::entryDither)return true;
                ++drawn;std::int32_t d;assert(depth.promoted().roundPixel(d)&&d==-200000);
                return local.slot==key.slot&&local.generation==key.generation;
            }));
            assert(drawn==(frame==120?0u:1u)); // No offscreen-origin culling.
        }
        assert(entries.size()==1&&owner->prepareDraw()&&entries.size()==0&&!owner->state(shared));
        const auto old=key;
        assert(entries.create(400002,camera,[](auto& v){v={};return true;},key));
        assert(key.slot==old.slot&&key.generation>old.generation&&!entries.find(old));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay);
        assert(entries.size()==0&&owner->valid()&&world->entryDithers()==&entries);
        const auto previous=key;
        assert(entries.create(400003,camera,[](auto& v){v={};return true;},key));
        assert(key.generation>previous.generation&&entries.clear()&&owner->valid());
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,true,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world));auto& entries=owner->entryDithers();InstanceHandle key,shared;
        // Birth allocation is still explicit; all subsequent scheduling is the
        // ordinary gameplay path, with no manual alarm or Step dispatch.
        assert(entries.create(410001,[](auto& x){x=MotionBinary64::fromInteger(240);return true;},
            [&](auto& y){y=world->roomEntryView()->y.promoted();return true;},key));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::entryDither,key,shared));
        assert(entries.activate(key,false)&&owner->reconcileActivation());
        world->step(0);
        assert(world->effectsSucceeded()&&entries.find(key)->effect.alarm0==120&&entries.find(key)->y.bits==0);
        assert(entries.activate(key,true)&&owner->reconcileActivation());
        for(unsigned tick=1;tick<=120;++tick){
            world->step(0);assert(world->effectsSucceeded()&&owner->valid());
            const auto* r=entries.find(key);assert(r);
            std::int32_t y;assert(r->y.promoted().roundPixel(y));
            assert(y==16*int(tick<120?tick:119)&&r->effect.alarm0==120-int(tick)&&
                r->destroyRequested==(tick==120)&&r->x.bits==0x43700000u);
            assert(owner->state(shared)->x.bits==r->x.bits&&owner->state(shared)->y.bits==r->y.bits);
        }
        assert(owner->prepareDraw()&&!entries.find(key)&&!owner->state(shared));
    }
    for(unsigned failure=0;failure<3;++failure){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain(),&rooms);
        assert(owner->attach(*world));auto& entries=owner->entryDithers();InstanceHandle key;
        unsigned cameras=0,views=0;
        auto camera=[&](MotionBinary64& v){++cameras;v=MotionBinary64::fromInteger(240);return failure!=1;};
        auto view=[&](MotionBinary64& v){++views;v={};return false;};
        assert(!entries.create(failure==0?0:400001,camera,view,key)&&!entries.valid());
        assert(cameras==(failure==0?0u:1u)&&views==(failure==2?1u:0u));
        assert(entries.size()==(failure==0?0u:1u));
        if(failure){
            const auto* r=entries.find(key);assert(r&&r->effect.alarm0==-1&&r->y.bits==0);
            assert(r->x.bits==(failure==2?0x43700000u:0)); // Accepted setter prefix survives.
        }
        assert(!entries.create(400002,camera,view,key));
        assert(cameras==(failure==0?0u:1u)&&views==(failure==2?1u:0u));
    }
    {
        struct PlayerViewProbe {
            GameplayWorld* world=nullptr;unsigned births=0;bool reject=false;
            bool birth(InstanceHandle,const GameplayWorld::PlayerInstance&){
                const auto* view=world->roomEntryView();
                assert(view&&view->enabled&&view->x.bits==0&&view->y.bits==0&&
                    view->width.bits==0x43200000u&&view->height.bits==0x438e0000u);
                assert(world->cameraOffsetX()==0&&world->cameraOffsetY()==0&&world->ringOrigin()==300);
                ++births;return !reject;
            }
            bool retire(InstanceHandle,const GameplayWorld::PlayerInstance&){return true;}
            bool activation(InstanceHandle,const GameplayWorld::PlayerInstance&,bool){return true;}
            bool membership(InstanceHandle,const GameplayWorld::PlayerInstance&,bool){return true;}
        } probe;
        // Supply the address before construction so the actual birth callback
        // reads the already-constructed cache, not a post-construction snapshot.
        void* storage=::operator new(sizeof(GameplayWorld));
        probe.world=static_cast<GameplayWorld*>(storage);
        std::unique_ptr<GameplayWorld> world(new(storage) GameplayWorld(300,false,1025,true,
            {},{},GameplayWorld::PlayerLifecycleBinding::bind(probe),nullptr,&rooms));
        assert(world->effectsSucceeded()&&probe.births==1&&world->cameraOffsetY()==5&&world->ringOrigin()==305);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::gameplay);
        assert(world->effectsSucceeded()&&probe.births==2&&world->roomEntryView()->y.bits==0&&
            world->cameraOffsetY()==5&&world->ringOrigin()==305);
        RoomPack invalid(nullptr,0);
        auto rejected=std::make_unique<GameplayWorld>(300,false,1025,true,
            GameplayWorld::EnemyLifecycleBinding{},GameplayWorld::BreakableLifecycleBinding{},
            GameplayWorld::PlayerLifecycleBinding::bind(probe),nullptr,&invalid);
        assert(!rejected->effectsSucceeded()&&!rejected->roomEntryView()&&
            !rejected->playerHandle().generation&&probe.births==2);
        probe.reject=true;
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)==ResultsDestination::none);
        assert(!world->effectsSucceeded()&&probe.births==3&&world->cameraOffsetY()==0&&world->ringOrigin()==300);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));InstanceHandle effect;
        auto frames=[&](int id,unsigned& count){PackedSpriteCollision art;
            if(!sprites.sprite(id,art))return false;count=art.frameCount;return true;};
        assert(world->spawnScriptEffect(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(300),
            603,{},{},MotionBinary64::fromInteger(-60000),frames,effect));
        InstanceHandle shared;assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::scriptEffect,effect,shared));
        RunnerImageValue requested;assert(requested.assign(MotionBinary64::fromInteger(-60000)));
        assert(owner->properties(shared)->depth.bits==requested.bits);
        auto drawDepth=[&](){int depth=123;unsigned seen=0;
            checkReadOnlyDraw(*world);
            assert(world->visitRoomActors([&](auto kind,auto key,auto value){
                if(kind==GameplayWorld::DrawActor::scriptEffect&&key.slot==effect.slot&&key.generation==effect.generation){
                    ++seen;std::int32_t integer;assert(RunnerDepthService::integerDepth(value,integer));depth=integer;
                }return true;
            }));assert(seen==1);return depth;};
        assert(drawDepth()==0); // Setter publication is not immediate relinking.
        world->step(0);
        assert(world->effectsSucceeded()&&drawDepth()==-60000);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,43,300)!=ResultsDestination::none&&
            !owner->state(shared)&&owner->valid());
    }
    {
        auto roomOwner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto roomWorld=std::make_unique<GameplayWorld>(300,true,0,false,roomOwner->enemyBinding(),
            roomOwner->breakableBinding(),roomOwner->playerBinding(),&roomOwner->terrain());
        assert(roomOwner->attach(*roomWorld));
        const auto oldPlayer=roomWorld->playerHandle();
        assert(roomWorld->findPlayer(oldPlayer)&&roomWorld->findPlayer(oldPlayer)->sourceInstanceId==0);
        unsigned calls=0,codes=0;
        assert(!roomWorld->roomEntryView());
        assert(roomWorld->loadRoomTerrain(rooms,room,[&](const PackedRoomInstance& r){
            const auto* view=roomWorld->roomEntryView();
            assert(view&&view->enabled&&view->x.bits==0&&view->y.bits==0&&
                view->width.bits==0x43200000u&&view->height.bits==0x438e0000u);
            ++calls;codes+=r.creationCodeId!=-1;return true;}));
        assert(calls==240&&codes==3&&roomOwner->terrain().size()==240);
        checkCollisionViewMembership(*roomWorld);
        unsigned authoredPlayers=0;
        for(unsigned i=0;i<room.instanceCount;++i){
            PackedRoomInstance r;assert(rooms.roomInstance(room,i,r));
            if(r.objectId!=35)continue;
            ++authoredPlayers;
            assert(r.instanceId==100855&&roomWorld->findPlayer(oldPlayer)->sourceInstanceId==r.instanceId);
        }
        assert(authoredPlayers==1);
        unsigned ordinal=0;
        for(unsigned i=0;i<room.instanceCount;++i){
            PackedRoomInstance r;assert(rooms.roomInstance(room,i,r));
            if(r.objectId!=60&&r.objectId!=93)continue;
            const auto local=roomOwner->terrain().handle(ordinal++);
            const auto stored=roomOwner->terrain().find(local);InstanceHandle shared;
            assert(stored&&stored->sourceInstanceId==r.instanceId&&stored->creationCodeId==r.creationCodeId&&
                stored->x==r.x&&stored->y==r.y&&stored->objectId==r.objectId&&
                GameplayCollisionView::encode(GameplayCollisionView::Kind::terrain,local,shared));
            const auto state=roomOwner->state(shared);std::int32_t x,y;
            assert(state&&state->x.promoted().roundPixel(x)&&state->y.promoted().roundPixel(y)&&x==r.x&&y==r.y);
            assert(stored->spriteId==51&&stored->imageSpeed==0&&roomOwner->properties(shared)->sprite.spriteId==51);
            assert(stored->wall.pillar==(r.creationCodeId==677?1:r.creationCodeId==676||r.creationCodeId==678?-1:0));
            assert(!stored->wall.checked&&!stored->wall.surrounded&&!stored->wall.deactivate&&!stored->wall.lights);
        }
        const auto old=roomOwner->terrain().handle(0);
        assert(roomWorld->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(roomOwner->terrain().size()==0&&!roomOwner->terrain().find(old));
        const auto newPlayer=roomWorld->playerHandle();
        assert(newPlayer.generation>oldPlayer.generation&&!roomWorld->findPlayer(oldPlayer)&&
            roomWorld->findPlayer(newPlayer)->sourceInstanceId==0);
        assert(roomWorld->loadRoomTerrain(rooms,room,[](const auto&){return true;}));
        assert(roomWorld->findPlayer(newPlayer)->sourceInstanceId==100855&&!roomWorld->findPlayer(oldPlayer));
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
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery})
    for(int initial:{(-2147483647-1),-2,-1,0,1,2}){
        // Compare the bound pending-counter path with the original unfiltered
        // captured traversal. Negative values must remain untouched; zero
        // still retires, and positive counters retain countdown/RNG order.
        const auto* alarmRooms=ownership==GameplayCollisionOwner::Ownership::nativeScenery?&rooms:nullptr;
        auto optimized=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},alarmRooms,ownership);
        auto reference=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},alarmRooms);
        auto a=std::make_unique<GameplayWorld>(300,true,0,false,optimized->enemyBinding(),
            optimized->breakableBinding(),optimized->playerBinding(),&optimized->terrain());
        auto b=std::make_unique<GameplayWorld>(300,true,0,false,reference->enemyBinding(),
            reference->breakableBinding(),reference->playerBinding(),&reference->terrain());
        assert(optimized->attach(*a)&&reference->attach(*b));
        for(auto* world:{a.get(),b.get()}){
            unsigned slot=0;
            for(int object:{112,113,116,117,119}){
                PackedObject metadata;assert(objects.findObject(object,metadata));
                assert(world->configureEnemy(slot,object,metadata.spriteId,int(slot)*16-32,128,
                    0,0,5,14,0,1,initial,object==113&&initial==1?-1:initial,0,0,true,true,true));
                ++slot;
            }
            assert(world->createGeneratedInstance({92,240,512},objects,[](const auto&){return true;}));
            auto* box=const_cast<GameplayWorld::Breakable*>(world->findBreakable({0,1}));
            assert(box);box->alarm0=initial;
        }
        RunnerStepSnapshot<32> snapshot;
        auto unfiltered=[&](){
            return reference->dispatchAlarmPhase(snapshot,[&](InstanceHandle h,unsigned alarm,const auto&){
                GameplayCollisionView::Kind kind;InstanceHandle local;
                if(!GameplayCollisionView::decode(h,kind,local))return false;
                if(kind==GameplayCollisionView::Kind::enemy)return b->advanceEnemyRoomAlarm(local,alarm);
                if(kind==GameplayCollisionView::Kind::breakable)return b->advanceBoxRoomAlarm(local,alarm);
                return true; // Other captured families are not combat alarms.
            });
        };
        b->configureRoomAlarms([](void* p){return (*static_cast<decltype(unfiltered)*>(p))();},&unfiltered);
        for(unsigned tick=0;tick<24;++tick){
            if(tick==8)for(auto* world:{a.get(),b.get()}){
                // A reused capture must observe counters rearmed between ticks.
                auto& enemy=const_cast<GameplayWorld::EnemyState&>(world->enemyAt(0));
                enemy.alarm1=1;
                const_cast<GameplayWorld::Breakable*>(world->findBreakable({0,1}))->alarm0=0;
            }
            a->step(0);b->step(0);
            assert(a->effectsSucceeded()&&b->effectsSucceeded()&&optimized->valid()&&reference->valid());
            assert(a->enemyCount()==b->enemyCount());
            for(unsigned i=0;i<a->enemyCount();++i){
                const auto& x=a->enemyAt(i);const auto& y=b->enemyAt(i);
                assert(x.objectId==y.objectId&&x.lifetimeId==y.lifetimeId&&x.active==y.active&&
                    x.alarm0==y.alarm0&&x.alarm1==y.alarm1&&x.health==y.health&&
                    x.preciseX==y.preciseX&&x.preciseY==y.preciseY&&x.speedX==y.speedX&&x.speedY==y.speedY&&
                    x.motionState==y.motionState&&x.auxiliaryState==y.auxiliaryState&&
                    x.spriteId==y.spriteId&&x.imageFrame==y.imageFrame);
                // Frog Alarm 0 arms the previously negative Alarm 1; that
                // later index must count down immediately, not next frame.
                if(!tick&&initial==1&&x.objectId==113)assert(x.alarm1==59);
            }
            const auto* x=a->findBreakable({0,1});const auto* y=b->findBreakable({0,1});
            assert(x&&y&&x->alarm0==y->alarm0&&x->active==y->active);
            assert(a->randomState().drawCount()==b->randomState().drawCount()&&
                a->randomState().stateIndex()==b->randomState().stateIndex());
            for(unsigned w=0;w<16;++w)assert(a->randomState().stateWord(w)==b->randomState().stateWord(w));
        }
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery}){
        auto ordered=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},
            ownership==GameplayCollisionOwner::Ownership::nativeScenery?&rooms:nullptr,ownership);
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
        // A failed external combat alarm phase is terminal before countdown.
        scene->configureRoomAlarms([](void*){return false;},nullptr);
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
        checkCollisionViewMembership(*fieldWorld);
        const auto state=fieldsOwner->state(fieldKey);std::int32_t x,y;
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
            const auto p=fieldsOwner->properties(key);const auto s=fieldsOwner->state(key);
            PackedObject definition;
            assert(objects.findObject(key.slot==exitKey.slot?306:74,definition)&&p&&s&&!(s->flags&0x100003u)&&
                p->sprite.spriteId==definition.spriteId&&p->sprite.maskId==definition.textureMaskId&&
                p->sprite.scaleX.bits==0x3f800000u&&p->sprite.scaleY.bits==0x3f800000u);
        }
        const auto goalLocal=fieldWorld->goalFieldInstances().handle(0);
        InstanceHandle goalKey;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::goalField,goalLocal,goalKey));
        const auto goal=fieldsOwner->state(goalKey);
        const auto goalProperties=fieldsOwner->properties(goalKey);
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
        // Nonpersistent debris must leave with the old room, whether still
        // active or already marked for deferred destruction before Draw.
        InstanceHandle roomDebris,markedDebris,roomDebrisKey,markedDebrisKey;
        assert(fieldWorld->spawnDebris({},{},1,roomDebris));
        assert(fieldWorld->spawnDebris({},{},1,markedDebris));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,roomDebris,roomDebrisKey));
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,markedDebris,markedDebrisKey));
        for(unsigned tick=0;tick<30;++tick)
            assert(fieldWorld->advanceOwnedDebrisRoomAlarm(markedDebris,fieldWorld->debrisEffects().beginPhase(),0));
        assert(fieldWorld->debrisEffects().retained(markedDebris.generation)->destroyRequested);
        assert(fieldsOwner->state(roomDebrisKey)&&fieldsOwner->state(markedDebrisKey));
        const auto debrisSequence=fieldWorld->debrisEffects().birthSequence();
        const auto transitionDraws=fieldWorld->randomDrawCount();
        assert(fieldWorld->prepareNextAreaOneLevel({},300)&&fieldWorld->runStage().level==2);
        assert(!fieldWorld->debrisInstances()->size()&&!fieldWorld->debrisEffects().retained(roomDebris.generation)&&
            !fieldWorld->debrisEffects().retained(markedDebris.generation));
        assert(!fieldsOwner->state(roomDebrisKey)&&!fieldsOwner->state(markedDebrisKey)&&
            fieldsOwner->population(370,population)&&population==0);
        assert(fieldWorld->debrisEffects().birthSequence().next==debrisSequence.next&&
            fieldWorld->randomDrawCount()==transitionDraws);
        InstanceHandle nextDebris,nextDebrisKey;
        assert(fieldWorld->spawnDebris({},{},1,nextDebris)&&nextDebris.generation>markedDebris.generation);
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::debris,nextDebris,nextDebrisKey));
        assert(fieldsOwner->state(nextDebrisKey)&&!fieldsOwner->state(roomDebrisKey)&&!fieldsOwner->state(markedDebrisKey));
        const auto startDraws=fieldWorld->randomDrawCount();
        assert(fieldWorld->createRoomStartField()&&fieldWorld->startTimeField()&&
            fieldsOwner->population(371,population)&&population==1&&fieldWorld->randomDrawCount()==startDraws+1);
        const auto startLocal=fieldWorld->goalFieldInstances().handle(0);
        checkCollisionViewMembership(*fieldWorld);
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
        assert(tape->run(*world,*owner,objects,sprites,300)&&tape->checks==255&&tape->gameSteps==108&&
            tape->clearSteps>0&&tape->clearSteps<400&&tape->firstGeneration<tape->laterGeneration&&
            tape->laterGeneration<tape->retryGeneration);
        assert(tape->preparedResources.objectsChecked==386&&
            tape->preparedResources.alarmsChecked==4632&&tape->preparedResources.spritesChecked==741);
        std::printf("Prepared resources: objects=%u alarm slots=%u sprites=%u\n",
            tape->preparedResources.objectsChecked,tape->preparedResources.alarmsChecked,
            tape->preparedResources.spritesChecked);
        std::printf("Field reentry tape: checks=%u gameplay=%u upgrade=%u generations=%u/%u/%u\n",
            tape->checks,tape->gameSteps,tape->clearSteps,tape->firstGeneration,tape->laterGeneration,tape->retryGeneration);
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        auto& drones=owner->droneBullets();InstanceHandle local,shared;
        assert(drones.create(105190,MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(176),local)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::drone,local,shared));
        for(unsigned tick=1;tick<=16;++tick){
            RunnerImageValue centre;assert(centre.assign(MotionBinary64::fromInteger(world->cameraOffsetY()+176)));
            assert(drones.dispatch(local,[&](auto&,auto& publish,bool&){return publish.publishPosition({0x43700000u},centre);}));
            world->step(0);
            const auto* record=drones.find(local);
            assert(world->effectsSucceeded()&&record&&record->state.decelerate==(tick==16));
        }
        // Exercise the actual pre-Step room animation dispatch and deferred
        // cleanup, not a direct call to the projectile animation helper.
        assert(drones.dispatch(local,[](auto& state,auto&,bool&){
            state.animation=true;state.imageSpeed={0x3f000000u};
            state.timeline={0x40b00000u};state.imageIndex=state.timeline;return true;
        }));
        world->step(0);
        assert(world->effectsSucceeded()&&owner->prepareDraw()&&!drones.find(local)&&!owner->state(shared));
        InstanceHandle edge;
        assert(drones.create(105193,MotionBinary64::fromInteger(240),
            MotionBinary64::fromInteger(world->cameraOffsetY()+34+316),edge));
        world->step(0);
        assert(world->effectsSucceeded()&&drones.find(edge)&&!drones.find(edge)->destroyRequested);
        RunnerImageValue below;assert(below.assign(MotionBinary64::fromInteger(world->cameraOffsetY()+34+317)));
        assert(drones.dispatch(edge,[&](auto&,auto& publish,bool&){return publish.publishPosition({0x43700000u},below);}));
        world->step(0);
        assert(world->effectsSucceeded()&&owner->prepareDraw()&&!drones.find(edge));
        InstanceHandle retained;
        assert(drones.create(105191,{},{},retained));
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,42,300)!=ResultsDestination::none);
        assert(world->droneBullets()==&drones&&!drones.find(retained)&&drones.size()==0&&owner->valid());
        InstanceHandle replacement;
        assert(drones.create(105192,{},{},replacement)&&replacement.generation>retained.generation&&drones.clear());
    }
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,false,1000,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));
        for(unsigned i=0;i<8;++i){world->step(4);assert(world->effectsSucceeded()&&owner->prepareDraw());}
        assert(world->bulletCount()==2);
        const auto local=[](const auto& b){return InstanceHandle{b.identitySlot,b.lifetimeId};};
        const auto first=local(world->bulletAt(0)),second=local(world->bulletAt(1));
        InstanceHandle firstKey,secondKey;
        assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,first,firstKey)&&
            GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,second,secondKey));
        for(unsigned i=0;i<2;++i){
            const auto& bullet=world->bulletAt(i);InstanceHandle key;
            assert(GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,local(bullet),key));
            const auto state=owner->state(key);const auto properties=owner->properties(key);
            RunnerImageValue x,y,angle;
            assert(ProjectilePublication::coordinate(bullet.trajectory.x,240,x)&&
                ProjectilePublication::coordinate(bullet.trajectory.y,176,y)&&
                ProjectilePublication::angle(bullet.trajectory.direction,angle));
            assert(state&&properties&&state->x.bits==x.bits&&state->y.bits==y.bits&&
                properties->sprite.imageIndex.bits==bullet.animation.index.bits&&
                properties->sprite.spriteId==machineGunProfile(bullet.gemHigh).sprite&&properties->sprite.angle.bits==angle.bits);
        }
        world->configureSolidResolver([](void*,int,int y){return y==70;},nullptr);
        world->step(4);
        unsigned population;
        assert(world->effectsSucceeded()&&world->resolveBulletLifetime(first)&&
            world->resolveBulletLifetime(first)->destroyRequested&&!world->findBullet(first.generation)&&
            owner->state(firstKey)&&(owner->state(firstKey)->flags&1u)&&
            owner->population(261,population)&&population==1);
        assert(owner->prepareDraw()&&!world->resolveBulletLifetime(first)&&!owner->state(firstKey));
        world->configureSolidResolver(nullptr,nullptr);
        for(unsigned i=9;i<=14;++i){world->step(4);assert(world->effectsSucceeded()&&owner->prepareDraw());}
        assert(world->bulletCount()==2&&world->resolveBulletLifetime(second)==&world->bulletAt(0)&&
            owner->state(secondKey)&&!owner->state(firstKey));
        const auto newest=local(world->bulletAt(1));
        assert(newest.generation>second.generation&&newest.slot==first.slot);
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,2,300)==ResultsDestination::gameplay&&
            !owner->state(secondKey)&&!world->resolveBulletLifetime(newest)&&owner->population(261,population)&&population==0);
        world->step(4);
        assert(world->effectsSucceeded()&&world->bulletCount()==1&&world->bulletAt(0).lifetimeId>newest.generation);
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
    const auto dying=terrainRecord.terrain;
    assert(owner->terrain().requestDestroy(wallLocal)&&owner->terrain().find(wallLocal)->destroyRequested&&
        owner->terrain().size()==3&&owner->terrain().find(wallLocal)==dying&&
        (owner->state(wall)->flags&1u)&&!(owner->state(otherWall)->flags&1u));
    assert(owner->terrain().requestDestroy(wallLocal)); // repeated request retains the same lifetime
    assert(owner->terrain().activation(wallLocal,false)&&!owner->terrain().find(wallLocal)->active&&
        (owner->state(wall)->flags&3u)==3u&&owner->terrain().find(wallLocal)==dying);
    const auto survivor=owner->terrain().find(owner->terrain().handle(1));
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
            const auto p=masks->properties(shared);
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
    {
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,
            GameplayTerrainInstances::Presentation{},GameplayTerrainInstances::WallCollision{},&rooms);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding(),&owner->terrain());
        assert(owner->attach(*world));const int bx=0,by=336;
        assert(world->appendBreakables(&bx,&by,1));const InstanceHandle box{0,1};
        unsigned lookups=0;bool accept=true;InstanceHandle observed;
        auto frames=[&](int sprite,unsigned& count){
            ++lookups;assert(sprite==603);observed=world->findBreakable(box)->myFx;
            const auto* child=owner->scriptEffects().find(observed);
            assert(child&&child->effect&&child->effect->spriteId==-1);
            PackedSpriteCollision art;assert(sprites.sprite(sprite,art));count=art.frameCount;return accept;
        };
        auto x=MotionBinary64::fromInteger(240),y=MotionBinary64::fromInteger(524);
        assert(x.add({0x3fd00000u,0})&&y.add({0x3fc00000u,0}));
        const auto draws=world->randomDrawCount();
        assert(world->spawnBoxScriptEffect(box,x,y,603,{0x3fe00000u,0},MotionBinary64::fromInteger(270),
            MotionBinary64::fromInteger(-60000),frames));
        const auto first=observed;const auto* child=owner->scriptEffects().find(first)->effect;
        assert(child->spriteId==603&&child->x.bits==0x43704000u&&child->y.bits==0x44030800u&&
            child->imageSpeed.bits==0x3f000000u&&child->angle.bits==0x43870000u&&world->randomDrawCount()==draws);
        accept=false;
        assert(!world->spawnBoxScriptEffect(box,x,y,603,{0x3fe00000u,0},MotionBinary64::fromInteger(270),
            MotionBinary64::fromInteger(-60000),frames));
        const auto second=world->findBreakable(box)->myFx;
        assert(!world->effectsSucceeded()&&second.generation>first.generation&&owner->scriptEffects().find(first)&&
            owner->scriptEffects().find(second)&&world->scriptEffects().size()==2&&lookups==2);
        assert(!world->spawnBoxScriptEffect(box,x,y,603,{},{},{},frames)&&lookups==2&&world->randomDrawCount()==draws);
    }
    for(unsigned failure=1;failure<=7;++failure){
        auto world=std::make_unique<GameplayWorld>(300,true);
        const int bx=0,by=336;assert(world->appendBreakables(&bx,&by,1));const InstanceHandle box{0,1};
        assert(BoxSoundVerification::run(*world,box,171,[&](){return &world->findBreakable(box)->sound;},failure));
    }
    for(auto ownership:{GameplayCollisionOwner::Ownership::reference,GameplayCollisionOwner::Ownership::native,
                       GameplayCollisionOwner::Ownership::nativeScenery}){
        // Native ownership skips box groups while no live box has Alarm 0
        // set; an armed box must still count down beside idle ones.
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites,GameplayTerrainInstances::Presentation{},
            GameplayTerrainInstances::WallCollision{},
            ownership==GameplayCollisionOwner::Ownership::nativeScenery?&rooms:nullptr,ownership);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,owner->enemyBinding(),
            owner->breakableBinding(),owner->playerBinding());
        assert(owner->attach(*world));
        const int xs[]={64,96},ys[]={512,512};
        assert(world->appendBreakables(xs,ys,2));
        const InstanceHandle idle{0,world->breakableSlot(0).lifetimeId},armed{1,world->breakableSlot(1).lifetimeId};
        world->step(0);
        assert(world->effectsSucceeded()&&world->findBreakable(idle)->alarm0==-1&&world->findBreakable(armed)->alarm0==-1);
        assert(world->publishBoxAlarm0(armed,2));
        // Expiry zeroes health; the box's own Step then marks destruction,
        // which excludes it (still at zero) from later alarm passes.
        for(int expected:{1,0,0}){
            world->step(0);
            assert(world->effectsSucceeded()&&world->findBreakable(idle)->alarm0==-1&&
                world->findBreakable(armed)->alarm0==expected);
        }
        assert(world->findBreakable(armed)->state.wallHp.zero()&&world->findBreakable(armed)->destroyRequested&&
            !world->findBreakable(idle)->state.wallHp.zero()&&!world->findBreakable(idle)->destroyRequested);
    }
    {
        GameplayWorld world(300,true);
        const int xs[]={64,96,128},ys[]={512,512,512};
        assert(world.appendBreakables(xs,ys,3));
        const InstanceHandle a{0,world.breakableSlot(0).lifetimeId},
            b{1,world.breakableSlot(1).lifetimeId},c{2,world.breakableSlot(2).lifetimeId};
        assert(world.publishBoxAlarm0(a,1)&&world.publishBoxAlarm0(b,0)&&world.publishBoxAlarm0(c,1));
        const auto phase=world.captureBoxAlarms();
        assert(world.markBreakableDestroyed(c));
        assert(world.appendBreakables(xs,ys,1));
        const InstanceHandle newborn{3,world.breakableSlot(3).lifetimeId};
        assert(world.publishBoxAlarm0(newborn,1)&&world.advanceBoxAlarmPrefix(phase));
        assert(world.findBreakable(a)->alarm0==0&&world.findBreakable(a)->state.wallHp.zero());
        assert(world.findBreakable(b)->alarm0==-1&&!world.findBreakable(b)->state.wallHp.zero());
        assert(world.findBreakable(c)->alarm0==1&&world.findBreakable(newborn)->alarm0==1);
        world.configureBreakables(xs,ys,3);
        const InstanceHandle replacement{0,world.breakableSlot(0).lifetimeId};
        assert(replacement.generation>phase.lastLifetime&&world.publishBoxAlarm0(replacement,1));
        assert(world.advanceBoxAlarmPrefix(phase)&&world.findBreakable(replacement)->alarm0==1);
        assert(world.advanceBoxAlarmPrefix(world.captureBoxAlarms())&&world.findBreakable(replacement)->alarm0==0);
    }
    for(bool registered:{false,true}){
        auto owner=std::make_unique<GameplayCollisionOwner>(objects,sprites);
        auto world=std::make_unique<GameplayWorld>(300,true,0,false,
            registered?owner->enemyBinding():GameplayWorld::EnemyLifecycleBinding{},
            registered?owner->breakableBinding():GameplayWorld::BreakableLifecycleBinding{},
            registered?owner->playerBinding():GameplayWorld::PlayerLifecycleBinding{});
        if(registered)assert(owner->attach(*world));
        const int x=64,y=512;
        assert(world->appendBreakables(&x,&y,1));
        const InstanceHandle key{0,1};
        if(registered){
            assert(world->publishBoxAlarm0(key,1));
            const auto draws=world->randomDrawCount();
            // Box is inside strict room sides, but well below any field.
            assert(owner->stopBoxAlarm0(key,true,480,masks)&&world->findBreakable(key)->alarm0==2&&
                world->findBreakable(key)->state.scriptIndex==1&&world->randomDrawCount()==draws);
            SpriteMaskPack absent(nullptr,0);
            assert(owner->stopBoxAlarm0(key,false,480,absent)&&world->findBreakable(key)->alarm0==2);
            for(int value:{-2,-1,0}){
                assert(world->publishBoxAlarm0(key,value)&&owner->stopBoxAlarm0(key,true,480,masks)&&
                    world->findBreakable(key)->alarm0==value);
            }
            RunnerImageValue py;assert(py.assign(MotionBinary64::fromInteger(20000)));
            for(int quarter:{639,640,1280,1281}){
                auto q=MotionBinary64::fromInteger(quarter);q.high-=2u<<20;RunnerImageValue px;assert(px.assign(q));
                assert(owner->breakableBinding().publishedPosition(key,*world->findBreakable(key),px,py)&&
                    world->publishBoxAlarm0(key,1)&&owner->stopBoxAlarm0(key,true,480,masks));
                assert(world->findBreakable(key)->alarm0==(quarter>=640&&quarter<=1280?2:1));
            }
            RunnerImageValue px;assert(px.assign(MotionBinary64::fromInteger(x+240))&&py.assign(MotionBinary64::fromInteger(y+176)));
            assert(owner->breakableBinding().publishedPosition(key,*world->findBreakable(key),px,py));
            assert(world->createGeneratedInstance({82,x+240,y+176},objects,[](const auto&){return true;}));
            assert(world->publishBoxAlarm0(key,1)&&owner->stopBoxAlarm0(key,true,480,masks)&&
                world->findBreakable(key)->alarm0==1);
        }
        assert(world->publishBoxAlarm0(key,2));
        world->step(0);
        assert(world->effectsSucceeded()&&world->findBreakable(key)->alarm0==1&&
            world->findBreakable(key)->state.wallHp.high==0x3ff00000u);
        // Zero retires to -1 without dispatching a second event.
        assert(world->publishBoxAlarm0(key,0)&&world->publishBoxHealth(key,MotionBinary64::fromInteger(3)));
        world->step(0);
        assert(world->effectsSucceeded()&&world->findBreakable(key)->alarm0==-1&&
            world->findBreakable(key)->state.wallHp.high==0x40080000u);
        assert(world->publishBoxAlarm0(key,-2));world->step(0);
        assert(world->effectsSucceeded()&&world->findBreakable(key)->alarm0==-2);
        if(registered){
            assert(world->breakableActiveAt(x,y));
            assert(world->publishBoxAlarm0(key,1)&&world->markBreakableDestroyed(key));
            assert(world->markBreakableDestroyed(key)&&world->findBreakable(key)&&!world->breakableActiveAt(x,y));
            // The event continues, but compatibility collision rejects the
            // destroyed self (flag 1), even while it overlaps a live field.
            assert(owner->stopBoxAlarm0(key,true,480,masks)&&world->findBreakable(key)->alarm0==2);
            world->step(0);
            assert(world->effectsSucceeded()&&world->findBreakable(key)->alarm0==2&&
                world->findBreakable(key)->state.wallHp.high==0x40080000u);
            assert(owner->prepareDraw()&&!world->findBreakable(key)&&!world->breakableActiveAt(x,y));
        }
        assert(world->restartFromResults(DeathMenuSequence::Action::Retry,2,300)!=ResultsDestination::none);
        assert(world->appendBreakables(&x,&y,1));
        const InstanceHandle fresh{0,2};unsigned queries=0;
        assert(world->publishBoxAlarm0(fresh,5));
        assert(!world->stopBoxAlarm0(fresh,[&](bool&){++queries;return false;})&&queries==1&&
            world->findBreakable(fresh)->alarm0==5&&world->findBreakable(fresh)->state.scriptIndex==0);
        assert(!world->stopBoxAlarm0(fresh,[&](bool&){++queries;return true;})&&queries==1);
        assert(!world->publishBoxAlarm0(key,1)&&!world->effectsSucceeded());
    }
    for(bool retire:{false,true}){
        auto world=std::make_unique<GameplayWorld>(300,true);
        const int x=64,y=512;assert(world->appendBreakables(&x,&y,1));
        const InstanceHandle box{0,1};unsigned queries=0;
        assert(world->publishBoxAlarm0(box,5));
        assert(!world->stopBoxAlarm0(box,[&](bool& allowed){
            ++queries;allowed=false;
            if(retire){
                assert(world->markBreakableDestroyed(box)&&world->retireMarkedBreakable(box));
            }else{
                // A callback cannot conceal a nested ownership failure by
                // returning true to its enclosing event.
                assert(!world->publishBoxAlarm0({box.slot,box.generation+1},17));
            }
            return true;
        })&&!world->effectsSucceeded()&&queries==1);
        if(!retire)assert(world->findBreakable(box)->alarm0==5);
        assert(!world->stopBoxAlarm0(box,[&](bool&){++queries;return true;})&&queries==1);
    }
    std::printf("Gameplay collision owner bytes (host): %zu\n",sizeof(GameplayCollisionOwner));
    std::puts("Native collision owner: actor/terrain births, dormant flags, movement, compaction, retirement, Retry and terminal failure passed");
}
#endif

#ifndef DOWNWELL_CORE_GAMEPLAY_COLLISION_VIEW_H
#define DOWNWELL_CORE_GAMEPLAY_COLLISION_VIEW_H
#include "GameplayWorld.h"
namespace downwell {
// Disjoint keys for the native world's existing typed stores. These are
// internal owner slots, NOT Windows numeric instance IDs or traversal order.
// Resolve every use: enemy records move during compaction, but identitySlot
// and lifetimeId do not. Borrowed views never cache pointers across a mutation.
class GameplayCollisionView {
public:
    enum class Kind {player,enemy,breakable,terrain,timeField,goalField,exitController,bullet,scriptEffect,debris,timeChange,entryDither,drone,bone,skeleton,blood,currency,ditherAbove,builder,timeShard,playerController,corpseExpiry,smoke,corpse,consumptionBlood,runController,result,levelNumber,limboShard,flashBall,blast,sideCameraLocker,scattered,spawner,whiteJelly,movingFx,redBall};
    static constexpr unsigned enemyBase=1;
    static constexpr unsigned breakableBase=enemyBase+GameplayWorld::enemyCapacity;
    static constexpr unsigned terrainBase=breakableBase+GameplayWorld::breakableCapacity;
    static constexpr unsigned timeFieldBase=terrainBase+GameplayTerrainInstances::capacity;
    static constexpr unsigned goalFieldBase=timeFieldBase+64;
    static constexpr unsigned exitControllerBase=goalFieldBase+GameplayGoalFields::capacity;
    static constexpr unsigned bulletBase=exitControllerBase+GameplayExitControllers::capacity;
    static constexpr unsigned scriptEffectBase=bulletBase+GameplayWorld::bulletCapacity;
    static constexpr unsigned debrisBase=scriptEffectBase+GameplayScriptSpawnInstances::capacity;
    static constexpr unsigned timeChangeBase=debrisBase+GameplayDebrisInstances::capacity;
    static constexpr unsigned entryDitherBase=timeChangeBase+8;
    static constexpr unsigned droneBase=entryDitherBase+GameplayEntryDithers::capacity;
    static constexpr unsigned boneBase=droneBase+GameplayWorld::droneBulletCapacity;
    static constexpr unsigned skeletonBase=boneBase+GameplayWorld::boneCapacity;
    static constexpr unsigned bloodBase=skeletonBase+GameplayWorld::skeletonCapacity;
    static constexpr unsigned currencyBase=bloodBase+GameplayWorld::bloodCapacity;
    static constexpr unsigned ditherAboveBase=currencyBase+GameplayWorld::currencyCapacity;
    static constexpr unsigned builderBase=ditherAboveBase+1;
    static constexpr unsigned timeShardBase=builderBase+1;
    static constexpr unsigned playerControllerBase=timeShardBase+64;
    static constexpr unsigned corpseExpiryBase=playerControllerBase+GameplayPlayerControllers::capacity;
    static constexpr unsigned smokeBase=corpseExpiryBase+32;
    static constexpr unsigned corpseBase=smokeBase+32;
    static constexpr unsigned consumptionBloodBase=corpseBase+32;
    static constexpr unsigned runControllerBase=consumptionBloodBase+32;
    static constexpr unsigned resultBase=runControllerBase+4;
    static constexpr unsigned levelNumberBase=resultBase+1;
    static constexpr unsigned limboShardBase=levelNumberBase+4;
    static constexpr unsigned flashBallBase=limboShardBase+32;
    static constexpr unsigned blastBase=flashBallBase+32;
    static constexpr unsigned sideCameraLockerBase=blastBase+8;
    static constexpr unsigned scatteredBase=sideCameraLockerBase+SideCameraLockers::capacity;
    static constexpr unsigned spawnerBase=scatteredBase+GameplayScattered::capacity;
    static constexpr unsigned whiteJellyBase=spawnerBase+GameplaySpawners::capacity;
    static constexpr unsigned movingFxBase=whiteJellyBase+GameplayWhiteJellies::capacity;
    static constexpr unsigned redBallBase=movingFxBase+32;
    static constexpr unsigned capacity=redBallBase+GameplayRedBalls::capacity;
    struct Record {
        const GameplayWorld::MovingEffects::Record* movingFx=nullptr;
        const GameplaySpawners::Record* spawner=nullptr;
        const GameplayWhiteJellies::Record* whiteJelly=nullptr;
        const GameplayRedBalls::Record* redBall=nullptr;
        const GameplayScattered::Record* scattered=nullptr;
        const SideCameraLocker* sideCameraLocker=nullptr;
        const GameplayWorld::RegisteredBlasts::Record* blast=nullptr;
        const GameplayWorld::RegisteredExpiry::Record* corpseExpiry=nullptr;
        const GameplayWorld::RegisteredSmoke::Record* smoke=nullptr;
        const GameplayWorld::RegisteredCorpse::Record* corpse=nullptr;
        const GameplayWorld::ConsumptionParticles::Record* consumptionBlood=nullptr;
        const GameplayWorld::RunControllers::Record* runController=nullptr;
        const GameplayResultInstance::Record* result=nullptr;
        const GameplayWorld::LevelNumbers::Record* levelNumber=nullptr;
        const GameplayWorld::LimboShards::Record* limboShard=nullptr;
        const GameplayWorld::RegisteredFlash::Record* flashBall=nullptr;
        const GameplayWorld::PlayerInstance* player=nullptr;
        const GameplayWorld::EnemyState* enemy=nullptr;
        const GameplayWorld::Breakable* breakable=nullptr;
        decltype(static_cast<const GameplayTerrainInstances*>(nullptr)->find(InstanceHandle{})) terrain=nullptr;
        // Native runs produce values, not stable per-cell Record addresses.
        // Read them explicitly with readTerrain; never fabricate a pointer.
        bool nativeTerrain=false;
        const OrdinaryTimeField* timeField=nullptr;
        const GameplayGoalFields::Record* goalField=nullptr;
        const GameplayExitControllers::Record* exitController=nullptr;
        const GameplayWorld::BulletState* bullet=nullptr;
        const GameplayScriptSpawnInstances::Record* scriptEffect=nullptr;
        const GameplayDebrisInstances::Record* debris=nullptr;
        const NativeTimeChangeRegistration::Record* timeChange=nullptr;
        const NativeTimeShardRegistration::Record* timeShard=nullptr;
        const GameplayEntryDithers::Record* entryDither=nullptr;
        const GameplayWorld::DroneBullets::Record* drone=nullptr;
        const GameplayWorld::Bones::Record* bone=nullptr;
        const GameplayWorld::Skeletons::Record* skeleton=nullptr;
        const GameplayWorld::Blood::Record* blood=nullptr;
        const GameplayWorld::Currency::Record* currency=nullptr;
        const GameplayDitherAbove::Record* ditherAbove=nullptr;
        const GameplayBuilderController::Record* builder=nullptr;
        const GameplayPlayerControllers::Record* playerController=nullptr;
    };
    explicit GameplayCollisionView(const GameplayWorld& world):world_(world){}
    static bool encode(Kind kind,InstanceHandle local,InstanceHandle& key){
        if(!local.generation)return false;
        unsigned base=0,count=0;
        switch(kind){
            case Kind::redBall:base=redBallBase;count=GameplayRedBalls::capacity;break;
            case Kind::movingFx:base=movingFxBase;count=32;break;
            case Kind::scattered:base=scatteredBase;count=GameplayScattered::capacity;break;
            case Kind::spawner:base=spawnerBase;count=GameplaySpawners::capacity;break;
            case Kind::whiteJelly:base=whiteJellyBase;count=GameplayWhiteJellies::capacity;break;
            case Kind::sideCameraLocker:base=sideCameraLockerBase;count=SideCameraLockers::capacity;break;
            case Kind::player:count=1;break;
            case Kind::enemy:base=enemyBase;count=GameplayWorld::enemyCapacity;break;
            case Kind::breakable:base=breakableBase;count=GameplayWorld::breakableCapacity;break;
            case Kind::terrain:base=terrainBase;count=GameplayTerrainInstances::capacity;break;
            case Kind::timeField:base=timeFieldBase;count=64;break;
            case Kind::goalField:base=goalFieldBase;count=GameplayGoalFields::capacity;break;
            case Kind::exitController:base=exitControllerBase;count=GameplayExitControllers::capacity;break;
            case Kind::bullet:base=bulletBase;count=GameplayWorld::bulletCapacity;break;
            case Kind::scriptEffect:base=scriptEffectBase;count=GameplayScriptSpawnInstances::capacity;break;
            case Kind::corpseExpiry:base=corpseExpiryBase;count=32;break;
            case Kind::smoke:base=smokeBase;count=32;break;
            case Kind::blast:base=blastBase;count=8;break;
            case Kind::corpse:base=corpseBase;count=32;break;
            case Kind::consumptionBlood:base=consumptionBloodBase;count=32;break;
            case Kind::runController:base=runControllerBase;count=4;break;
            case Kind::result:base=resultBase;count=1;break;
            case Kind::levelNumber:base=levelNumberBase;count=4;break;
            case Kind::limboShard:base=limboShardBase;count=32;break;
            case Kind::flashBall:base=flashBallBase;count=32;break;
            case Kind::debris:base=debrisBase;count=GameplayDebrisInstances::capacity;break;
            case Kind::timeChange:base=timeChangeBase;count=8;break;
            case Kind::timeShard:base=timeShardBase;count=64;break;
            case Kind::playerController:base=playerControllerBase;count=GameplayPlayerControllers::capacity;break;
            case Kind::entryDither:base=entryDitherBase;count=GameplayEntryDithers::capacity;break;
            case Kind::drone:base=droneBase;count=GameplayWorld::droneBulletCapacity;break;
            case Kind::bone:base=boneBase;count=GameplayWorld::boneCapacity;break;
            case Kind::skeleton:base=skeletonBase;count=GameplayWorld::skeletonCapacity;break;
            case Kind::blood:base=bloodBase;count=GameplayWorld::bloodCapacity;break;
            case Kind::currency:base=currencyBase;count=GameplayWorld::currencyCapacity;break;
            case Kind::ditherAbove:base=ditherAboveBase;count=1;break;
            case Kind::builder:base=builderBase;count=1;break;
            default:return false;
        }
        if(local.slot>=count)return false;
        key={base+local.slot,local.generation};return true;
    }
    static bool decode(InstanceHandle key,Kind& kind,InstanceHandle& local){
        if(!key.generation||key.slot>=capacity)return false;
        if(key.slot>=redBallBase){kind=Kind::redBall;local={key.slot-redBallBase,key.generation};return true;}
        if(key.slot>=movingFxBase){kind=Kind::movingFx;local={key.slot-movingFxBase,key.generation};return true;}
        if(key.slot>=whiteJellyBase){kind=Kind::whiteJelly;local={key.slot-whiteJellyBase,key.generation};return true;}
        if(key.slot>=spawnerBase){kind=Kind::spawner;local={key.slot-spawnerBase,key.generation};return true;}
        if(key.slot>=scatteredBase){kind=Kind::scattered;local={key.slot-scatteredBase,key.generation};return true;}
        if(key.slot>=sideCameraLockerBase){kind=Kind::sideCameraLocker;local={key.slot-sideCameraLockerBase,key.generation};return true;}
        if(key.slot>=blastBase){kind=Kind::blast;local={key.slot-blastBase,key.generation};return true;}
        if(key.slot>=flashBallBase){kind=Kind::flashBall;local={key.slot-flashBallBase,key.generation};return true;}
        if(key.slot>=limboShardBase){kind=Kind::limboShard;local={key.slot-limboShardBase,key.generation};return true;}
        if(key.slot>=levelNumberBase){kind=Kind::levelNumber;local={key.slot-levelNumberBase,key.generation};return true;}
        if(key.slot>=resultBase){kind=Kind::result;local={key.slot-resultBase,key.generation};return true;}
        if(key.slot>=runControllerBase){kind=Kind::runController;local={key.slot-runControllerBase,key.generation};return true;}
        if(key.slot>=consumptionBloodBase){kind=Kind::consumptionBlood;local={key.slot-consumptionBloodBase,key.generation};return true;}
        if(key.slot>=corpseBase){kind=Kind::corpse;local={key.slot-corpseBase,key.generation};return true;}
        if(key.slot>=smokeBase){kind=Kind::smoke;local={key.slot-smokeBase,key.generation};return true;}
        if(key.slot>=corpseExpiryBase){kind=Kind::corpseExpiry;local={key.slot-corpseExpiryBase,key.generation};return true;}
        if(key.slot>=playerControllerBase){kind=Kind::playerController;local={key.slot-playerControllerBase,key.generation};return true;}
        if(key.slot>=timeShardBase){kind=Kind::timeShard;local={key.slot-timeShardBase,key.generation};return true;}
        if(key.slot>=builderBase){kind=Kind::builder;local={key.slot-builderBase,key.generation};return true;}
        if(key.slot>=ditherAboveBase){kind=Kind::ditherAbove;local={key.slot-ditherAboveBase,key.generation};return true;}
        if(key.slot>=currencyBase){kind=Kind::currency;local={key.slot-currencyBase,key.generation};return true;}
        if(key.slot>=bloodBase){kind=Kind::blood;local={key.slot-bloodBase,key.generation};return true;}
        if(key.slot>=skeletonBase){kind=Kind::skeleton;local={key.slot-skeletonBase,key.generation};return true;}
        if(key.slot>=boneBase){kind=Kind::bone;local={key.slot-boneBase,key.generation};return true;}
        if(key.slot>=droneBase){kind=Kind::drone;local={key.slot-droneBase,key.generation};return true;}
        if(key.slot>=entryDitherBase){kind=Kind::entryDither;local={key.slot-entryDitherBase,key.generation};return true;}
        if(key.slot>=timeChangeBase){kind=Kind::timeChange;local={key.slot-timeChangeBase,key.generation};return true;}
        const unsigned base=key.slot<enemyBase?0:key.slot<breakableBase?enemyBase:key.slot<terrainBase?breakableBase:key.slot<timeFieldBase?terrainBase:key.slot<goalFieldBase?timeFieldBase:key.slot<exitControllerBase?goalFieldBase:key.slot<bulletBase?exitControllerBase:key.slot<scriptEffectBase?bulletBase:key.slot<debrisBase?scriptEffectBase:debrisBase;
        kind=base==0?Kind::player:base==enemyBase?Kind::enemy:base==breakableBase?Kind::breakable:base==terrainBase?Kind::terrain:base==timeFieldBase?Kind::timeField:base==goalFieldBase?Kind::goalField:base==exitControllerBase?Kind::exitController:base==bulletBase?Kind::bullet:base==scriptEffectBase?Kind::scriptEffect:Kind::debris;
        local={key.slot-base,key.generation};return true;
    }
    bool resolve(InstanceHandle key,Record& record)const{
        return resolveImpl<true>(key,&record);
    }
    // The same typed-store/generation validation without constructing a wide
    // borrowed-record result when a caller only needs lifetime membership.
    bool contains(InstanceHandle key)const{return resolveImpl<false>(key,nullptr);}
    bool readTerrain(InstanceHandle key,GameplayWorld::NativeTerrain::Identity& identity,
        bool& active,bool& destroyed)const{
        Kind kind;InstanceHandle local;
        if(!decode(key,kind,local)||kind!=Kind::terrain)return false;
        if(const auto* native=world_.nativeTerrainInstances())return native->read(local,identity,active,destroyed);
        const auto* store=world_.terrainInstances();
        const auto* r=store?store->find(local):nullptr;if(!r)return false;
        identity={local,r->objectId,r->x,r->y,r->creationCodeId,r->sourceInstanceId};
        active=r->active;destroyed=r->destroyRequested;return true;
    }
    // Typed bindings already know their family. Resolve the same local
    // lifetime without constructing the thirteen-pointer generic record.
    // Nothing is cached across mutation or enemy-store compaction.
    template<Kind Family> auto resolveLocal(InstanceHandle local)const{
        auto lookup=[&](){
            if constexpr(Family==Kind::player)return world_.findPlayer(local);
            else if constexpr(Family==Kind::scattered)return world_.scattered()?world_.scattered()->find(local):nullptr;
            else if constexpr(Family==Kind::spawner)return world_.spawners()?world_.spawners()->find(local):nullptr;
            else if constexpr(Family==Kind::whiteJelly)return world_.whiteJellies()?world_.whiteJellies()->find(local):nullptr;
            else if constexpr(Family==Kind::redBall)return world_.redBalls()?world_.redBalls()->find(local):nullptr;
            else if constexpr(Family==Kind::movingFx)return world_.movingEffects()?world_.movingEffects()->find(local):nullptr;
            else if constexpr(Family==Kind::sideCameraLocker)return world_.sideCameraLockers().find(local);
            else if constexpr(Family==Kind::enemy)return world_.resolveEnemyLifetime(local);
            else if constexpr(Family==Kind::breakable)return world_.findBreakable(local);
            else if constexpr(Family==Kind::terrain)return world_.terrainInstances()?world_.terrainInstances()->find(local):nullptr;
            else if constexpr(Family==Kind::timeField)return world_.timeFields().identityAt(local.slot)==local.generation?
                world_.timeFields().at(local.slot):nullptr;
            else if constexpr(Family==Kind::goalField)return world_.goalFieldInstances().find(local);
            else if constexpr(Family==Kind::exitController)return world_.exitControllers().find(local);
            else if constexpr(Family==Kind::bullet)return world_.resolveBulletLifetime(local);
            else if constexpr(Family==Kind::scriptEffect)return world_.scriptSpawnInstances()?world_.scriptSpawnInstances()->find(local):nullptr;
            else if constexpr(Family==Kind::corpseExpiry)return world_.registeredExpiry()?world_.registeredExpiry()->find(local):nullptr;
            else if constexpr(Family==Kind::blast)return world_.registeredBlasts()?world_.registeredBlasts()->find(local):nullptr;
            else if constexpr(Family==Kind::smoke)return world_.registeredSmoke()?world_.registeredSmoke()->find(local):nullptr;
            else if constexpr(Family==Kind::corpse)return world_.registeredCorpses()?world_.registeredCorpses()->find(local):nullptr;
            else if constexpr(Family==Kind::consumptionBlood)return world_.consumptionParticles()?world_.consumptionParticles()->find(local):nullptr;
            else if constexpr(Family==Kind::limboShard)return world_.limboShards()?world_.limboShards()->find(local):nullptr;
            else if constexpr(Family==Kind::flashBall)return world_.registeredFlash()?world_.registeredFlash()->find(local):nullptr;
            else if constexpr(Family==Kind::runController)return world_.runControllers()?world_.runControllers()->find(local):nullptr;
            else if constexpr(Family==Kind::result)return world_.resultInstance()?world_.resultInstance()->find(local):nullptr;
            else if constexpr(Family==Kind::levelNumber)return world_.levelNumbers()?world_.levelNumbers()->find(local):nullptr;
            else if constexpr(Family==Kind::debris)return world_.debrisInstances()?world_.debrisInstances()->find(local):nullptr;
            else if constexpr(Family==Kind::timeChange)return world_.timeChangeInstances()?world_.timeChangeInstances()->find(local):nullptr;
            else if constexpr(Family==Kind::timeShard)return world_.timeShardInstances()?world_.timeShardInstances()->find(local):nullptr;
            else if constexpr(Family==Kind::entryDither)return world_.entryDithers()?world_.entryDithers()->find(local):nullptr;
            else if constexpr(Family==Kind::drone)return world_.droneBullets()?world_.droneBullets()->find(local):nullptr;
            else if constexpr(Family==Kind::bone)return world_.bones()?world_.bones()->find(local):nullptr;
            else if constexpr(Family==Kind::blood)return world_.bloodInstances()?world_.bloodInstances()->find(local):nullptr;
            else if constexpr(Family==Kind::currency)return world_.currencyInstances()?world_.currencyInstances()->find(local):nullptr;
            else if constexpr(Family==Kind::ditherAbove)return world_.ditherAbove()?world_.ditherAbove()->find(local):nullptr;
            else if constexpr(Family==Kind::builder)return world_.builderController()?world_.builderController()->find(local):nullptr;
            else if constexpr(Family==Kind::playerController)return world_.playerControllers()?world_.playerControllers()->find(local):nullptr;
            else {static_assert(Family==Kind::skeleton,"Unknown collision family");
                return world_.skeletons()?world_.skeletons()->find(local):nullptr;}
        };
        using Pointer=decltype(lookup());
        InstanceHandle shared;
        return encode(Family,local,shared)?lookup():Pointer(nullptr);
    }
private:
    template<bool Publish> bool resolveImpl(InstanceHandle key,Record* record)const{
        Kind kind;InstanceHandle local;
        if(!decode(key,kind,local))return false;
        Record next;
        switch(kind){
            case Kind::redBall:
                if(!world_.redBalls()||!(next.redBall=world_.redBalls()->find(local)))return false;
                break;
            case Kind::movingFx:
                if(!world_.movingEffects()||!(next.movingFx=world_.movingEffects()->find(local)))return false;
                break;
            case Kind::whiteJelly:
                if(!world_.whiteJellies()||!(next.whiteJelly=world_.whiteJellies()->find(local)))return false;
                break;
            case Kind::spawner:
                if(!world_.spawners()||!(next.spawner=world_.spawners()->find(local)))return false;
                break;
            case Kind::scattered:
                if(!world_.scattered()||!(next.scattered=world_.scattered()->find(local)))return false;
                break;
            case Kind::playerController:
                if(!world_.playerControllers()||!(next.playerController=world_.playerControllers()->find(local)))return false;
                break;
            case Kind::timeShard:
                if(!world_.timeShardInstances()||!(next.timeShard=world_.timeShardInstances()->find(local)))return false;
                break;
            case Kind::player:next.player=world_.findPlayer(local);if(!next.player)return false;break;
            case Kind::enemy:next.enemy=world_.resolveEnemyLifetime(local);if(!next.enemy)return false;break;
            case Kind::breakable:next.breakable=world_.findBreakable(local);if(!next.breakable)return false;break;
            case Kind::terrain:
                if(const auto* native=world_.nativeTerrainInstances()){
                    if(!native->contains(local))return false;
                    next.nativeTerrain=true;
                }else if(!world_.terrainInstances()||!(next.terrain=world_.terrainInstances()->find(local)))return false;
                break;
            case Kind::timeField:
                if(world_.timeFields().identityAt(local.slot)!=local.generation||
                    !(next.timeField=world_.timeFields().at(local.slot)))return false;
                break;
            case Kind::goalField:
                if(!(next.goalField=world_.goalFieldInstances().find(local)))return false;
                break;
            case Kind::exitController:
                if(!(next.exitController=world_.exitControllers().find(local)))return false;
                break;
            case Kind::sideCameraLocker:
                if(!(next.sideCameraLocker=world_.sideCameraLockers().find(local)))return false;
                break;
            case Kind::bullet:
                if(!(next.bullet=world_.resolveBulletLifetime(local)))return false;
                break;
            case Kind::scriptEffect:
                if(!world_.scriptSpawnInstances()||!(next.scriptEffect=world_.scriptSpawnInstances()->find(local)))return false;
                break;
            case Kind::corpseExpiry:
                if(!world_.registeredExpiry()||!(next.corpseExpiry=world_.registeredExpiry()->find(local)))return false;
                break;
            case Kind::consumptionBlood:
                if(!world_.consumptionParticles()||!(next.consumptionBlood=world_.consumptionParticles()->find(local)))return false;
                break;
            case Kind::runController:
                if(!world_.runControllers()||!(next.runController=world_.runControllers()->find(local)))return false;
                break;
            case Kind::result:
                if(!world_.resultInstance()||!(next.result=world_.resultInstance()->find(local)))return false;
                break;
            case Kind::levelNumber:
                if(!world_.levelNumbers()||!(next.levelNumber=world_.levelNumbers()->find(local)))return false;
                break;
            case Kind::limboShard:
                if(!world_.limboShards()||!(next.limboShard=world_.limboShards()->find(local)))return false;
                break;
            case Kind::flashBall:
                if(!world_.registeredFlash()||!(next.flashBall=world_.registeredFlash()->find(local)))return false;
                break;
            case Kind::corpse:
                if(!world_.registeredCorpses()||!(next.corpse=world_.registeredCorpses()->find(local)))return false;
                break;
            case Kind::blast:
                if(!world_.registeredBlasts()||!(next.blast=world_.registeredBlasts()->find(local)))return false;
                break;
            case Kind::smoke:
                if(!world_.registeredSmoke()||!(next.smoke=world_.registeredSmoke()->find(local)))return false;
                break;
            case Kind::debris:
                if(!world_.debrisInstances()||!(next.debris=world_.debrisInstances()->find(local)))return false;
                break;
            case Kind::timeChange:
                if(!world_.timeChangeInstances()||!(next.timeChange=world_.timeChangeInstances()->find(local)))return false;
                break;
            case Kind::entryDither:
                if(!world_.entryDithers()||!(next.entryDither=world_.entryDithers()->find(local)))return false;
                break;
            case Kind::drone:
                if(!world_.droneBullets()||!(next.drone=world_.droneBullets()->find(local)))return false;
                break;
            case Kind::bone:
                if(!world_.bones()||!(next.bone=world_.bones()->find(local)))return false;
                break;
            case Kind::blood:
                if(!world_.bloodInstances()||!(next.blood=world_.bloodInstances()->find(local)))return false;
                break;
            case Kind::currency:
                if(!world_.currencyInstances()||!(next.currency=world_.currencyInstances()->find(local)))return false;
                break;
            case Kind::ditherAbove:
                if(!world_.ditherAbove()||!(next.ditherAbove=world_.ditherAbove()->find(local)))return false;
                break;
            case Kind::builder:
                if(!world_.builderController()||!(next.builder=world_.builderController()->find(local)))return false;
                break;
            case Kind::skeleton:
                if(!world_.skeletons()||!(next.skeleton=world_.skeletons()->find(local)))return false;
                break;
        }
        if constexpr(Publish)*record=next;
        return true;
    }
private:
    const GameplayWorld& world_;
};
}
#endif

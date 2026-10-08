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
    enum class Kind {player,enemy,breakable,terrain,timeField,goalField,exitController,bullet,scriptEffect,debris,timeChange,entryDither,drone,bone};
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
    static constexpr unsigned capacity=boneBase+GameplayWorld::boneCapacity;
    struct Record {
        const GameplayWorld::PlayerInstance* player=nullptr;
        const GameplayWorld::EnemyState* enemy=nullptr;
        const GameplayWorld::Breakable* breakable=nullptr;
        decltype(static_cast<const GameplayTerrainInstances*>(nullptr)->find(InstanceHandle{})) terrain=nullptr;
        const OrdinaryTimeField* timeField=nullptr;
        const GameplayGoalFields::Record* goalField=nullptr;
        const GameplayExitControllers::Record* exitController=nullptr;
        const GameplayWorld::BulletState* bullet=nullptr;
        const GameplayScriptSpawnInstances::Record* scriptEffect=nullptr;
        const GameplayDebrisInstances::Record* debris=nullptr;
        const NativeTimeChangeRegistration::Record* timeChange=nullptr;
        const GameplayEntryDithers::Record* entryDither=nullptr;
        const GameplayWorld::DroneBullets::Record* drone=nullptr;
        const GameplayWorld::Bones::Record* bone=nullptr;
    };
    explicit GameplayCollisionView(const GameplayWorld& world):world_(world){}
    static bool encode(Kind kind,InstanceHandle local,InstanceHandle& key){
        if(!local.generation)return false;
        unsigned base=0,count=0;
        switch(kind){
            case Kind::player:count=1;break;
            case Kind::enemy:base=enemyBase;count=GameplayWorld::enemyCapacity;break;
            case Kind::breakable:base=breakableBase;count=GameplayWorld::breakableCapacity;break;
            case Kind::terrain:base=terrainBase;count=GameplayTerrainInstances::capacity;break;
            case Kind::timeField:base=timeFieldBase;count=64;break;
            case Kind::goalField:base=goalFieldBase;count=GameplayGoalFields::capacity;break;
            case Kind::exitController:base=exitControllerBase;count=GameplayExitControllers::capacity;break;
            case Kind::bullet:base=bulletBase;count=GameplayWorld::bulletCapacity;break;
            case Kind::scriptEffect:base=scriptEffectBase;count=GameplayScriptSpawnInstances::capacity;break;
            case Kind::debris:base=debrisBase;count=GameplayDebrisInstances::capacity;break;
            case Kind::timeChange:base=timeChangeBase;count=8;break;
            case Kind::entryDither:base=entryDitherBase;count=GameplayEntryDithers::capacity;break;
            case Kind::drone:base=droneBase;count=GameplayWorld::droneBulletCapacity;break;
            case Kind::bone:base=boneBase;count=GameplayWorld::boneCapacity;break;
            default:return false;
        }
        if(local.slot>=count)return false;
        key={base+local.slot,local.generation};return true;
    }
    static bool decode(InstanceHandle key,Kind& kind,InstanceHandle& local){
        if(!key.generation||key.slot>=capacity)return false;
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
    // Typed bindings already know their family. Resolve the same local
    // lifetime without constructing the thirteen-pointer generic record.
    // Nothing is cached across mutation or enemy-store compaction.
    template<Kind Family> auto resolveLocal(InstanceHandle local)const{
        auto lookup=[&](){
            if constexpr(Family==Kind::player)return world_.findPlayer(local);
            else if constexpr(Family==Kind::enemy)return world_.resolveEnemyLifetime(local);
            else if constexpr(Family==Kind::breakable)return world_.findBreakable(local);
            else if constexpr(Family==Kind::terrain)return world_.terrainInstances()?world_.terrainInstances()->find(local):nullptr;
            else if constexpr(Family==Kind::timeField)return world_.timeFields().identityAt(local.slot)==local.generation?
                world_.timeFields().at(local.slot):nullptr;
            else if constexpr(Family==Kind::goalField)return world_.goalFieldInstances().find(local);
            else if constexpr(Family==Kind::exitController)return world_.exitControllers().find(local);
            else if constexpr(Family==Kind::bullet)return world_.resolveBulletLifetime(local);
            else if constexpr(Family==Kind::scriptEffect)return world_.scriptSpawnInstances()?world_.scriptSpawnInstances()->find(local):nullptr;
            else if constexpr(Family==Kind::debris)return world_.debrisInstances()?world_.debrisInstances()->find(local):nullptr;
            else if constexpr(Family==Kind::timeChange)return world_.timeChangeInstances()?world_.timeChangeInstances()->find(local):nullptr;
            else if constexpr(Family==Kind::entryDither)return world_.entryDithers()?world_.entryDithers()->find(local):nullptr;
            else if constexpr(Family==Kind::drone)return world_.droneBullets()?world_.droneBullets()->find(local):nullptr;
            else {static_assert(Family==Kind::bone,"Unknown collision family");
                return world_.bones()?world_.bones()->find(local):nullptr;}
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
            case Kind::player:next.player=world_.findPlayer(local);if(!next.player)return false;break;
            case Kind::enemy:next.enemy=world_.resolveEnemyLifetime(local);if(!next.enemy)return false;break;
            case Kind::breakable:next.breakable=world_.findBreakable(local);if(!next.breakable)return false;break;
            case Kind::terrain:
                if(!world_.terrainInstances()||!(next.terrain=world_.terrainInstances()->find(local)))return false;
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
            case Kind::bullet:
                if(!(next.bullet=world_.resolveBulletLifetime(local)))return false;
                break;
            case Kind::scriptEffect:
                if(!world_.scriptSpawnInstances()||!(next.scriptEffect=world_.scriptSpawnInstances()->find(local)))return false;
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
        }
        if constexpr(Publish)*record=next;
        return true;
    }
private:
    const GameplayWorld& world_;
};
}
#endif

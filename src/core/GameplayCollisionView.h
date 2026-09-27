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
    enum class Kind {player,enemy,breakable,terrain,timeField};
    static constexpr unsigned enemyBase=1;
    static constexpr unsigned breakableBase=enemyBase+GameplayWorld::enemyCapacity;
    static constexpr unsigned terrainBase=breakableBase+GameplayWorld::breakableCapacity;
    static constexpr unsigned timeFieldBase=terrainBase+GameplayTerrainInstances::capacity;
    static constexpr unsigned capacity=timeFieldBase+64;
    struct Record {
        const GameplayWorld::PlayerInstance* player=nullptr;
        const GameplayWorld::EnemyState* enemy=nullptr;
        const GameplayWorld::Breakable* breakable=nullptr;
        const GameplayTerrainInstances::Record* terrain=nullptr;
        const OrdinaryTimeField* timeField=nullptr;
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
            default:return false;
        }
        if(local.slot>=count)return false;
        key={base+local.slot,local.generation};return true;
    }
    static bool decode(InstanceHandle key,Kind& kind,InstanceHandle& local){
        if(!key.generation||key.slot>=capacity)return false;
        const unsigned base=key.slot<enemyBase?0:key.slot<breakableBase?enemyBase:key.slot<terrainBase?breakableBase:key.slot<timeFieldBase?terrainBase:timeFieldBase;
        kind=base==0?Kind::player:base==enemyBase?Kind::enemy:base==breakableBase?Kind::breakable:base==terrainBase?Kind::terrain:Kind::timeField;
        local={key.slot-base,key.generation};return true;
    }
    bool resolve(InstanceHandle key,Record& record)const{
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
        }
        record=next;return true;
    }
private:
    const GameplayWorld& world_;
};
}
#endif

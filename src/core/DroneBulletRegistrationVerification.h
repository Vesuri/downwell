#ifndef DOWNWELL_DRONE_BULLET_REGISTRATION_VERIFICATION_H
#define DOWNWELL_DRONE_BULLET_REGISTRATION_VERIFICATION_H
#include "GameplayCollisionOwner.h"
namespace downwell {
// Resource-backed publication tape, not admission of natural projectile births.
struct DroneBulletRegistrationVerification {
    static bool run(GameplayWorld& world,GameplayCollisionOwner& owner){
        using View=GameplayCollisionView;
        auto& store=owner.droneBullets();
        const RunnerImageValue x{0x43704000u},y{0x43308000u};
        InstanceHandle local,shared;
        if(!store.create(105190,x.promoted(),y.promoted(),local)||
            !View::encode(View::Kind::drone,local,shared))return false;
        View::Record record;
        const auto* properties=owner.properties(shared);const auto* state=owner.state(shared);
        unsigned count=0;
        if(!View(world).resolve(shared,record)||record.drone!=store.find(local)||!state||!properties||
            state->x.bits!=x.bits||state->y.bits!=y.bits||properties->sprite.spriteId!=459||
            !owner.population(266,count)||count!=1||!owner.population(89,count)||count!=1)return false;
        if(!store.dispatch(local,[](auto&,auto& publish,bool&){
            return publish.publishAngle({0x42b40000u})&&publish.publishImageIndex({0x3f000000u})&&
                publish.publishPosition({0x43708000u},{0x4330c000u});
        }))return false;
        properties=owner.properties(shared);state=owner.state(shared);
        if(!properties||!state||properties->sprite.angle.bits!=0x42b40000u||
            properties->sprite.imageIndex.bits!=0x3f000000u||state->x.bits!=0x43708000u||
            state->y.bits!=0x4330c000u)return false;
        if(!store.activate(local,false)||!owner.reconcileActivation()||
            !store.advanceAlarm(local)||store.find(local)->state.alarm0!=16||
            !store.activate(local,true)||!owner.reconcileActivation())return false;
        for(unsigned i=0;i<16;++i)if(!store.advanceAlarm(local))return false;
        if(!store.find(local)->state.decelerate)return false;
        // Destruction excludes queries but does not interrupt the caller's
        // remaining x/y setters or invalidate the retained collision record.
        if(!store.dispatch(local,[](auto&,auto& publish,bool&){
            return publish.destroySelf()&&publish.publishPosition({0x4370c000u},{0x43310000u});
        })||!View(world).contains(shared)||!owner.state(shared)||
            !(owner.state(shared)->flags&1u)||owner.state(shared)->x.bits!=0x4370c000u||
            !owner.prepareDraw()||View(world).contains(shared)||store.find(local)||
            !owner.population(266,count)||count)return false;
        InstanceHandle replacement;
        if(!store.create(105191,x.promoted(),y.promoted(),replacement)||replacement.slot!=local.slot||
            replacement.generation<=local.generation||View(world).contains(shared)||
            !store.clear()||!owner.prepareDraw()||store.size())return false;
        return owner.valid()&&world.effectsSucceeded();
    }
};
}
#endif

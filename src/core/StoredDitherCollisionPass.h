#ifndef DOWNWELL_CORE_STORED_DITHER_COLLISION_PASS_H
#define DOWNWELL_CORE_STORED_DITHER_COLLISION_PASS_H
#include "StoredWorldCollision.h"
namespace downwell {
// Migration boundary for the authored wall-pruning controller. Collection uses
// the complete owner queue/tree order; the implemented 61/60 pair and, when
// explicitly bound, registered 225/60, 223/60, 261/120 and entry/exit-field/player pairs are executed here. Other families
// still use the existing gameplay adapters.
// This is not a second pass to run alongside a complete collision dispatcher.
class StoredDitherCollisionPass {
public:
    explicit StoredDitherCollisionPass(StoredWorldCollision& owner):owner_(owner){}
    // Migration admission for source-owned usual bullets and registered
    // skeletons. Use this instead of run(), never as a second family pass.
    // Unbound legacy actors remain with their existing adapters.
    template<unsigned Capacity,class Frames,class Audio,class Atan>
    bool runSkeletonContacts(StartupRuntime& world,
        StoredSkeletonRegistration<StartupRuntime::instanceCapacity,Capacity>& skeletons,
        Frames frames,SoundPlayEnvironment& environment,Audio& audio,Atan atan){
        return runSkeletonContacts(world,skeletons,frames,[&]{return world.requestMomentDelay();},environment,audio,atan);
    }
    // Explicit delay provider remains for source-prefix/reference tests.
    template<unsigned Capacity,class Frames,class Delay,class Audio,class Atan>
    bool runSkeletonContacts(StartupRuntime& world,
        StoredSkeletonRegistration<StartupRuntime::instanceCapacity,Capacity>& skeletons,
        Frames frames,Delay delay,SoundPlayEnvironment& environment,Audio& audio,Atan atan){
        return runSkeletonContactsSelected(world,skeletons,frames,delay,environment,audio,atan,
            [](const auto&,const auto&){return false;},
            [&](const auto& event){return world.dispatchDitherAboveCollision(event);});
    }
    // Damage and its registered smoke/consumption particles share collection,
    // dispatch and deferred destruction with the authored wall controller.
    template<unsigned Capacity,unsigned SmokeCapacity,unsigned ParticleCapacity,class Frames,class Audio,class Atan>
    bool runSkeletonContacts(StartupRuntime& world,
        StoredSkeletonRegistration<StartupRuntime::instanceCapacity,Capacity>& skeletons,
        StoredSmokeRegistration<StartupRuntime::instanceCapacity,SmokeCapacity>& smoke,
        StoredConsumptionBloodRegistration<StartupRuntime::instanceCapacity,ParticleCapacity>& particles,
        Frames frames,SoundPlayEnvironment& environment,Audio& audio,Atan atan){
        if(!smoke.valid()||!particles.valid())return false;
        auto effect=[&](const auto& r){return (r.objectId==225&&smoke.state(r))||
            (r.objectId==223&&particles.state(r));};
        return runSkeletonContactsSelected(world,skeletons,frames,[&]{return world.requestMomentDelay();},
            environment,audio,atan,[&](const auto& a,const auto& b){
                return (b.objectId==60&&effect(a))||(a.objectId==60&&effect(b));
            },[&](const auto& event){
                const auto* self=owner_.resolve(event.self);if(!self)return false;
                if(self->objectId==225)return world.dispatchSmokeCollision(smoke,event);
                if(self->objectId==223)return world.dispatchConsumptionBloodCollision(particles,event);
                return world.dispatchDitherAboveCollision(event);
            });
    }
    // Registered bones share the same collected stream as skeleton damage,
    // fields, smoke and authored wall pruning. This replaces the narrower
    // overload above; it must never be run as a second family collision pass.
    template<unsigned Capacity,unsigned BoneCapacity,unsigned SmokeCapacity,unsigned ParticleCapacity,unsigned StopCapacity,
             class Frames,class Audio,class Atan,class Damage>
    bool runSkeletonContacts(StartupRuntime& world,
        StoredSkeletonRegistration<StartupRuntime::instanceCapacity,Capacity>& skeletons,
        StoredBoneRegistration<StartupRuntime::instanceCapacity,BoneCapacity>& bones,
        StoredSmokeRegistration<StartupRuntime::instanceCapacity,SmokeCapacity>& smoke,
        StoredConsumptionBloodRegistration<StartupRuntime::instanceCapacity,ParticleCapacity>& particles,
        const TimeStopEffects<StopCapacity>& stop,Frames frames,SoundPlayEnvironment& environment,Audio& audio,Atan atan,Damage damage){
        return runSkeletonContactsUsing(world,skeletons,bones,smoke,particles,stop,frames,environment,audio,atan,damage,
            [](const auto&,const auto&){return false;},[](const auto&){return false;});
    }
    // Full registered encounter admission. The player factory must supply the
    // real damage/stomp environment; the narrower overload is retained for
    // callers that have not yet installed that environment.
    template<unsigned Capacity,unsigned BoneCapacity,unsigned SmokeCapacity,unsigned ParticleCapacity,unsigned StopCapacity,
             class Frames,class Audio,class Atan,class Damage,class MakePlayer>
    bool runSkeletonContacts(StartupRuntime& world,
        StoredSkeletonRegistration<StartupRuntime::instanceCapacity,Capacity>& skeletons,
        StoredBoneRegistration<StartupRuntime::instanceCapacity,BoneCapacity>& bones,
        StoredSmokeRegistration<StartupRuntime::instanceCapacity,SmokeCapacity>& smoke,
        StoredConsumptionBloodRegistration<StartupRuntime::instanceCapacity,ParticleCapacity>& particles,
        const TimeStopEffects<StopCapacity>& stop,Frames frames,SoundPlayEnvironment& environment,Audio& audio,
        Atan atan,Damage damage,MakePlayer makePlayer){
        return runSkeletonContactsUsing(world,skeletons,bones,smoke,particles,stop,frames,environment,audio,atan,damage,
            [&](const auto& a,const auto& b){return a.objectId==120&&b.objectId==35&&skeletons.handle(a).generation;},
            [&](const auto& event){return world.dispatchSkeletonCollision(skeletons,event,makePlayer);});
    }
    // Extend the same collected encounter stream with registered blast targets.
    // Do not run this after another encounter pass in the same collision phase.
    template<unsigned Capacity,unsigned BoneCapacity,unsigned SmokeCapacity,unsigned ParticleCapacity,unsigned BlastCapacity,unsigned StopCapacity,
             class Frames,class Audio,class Atan,class Damage,class MakePlayer>
    bool runSkeletonContacts(StartupRuntime& world,
        StoredSkeletonRegistration<StartupRuntime::instanceCapacity,Capacity>& skeletons,
        StoredBoneRegistration<StartupRuntime::instanceCapacity,BoneCapacity>& bones,
        StoredSmokeRegistration<StartupRuntime::instanceCapacity,SmokeCapacity>& smoke,
        StoredConsumptionBloodRegistration<StartupRuntime::instanceCapacity,ParticleCapacity>& particles,
        StoredBlastRegistration<StartupRuntime::instanceCapacity,BlastCapacity>& blasts,
        const TimeStopEffects<StopCapacity>& stop,Frames frames,SoundPlayEnvironment& environment,Audio& audio,
        Atan atan,Damage damage,MakePlayer makePlayer){
        return runBlastContactsUsing(world,skeletons,bones,smoke,particles,blasts,stop,frames,environment,audio,atan,damage,makePlayer,
            [](const auto&,const auto&){return false;},[](const auto&){return false;});
    }
    template<unsigned Capacity,unsigned BoneCapacity,unsigned SmokeCapacity,unsigned ParticleCapacity,unsigned BlastCapacity,unsigned FlashCapacity,unsigned StopCapacity,
             class Frames,class Audio,class Atan,class Damage,class MakePlayer>
    bool runSkeletonContacts(StartupRuntime& world,
        StoredSkeletonRegistration<StartupRuntime::instanceCapacity,Capacity>& skeletons,
        StoredBoneRegistration<StartupRuntime::instanceCapacity,BoneCapacity>& bones,
        StoredSmokeRegistration<StartupRuntime::instanceCapacity,SmokeCapacity>& smoke,
        StoredConsumptionBloodRegistration<StartupRuntime::instanceCapacity,ParticleCapacity>& particles,
        StoredBlastRegistration<StartupRuntime::instanceCapacity,BlastCapacity>& blasts,
        StoredFlashBallRegistration<StartupRuntime::instanceCapacity,FlashCapacity>& flashes,
        const TimeStopEffects<StopCapacity>& stop,Frames frames,SoundPlayEnvironment& environment,Audio& audio,
        Atan atan,Damage damage,MakePlayer makePlayer){
        if(!flashes.valid())return false;
        return runBlastContactsUsing(world,skeletons,bones,smoke,particles,blasts,stop,frames,environment,audio,atan,damage,makePlayer,
            [&](const auto& a,const auto& b){return a.objectId==276&&b.objectId==269&&blasts.handle(a).generation&&bones.handle(b).generation;},
            [&](const auto& event){return world.dispatchBlastBoneCollision(blasts,bones,flashes,event);});
    }
private:
    template<unsigned Capacity,unsigned BoneCapacity,unsigned SmokeCapacity,unsigned ParticleCapacity,unsigned BlastCapacity,unsigned StopCapacity,
             class Frames,class Audio,class Atan,class Damage,class MakePlayer,class ExtraPair,class ExtraEvent>
    bool runBlastContactsUsing(StartupRuntime& world,
        StoredSkeletonRegistration<StartupRuntime::instanceCapacity,Capacity>& skeletons,
        StoredBoneRegistration<StartupRuntime::instanceCapacity,BoneCapacity>& bones,
        StoredSmokeRegistration<StartupRuntime::instanceCapacity,SmokeCapacity>& smoke,
        StoredConsumptionBloodRegistration<StartupRuntime::instanceCapacity,ParticleCapacity>& particles,
        StoredBlastRegistration<StartupRuntime::instanceCapacity,BlastCapacity>& blasts,
        const TimeStopEffects<StopCapacity>& stop,Frames frames,SoundPlayEnvironment& environment,Audio& audio,
        Atan atan,Damage damage,MakePlayer makePlayer,ExtraPair extraPair,ExtraEvent extraEvent){
        if(!owner_.boundTo(world)||!blasts.valid())return false;
        return runSkeletonContactsUsing(world,skeletons,bones,smoke,particles,stop,frames,environment,audio,atan,damage,
            [&](const auto& a,const auto& b){
                return (a.objectId==120&&b.objectId==35&&skeletons.handle(a).generation)||
                    (a.objectId==276&&b.objectId==35&&blasts.handle(a).generation)||
                    (a.objectId==276&&(b.objectId==60||b.objectId==92)&&blasts.handle(a).generation)||
                    (a.objectId==276&&b.objectId==120&&blasts.handle(a).generation&&skeletons.handle(b).generation)||extraPair(a,b);
            },[&](const auto& event){
                const auto* self=owner_.resolve(event.self);if(!self)return false;
                if(self->objectId==276&&event.subtype==35)return world.dispatchBlastPlayerCollision(blasts,event);
                if(self->objectId==276&&event.code==2330)return world.dispatchBlastWallCollision(blasts,event);
                if(self->objectId==276&&event.code==2329)return extraEvent(event);
                return self->objectId==276?world.dispatchBlastSkeletonCollisionWithEffects(blasts,skeletons,event,frames,environment,audio):
                    world.dispatchSkeletonCollision(skeletons,event,makePlayer);
            });
    }
private:
    template<unsigned Capacity,unsigned BoneCapacity,unsigned SmokeCapacity,unsigned ParticleCapacity,unsigned StopCapacity,
             class Frames,class Audio,class Atan,class Damage,class AdmitPlayer,class PlayerContact>
    bool runSkeletonContactsUsing(StartupRuntime& world,
        StoredSkeletonRegistration<StartupRuntime::instanceCapacity,Capacity>& skeletons,
        StoredBoneRegistration<StartupRuntime::instanceCapacity,BoneCapacity>& bones,
        StoredSmokeRegistration<StartupRuntime::instanceCapacity,SmokeCapacity>& smoke,
        StoredConsumptionBloodRegistration<StartupRuntime::instanceCapacity,ParticleCapacity>& particles,
        const TimeStopEffects<StopCapacity>& stop,Frames frames,SoundPlayEnvironment& environment,Audio& audio,
        Atan atan,Damage damage,AdmitPlayer admitPlayer,PlayerContact playerContact){
        if(!stop.succeeded||!bones.valid()||!smoke.valid()||!particles.valid())return false;
        auto effect=[&](const auto& r){return (r.objectId==225&&smoke.state(r))||
            (r.objectId==223&&particles.state(r));};
        auto bonePair=[&](const auto& a,const auto& b){
            // Plain walls have no reverse bone handler. Other wall families
            // need their reverse-event audit before admission here.
            return a.objectId==269&&bones.handle(a).generation&&(b.objectId==60||b.objectId==35);
        };
        return runSkeletonContactsSelected(world,skeletons,frames,[&]{return world.requestMomentDelay();},
            environment,audio,atan,[&](const auto& a,const auto& b){
                return bonePair(a,b)||bonePair(b,a)||admitPlayer(a,b)||admitPlayer(b,a)||
                    (b.objectId==60&&effect(a))||(a.objectId==60&&effect(b));
            },[&](const auto& event){
                const auto* self=owner_.resolve(event.self);if(!self)return false;
                if(self->objectId==120||self->objectId==276)return playerContact(event);
                if(self->objectId==269)return stop.succeeded&&
                    world.dispatchBoneCollision(bones,event,stop.player.stopped,frames,damage);
                if(self->objectId==225)return world.dispatchSmokeCollision(smoke,event);
                if(self->objectId==223)return world.dispatchConsumptionBloodCollision(particles,event);
                return world.dispatchDitherAboveCollision(event);
            });
    }
private:
    template<unsigned Capacity,class Frames,class Delay,class Audio,class Atan,class Admit,class Execute>
    bool runSkeletonContactsSelected(StartupRuntime& world,
        StoredSkeletonRegistration<StartupRuntime::instanceCapacity,Capacity>& skeletons,
        Frames frames,Delay delay,SoundPlayEnvironment& environment,Audio& audio,Atan atan,
        Admit admit,Execute execute){
        if(!owner_.boundTo(world)||!skeletons.valid())return false;
        auto selected=[&](InstanceHandle key,const auto& bullet,const auto& enemy){
            return bullet.objectId==261&&enemy.objectId==120&&
                world.retainedSourceBullet(key)&&skeletons.handle(enemy).generation;
        };
        auto fieldContact=[&](const auto& enemy,const auto& field){
            // These admitted field families have no reverse enemy handler.
            // Broader descendants require their reverse event audit first.
            return enemy.objectId==120&&skeletons.handle(enemy).generation&&
                (field.objectId==82||field.objectId==255||field.objectId==371);
        };
        return runSelected(world,[&](auto ka,auto kb,const auto& a,const auto& b){
                return selected(ka,a,b)||selected(kb,b,a)||fieldContact(a,b)||fieldContact(b,a)||windPair(a,b)||admit(a,b);},
            [&](const auto& event){
                const auto* self=owner_.resolve(event.self);if(!self)return false;
                if(event.owner==255)return world.dispatchTimeFieldWind(event,environment.gameVolume,audio);
                if(self->objectId==120&&event.code==1066)return world.dispatchSkeletonFieldCollision(skeletons,event);
                return self->objectId==261?world.dispatchSkeletonBulletCollision(skeletons,event,frames,delay,environment,audio,atan):
                    execute(event);
            });
    }
public:
    // Non-combat admission using the SAME collected stream as authored dither.
    template<class Audio> bool run(StartupRuntime& world,MotionBinary64 volume,Audio& audio){
        if(!owner_.boundTo(world))return false;
        return runSelected(world,[](auto,auto,const auto& a,const auto& b){return windPair(a,b);},
            [&](const auto& event){return event.owner==255?world.dispatchTimeFieldWind(event,volume,audio):
                world.dispatchDitherAboveCollision(event);});
    }
    bool run(StartupRuntime& world){
        return runSelected(world,[](auto,auto,const auto&,const auto&){return false;},
            [&](const auto& event){return world.dispatchDitherAboveCollision(event);});
    }
    // Admit registered smoke in the SAME pair stream, not a later smoke-only
    // pass. Broader wall families still need their reverse handlers/motion.
    template<unsigned Capacity> bool run(StartupRuntime& world,
        StoredSmokeRegistration<StartupRuntime::instanceCapacity,Capacity>& smoke){
        if(!smoke.valid())return false;
        return runSelected(world,[&](auto,auto,const auto& a,const auto& b){
            return (a.objectId==225&&b.objectId==60&&smoke.state(a))||
                   (b.objectId==225&&a.objectId==60&&smoke.state(b));
        },[&](const auto& event){
            const auto* self=owner_.resolve(event.self);
            return self&&(self->objectId==225?world.dispatchSmokeCollision(smoke,event):
                         world.dispatchDitherAboveCollision(event));
        });
    }
    template<unsigned SmokeCapacity,unsigned ParticleCapacity> bool run(StartupRuntime& world,
        StoredSmokeRegistration<StartupRuntime::instanceCapacity,SmokeCapacity>& smoke,
        StoredConsumptionBloodRegistration<StartupRuntime::instanceCapacity,ParticleCapacity>& particles){
        if(!smoke.valid()||!particles.valid())return false;
        const auto effect=[&](const auto& r){
            return (r.objectId==225&&smoke.state(r))||(r.objectId==223&&particles.state(r));
        };
        return runSelected(world,[&](auto,auto,const auto& a,const auto& b){
            return (b.objectId==60&&effect(a))||(a.objectId==60&&effect(b));
        },[&](const auto& event){
            const auto* self=owner_.resolve(event.self);if(!self)return false;
            if(self->objectId==223)return world.dispatchConsumptionBloodCollision(particles,event);
            if(self->objectId==225)return world.dispatchSmokeCollision(smoke,event);
            return world.dispatchDitherAboveCollision(event);
        });
    }
private:
    static bool windPair(const InstanceRecord& a,const InstanceRecord& b){
        return ((a.objectId==255||a.objectId==371)&&b.objectId==35)||
            ((b.objectId==255||b.objectId==371)&&a.objectId==35);
    }
    template<class Admit,class Execute> bool runSelected(StartupRuntime& world,Admit admit,Execute execute){
        if(!world.effectsSucceeded())return false;
        if(world.roomId()!=11)return true;
        if(!owner_.collectCollisionPairs(pairs_,true))return false;
        RunnerCollisionEventContext context;
        struct Operations {
            StoredWorldCollision& owner;StartupRuntime& world;RunnerCollisionEventContext& context;
            Execute& execute;
            bool overlap(InstanceHandle a,InstanceHandle b,bool& hit){return owner.collisionOverlap(a,b,hit);}
            bool flags(InstanceHandle h,std::uint32_t& value){return owner.collisionFlags(h,value);}
            bool queueCollision(InstanceHandle h){return owner.queueCollision(h);}
            bool dispatch(InstanceHandle a,InstanceHandle b){
                return owner.dispatchCollisionEvent(context,a,b,{world.roomId()==11,false},
                    execute);
            }
            // Admitted resource objects are non-solid in runner terms.
            // Never invent engine/path motion if this contract changes.
            bool rollback(InstanceHandle){return false;}
            bool prepareMotion(InstanceHandle){return false;}
            bool advanceMotion(InstanceHandle){return false;}
        } operations{owner_,world,context,execute};
        RunnerCollisionDispatch dispatch;
        return pairs_.dispatch([&](InstanceHandle a,InstanceHandle b){
            const auto* first=owner_.resolve(a);const auto* second=owner_.resolve(b);
            if(!first||!second)return false;
            const bool selected=(first->objectId==61&&second->objectId==60)||
                (first->objectId==60&&second->objectId==61)||admit(a,b,*first,*second);
            return !selected||dispatch.pair(a,b,operations);
        });
    }
private:
    StoredWorldCollision& owner_;
    RunnerCollisionPairs<StartupRuntime::instanceCapacity,16384> pairs_;
};
}
#endif

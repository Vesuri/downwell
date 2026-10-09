#ifndef DOWNWELL_CORE_BLAST_SKELETON_CONTACT_H
#define DOWNWELL_CORE_BLAST_SKELETON_CONTACT_H
#include "BlastActor.h"
#include "SkeletonActor.h"
namespace downwell {
// Both lifetimes must be held by their owners during this event. Object 120
// inherits false bulletThrough/bulletImmune; do not apply this specialization
// to other enemy families. The impact callback must perform synchronous 219
// Create and publish the source numeric myFx before its ordered setters.
template<unsigned Capacity,class Publish,class Delay,class Impact,class Audio>
bool blastSkeletonContact(BlastActor<Capacity>& blast,SkeletonActor& target,unsigned targetId,
    MotionBinary64 x,MotionBinary64 y,Publish& publish,Delay delay,Impact impact,
    SoundPlayEnvironment& environment,Audio& audio,bool& good){
    struct Events {
        BlastActor<Capacity>& blast;SkeletonActor& target;unsigned id;MotionBinary64 x,y;
        Publish& publish;Delay& delay;Impact& impact;SoundPlayEnvironment& environment;Audio& audio;bool& good;
        bool bulletThrough(bool& value){value=false;return true;}
        bool bulletImmune(bool& value){value=false;return true;}
        bool targetId(std::uint32_t& value){value=id;return id&&id<=0x7fffffffu;}
        bool enemyHealth(MotionBinary64& value){value=MotionBinary64::fromInteger(target.health);return true;}
        bool setEnemyHit(){target.hitPending=true;return true;}
        bool setEnemyDirection(MotionBinary64 value){target.hitDirection=value;return true;}
        bool setEnemyHealth(MotionBinary64 value){
            std::int32_t whole;if(!value.roundPixel(whole))return false;
            const auto exact=MotionBinary64::fromInteger(whole);
            if(exact.high!=value.high||exact.low!=value.low)return false;
            target.health=whole;return true;
        }
        bool setEnemyStun(){target.damageStun=true;return true;}
        bool setEnemyImpact(MotionBinary64 value){target.takenImpact=value;return true;}
        bool momentDelay(){return delay();}
        bool hitSound(int id,int priority,bool loop,bool cut){int voice=-1;
            return SoundPlayScript::plain(blast.sound,environment,audio,id,priority,loop,cut,voice,good);
        }
        MotionBinary64 callerX(){return x;}MotionBinary64 callerY(){return y;}
        bool impactEffect(MotionBinary64 x,MotionBinary64 y,int sprite,MotionBinary64 speed,MotionBinary64 angle,int depth){
            return impact(blast,x,y,sprite,speed,angle,depth);
        }
        bool destroySelf(){return publish.destroySelf();}
        bool createExplosion(int,MotionBinary64,MotionBinary64){return false;} // Not object-276's source state.
    } events{blast,target,targetId,x,y,publish,delay,impact,environment,audio,good};
    return blast.enemyContact(events,good);
}
}
#endif

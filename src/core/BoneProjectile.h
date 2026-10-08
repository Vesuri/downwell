#ifndef DOWNWELL_CORE_BONE_PROJECTILE_H
#define DOWNWELL_CORE_BONE_PROJECTILE_H
#include "BoneMotion.h"
#include "RandomBinary64.h"
#include "UnitSequenceFrames.h"
#include "EnemyContact.h"
#include "RunnerRealComparison.h"
namespace downwell {
// Object 269's Create and event state. The owner must call create synchronously
// before installing the skeleton's velocity overrides.
// It supplies collision dispatch, precise view bounds and effect/damage sinks.
// Requests never free the instance during an executing event.
struct BoneProjectile {
    enum class Animation { instanceDefault, sampledRate, stopped };
    BoneMotion motion;
    MotionBinary64 sampledAnimationRate;
    RunnerImageValue timeline{}, imageIndex{}, imageSpeed{0x3f800000u};
    int health = 1, mask = -1;
    bool active = true, destroyRequested = false;
    bool hit = false, hitEffect = false;
    Animation animation = Animation::instanceDefault;
    ContactBounds collisionBounds() const {
        // The original uses published x/y and an unrotated, unit-scale
        // AxisAlignedRect. Animation changes artwork, not this shared mask.
        auto bounds=mask==582?ContactBounds{0,0,0,0}:contactSpriteBounds(460);
        bounds.left+=motion.x; bounds.right+=motion.x;
        bounds.top+=motion.y; bounds.bottom+=motion.y;
        return bounds;
    }
    bool meetsPlayer(const ContactActor& player) const {
        const auto a=collisionBounds(), b=contactWorldBounds(player);
        return contactBoundsOverlap(a,b);
    }
    // The owner supplies the spatial index's rectangle query, not a per-pixel
    // scan. It dispatches each collision family in original engine order.
    template<class WallQuery,class Events>
    bool dispatchWall(WallQuery query,Events& events) {
        const auto bounds=collisionBounds();
        if(bounds.left>=bounds.right || bounds.top>=bounds.bottom || !query(bounds)) return false;
        collideWall(events); return true;
    }
    template<class Events>
    bool dispatchPlayer(const ContactActor& player,bool stopped,Events& events) {
        if(!meetsPlayer(player)) return false;
        collidePlayer(stopped,events); return true;
    }
    // Sprite 460: four consecutive unit sequence keys. The owner advances
    // animation before Begin Step/alarms, using the speed from the prior Step.
    // Bone events do not write image_index or change the sprite.
    bool advanceImage(bool& crossed) {
        return UnitSequenceFrames::advance(timeline,imageSpeed,4,imageIndex,crossed);
    }
    unsigned drawFrame() const {
        // Valid sequence evaluation is nonnegative and strictly below four.
        const unsigned exponent=(imageIndex.bits>>23)&255u;
        if (exponent<127u) return 0;
        return ((imageIndex.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
    }
    template<class Random>
    bool create(std::int32_t x, std::int32_t y, Random& random) {
        BoneProjectile next;
        next.motion.x = x; next.motion.y = y;
        next.motion.xx = MotionBinary64::fromInteger(x);
        next.motion.yy = MotionBinary64::fromInteger(y);
        // Inherited parent Create sets flags only. Keep all three range draws
        // even when SkeletonThrow immediately overwrites the first two values.
        if (!randomBinary64(random.nextU32(),{0x3ff80000u,0},next.motion.xsp) ||
            !next.motion.xsp.add({0x3fe00000u,0}) ||
            !randomBinary64(random.nextU32(),{0x40080000u,0},next.motion.ysp) ||
            !next.motion.ysp.add({0xc0100000u,0}) ||
            !randomBinary64(random.nextU32(),{0x3fdcccccu,0xcccccccdu},next.sampledAnimationRate) ||
            !next.sampledAnimationRate.add({0x3fa99999u,0x9999999au})) return false;
        *this = next;
        return true;
    }
    template<class Events> void requestDestroy(Events& events) {
        destroyRequested = true;
        events.destroy();
    }
    template<class InView, class Events>
    bool step(bool timeStopped, InView inView, Events& events) {
        if (!timeStopped) {
            // GML stores the sampled double into the builtin binary32 field
            // before motion; the newly selected speed affects next animation.
            if (!imageSpeed.assign(sampledAnimationRate)) return false;
            animation = Animation::sampledRate;
            if (!motion.step(false)) return false;
            if (health <= 0 && active) {
                animation = Animation::stopped;
                imageSpeed = {};
                mask = 582;
                motion.xsp = {}; motion.ysp = {};
                active = false;
                requestDestroy(events);
                events.effect(85, 3); // scrFxNol(85, 0.3), speed in tenths.
            }
        } else { animation = Animation::stopped; imageSpeed = {}; }
        // Source executes this after health destruction too. Do not early-out
        // on destroyRequested; the owner retires storage at the event boundary.
        if (!inView(motion.x, motion.y)) requestDestroy(events);
        return true;
    }
    template<class Events> void collideWall(Events& events) {
        requestDestroy(events); events.effect(85, 3);
    }
    template<class Events> void collidePlayer(bool timeStopped, Events& events) {
        if (!active) return;
        if (!timeStopped) events.damage(1, 3, 2);
        events.effect(85, 3); requestDestroy(events);
    }
    // Registered-owner entry points. The scalar methods above are retained
    // as the successful-event reference. Every callback below can fail; a
    // failed world must never replay an already completed event prefix.
    template<class Ops> bool destroyPublished(Ops& ops,bool& good){
        if(!good)return false;
        if(!ops.destroy())return good=false;
        destroyRequested=true;return true;
    }
    template<class Ops> bool advanceImagePublished(Ops& ops,bool& good){
        if(!good)return false;
        auto next=timeline;RunnerImageValue index;bool crossed;
        if(!UnitSequenceFrames::advance(next,imageSpeed,4,index,crossed)||!ops.imageIndex(index))return good=false;
        timeline=next;imageIndex=index;return true;
    }
    template<class Ops> bool stepPublished(bool stopped,Ops& ops,bool& good){
        if(!good)return false;
        const auto speed=[&](RunnerImageValue value){
            if(!ops.imageSpeed(value))return false;
            imageSpeed=value;return true;
        };
        if(!stopped){
            int comparison;RunnerImageValue sampled;
            // Compare before narrowing: imgSp is a custom double, while the
            // builtin image_speed read promotes its stored float. The VM
            // comparison uses its real epsilon, not a raw-bit inequality.
            if(!compareRunnerReal(imageSpeed.promoted(),sampledAnimationRate,comparison))return good=false;
            if(comparison&&(!sampled.assign(sampledAnimationRate)||!speed(sampled)))return good=false;
            animation=Animation::sampledRate;
            // VM writes custom accumulators before either builtin setter.
            if(!motion.ysp.add({0x3fb99999u,0x9999999au})||!motion.xx.add(motion.xsp)||
               !motion.yy.add(motion.ysp))return good=false;
            RunnerImageValue px,py;std::int32_t rounded;
            if(!px.assignInteger(motion.x)||!py.assignInteger(motion.y)||
               !motion.xx.roundPixel(rounded)||!px.assignInteger(rounded)||
               !px.promoted().roundPixel(rounded)||!ops.position(px,py))return good=false;
            motion.x=rounded;
            if(!motion.yy.roundPixel(rounded)||!py.assignInteger(rounded)||
               !py.promoted().roundPixel(rounded)||!ops.position(px,py))return good=false;
            motion.y=rounded;
            if(health<=0&&active){
                if(!speed({}))return good=false;
                animation=Animation::stopped;
                if(!ops.mask(582))return good=false;
                mask=582;
                motion.xsp={};motion.ysp={};active=false;
                if(!destroyPublished(ops,good)||!ops.effect(85,3))return good=false;
            }
        }else{
            if(!speed({}))return good=false;
            animation=Animation::stopped;
        }
        bool visible;
        // Deliberately after destruction/effect, including when health died.
        if(!ops.inView(motion.x,motion.y,visible))return good=false;
        if(!visible&&!destroyPublished(ops,good))return false;
        return true;
    }
    // Both world owners use their real publication, impact and view adapters.
    // The selected room event supplies pTimeStop after player Begin Step.
    template<class Publisher,class Effect,class View>
    bool stepInWorld(bool stopped,Publisher& publisher,Effect effect,View view,bool& good){
        struct Ops {
            Publisher& publish;Effect& spawn;View& query;
            bool imageSpeed(RunnerImageValue value){return publish.imageSpeed(value);}
            bool position(RunnerImageValue x,RunnerImageValue y){return publish.position(x,y);}
            bool mask(int id){return publish.mask(id);}
            bool destroy(){return publish.destroy();}
            bool effect(int sprite,int tenths){return sprite==85&&tenths==3&&spawn();}
            bool inView(int,int,bool& visible){return query(visible);}
        } ops{publisher,effect,view};
        return stepPublished(stopped,ops,good);
    }
    template<class Ops> bool collideWallPublished(Ops& ops,bool& good){
        return destroyPublished(ops,good)&&(ops.effect(85,3)||(good=false));
    }
    template<class Publish,class Impact,class Damage>
    bool collideInWorld(bool player,bool stopped,Publish& publish,Impact impact,Damage damage,bool& good){
        struct Ops {
            Publish& publish;Impact& impact;Damage& applyDamage;
            bool destroy(){return publish.destroy();}
            bool effect(int sprite,int tenths){return sprite==85&&tenths==3&&impact();}
            bool damage(int amount,int horizontal,int vertical){return applyDamage(amount,horizontal,vertical);}
        } ops{publish,impact,damage};
        return player?collidePlayerPublished(stopped,ops,good):collideWallPublished(ops,good);
    }
    template<class Ops> bool collidePlayerPublished(bool stopped,Ops& ops,bool& good){
        if(!good)return false;
        if(!active)return true;
        if((!stopped&&!ops.damage(1,3,2))||!ops.effect(85,3))return good=false;
        return destroyPublished(ops,good);
    }
};
}
#endif

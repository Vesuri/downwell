#ifndef DOWNWELL_CORE_FLASH_BALL_EFFECTS_H
#define DOWNWELL_CORE_FLASH_BALL_EFFECTS_H
#include "SkeletonEffectBursts.h"
#include "UnitSequenceFrames.h"
namespace downwell {
// Object 224, sprite 96: Create jitters builtin x/y, starts at frame one,
// and sets speed 0.3. There is no Step/time-stop guard; Animation End kills it.
struct FlashBallEffect {
    // The verified three unit keys map the Create image_index=1 back to
    // timeline 1 on the runner's first animation phase (0x50ed71..0x50ee09).
    // Seed that equivalent timeline directly; arbitrary frame writes are not
    // supported by this effect's source events.
    RunnerImageValue x{},y{},timeline{0x3f800000u},imageIndex{0x3f800000u};
    bool destroyRequested=false;
    template<class Random> bool create(MotionBinary64 px,MotionBinary64 py,Random& random) {
        FlashBallEffect next;
        if(!next.x.assign(px) || !next.y.assign(py)) return false;
        auto jitterX=next.x.promoted();
        if(!jitterX.add(MotionBinary64::fromInteger(int(random.irandomSmall(4))-2)) ||
           !next.x.assign(jitterX)) return false;
        auto jitterY=next.y.promoted();
        if(!jitterY.add(MotionBinary64::fromInteger(int(random.irandomSmall(4))-2)) ||
           !next.y.assign(jitterY)) return false;
        *this=next;return true;
    }
    bool advanceImage() {
        bool crossed;
        if(!UnitSequenceFrames::advance(timeline,{0x3e99999au},3,imageIndex,crossed)) return false;
        // action_kill_object in Other 7. No additional RNG or death effects.
        if(crossed) destroyRequested=true;
        return true;
    }
    unsigned drawFrame() const {
        const unsigned exponent=(imageIndex.bits>>23)&255u;
        return exponent<127?0:((imageIndex.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
    }
};

// Owned fixed storage; no target heap allocation or per-frame asset conversion.
// Capture the animation cutoff at the start of that global phase. Creation
// after it must not accidentally age a new effect because a free slot precedes
// another live slot. Slot order is not an event-order contract.
template<unsigned Capacity> class FlashBallEffects {
    static_assert(Capacity>0,"Flash effect capacity must be nonzero");
public:
    struct Slot {std::uint32_t handle=0;FlashBallEffect effect;};
    struct Phase {std::uint32_t newest;};
    static constexpr unsigned capacity=Capacity;
    const Slot& slot(unsigned index) const {return slots_[index];}
    Phase beginAnimation() const {return {nextHandle_?nextHandle_-1u:0xffffffffu};}
    template<class Random> std::uint32_t create(MotionBinary64 x,MotionBinary64 y,Random& random) {
        if(!nextHandle_)return 0;
        for(auto& slot:slots_)if(!slot.handle) {
            if(!slot.effect.create(x,y,random))return 0;
            slot.handle=nextHandle_++;return slot.handle;
        }
        return 0;
    }
    bool advanceAnimation(Phase phase) {
        for(auto& slot:slots_)if(slot.handle && slot.handle<=phase.newest) {
            if(!slot.effect.advanceImage())return false;
            if(slot.effect.destroyRequested)slot.handle=0;
        }
        return true;
    }
    template<class Random> bool skeletonBurst(MotionBinary64 x,MotionBinary64 y,Random& random) {
        return skeletonFlashBurst(x,y,random,[&](int object,MotionBinary64 px,MotionBinary64 py){
            return object==224 && create(px,py,random)!=0;
        });
    }
private:
    Slot slots_[Capacity];
    std::uint32_t nextHandle_=1;
};
}
#endif

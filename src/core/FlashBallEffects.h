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
    RunnerImageValue imageSpeed{0x3e99999au};
    int vrange=2;
    bool destroyRequested=false;
    // Called after positioned birth. Unlike the legacy atomic create below,
    // every source setter is published before the next RNG site is reached.
    template<class Random,class Publish> bool createPublished(Random& random,Publish& publish) {
        imageIndex={0x3f800000u};timeline=imageIndex;
        if(!publish.imageIndex(imageIndex))return false;
        vrange=2;
        auto px=x.promoted();
        if(!px.add(MotionBinary64::fromInteger(int(random.irandomSmall(4))-2))||
           !x.assign(px)||!publish.position(x,y))return false;
        auto py=y.promoted();
        if(!py.add(MotionBinary64::fromInteger(int(random.irandomSmall(4))-2))||
           !y.assign(py)||!publish.position(x,y))return false;
        imageSpeed={0x3e99999au};
        return publish.imageSpeed(imageSpeed);
    }
    template<class Random> bool create(MotionBinary64 px,MotionBinary64 py,Random& random) {
        FlashBallEffect next;
        if(!next.x.assign(px) || !next.y.assign(py)) return false;
        auto jitterX=next.x.promoted();
        if(!jitterX.add(MotionBinary64::fromInteger(int(random.irandomSmall(4))-2)) ||
           !next.x.assign(jitterX)) return false;
        auto jitterY=next.y.promoted();
        if(!jitterY.add(MotionBinary64::fromInteger(int(random.irandomSmall(4))-2)) ||
           !next.y.assign(jitterY)) return false;
        std::int32_t published;
        if(!next.x.promoted().roundPixel(published) || !next.y.promoted().roundPixel(published)) return false;
        *this=next;return true;
    }
    bool advanceImage() {
        bool crossed;
        if(!UnitSequenceFrames::advance(timeline,{0x3e99999au},3,imageIndex,crossed)) return false;
        // action_kill_object in Other 7. No additional RNG or death effects.
        if(crossed) destroyRequested=true;
        return true;
    }
    template<class Publish> bool advanceImage(Publish& publish) {
        if(destroyRequested)return true;
        bool crossed;
        if(!UnitSequenceFrames::advance(timeline,imageSpeed,3,imageIndex,crossed)||
           !publish.imageIndex(imageIndex))return false;
        if(crossed){destroyRequested=true;return publish.destroy();}
        return true;
    }
    unsigned drawFrame() const {
        const unsigned exponent=(imageIndex.bits>>23)&255u;
        return exponent<127?0:((imageIndex.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
    }
    int pixelX() const {std::int32_t pixel=0;x.promoted().roundPixel(pixel);return pixel;}
    int pixelY() const {std::int32_t pixel=0;y.promoted().roundPixel(pixel);return pixel;}
};

// Owned fixed storage; no target heap allocation or per-frame asset conversion.
// Capture the animation cutoff at the start of that global phase. Creation
// after it must not accidentally age a new effect because a free slot precedes
// another live slot. Slot order is not an event-order contract.
template<unsigned Capacity> class FlashBallEffects {
    static_assert(Capacity>0,"Flash effect capacity must be nonzero");
public:
    struct BirthSequence {std::uint32_t next=1;};
    BirthSequence birthSequence()const{return {nextHandle_};}
    bool restoreEmptyBirthSequence(BirthSequence sequence){
        if(count_ || (!nextHandle_ && sequence.next) || (sequence.next && sequence.next<nextHandle_))return false;
        nextHandle_=sequence.next;return true;
    }
    struct Slot {std::uint32_t handle=0;FlashBallEffect effect;};
    struct Phase {std::uint32_t newest;};
    static constexpr unsigned capacity=Capacity;
    const Slot& slot(unsigned index) const {return slots_[index];}
    void clear() {for(auto& slot:slots_)slot.handle=0;count_=0;} // Never rewind lifetime keys.
    template<class Draw> void visitNewest(Draw draw) const {
        for(unsigned i=0;i<count_;++i)draw(slots_[order_[i]].effect);
    }
    Phase beginAnimation() const {return {nextHandle_?nextHandle_-1u:0xffffffffu};}
    template<class Random> std::uint32_t create(MotionBinary64 x,MotionBinary64 y,Random& random) {
        if(!nextHandle_)return 0;
        for(unsigned index=0;index<Capacity;++index)if(!slots_[index].handle) {
            auto& slot=slots_[index];
            if(!slot.effect.create(x,y,random))return 0;
            for(unsigned i=count_;i>0;--i)order_[i]=order_[i-1];
            order_[0]=index;++count_;
            slot.handle=nextHandle_++;return slot.handle;
        }
        return 0;
    }
    bool advanceAnimation(Phase phase) {
        for(unsigned i=0;i<count_;++i) {
            auto& slot=slots_[order_[i]];
            if(slot.handle>phase.newest) continue;
            if(!slot.effect.advanceImage())return false;
            if(slot.effect.destroyRequested)slot.handle=0;
        }
        unsigned kept=0;
        for(unsigned i=0;i<count_;++i)if(slots_[order_[i]].handle)order_[kept++]=order_[i];
        count_=kept;
        return true;
    }
    template<class Random> bool skeletonBurst(MotionBinary64 x,MotionBinary64 y,Random& random) {
        return skeletonFlashBurst(x,y,random,[&](int object,MotionBinary64 px,MotionBinary64 py){
            return object==224 && create(px,py,random)!=0;
        });
    }
    // scrFlashballfx(x,y,Count,Variation,Radius) for the compact callers:
    // steering deaths (1,0,0) and opened gem chunks (3,0,5).
    template<unsigned Count,unsigned Variation,unsigned Radius,class Random>
    bool burst(MotionBinary64 x,MotionBinary64 y,Random& random) {
        return flashBurstScript<Count,Variation,Radius>(x,y,random,[&](int object,MotionBinary64 px,MotionBinary64 py){
            return object==224 && create(px,py,random)!=0;
        });
    }
    template<class Random> bool boxBurst(MotionBinary64 x,MotionBinary64 y,Random& random) {
        return boxFlashBurst(x,y,random,[&](int object,MotionBinary64 px,MotionBinary64 py){
            return object==224 && create(px,py,random)!=0;
        });
    }
private:
    Slot slots_[Capacity];
    unsigned order_[Capacity]{};
    unsigned count_=0;
    std::uint32_t nextHandle_=1;
};
}
#endif

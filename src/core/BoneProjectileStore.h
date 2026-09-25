#ifndef DOWNWELL_CORE_BONE_PROJECTILE_STORE_H
#define DOWNWELL_CORE_BONE_PROJECTILE_STORE_H
#include "BoneProjectile.h"
#include "SkeletonThrow.h"
namespace downwell {
// Fixed storage with stable nonzero handles. Destruction is an event request,
// not immediate slot reuse; the world retires it after the event returns.
// Scheduling/creation-frame eligibility remain explicit responsibilities of
// the world's event dispatcher, never an accidental consequence of slot order.
template<unsigned Capacity> class BoneProjectileStore {
    static_assert(Capacity>0,"Bone projectile storage must have at least one slot");
public:
    using Handle=std::uint32_t;
    struct Slot { Handle handle=0; BoneProjectile projectile; };
    static constexpr unsigned capacity=Capacity;
    template<class Random> Handle create(int x,int y,Random& random) {
        if (!nextHandle_) return 0; // Do not alias stale handles on wrap.
        for (unsigned i=0;i<Capacity;++i) if (!slots_[i].handle) {
            BoneProjectile candidate;
            if (!candidate.create(x,y,random)) return 0;
            slots_[i].projectile=candidate;
            slots_[i].handle=nextHandle_++;
            return slots_[i].handle;
        }
        return 0; // Capacity failure consumes no Create RNG.
    }
    BoneProjectile* get(Handle handle) {
        if (handle) for (auto& slot:slots_) if (slot.handle==handle) return &slot.projectile;
        return nullptr;
    }
    const Slot& slot(unsigned i) const { return slots_[i]; }
    bool retire(Handle handle) {
        if (handle) for (auto& slot:slots_) if (slot.handle==handle) {
            if (!slot.projectile.destroyRequested) return false;
            slot.handle=0; return true;
        }
        return false;
    }
private:
    Slot slots_[Capacity];
    Handle nextHandle_=1;
};

// Synchronous bridge from SkeletonThrow to actual owned child state. The
// caller supplies precise horizontal alignment (sign(plx-x)!=0); never round
// player position to an integer merely to select the RNG branch.
template<class Store,class Random,class Sound> struct SkeletonBoneCreation {
    Store& store;
    Random& random;
    Sound& sound;
    bool& shot;
    typename Store::Handle child=0;
    bool createBone(int object,int x,int y) {
        if (object!=269) return false;
        child=store.create(x,y,random); return child!=0;
    }
    void throwSound(int id) { sound(id); }
    unsigned choose(unsigned count) { return random.chooseIndexSmall(count); }
    void setVelocityTenths(int x,int y) {
        auto& motion=store.get(child)->motion;
        // SkeletonThrow supplies only the nine validated velocity pairs.
        motion.launch(motion.x,motion.y,x,y);
    }
    void multiplyAnimationSpeed(int sign) {
        if (sign<0) store.get(child)->sampledAnimationRate.high^=0x80000000u;
    }
    void latchShot() { shot=true; }
    bool run(int x,int y,bool horizontallySeparated,int facing) {
        return skeletonThrowAligned(x,y,horizontallySeparated,facing,*this);
    }
};
}
#endif

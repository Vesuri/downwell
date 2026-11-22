#ifndef DOWNWELL_CORE_GARDEN_MOTION_H
#define DOWNWELL_CORE_GARDEN_MOTION_H

#include "core/GardenGround.h"
#include "core/Input.h"
#include "core/PlayerMotion.h"

// W1.14: the garden's objPlayer_n after its nap. rmMenu's player is the full
// player, so it walks, jumps (scrJump, release halving), hops after a hard
// landing, wall-kicks off the rim and the room ends, stands on the bench from
// above and has to jump onto the rim to drop into the well. The shared
// PlayerMotion owns the Step; GardenGround supplies rmMenu's solids.
//
// Not ported: the garden's gun (no room check stops scrPlayerShootN, so an
// airborne press that is neither a wall kick nor the hard-landing re-jump
// would fire). Such a press is held back until fire is released.
namespace downwell {

class GardenMotion {
public:
    // A sound the Step's scripts start, in source order. table: soundLand
    // or soundFootstep, whose sample is first + irandom_range(0, 3) on the
    // owner's stream ("footsteps", 60); otherwise soundPlay(first, priority).
    struct Sound { int first; int priority; bool table; };

    // playerSpriteControl's nap wake: the player stays where it slept. A fire
    // held through the wake is not a new jump press.
    void wake(int x, int y, int facing, bool fireHeld) {
        motion_ = PlayerMotion(true, 0);
        motion_.face(facing);
        motion_.initializeEntryInput(fireHeld);
        originX_ = x; originY_ = y;
        heldBack_ = false; priorFire_ = fireHeld;
    }

    // One objPlayer_n Step. Returns the number of sounds written (at most 4).
    unsigned step(const Input& raw, bool controlsLocked, Sound* sounds) {
        Input input = raw;
        if (!raw.fire) heldBack_ = false;
        else if (!heldBack_ && !priorFire_ && !motion_.grounded() && !controlsLocked &&
                 !motion_.hardLandJump() && !wallKickReady(raw))
            heldBack_ = true;
        if (heldBack_) input.fire = false;
        priorFire_ = raw.fire;
        configure();
        motion_.step(input, controlsLocked);
        (void)motion_.takeRechargeSounds(); // the garden never spends a charge
        unsigned count = 0;
        for (unsigned i = 0; i < motion_.soundEventCount() && count < 4; ++i) {
            switch (motion_.soundEvent(i)) {
            case PlayerMotion::SoundEvent::jump:
            case PlayerMotion::SoundEvent::wallKick:
                sounds[count++] = {86, 80, false}; break; // soundPlay(choose(86), 80)
            case PlayerMotion::SoundEvent::hardLandTurn:
                sounds[count++] = {88, 30, false}; break;
            case PlayerMotion::SoundEvent::land:
                sounds[count++] = {GardenGround::landingSound(material()), 60, true}; break;
            case PlayerMotion::SoundEvent::footstep:
                sounds[count++] = {GardenGround::footstepSound(material()), 60, true}; break;
            default: break;
            }
        }
        return count;
    }

    int x() const { return originX_ + motion_.offsetX(); }
    int y() const { return originY_ + motion_.offsetY(); }
    long xFixed() const { return long(originX_) * 256L + motion_.offsetXFixed(); }
    long yFixed() const { return long(originY_) * 256L + motion_.offsetYFixed(); }
    long speedXFixed() const { return motion_.speedXFixed(); }
    bool grounded() const { return motion_.grounded(); }
    int facing() const { return motion_.facing(); }
    int spriteId() const { return motion_.spriteId(); }
    unsigned spriteFrame() const { return motion_.spriteFrame(); }
    bool succeeded() const { return motion_.motionSucceeded(); }

private:
    GardenGround::Material material() const { return GardenGround::under(x(), y()); }
    // scrUpButtonFunctions' wall kick: spinning, holding toward a solid
    // three pixels to that side (PlayerMotion's own test).
    bool wallKickReady(const Input& raw) const {
        if (!motion_.spinJumping()) return false;
        const int px = x(), py = y();
        if (raw.left) return maskSolid(px + 3, py);
        if (raw.right) return maskSolid(px - 3, py);
        return false;
    }
    static bool maskSolid(int x, int y) {
        return GardenGround::solid(x - 4, y - 4) || GardenGround::solid(x + 3, y - 4) ||
               GardenGround::solid(x - 4, y + 7) || GardenGround::solid(x + 3, y + 7);
    }
    // Rebound every Step so a copied owner never calls into the old one.
    void configure() {
        motion_.configureSolidResolver([](void* context, int x, int y) {
            const auto& self = *static_cast<const GardenMotion*>(context);
            return GardenGround::solid(x + self.originX_, y + self.originY_);
        }, this, -1000, 1000);
        motion_.configureLandingResolver([](void* context, int x, int minimumY) {
            const auto& self = *static_cast<const GardenMotion*>(context);
            return GardenGround::landing(x + self.originX_, minimumY + self.originY_) - self.originY_;
        }, this);
        motion_.configureCollisionOrigin(originX_, originY_);
        motion_.configureRoundedHorizontal(true);
    }

    PlayerMotion motion_{true, 0};
    int originX_ = 112, originY_ = 512;
    bool heldBack_ = false, priorFire_ = false;
};

// The garden's natural input-only route for the verifiers and replays (also
// in tools/verify-world-one-native.py and tools/verify-menus-native.py):
// right until over the well; fire held from x 240 until then, so one jump
// carries the player over the rim before it drops into the gap.
inline Input gardenRouteInput(int x) {
    Input input;
    input.right = x < 300;
    input.fire = x >= 240 && x < 300;
    return input;
}

} // namespace downwell

#endif

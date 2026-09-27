#ifndef DOWNWELL_CORE_PROJECTILE_ANIMATION_H
#define DOWNWELL_CORE_PROJECTILE_ANIMATION_H
#include "UnitSequenceFrames.h"
namespace downwell {
// bulletStandard Create writes image_index=0 and image_speed=0.3; the builtin
// setter narrows the latter to binary32. Sprites 458/459 have six consecutive
// unit sequence keys at one frame/game-frame. Only existing bullets advance
// in the runner's animation phase, before Begin Step can create new ones.
// No subsequent image_index writes: timeline and evaluated image coincide.
struct ProjectileAnimation {
    RunnerImageValue index;
    bool advance() {
        RunnerImageValue image;bool crossed;
        return UnitSequenceFrames::advance(index,{0x3e99999au},6,image,crossed);
    }
    unsigned frame() const {
        const unsigned exponent=(index.bits>>23)&255u;
        return exponent<127?0:((index.bits&0x7fffffu)|0x800000u)>>(150u-exponent);
    }
};
}
#endif

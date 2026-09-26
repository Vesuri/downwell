#ifndef DOWNWELL_CORE_FORCED_PLAYER_JUMP_H
#define DOWNWELL_CORE_FORCED_PLAYER_JUMP_H
#include "MotionBinary64.h"
namespace downwell {
// scrFjump's caller-owned fields. Recharge is synchronous and may mutate the
// caller; read the fields again after it, as the VM does. Player writes are
// object-selector writes, not an invented loop over player instances.
struct ForcedPlayerJump {
    MotionBinary64 horizontal{}, vertical{}, defaultVertical{0x40059999u,0x9999999au};
    template<class Events>
    bool execute(MotionBinary64 x, MotionBinary64 y, Events& events) {
        if (!x.valid() || !y.valid()) return false;
        horizontal=x; vertical=y; defaultVertical={0x40059999u,0x9999999au};
        if (vertical.zero()) vertical=defaultVertical;
        events.setSpinJumping();
        if (!events.recharge() || !events.setGrounded(false)) return false;
        if (!horizontal.zero() && !events.setHorizontal(horizontal)) return false;
        auto speed=vertical; speed.high^=0x80000000u;
        return events.setVertical(speed);
    }
};
}
#endif

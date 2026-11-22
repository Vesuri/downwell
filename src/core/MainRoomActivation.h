#ifndef DOWNWELL_CORE_MAIN_ROOM_ACTIVATION_H
#define DOWNWELL_CORE_MAIN_ROOM_ACTIVATION_H
#include "EnemyContact.h"

namespace downwell {
struct ActivationInstance {
    int objectId;
    ContactBounds bounds; // Published world-space mask bounds, exclusive ends.
    bool active;
    bool pending;
};

// objPlayer_n Begin Step advances exactly once, before gravity/movement.
// ASM uses strict GT, adds 16, then calls instance_activate_region.
// instance_activate_region tests bounding-box overlap, not the origin or
// precise mask pixels. rmMain's serialized width is 480. This is the supported
// enemy store, not a generic activation implementation for all resources.
// Windows 0x52d040 includes equality at region right/bottom. In this build's
// compatibility mode the runner's mask upper edges are inclusive; ContactBounds
// stores them plus one, hence >0 on those edges but <= on left/top below.
struct MainRoomActivationResult { bool succeeded; unsigned activated; };
// Failure retains the advanced frontier and completed activations. The caller
// must latch it and stop the frame, not retry a partially published region.
// steering admits the compact owner's 114/115/118/173 records (GameplayWorld).
template<class Read, class Activate>
MainRoomActivationResult advanceMainRoomActivationChecked(int& activationY, long viewBottomQ8,
                                   unsigned count, Read read, Activate activate, bool steering = false) {
    if (viewBottomQ8 <= (activationY - 112L) * 256L) return {true,0};
    activationY += 16;
    unsigned activated = 0;
    for (unsigned i = 0; i < count; ++i) {
        const ActivationInstance instance = read(i);
        const bool enemy = instance.objectId == 111 || instance.objectId == 112 ||
            instance.objectId == 113 || instance.objectId == 116 ||
            instance.objectId == 117 || instance.objectId == 119 ||
            (steering && (instance.objectId == 114 || instance.objectId == 115 ||
                          instance.objectId == 118 || instance.objectId == 173));
        if (enemy && !instance.active && instance.pending &&
            instance.bounds.left < instance.bounds.right && instance.bounds.top < instance.bounds.bottom &&
            instance.bounds.left <= 480 && instance.bounds.right > 0 &&
            instance.bounds.top <= activationY && instance.bounds.bottom > 0) {
            if (!activate(i)) return {false,activated};
            ++activated;
        }
    }
    return {true,activated};
}
template<class Read, class Activate>
unsigned advanceMainRoomActivation(int& activationY, long viewBottomQ8,
                                   unsigned count, Read read, Activate activate) {
    return advanceMainRoomActivationChecked(activationY,viewBottomQ8,count,read,
        [&](unsigned i){activate(i);return true;}).activated;
}
} // namespace downwell
#endif

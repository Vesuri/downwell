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
template<class Read, class Activate>
unsigned advanceMainRoomActivation(int& activationY, long viewBottomQ8,
                                   unsigned count, Read read, Activate activate) {
    if (viewBottomQ8 <= (activationY - 112L) * 256L) return 0;
    activationY += 16;
    unsigned activated = 0;
    for (unsigned i = 0; i < count; ++i) {
        const ActivationInstance instance = read(i);
        const bool enemy = instance.objectId == 111 || instance.objectId == 112 ||
            instance.objectId == 113 || instance.objectId == 116 ||
            instance.objectId == 117 || instance.objectId == 119;
        if (enemy && !instance.active && instance.pending &&
            instance.bounds.left < instance.bounds.right && instance.bounds.top < instance.bounds.bottom &&
            instance.bounds.left < 480 && instance.bounds.right > 0 &&
            instance.bounds.top < activationY && instance.bounds.bottom > 0) {
            activate(i);
            ++activated;
        }
    }
    return activated;
}
} // namespace downwell
#endif

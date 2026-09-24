#ifndef DOWNWELL_CORE_MAIN_ROOM_ACTIVATION_H
#define DOWNWELL_CORE_MAIN_ROOM_ACTIVATION_H

namespace downwell {
struct ActivationInstance {
    int objectId;
    long worldYQ8;
    bool active;
    bool pending;
};

// objPlayer_n Begin Step advances exactly once, before gravity/movement.
// ASM uses strict GT, adds 16, then calls instance_activate_region.
// The currently implemented enemy store approximates that region with the
// published Y coordinate. This is NOT a generic GameMaker region/mask query.
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
            instance.worldYQ8 <= activationY * 256L) {
            activate(i);
            ++activated;
        }
    }
    return activated;
}
} // namespace downwell
#endif

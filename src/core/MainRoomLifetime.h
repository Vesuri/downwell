#ifndef DOWNWELL_CORE_MAIN_ROOM_LIFETIME_H
#define DOWNWELL_CORE_MAIN_ROOM_LIFETIME_H

namespace downwell {
struct LifetimeInstance {
    int objectId;
    int y; // Published world coordinate, not the precise movement accumulator.
    bool active;
};

enum class MainRoomLifetimePhase { step, endStep };
// Implemented families calling scrOutofview. Scattered16 does so in Step;
// the others in End Step. objWall_n additionally guards on area five.
inline bool usesMainRoomOutOfView(int objectId, int area) {
    switch (objectId) {
    case 60: return area == 5;
    case 92: case 93: case 111: case 112: case 113: case 116:
    case 117: case 119: case 289: return true;
    default: return false;
    }
}

// scrOutofview assembly: strict LT/GT bounds, followed by room == rmMain.
// Q12 fits the serialized 300000-pixel room using 32-bit longs on 68000;
// default gameplay supplies camMain's rounded, published view, not its easing
// accumulator. Inactive instances do not receive End Step.
// Storage adapters only expose records and apply the resulting retirement.
template<class Read, class Retire>
bool retireMainRoomObjectsChecked(unsigned count, int roomId, int area, long viewTopQ12,
                          Read read, Retire retire,
                          MainRoomLifetimePhase phase=MainRoomLifetimePhase::endStep) {
    if (roomId != 11) return true;
    for (unsigned i = 0; i < count; ++i) {
        const LifetimeInstance instance = read(i);
        if (!instance.active || !usesMainRoomOutOfView(instance.objectId, area)) continue;
        if((instance.objectId==289)!=(phase==MainRoomLifetimePhase::step))continue;
        const long y = instance.y * 4096L;
        if (y < viewTopQ12 - 160L * 4096 || y > viewTopQ12 + 3000L * 4096)
            if (!retire(i)) return false;
    }
    return true;
}
// Compatibility for infallible adapters. Checked callers must latch a failure:
// previous retirements are committed and the partial sweep must not be retried.
template<class Read, class Retire>
void retireMainRoomObjects(unsigned count, int roomId, int area, long viewTopQ12,
                          Read read, Retire retire) {
    retireMainRoomObjectsChecked(count,roomId,area,viewTopQ12,read,
        [&](unsigned i){retire(i);return true;});
}
} // namespace downwell
#endif

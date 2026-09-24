#ifndef DOWNWELL_CORE_MAIN_ROOM_LIFETIME_H
#define DOWNWELL_CORE_MAIN_ROOM_LIFETIME_H

namespace downwell {
struct LifetimeInstance {
    int objectId;
    int y; // Published world coordinate, not the precise movement accumulator.
    bool active;
};

// Implemented object families whose End Step calls scrOutofview. objWall_n
// has an additional area-five guard; this is not a universal object policy.
inline bool usesMainRoomOutOfView(int objectId, int area) {
    switch (objectId) {
    case 60: return area == 5;
    case 92: case 93: case 111: case 112: case 113: case 116:
    case 117: case 119: case 289: return true;
    default: return false;
    }
}

// scrOutofview assembly: strict LT/GT bounds, followed by room == rmMain.
// Q12 retains camera subpixels and fits the serialized 300000-pixel room
// using 32-bit longs on 68000. Inactive instances do not receive End Step.
// Storage adapters only expose records and apply the resulting retirement.
template<class Read, class Retire>
void retireMainRoomObjects(unsigned count, int roomId, int area, long viewTopQ12,
                          Read read, Retire retire) {
    if (roomId != 11) return;
    for (unsigned i = 0; i < count; ++i) {
        const LifetimeInstance instance = read(i);
        if (!instance.active || !usesMainRoomOutOfView(instance.objectId, area)) continue;
        const long y = instance.y * 4096L;
        if (y < viewTopQ12 - 160L * 4096 || y > viewTopQ12 + 3000L * 4096)
            retire(i);
    }
}
} // namespace downwell
#endif

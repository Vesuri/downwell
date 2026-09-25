#ifndef DOWNWELL_CORE_SIDE_CAMERA_LOCKER_H
#define DOWNWELL_CORE_SIDE_CAMERA_LOCKER_H
#include "MainCameraMotion.h"
#include "StateDigest.h"
namespace downwell {
// Object 62 Create and the default unscaled rectangle masks: camera 583
// (160x284, origin 80,142), locker 596 (100x160, origin 50,80).
// Coordinates here are published integer instance positions, not Q12 easing.
struct SideCameraLocker {
    int x, y;
    static int createdX(int x, int roomWidth = 480) {
        return x < 160 ? 80 : x > roomWidth - 160 ? roomWidth - 80 : x;
    }
    bool overlaps(int cameraX, int cameraY) const {
        return cameraX + 79 >= x - 50 && cameraX - 80 <= x + 49 &&
               cameraY + 141 >= y - 80 && cameraY - 142 <= y + 79;
    }
};
// Room-owned, creation-ordered registrations. Fail capacity explicitly rather
// than dropping camera behavior. No per-frame allocation or pixel conversion.
struct SideCameraLockers {
    static constexpr unsigned capacity = 128;
    SideCameraLocker entries[capacity]{};
    unsigned count = 0;
    void clear() { count = 0; }
    bool create(int x, int y) {
        if (count == capacity) return false;
        entries[count++] = {SideCameraLocker::createdX(x), y};
        return true;
    }
    bool overlaps(const MainCameraMotion& camera) const {
        for (unsigned i = 0; i < count; ++i)
            if (entries[i].overlaps(camera.pixelX(), camera.pixelY())) return true;
        return false;
    }
    void collide(MainCameraMotion& camera) const {
        for (unsigned i = 0; i < count; ++i)
            if (entries[i].overlaps(camera.pixelX(), camera.pixelY()))
                camera.collideSideLocker(entries[i].y);
    }
    std::uint64_t digest(const MainCameraMotion& camera) const {
        StateDigest state;
        state.addBool(camera.sideLocked);
        state.addI32(camera.x); state.addI32(camera.y);
        state.addI32(camera.targetX); state.addI32(camera.targetY);
        state.addI32(camera.slowdownX); state.addI32(camera.slowdownY);
        state.addU32(count);
        for (unsigned i = 0; i < count; ++i) {
            state.addI32(entries[i].x); state.addI32(entries[i].y);
        }
        return state.value();
    }
};
}
#endif

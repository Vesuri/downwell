#ifndef DOWNWELL_CORE_SIDE_CAMERA_LOCKER_H
#define DOWNWELL_CORE_SIDE_CAMERA_LOCKER_H
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
}
#endif

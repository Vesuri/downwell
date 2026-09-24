#ifndef DOWNWELL_CORE_MAIN_CAMERA_MOTION_H
#define DOWNWELL_CORE_MAIN_CAMERA_MOTION_H
namespace downwell {
// camMain's default 480-pixel rmMain shaft/side-room rules. Q12 easing
// accommodates the serialized 300000-pixel room height in signed 32 bits;
// freeCam, focus, side lockers, shake, autoscroll and ending are separate work.
struct MainCameraMotion {
    long x = 240L * 4096, y = 176L * 4096;
    long targetX = x, targetY = y;
    int slowdownX = 1, slowdownY = 5;
    void reset(int px, int py) {
        x = targetX = px * 4096L; y = targetY = py * 4096L;
        slowdownX = 1; slowdownY = 5;
    }
    void step(long playerXQ8, long playerYQ8, bool dead = false) {
        if (!dead) {
            if (playerXQ8 > 160L * 256 && playerXQ8 < 320L * 256) {
                targetX = 240L * 4096; // Both source horizontal clamps coincide.
                targetY = (playerYQ8 + 24L * 256) * 16;
                const bool cameraInShaft = x > 160L * 4096 && x < 320L * 4096;
                slowdownX = cameraInShaft ? 10 : 1;
                slowdownY = cameraInShaft ? 5 : 1;
            } else {
                slowdownX = 1;
                if (playerXQ8 < 160L * 256) targetX = 80L * 4096;
                else if (playerXQ8 > 320L * 256) targetX = 400L * 4096;
                // Exact boundary equality preserves the old target.
                // Side rooms preserve targetY and slowdownY.
            }
        }
        x += (targetX - x) / slowdownX;
        y += (targetY - y) / slowdownY;
        if (y < 142L * 4096) y = 142L * 4096;
    }
};
} // namespace downwell
#endif

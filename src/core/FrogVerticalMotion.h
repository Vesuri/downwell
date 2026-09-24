#ifndef DOWNWELL_CORE_FROG_VERTICAL_MOTION_H
#define DOWNWELL_CORE_FROG_VERTICAL_MOTION_H

namespace downwell {

// Dry frog vertical state in fifths of Q8: .15 gravity is exactly 192.
// The gravity/integration arithmetic is shared. Collision is still the
// provisional centre/foot point query, NOT source scrCheckCollisionWith.
struct FrogVerticalMotion {
    long position = 0;
    long velocity = 0;
    bool grounded = false;
    static long floorDivide(long value, long unit) {
        return value / unit - (value % unit < 0 ? 1 : 0);
    }
    int pixel() const { return static_cast<int>(floorDivide(position, 1280)); }
    template<class SolidAtY>
    void step(SolidAtY solid, bool integrate = true) {
        velocity += 192;
        if (velocity >= 7 * 1280) velocity = 7 * 1280;
        const long next = position + velocity;
        const bool supported = grounded && solid(pixel() + 9);
        if (velocity > 0 && (supported || solid(static_cast<int>(floorDivide(next, 1280)) + 8))) {
            while (!solid(pixel() + 9)) position += 1280;
            velocity = 0;
            grounded = true;
        } else {
            if (integrate) position = next;
            grounded = false;
        }
    }
};

} // namespace downwell
#endif

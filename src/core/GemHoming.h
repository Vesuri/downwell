#ifndef DOWNWELL_CORE_GEM_HOMING_H
#define DOWNWELL_CORE_GEM_HOMING_H
#include "BatMotion.h"
namespace downwell {
// objCurrency Create/attracted Step. Q16 degrees, exact fifth-degree turn
// acceleration and eighth-pixel speed. No age-derived acceleration.
struct GemHoming {
    long direction = 0;
    int turnFifths = 25;
    int speedEighths = 8;
    static long normalize(long angle) {
        angle %= 360L * 65536;
        return angle < 0 ? angle + 360L * 65536 : angle;
    }
    template<class Random>
    void initialize(Random& random) {
        random.irandomSmall(3); // image-speed choice precedes heading
        // VM evaluates choose's second argument before its first.
        const int second = 170 + static_cast<int>(random.irandomSmall(70));
        const int first = static_cast<int>(random.irandomSmall(70)) - 60;
        direction = normalize((random.irandomSmall(1) ? second : first) * 65536L);
        turnFifths = 25;
        speedEighths = 8;
    }
    void acquire(long dx, long dy) {
        direction = batPointDirection(dx, dy);
        turnFifths = 160;
        speedEighths = 32;
    }
    static long sine(long angle) {
        angle = normalize(angle);
        const int degrees = static_cast<int>(angle / 65536);
        const long fraction = angle % 65536;
        const long a = batSine(degrees), b = batSine(degrees + 1);
        return a + (b - a) * fraction / 65536;
    }
    void step(long dx, long dy, long& vx, long& vy) {
        const long target = batPointDirection(dx, dy);
        long difference = direction - target;
        if (difference > 180L * 65536) difference -= 360L * 65536;
        else if (difference < -180L * 65536) difference += 360L * 65536;
        const long turn = turnFifths * 65536L / 5;
        if (difference > turn) direction -= turn;
        else if (difference < -turn) direction += turn;
        else direction = target;
        direction = normalize(direction);
        turnFifths += 3;
        if (turnFifths > 160) turnFifths = 160;
        if (++speedEighths > 72) speedEighths = 72;
        vx = sine(direction + 90L * 65536) * speedEighths / 1024;
        vy = -sine(direction) * speedEighths / 1024;
    }
};
} // namespace downwell
#endif

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
    // Callers normally pass (-360, 720) degrees (Create's choose gives
    // [-60, 240), a turn moves a normalized heading at most 32 and sine adds
    // 90): one correction, else the modulo.
    static long normalize(long angle) {
        if (angle >= 0 && angle < 360L * 65536) return angle;
        if (angle < 0 && angle >= -360L * 65536) return angle + 360L * 65536;
        if (angle >= 360L * 65536 && angle < 720L * 65536) return angle - 360L * 65536;
        angle %= 360L * 65536;
        return angle < 0 ? angle + 360L * 65536 : angle;
    }
    template<class Random>
    void initialize(Random& random) {
        // VM evaluates choose's second argument before its first.
        const int second = 170 + static_cast<int>(random.irandomSmall(70));
        const int first = static_cast<int>(random.irandomSmall(70)) - 60;
        direction = normalize((random.chooseIndexSmall(2) ? second : first) * 65536L);
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
        // |b - a| <= 572; a negative angle keeps the original product.
        return a + (fraction >= 0 ? mulsu16(b - a, std::uint16_t(fraction)) : (b - a) * fraction) / 65536;
    }
    void step(long dx, long dy, long& vx, long& vy) {
        const long target = batPointDirection(dx, dy);
        long difference = direction - target;
        if (difference > 180L * 65536) difference -= 360L * 65536;
        else if (difference < -180L * 65536) difference += 360L * 65536;
        const long turn = mulu16(turnFifths, 13107) + std::uint16_t(turnFifths) / 5; // 65536 = 5 * 13107 + 1; [25, 160]
        if (difference > turn) direction -= turn;
        else if (difference < -turn) direction += turn;
        else direction = target;
        direction = normalize(direction);
        turnFifths += 3;
        if (turnFifths > 160) turnFifths = 160;
        if (++speedEighths > 72) speedEighths = 72;
        vx = mulsu16(sine(direction + 90L * 65536), speedEighths) / 1024; // |sine| <= 32768, speed <= 72
        vy = -mulsu16(sine(direction), speedEighths) / 1024;
    }
};
} // namespace downwell
#endif

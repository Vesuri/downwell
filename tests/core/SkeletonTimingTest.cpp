#include "core/SkeletonTiming.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>

static void check(bool value) { if (!value) std::abort(); }
static bool same(downwell::MotionBinary64 value, double expected) {
    std::uint64_t bits; std::memcpy(&bits, &expected, 8);
    return value.high == std::uint32_t(bits >> 32) && value.low == std::uint32_t(bits);
}
int main() {
    using downwell::SkeletonTiming;
    SkeletonTiming timing;
    for (auto alarm : timing.alarms) check(alarm == -1);
    check(same(timing.rate, 0) && same(timing.imageSpeed, 0));
    timing.alarm(3);
    check(same(timing.rate, 0.3) && same(timing.imageSpeed, 0));
    timing.beginStep(false);
    timing.finishThrow();
    check(same(timing.rate, 0) && same(timing.imageSpeed, 0.3) && timing.alarms[2] == 30);
    timing.alarms[2] = 7;
    timing.finishThrow();
    check(timing.alarms[2] == 7);
    timing.beginStep(false);
    check(same(timing.imageSpeed, 0));
    timing.shot = true;
    timing.imageIndex = downwell::MotionBinary64::fromInteger(7);
    timing.alarm(2);
    check(!timing.shot && same(timing.imageIndex, 0) && timing.alarms[3] == 15);

    unsigned cases = 0;
    for (int a = -1; a <= 30; ++a) for (bool stopped : {false, true}) {
        SkeletonTiming s;
        s.alarm(3);
        for (auto& value : s.alarms) value = a;
        s.beginStep(stopped);
        check(same(s.imageSpeed, stopped ? 0 : 0.3));
        check(s.alarms[0] == a + (stopped && a > 0));
        for (unsigned i = 1; i < 4; ++i) check(s.alarms[i] == a);
        s.hit();
        check(s.hitStun && s.alarms[1] == a + 5 && s.alarms[3] == 30);
        check(same(s.rate, 0) && same(s.imageIndex, 0));
        // The hit changes the desired rate, not this Step's image_speed.
        check(same(s.imageSpeed, stopped ? 0 : 0.3));
        s.hit();
        check(s.alarms[1] == a + 10);
        s.damaged = true;
        s.alarm(0); check(!s.damaged && s.hitStun);
        s.alarm(1); check(!s.hitStun);
        s.alarm(3);
        s.beginStep(true);
        check(same(s.rate, 0.3) && same(s.imageSpeed, 0));
        s.beginStep(false);
        check(same(s.imageSpeed, 0.3));
        ++cases;
    }
    std::printf("Skeleton timing: %u alarm/hit/time-stop cases and throw restart passed\n", cases);
}

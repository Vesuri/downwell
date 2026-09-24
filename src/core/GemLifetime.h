#ifndef DOWNWELL_CORE_GEM_LIFETIME_H
#define DOWNWELL_CORE_GEM_LIFETIME_H
namespace downwell {
// Default objCurrency alarms: 10 ticks until obtainable, then 180 until
// disappearing, then 120 until Alarm 2. Alarm 3 enables attraction at 15.
// Alarm 2 is one-shot and does not destroy an already attracted gem.
// Time-stop and upgrades which reschedule alarms are not modeled here yet.
struct GemLifetime {
    unsigned age = 0;
    bool advance(bool attracted) {
        ++age;
        return age != 310 || attracted;
    }
    bool obtainable() const { return age >= 10; }
    bool suckable() const { return age >= 15; }
    bool disappearing() const { return age >= 190; }
};
} // namespace downwell
#endif

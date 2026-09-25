#ifndef DOWNWELL_CORE_GEM_LIFETIME_H
#define DOWNWELL_CORE_GEM_LIFETIME_H
namespace downwell {
// Default objCurrency alarms: 10 ticks until obtainable, then 180 until
// disappearing, then 120 until Alarm 2. Alarm 3 enables attraction at 15.
// Alarm 2 is one-shot and does not destroy an already attracted gem.
// Gem Attractor snapshots Alarm 3=30 at creation. Time-stop is separate work.
struct GemLifetime {
    unsigned age = 0;
    bool advance(bool attracted) {
        ++age;
        return age != 310 || attracted;
    }
    bool obtainable() const { return age >= 10; }
    bool suckable(bool attractor = false) const { return age >= (attractor ? 30u : 15u); }
    bool disappearing() const { return age >= 190; }
    // Create sets dflash=-1. Alarm 1 enables flashing at age 190; Step
    // negates dflash on that update and Draw runs only when it is negative.
    // Visibility never gates movement, attraction or collection.
    bool visible() const { return !disappearing() || (age & 1u) != 0; }
};
} // namespace downwell
#endif

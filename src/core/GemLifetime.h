#ifndef DOWNWELL_CORE_GEM_LIFETIME_H
#define DOWNWELL_CORE_GEM_LIFETIME_H
namespace downwell {
// Default, unfrozen objCurrency alarms. Writes to a later alarm are seen in
// the same alarm-index phase: 0 starts 1, and 1 starts 2. Their first decrement
// is on the setter's update, so the chained boundaries are 189 and 308.
// Alarm 3 enables attraction at 15.
// Alarm 2 is one-shot and does not destroy an already attracted gem.
// Gem Attractor snapshots Alarm 3=30 at creation. Time-stop is separate work.
struct GemLifetime {
    static constexpr unsigned obtainableAge=10;
    static constexpr unsigned disappearingAge=obtainableAge+180-1;
    static constexpr unsigned expiryAge=disappearingAge+120-1;
    unsigned age = 0;
    bool advance(bool attracted) {
        ++age;
        return age != expiryAge || attracted;
    }
    bool obtainable() const { return age >= obtainableAge; }
    bool suckable(bool attractor = false) const { return age >= (attractor ? 30u : 15u); }
    bool disappearing() const { return age >= disappearingAge; }
    // Create sets dflash=-1. Alarm 1 enables flashing at age 189; Step
    // negates dflash on that update and Draw runs only when it is negative.
    // Visibility never gates movement, attraction or collection.
    bool visible() const { return !disappearing() || ((age-disappearingAge)&1u)!=0; }
};
} // namespace downwell
#endif

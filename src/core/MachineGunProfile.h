#ifndef DOWNWELL_CORE_MACHINE_GUN_PROFILE_H
#define DOWNWELL_CORE_MACHINE_GUN_PROFILE_H

namespace downwell {
// bStatInitLevel1, gunnum=0, selected by bStatUpdate. These are the selected
// table values, not bStatInitialize's temporary defaults. Keep profiles as
// values: future bullets must retain their birth profile across level changes.
// Integer degrees/speeds suffice for the table, not randomized trajectories.
struct MachineGunProfile {
    unsigned level;
    int object, sprite, sound, muzzle;
    int damage, speed, accuracy, consumption, interval, recoil;
    int shake, shakeDuration, rangeRandom, rangeTimer;
    bool consistent, wave, explode;
};

constexpr MachineGunProfile machineGunProfile(bool gemHigh) {
    return gemHigh
        ? MachineGunProfile{1, 261, 458, 19, 602, 15, 8, 3, 1, 6, 0, 3, 4, 2, 13, false, false, false}
        : MachineGunProfile{0, 261, 459, 18, 603, 9, 8, 3, 1, 7, 0, 2, 3, 2, 12, false, false, false};
}

// scrControlBeginstepCheck evaluates the updated streak before later pickup
// events. The caller controls phase ordering; this selector does not tick it.
constexpr MachineGunProfile machineGunProfileForStreak(unsigned streak) {
    return machineGunProfile(streak >= 100);
}
} // namespace downwell
#endif

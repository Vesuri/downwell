#ifndef DOWNWELL_CORE_MACHINE_GUN_EMITTER_H
#define DOWNWELL_CORE_MACHINE_GUN_EMITTER_H
namespace downwell {
// scrPlayerInit captures Laser Sight when the player instance is created.
// scrPlayerEmitBullet flips shootLeg after every shot, including zero.
struct MachineGunEmitter {
    int leg = -1;
    void reset(bool laserSight) { leg = laserSight ? 0 : -1; }
    int nextOffset() { const int offset = leg; leg = -leg; return offset; }
};
}
#endif

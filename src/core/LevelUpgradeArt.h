#ifndef DOWNWELL_CORE_LEVEL_UPGRADE_ART_H
#define DOWNWELL_CORE_LEVEL_UPGRADE_ART_H
namespace downwell {
// scrUpgrades sprite column. IDs remain original resource IDs, not bank slots.
inline unsigned levelUpgradeSprite(unsigned id) {
    static const unsigned short sprites[] = {524,528,512,520,518,521,522,
        506,507,508,509,510,511,516,523,525,529,530,531,532,533,534,535};
    return id < 23 ? sprites[id] : id == 100 ? 152 : 0;
}
}
#endif

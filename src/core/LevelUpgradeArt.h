#ifndef DOWNWELL_CORE_LEVEL_UPGRADE_ART_H
#define DOWNWELL_CORE_LEVEL_UPGRADE_ART_H
namespace downwell {
inline unsigned levelUpgradeNameLabel(unsigned id, unsigned owned = 0) {
    if (id > 22 && id != 100) return 0;
    if (!owned) return 64100 + id;
    return id <= 22 && owned == 1 ? 64600 + id : 0;
}
inline unsigned levelClearStageLabel(unsigned area, unsigned level) {
    return area >= 1 && area <= 4 && level >= 1 && level <= 3 ?
        64500 + (area - 1) * 3 + level - 1 : 0;
}
// scrInitialize's levelTile resource table (including the incoming final area).
inline unsigned levelBorderSprite(unsigned area) {
    static const unsigned short sprites[] = {76, 51, 74, 72, 70, 55};
    return area < 6 ? sprites[area] : 0;
}
// scrUpgrades sprite column. IDs remain original resource IDs, not bank slots.
inline unsigned levelUpgradeSprite(unsigned id) {
    static const unsigned short sprites[] = {524,528,512,520,518,521,522,
        506,507,508,509,510,511,516,523,525,529,530,531,532,533,534,535};
    return id < 23 ? sprites[id] : id == 100 ? 152 : 0;
}
}
#endif

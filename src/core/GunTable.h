#ifndef DOWNWELL_CORE_GUN_TABLE_H
#define DOWNWELL_CORE_GUN_TABLE_H
namespace downwell {
// bStatInitLevel1's seven guns at both levels (pGunLevel: gemHigh), as
// bStatUpdate copies them into the pBul* globals. tools/verify-gun-table.py
// checks every value against the pinned UTMT export. Units:
// - recoil in fifths of Q8 (1/1280 pixel per Step), exact for every entry;
// - accuracy in half degrees (bdirRand), exact for BURST's 2.5;
// - rof keeps its sign: negative means nonAuto (release before the next shot).
// bStatSp3 is never read; the source keeps no other consumer of it.
struct GunStats {
    const char* name;
    int letter;        // bStatGunSprite: sprModText (141) frame
    int object;        // 260 bulletLaser, 261 bulletRanged
    int sprite, sound, muzzle;
    int consumption, rof, recoilFifths, shake, shakeDuration;
    bool delayKill, burst;
    int burstRate, burstAmount, damage, speed, accuracyHalves;
    bool pierce;
    int rangeRandom, rangeTimer, spType, sp1, sp2;
    int aimAcceleration, aimDeceleration, aimLimit;
};

enum : int { gunMachine = 0, gunShotgun, gunLaser, gunTriple, gunPuncher, gunBurst, gunNoppy, gunCount };
constexpr int gunMaximumNumber = gunCount - 1; // global.bulletMaxNum

constexpr GunStats gunTable[gunCount][2] = {
    {{"MACHINEGUN",12,261,459,18,603,1,7,0,2,3,true,false,6,1,9,8,6,false,2,12,0,0,0,5,2,0},
     {"MACHINEGUN",12,261,458,19,602,1,6,0,3,4,true,false,5,1,15,8,6,false,2,13,0,0,0,5,2,0}},
    {{"SHOTGUN",18,261,472,22,603,5,-20,-1792,4,6,false,true,0,7,6,15,20,false,5,4,0,0,0,5,2,0},
     {"SHOTGUN",18,261,473,23,602,5,-15,-2560,5,10,false,true,0,15,6,18,24,false,10,5,0,0,0,5,2,0}},
    {{"LASER",11,260,466,20,602,4,-5,-1280,3,8,false,false,6,1,25,0,0,true,3,8,0,0,0,5,2,0},
     {"LASER",11,260,465,21,601,4,-12,-1920,4,8,false,false,0,0,35,0,0,true,5,6,0,0,0,5,2,0}},
    {{"TRIPLE",19,261,472,24,603,2,8,-384,3,4,true,false,6,1,5,6,2,false,0,14,1,1,14,5,2,0},
     {"TRIPLE",19,261,473,25,602,2,7,-512,4,4,true,false,5,1,8,6,2,false,0,20,1,1,14,5,2,0}},
    {{"PUNCHER",15,261,459,43,603,2,11,-640,4,4,true,false,6,1,7,7,0,false,0,14,2,1,5,5,2,0},
     {"PUNCHER",15,261,458,44,602,2,10,-640,5,5,true,false,5,1,9,7,0,false,0,16,2,1,6,5,2,0}},
    {{"BURST",1,261,472,45,603,3,-13,0,2,3,false,true,5,2,7,8,5,false,0,20,0,0,0,5,2,0},
     {"BURST",1,261,473,46,602,3,-10,0,3,4,false,true,5,2,14,8,4,false,0,20,0,0,0,5,2,0}},
    {{"NOPPY",13,261,472,41,603,1,5,0,4,2,true,false,6,1,8,8,12,false,2,13,0,0,0,4,2,25},
     {"NOPPY",13,261,473,42,602,1,5,0,5,3,true,false,5,1,12,9,10,false,2,14,0,0,0,4,2,25}},
};

constexpr const GunStats& gunStats(int gun, bool gemHigh) { return gunTable[gun][gemHigh ? 1 : 0]; }
constexpr bool validGun(int gun) { return gun >= 0 && gun < gunCount; }
}
#endif

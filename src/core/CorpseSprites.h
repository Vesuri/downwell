#ifndef DOWNWELL_CORE_CORPSE_SPRITES_H
#define DOWNWELL_CORE_CORPSE_SPRITES_H
namespace downwell {
// One-frame unit-rate dead sprites admitted for objDeadBody 263: frog 172,
// skeleton 206 and the other world-one families' scrDeadBody art (bat 158,
// snake 165, snail 179, crawler 281, turtle 337).
inline bool corpseSpriteAdmitted(int sprite){
    switch(sprite){case 158:case 165:case 172:case 179:case 206:case 281:case 337:return true;default:return false;}
}
}
#endif

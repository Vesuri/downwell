#ifndef DOWNWELL_CORE_ENEMY_PRESENTATION_H
#define DOWNWELL_CORE_ENEMY_PRESENTATION_H

namespace downwell {

// Ground turtle Create/Alarm 1 alternate walking between +1 and -1.
// GML's numeric boolean conversion is not C++'s nonzero conversion.
inline bool groundTurtleWalking(int walking) { return walking > 0; }

// Fortieths also represent the catacomb skull's 0.175 (7/40) exactly.
// 256/40 = 32/5: the stored remainder is still fifths of a Q8 phase unit,
// so existing state snapshots, frame resets and rate changes keep their units.
inline void advanceEnemyImageFortiethsQ8(unsigned& phase, unsigned char& remainder,
                                        unsigned fortieths) {
    const unsigned numerator = fortieths * 32u + remainder;
    phase += numerator / 5u;
    remainder = static_cast<unsigned char>(numerator % 5u);
}

// Existing actors and snapshots retain twentieth-frame rates. Do not silently
// reinterpret those fields when introducing an actor with a half-step rate.
inline void advanceEnemyImageQ8(unsigned& phase, unsigned char& remainder,
                                 unsigned twentieths) {
    advanceEnemyImageFortiethsQ8(phase, remainder, twentieths * 2u);
}

// Recovered resource IDs and Step-state choices; no asset payloads.
inline int enemyPresentationSprite(int object, bool grounded, int auxiliary) {
    switch (object) {
        case 111: return grounded ? 163 : 164;
        case 112: return !grounded ? 187 : groundTurtleWalking(auxiliary) ? 188 : 186;
        case 113: return !grounded ? 168 : auxiliary != 0 ? 170 : 166;
        case 116: return 153;
        case 117: return 177;
        case 119: return 278;
        default: return -1;
    }
}

inline unsigned enemySpriteFrameCount(int sprite) {
    switch (sprite) {
        case 153: case 157: case 162: case 163: case 166: case 170: case 173: case 177: case 186: return 4;
        case 168: case 278: return 2;
        case 188: return 6;
        default: return 1;
    }
}

} // namespace downwell
#endif

#ifndef DOWNWELL_CORE_ENEMY_PRESENTATION_H
#define DOWNWELL_CORE_ENEMY_PRESENTATION_H

namespace downwell {

// Recovered resource IDs and Step-state choices; no asset payloads.
inline int enemyPresentationSprite(int object, bool grounded, int auxiliary) {
    switch (object) {
        case 111: return grounded ? 163 : 164;
        case 112: return !grounded ? 187 : auxiliary != 0 ? 188 : 186;
        case 113: return !grounded ? 168 : auxiliary != 0 ? 170 : 166;
        case 116: return 153;
        case 117: return 177;
        case 119: return 278;
        default: return -1;
    }
}

inline unsigned enemySpriteFrameCount(int sprite) {
    switch (sprite) {
        case 153: case 163: case 166: case 170: case 177: case 186: return 4;
        case 168: case 278: return 2;
        case 188: return 6;
        default: return 1;
    }
}

} // namespace downwell
#endif

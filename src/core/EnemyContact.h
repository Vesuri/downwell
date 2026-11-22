#ifndef DOWNWELL_CORE_ENEMY_CONTACT_H
#define DOWNWELL_CORE_ENEMY_CONTACT_H

#include "GameMakerRounding.h"

namespace downwell {

// Integer pixel-cell edges: right/bottom are exclusive. Margin/origin values
// are verified against the pinned data.win export by verify-enemy-contact.py.
struct ContactBounds { int left, right, top, bottom; };
inline ContactBounds contactSpriteBounds(int sprite) {
    switch (sprite) {
        case 31: return {-4, 4, -4, 8};
        case 84: return {-2, 2, -1, 2};
        case 91: return {-4, 4, -2, 1};
        case 133: return {-4, 4, -4, 4};
        case 134: return {-6, 6, -6, 6};
        case 139: return {-8, 8, -7, 9};
        case 140: return {-8, 8, -7, 9};
        case 153: return {-6, 7, -4, 4};
        case 154: return {-6, 7, -4, 4};
        case 157: return {-5, 5, -6, 6};
        case 162: return {-6, 0, -6, 7};
        case 163: return {-6, 7, -4, 5};
        case 164: return {-6, 7, -4, 5};
        case 166: return {-6, 6, -5, 8};
        case 174: return {-9, 9, -6, 4};
        case 177: return {-7, 7, -6, 7};
        case 186: return {-8, 8, -4, 8};
        case 208: return {-10, 10, -7, 8};
        case 278: return {-5, 5, -5, 5};
        case 289: return {-9, 9, -4, 4};
        case 458: return {2, 8, -3, 3};
        case 459: return {-4, 6, -3, 3};
        case 457: return {-4, 4, -4, 4};
        case 460: return {-5, 5, -4, 5};
        case 465: return {-4, 4, -4, 4};
        case 466: return {-2, 2, -4, 4};
        case 461: return {-20, 20, -30, 13};
        case 463: return {-25, 25, -26, 25};
        case 467: return {-3, 3, -3, 3};
        case 470: return {-8, 8, -8, 8};
        case 471: return {-16, 16, -10, 66};
        case 472: return {4, 8, -2, 2};
        case 473: return {4, 9, -3, 3};
        case 526: return {-5, 6, -13, -1};
        case 550: return {-6, 6, -1, 8};
        case 568: return {-6, 6, -5, 7};
        case 569: return {-9, 10, -7, 9};
        case 585: return {-8, 8, -8, 8};
        case 598: return {-5, 5, -5, 5};
        case 599: return {-7, 7, -8, 0};
        case 702: return {-8, 8, -10, 7};
        case 710: return {-2, 2, -2, 2};
        default: return {0, 0, 0, 0};
    }
}

inline int enemyContactMask(int object, int sprite,int explicitMask=-1) {
    switch (object) {
        case 111: return sprite;
        case 116: return explicitMask>=0?explicitMask:sprite;
        case 112: return 186; // Create: mask_index = sprite_index
        case 113: return 166; // serialized explicit mask
        case 115: return 174; // serialized explicit masks, unchanged by hit art
        case 114: return 289; // Create: mask_index = normalSpr
        case 118: case 173: return 153;
        case 117: return 177;
        case 119: return 278;
        case 120: return 208; // Explicit mask: unchanged by throw/hit artwork.
        default: return -1;
    }
}

struct ContactActor {
    long x, y; // Q8 precise state, rounded like the source x/y assignments
    int scaleX, scaleY; // supported active states use unit flips only
    int mask;
};

inline bool contactBoundsOverlap(const ContactBounds& a,const ContactBounds& b) {
    return a.left<a.right && a.top<a.bottom && b.left<b.right && b.top<b.bottom &&
        a.left<b.right && b.left<a.right && a.top<b.bottom && b.top<a.bottom;
}

inline ContactBounds contactWorldBounds(const ContactActor& actor) {
    ContactBounds bounds = contactSpriteBounds(actor.mask);
    if (actor.scaleX < 0) {
        const int left = bounds.left;
        bounds.left = -bounds.right; bounds.right = -left;
    }
    if (actor.scaleY < 0) {
        const int top = bounds.top;
        bounds.top = -bounds.bottom; bounds.bottom = -top;
    }
    const int x = (int)roundGameMakerQ8(actor.x);
    const int y = (int)roundGameMakerQ8(actor.y);
    bounds.left += x; bounds.right += x;
    bounds.top += y; bounds.bottom += y;
    return bounds;
}

// contactWorldBounds for an actor at whole pixels (Q8 x*256, y*256), which
// round to themselves: the motion scripts' solid tests.
inline ContactBounds contactPixelBounds(int x, int y, int scaleX, int scaleY, int mask) {
    ContactBounds bounds = contactSpriteBounds(mask);
    if (scaleX < 0) {
        const int left = bounds.left;
        bounds.left = -bounds.right; bounds.right = -left;
    }
    if (scaleY < 0) {
        const int top = bounds.top;
        bounds.top = -bounds.bottom; bounds.bottom = -top;
    }
    bounds.left += x; bounds.right += x;
    bounds.top += y; bounds.bottom += y;
    return bounds;
}

enum class EnemyContact { none, stomp, damage };
// stompY is the Q8 value compared with global.ply. Families whose private yy
// is not on the Q8 lattice pass its exact ceiling: yy > p+2 <=> ceil > p+512.
inline EnemyContact classifyEnemyContact(const ContactActor& player,
    const ContactActor& enemy, int object, long playerStartY, long stompY) {
    const ContactBounds a = contactWorldBounds(player);
    const ContactBounds b = contactWorldBounds(enemy);
    if (a.left == a.right || b.left == b.right ||
        a.left >= b.right || b.left >= a.right ||
        a.top >= b.bottom || b.top >= a.bottom) return EnemyContact::none;
    const bool stompable = object == 111 || object == 112 || object == 113 ||
        object == 114 || object == 115 || object == 116;
    // typStompable compares enemy yy, not rounded y, to global.ply published
    // by scrPlMovement BEFORE the player's integration in this frame.
    return stompable && stompY > playerStartY + 2 * 256
        ? EnemyContact::stomp : EnemyContact::damage;
}
inline EnemyContact classifyEnemyContact(const ContactActor& player,
    const ContactActor& enemy, int object, long playerStartY) {
    return classifyEnemyContact(player, enemy, object, playerStartY, enemy.y);
}

} // namespace downwell
#endif

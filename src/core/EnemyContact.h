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
        case 133: return {-4, 4, -4, 4};
        case 134: return {-6, 6, -6, 6};
        case 153: return {-6, 7, -4, 4};
        case 154: return {-6, 7, -4, 4};
        case 157: return {-5, 5, -6, 6};
        case 162: return {-6, 0, -6, 7};
        case 163: return {-6, 7, -4, 5};
        case 164: return {-6, 7, -4, 5};
        case 166: return {-6, 6, -5, 8};
        case 177: return {-7, 7, -6, 7};
        case 186: return {-8, 8, -4, 8};
        case 208: return {-10, 10, -7, 8};
        case 278: return {-5, 5, -5, 5};
        case 460: return {-5, 5, -4, 5};
        case 569: return {-9, 10, -7, 9};
        case 585: return {-8, 8, -8, 8};
        default: return {0, 0, 0, 0};
    }
}

inline int enemyContactMask(int object, int sprite,int explicitMask=-1) {
    switch (object) {
        case 111: return sprite;
        case 116: return explicitMask>=0?explicitMask:sprite;
        case 112: return 186; // Create: mask_index = sprite_index
        case 113: return 166; // serialized explicit mask
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
inline EnemyContact classifyEnemyContact(const ContactActor& player,
    const ContactActor& enemy, int object, long playerStartY) {
    const ContactBounds a = contactWorldBounds(player);
    const ContactBounds b = contactWorldBounds(enemy);
    if (a.left == a.right || b.left == b.right ||
        a.left >= b.right || b.left >= a.right ||
        a.top >= b.bottom || b.top >= a.bottom) return EnemyContact::none;
    const bool stompable = object == 111 || object == 112 || object == 113 || object == 116;
    // typStompable compares enemy yy, not rounded y, to global.ply published
    // by scrPlMovement BEFORE the player's integration in this frame.
    return stompable && enemy.y > playerStartY + 2 * 256
        ? EnemyContact::stomp : EnemyContact::damage;
}

} // namespace downwell
#endif

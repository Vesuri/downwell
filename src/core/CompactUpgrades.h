#ifndef DOWNWELL_CORE_COMPACT_UPGRADES_H
#define DOWNWELL_CORE_COMPACT_UPGRADES_H
#include "ConsumptionBloodEffect.h"
#include "EnemyContact.h"
#include "RandomBinary64.h"
#include "RunnerImageValue.h"
#include "UnitSequenceFrames.h"
#include <cstdint>
namespace downwell {
// Compact owners for the world-one upgrades' own objects (W1.9). Room pixel
// coordinates unless noted; Q16 reals as in CompactGuns. This is the compact
// world's arithmetic, not the binary64 VM path.

// bulletBlast 276 (Exploding Knees at the stomped enemy, sprBlast 471;
// Rocket Jump's at the player, spSplosion1 461) and bulletExplosion1 272
// with its child explosionSmall 274 (Rest in Pieces' corpses, the decoy).
// All four sprites have one axis-aligned collision rectangle.
struct CompactBlastRecord {
    static constexpr unsigned hitCapacity = 16;
    bool live = false;
    int object = 276, sprite = 471, x = 0, y = 0, xscale = 1, turn = 0; // turn = image_angle / 90
    RunnerImageValue imageIndex{}, imageSpeed{}, speed{}; // speed: bulletExplosion1's imgSp
    bool damageStart = false, damageEnd = false;
    std::uint32_t serial = 0;
    // hitIgnoreCheck's hitMem: tagged keys (kind << 28 | identity).
    std::uint32_t hits[hitCapacity] = {};
    unsigned hitCount = 0;
    static unsigned frames(int sprite) {
        switch (sprite) { case 471: return 14; case 461: return 12; case 463: return 12; case 470: return 8; default: return 0; }
    }
    // The rotated and mirrored collision rectangle in room pixels:
    // image_xscale, then image_angle counter-clockwise on screen, with the
    // same edge negation as contactPixelBounds' mirror.
    ContactBounds bounds() const {
        ContactBounds b = contactSpriteBounds(sprite);
        if (xscale < 0) { const int l = b.left; b.left = -b.right; b.right = -l; }
        for (int t = 0; t < turn; ++t) {
            const ContactBounds r{b.top, b.bottom, -b.right, -b.left}; // (dx, dy) -> (dy, -dx)
            b = r;
        }
        b.left += x; b.right += x; b.top += y; b.bottom += y;
        return b;
    }
    bool damaging() const { return (object == 276 || damageStart) && !damageEnd; }
    // hitIgnoreCheck: true the first time this key is met.
    bool remember(std::uint32_t key) {
        for (unsigned i = 0; i < hitCount; ++i) if (hits[i] == key) return false;
        if (hitCount < hitCapacity) hits[hitCount++] = key;
        return true;
    }
    // Step: bulletBlast tests only damageEndFrame 3; bulletExplosion1 starts
    // at frame 2. image_index > n compares the runner float with n.
    void step(bool moving) {
        const std::uint32_t two = 0x40000000u, three = 0x40400000u;
        if (object != 276 && imageIndex.bits > two && !(imageIndex.bits >> 31)) damageStart = true;
        if (imageIndex.bits > three && !(imageIndex.bits >> 31)) damageEnd = true;
        if (object != 276) imageSpeed = moving ? speed : RunnerImageValue{};
    }
    // Runner animation; Animation End (Other 7) destroys every one of them.
    bool advanceAnimation() {
        RunnerImageValue image; bool crossed = false;
        if (!UnitSequenceFrames::advance(imageIndex, imageSpeed, frames(sprite), image, crossed)) return false;
        if (crossed) live = false;
        return true;
    }
    unsigned frame() const {
        const unsigned exponent = (imageIndex.bits >> 23) & 255u;
        return exponent < 127 ? 0 : ((imageIndex.bits & 0x7fffffu) | 0x800000u) >> (150u - exponent);
    }
};

// objDrone 249 (Drone, upgrade 9): image_speed 0.5 over sprDrone's two
// frames; the Step eases towards the player along a small ellipse.
struct CompactDroneRecord {
    bool live = false, shooting = false;
    long x = 0, y = 0; // Q16 room coordinates
    int xdir = 0, ydir = 0, alarm0 = -1;
    unsigned phaseHalves = 0; // image_index in halves, two frames
    unsigned frame() const { return (phaseHalves / 2) & 1u; }
};

// decoyBalloon 256 (Heart Balloon, upgrade 15). The player holds it from its
// first Step; enemies then chase it (global.eplx/eply) until one touches it.
// xx/yy are the Step's VM doubles; x/y their rounded builtin position.
struct CompactBalloonRecord {
    bool live = false, held = false;
    MotionBinary64 xx{}, yy{}, ysp{};
    int x = 0, y = 0, sprite = 526, stringLength = 20;
    RunnerImageValue imageIndex{}, imageSpeed{0x3da3d70au}; // imgSp 0.08
    // mask_index 526 (sprDecoyBalloon).
    ContactBounds bounds() const { return contactPixelBounds(x, y, 1, 1, 526); }
    unsigned frame() const {
        std::int32_t whole = 0;
        const unsigned exponent = (imageIndex.bits >> 23) & 255u;
        if (exponent >= 127)
            whole = std::int32_t((((imageIndex.bits & 0x7fffffu) | 0x800000u) >> (150u - exponent)));
        return unsigned(whole) & 3u;
    }
};

// fxBlood 223 (Knife and Fork's five drops at the player): default Draw of
// sprWaterSplash 92. Alarm 0 (120) and any parentWall contact destroy it.
struct CompactBloodRecord {
    bool live = false;
    ConsumptionBloodEffect effect;
    std::uint32_t birth = 0;
    struct Publish {
        bool gravity(MotionBinary64) { return true; }
        bool imageSpeed(RunnerImageValue) { return true; }
        bool imageIndex(RunnerImageValue) { return true; }
        bool destroy() { return true; }
    };
    struct Environment {
        bool position(RunnerImageValue, RunnerImageValue) { return true; }
        MotionBinary64 gravity() { return {0x3fc99999u, 0x9999999au}; } // global.grav 0.2
        MotionBinary64 fallSpeed() { return {}; } // global.fallspeed stays 0
    };
};
}
#endif

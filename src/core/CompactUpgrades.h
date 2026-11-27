#ifndef DOWNWELL_CORE_COMPACT_UPGRADES_H
#define DOWNWELL_CORE_COMPACT_UPGRADES_H
#include "EnemyContact.h"
#include "RandomBinary64.h"
#include "RunnerImageValue.h"
#include "UnitSequenceFrames.h"
#include "FixedPoint.h"
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
    std::int32_t xx = 0, yy = 0, ysp = 0; // Q16
    int rising = 0; // the loose rise's 0.02 steps, at most 40 (0.8)
    int x = 0, y = 0, sprite = 526, stringLength = 20;
    // decoyBalloon's motion in Q16 (W3.20). The source's comparisons are
    // exact (no 1e-5 band); each constant here is the Q16 bound that keeps
    // the comparison's outcome. The easing divisions round to the nearest
    // Q16, halves away from zero, so the eased position drifts within a few
    // Q16 units of the VM's binary64.
    static std::int32_t divide(std::int32_t v, std::int32_t d) {
        const std::int32_t m = v < 0 ? -v : v, q = (m + d / 2) / d;
        return v < 0 ? -q : q;
    }
    static std::int32_t magnitude(std::int32_t v) { return v < 0 ? -v : v; }
    // Held: the string's top is ply - climb + |plx - xx| / 2; xx eases by
    // (plx - xx) / (stringLength / 2), yy by (top - yy) / 4 below the string
    // limit, / 10 within it. Returns false outside the Q16 range.
    bool ease(std::int32_t plx, std::int32_t ply, int climb, std::int32_t& xMove) {
        constexpr std::int32_t range = std::int32_t(1) << 29;
        if (magnitude(plx) >= range || magnitude(ply) >= range || magnitude(xx) >= range || magnitude(yy) >= range ||
            climb < 0 || climb > 32 || (stringLength != 10 && stringLength != 20)) return false;
        const std::int32_t dx = plx - xx;
        const std::int32_t top = ply - climb * 65536 + divide(magnitude(dx), 2);
        xMove = divide(dx, stringLength / 2);
        xx += xMove;
        const std::int32_t limit = ply - stringLength * 65536;
        yy += divide(top - yy, yy < limit ? 4 : 10);
        return true;
    }
    // The held art: sprDecoyBalloonMove 700 and its pull frame when
    // 0.2 < |xMove|, else sprDecoyBalloon 701 at image_speed 0.08.
    bool pose(std::int32_t xMove) {
        if (magnitude(xMove) > 13107) { // 0.2 < |xMove|
            sprite = 700; imageSpeed = {};
            constexpr std::int32_t pull = 78643; // 1.2 < xMove
            const int frame = xMove > 0 ? (xMove > pull ? 1 : 0) : (xMove < -pull ? 3 : 2);
            return imageIndex.assignInteger(frame);
        }
        sprite = 701; imageSpeed = {0x3da3d70au};
        return true;
    }
    // Loose, after the wall query at yy + ysp: rise by 0.02 to 0.8. ysp is
    // the nearest Q16 of rising * -0.02 rather than a sum of rounded steps,
    // which would drift 0.28 Q16 further per step.
    bool rise(bool wall) {
        if (magnitude(yy) >= (std::int32_t(1) << 29)) return false;
        if (wall) rising = 0;
        else if (rising < 40) ++rising; // -0.8 < ysp: ysp += -0.02
        ysp = -((rising * 131072 + 50) / 100);
        yy += ysp; // xsp stays 0
        return true;
    }
    void publish() { x = Fixed16::roundQ16(xx); y = Fixed16::roundQ16(yy); }
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
    // Q16 (W3.20). The VM keeps x/y as binary32 builtins; Q16 is finer than
    // binary32 below 32768 pixels, so the drops stay within a pixel of it.
    // ysp is ysp0 plus the nearest Q16 of falls * 0.2 (global.grav), not a
    // sum of rounded steps.
    std::int32_t x = 0, y = 0, xsp = 0, ysp0 = 0;
    int alarm0 = -1, falls = 0;
    RunnerImageValue imageSpeed{0x3e99999au}, imageIndex{}, timeline{}; // imgSp 0.3
    std::uint32_t birth = 0;
    // Create: ysp = random(3) - 2, xsp = random(1) * choose(1, -1), alarm 0
    // 120, image_index irandom(6). Gravity and image_speed are fixed.
    template<class Random> bool create(int px, int py, Random& random) {
        x = px * 65536; y = py * 65536;
        ysp0 = Fixed16::random(random.nextU32(), Fixed16::fromInteger(3)).v - 131072;
        xsp = Fixed16::random(random.nextU32(), Fixed16::fromInteger(1)).v;
        if (random.chooseIndexSmall(2)) xsp = -xsp;
        alarm0 = 120; falls = 0;
        return imageIndex.assignInteger(int(random.irandomSmall(7)));
    }
    std::int32_t ysp() const { return ysp0 + (falls * 131072 + 5) / 10; }
    // Step: x += xsp, y += ysp, xsp /= 1.01, ysp += grav; global.fallspeed
    // stays 0. Alarm 0 runs out before falls can leave the range.
    bool step() {
        constexpr std::int32_t range = std::int32_t(1) << 30;
        const std::int32_t vy = ysp();
        if (x <= -range || x >= range || y <= -range || y >= range || falls >= 1000) return false;
        x += xsp; y += vy;
        const std::int32_t m = (xsp < 0 ? -xsp : xsp) * 100, q = (m + 50) / 101;
        xsp = xsp < 0 ? -q : q;
        ++falls;
        return true;
    }
    bool advanceImage() {
        RunnerImageValue evaluated; bool reconcile = false, crossed = false;
        if (!UnitSequenceFrames::evaluate(timeline, 8, evaluated) ||
            !UnitSequenceFrames::needsReconcile(imageIndex, evaluated, reconcile)) return false;
        auto next = reconcile ? imageIndex : timeline;
        if (!UnitSequenceFrames::advance(next, imageSpeed, 8, evaluated, crossed)) return false;
        timeline = next; imageIndex = evaluated;
        return true; // No Animation End handler.
    }
    bool drawFrame(unsigned& frame) const {
        const unsigned magnitude = imageIndex.bits & 0x7fffffffu;
        if (magnitude >= 0x41000000u || ((imageIndex.bits >> 31) && magnitude)) return false;
        const unsigned exponent = (imageIndex.bits >> 23) & 255u;
        frame = exponent < 127 ? 0 : ((imageIndex.bits & 0x7fffffu) | 0x800000u) >> (150u - exponent);
        return true;
    }
    int pixelX() const { return Fixed16::roundQ16(x); }
    int pixelY() const { return Fixed16::roundQ16(y); }
};
}
#endif

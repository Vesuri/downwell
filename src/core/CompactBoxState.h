#ifndef DOWNWELL_CORE_COMPACT_BOX_STATE_H
#define DOWNWELL_CORE_COMPACT_BOX_STATE_H
#include "BreakableState.h"
#include "FixedPoint.h"
#include <cstdint>
namespace downwell {
// The shipping world's objBox_n Create fields (W3.20): BreakableState with
// wallHp in Q16. The world's own writes are whole: Create's 1, bullet and
// blast damage, Alarm 0's and the wall contact's 0. The owned reference may
// publish fractional health; reference() and fromReference() are its exact
// binary64 boundary and are not linked by the world-one build.
struct CompactBoxState {
    std::int32_t wallHp = 65536; // Q16
    unsigned short breakSound = 171;
    unsigned char haveGem = 0;
    bool hit = false, drilled = false, autoTile = false, checked = false;
    bool materialBreakable = true, groundRoomResult = false;
    std::int32_t scriptIndex = 0;
    // scrBulCheckSolid [11..13]: positive health only; writes precede the
    // bullet's destruction, and the box's own Step destroys it later.
    bool bulletHit(int damage, bool& applied) {
        applied = false;
        if (damage < 0 || damage > 16384) return false;
        if (wallHp <= 0) return true;
        wallHp -= damage * 65536; hit = true; applied = true; return true;
    }
    // A blast's bdmg after its own positive-health test.
    bool blastHit(int damage) {
        if (damage < 0 || damage > 16384 || wallHp <= 0) return false;
        wallHp -= damage * 65536; return true;
    }
    bool exhausted() const { return wallHp <= 0; }
    void alarmZero() { wallHp = 0; }
    bool stopAlarmZero(bool timeAllowed, std::int32_t& alarm) {
        if (timeAllowed) return true;
        scriptIndex = 0;
        if (alarm > 0) {
            if (alarm == 0x7fffffff) return false;
            ++alarm;
        }
        scriptIndex = 1; return true;
    }
    BreakableState reference() const {
        BreakableState r;
        r.wallHp = Fixed16::raw(wallHp).binary64();
        r.breakSound = breakSound; r.haveGem = haveGem; r.hit = hit; r.drilled = drilled;
        r.autoTile = autoTile; r.checked = checked; r.materialBreakable = materialBreakable;
        r.groundRoomResult = groundRoomResult; r.scriptIndex = scriptIndex;
        return r;
    }
    // Exact binary64 health only (whole or a Q16 fraction such as -0.5).
    static bool healthFromReference(MotionBinary64 health, std::int32_t& out) {
        Fixed16 q;
        if (!Fixed16::fromBinary64(health, q)) return false;
        const auto back = q.binary64();
        if (back.high != health.high || back.low != health.low) return false;
        out = q.v; return true;
    }
    static bool fromReference(const BreakableState& s, CompactBoxState& out) {
        CompactBoxState r;
        if (!healthFromReference(s.wallHp, r.wallHp)) return false;
        r.breakSound = s.breakSound; r.haveGem = s.haveGem; r.hit = s.hit; r.drilled = s.drilled;
        r.autoTile = s.autoTile; r.checked = s.checked; r.materialBreakable = s.materialBreakable;
        r.groundRoomResult = s.groundRoomResult; r.scriptIndex = s.scriptIndex;
        out = r; return true;
    }
};
}
#endif

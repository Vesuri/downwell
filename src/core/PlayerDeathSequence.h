#ifndef DOWNWELL_CORE_PLAYER_DEATH_SEQUENCE_H
#define DOWNWELL_CORE_PLAYER_DEATH_SEQUENCE_H
#include "AlarmBank.h"

namespace downwell {
// scrPlMovement + objPlayer_n Alarm 0. Call Begin Step before movement and
// alarms after it. Entry is explicit: adapters currently publish depletion at
// contact time, not the original runner's later player Begin Step boundary.
struct PlayerDeathSequence {
    bool active = false;
    bool started = false;
    bool priorFire = false;
    bool menuReady = false;
    unsigned frames = 0;
    unsigned skipPresses = 0;
    int animation = 0;
    int alarm = -1;

    void enter() {
        if (active) return;
        active = true;
        animation = 1;
    }
    // DeathFlashFx Create runs synchronously before scrPlMovement reads
    // room_speed for alarm[0], and sets it to 20 (not the usual 60).
    void beginStep(bool fireHeld, int roomSpeedAfterFlash = 20) {
        const bool pressed = fireHeld && !priorFire;
        priorFire = fireHeld;
        if (!active) return;
        ++frames;
        if (!started) {
            started = true;
            alarm = roomSpeedAfterFlash * 3 / 2;
        }
        if (pressed && ++skipPresses == 3 && animation >= -1) {
            animation = -2;
            menuReady = true;
        }
    }
    // Return the source's optional grounded objJumpSmallerFx request.
    bool advanceAlarms(bool grounded) {
        if (!AlarmBank::advanceValue(alarm)) return false;
        if (animation == -1) {
            animation = -2;
            menuReady = true;
        } else if (animation > 0) { // GameMaker numeric truth: negative is false.
            animation = -1;
            alarm = 120;
            return grounded;
        }
        return false;
    }
};
} // namespace downwell
#endif

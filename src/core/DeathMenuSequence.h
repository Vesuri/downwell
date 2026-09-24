#ifndef DOWNWELL_CORE_DEATH_MENU_SEQUENCE_H
#define DOWNWELL_CORE_DEATH_MENU_SEQUENCE_H
#include "AlarmBank.h"

namespace downwell {
// DeathMenu Create, Begin Step and Alarms 0/1/2. Input arguments are press
// edges (global.dUp/dLeftPressed/dRightPressed), not held buttons. Call Begin
// Step before alarms; the original Draw phase separately drives gem filling,
// sets sequenceOver and starts unlock notices. No renderer belongs here.
struct DeathMenuSequence {
    enum class Action { None, Retry, StyleSelect, Surface };
    struct Effects {
        Action action = Action::None;
        unsigned navigationSounds = 0; // sound 324, twice for simultaneous edges
        bool acknowledgeUnlock = false;
    };
    int resultShown = -1;
    int revealDelay = 8;
    int revealAlarm = 45;
    int noticeAlarm = -1;
    int inputAlarm = -1;
    bool visible[7] = {};
    int rowOffset[7] = {4, 4, 4, 4, 4, 4, 4};
    int cursor = 0;
    bool sequenceOver = false;
    bool inputDisabled = false;
    bool goalPassed = false;
    int goalNotice = 0;
    int titleOffset = 4;
    int bodyOffset = 2;
    int cursorOffset = 0;
    int cursorBounceTenths = 0;

    Effects beginStep(bool firePressed, bool leftPressed, bool rightPressed) {
        Effects effects;
        // The source adds .4 while below 1, so it settles at 1.2, not 1.
        if (cursorBounceTenths < 10) cursorBounceTenths += 4;
        if (titleOffset > 0) --titleOffset;
        if (bodyOffset > 0) --bodyOffset;
        if (cursorOffset > 0) --cursorOffset;
        for (int i = 0; i <= resultShown; ++i) {
            visible[i] = true;
            if (rowOffset[i] > 0) --rowOffset[i];
        }
        if (sequenceOver) {
            if (rightPressed) { ++cursor; ++effects.navigationSounds; }
            if (leftPressed) { --cursor; ++effects.navigationSounds; }
            if (effects.navigationSounds) {
                cursorBounceTenths = 0;
                cursorOffset = 1;
            }
            // Create unconditionally assigns maxMenu=2 after its OS branch.
            if (cursor > 2) cursor = 0;
            if (cursor < 0) cursor = 2;
        }
        if (visible[5] && firePressed) {
            if (goalPassed && goalNotice >= 3) {
                goalPassed = false;
                goalNotice = 0;
                effects.acknowledgeUnlock = true;
            }
            if (sequenceOver && !inputDisabled) {
                effects.action = cursor == 0 ? Action::Retry :
                    cursor == 1 ? Action::StyleSelect : Action::Surface;
            }
        }
        if (firePressed && !sequenceOver) {
            resultShown = 6;
            inputDisabled = true;
            inputAlarm = 20;
        }
        return effects;
    }

    // Bit 0: result sound 325; bit 1: notice sound 327. Alarm callbacks do
    // not immediately decrement replacement values (including the final 15).
    unsigned advanceAlarms() {
        unsigned sounds = 0;
        if (AlarmBank::advanceValue(revealAlarm) && resultShown < 6) {
            ++resultShown;
            revealDelay += 2;
            revealAlarm = resultShown == 4 ? 30 : revealDelay;
            sounds |= 1;
        }
        if (AlarmBank::advanceValue(noticeAlarm)) {
            if (goalNotice == 1) {
                goalNotice = 2;
                noticeAlarm = 15;
                sounds |= 2;
            } else if (goalNotice == 2) {
                goalNotice = 3;
                noticeAlarm = 15;
            }
        }
        if (AlarmBank::advanceValue(inputAlarm)) inputDisabled = false;
        return sounds;
    }

    // Called by the recovered results/progress phase on crossing a goal.
    void startUnlockNotice() {
        goalPassed = true;
        goalNotice = 1;
        noticeAlarm = 45;
    }
};
} // namespace downwell
#endif

#ifndef DOWNWELL_CORE_LEVEL_CLEAR_SELECTION_H
#define DOWNWELL_CORE_LEVEL_CLEAR_SELECTION_H
#include "AlarmBank.h"
namespace downwell {
// Result's default Create -> alarm[4] -> scrResult next=4 selection path.
// Presentation easing, inventory grants and audio remain external consumers.
// Caller applies scrResult's wasHeartGoal branch before step when next==4.
// This is not the unused next=1..3 bonus-counting path or a complete room.
struct LevelClearSelection {
    enum class Effect { showChoices, rollOffers, moveSound, grantChoice,
        confirmSound, confirmationEffect, dither, fadeWhoosh, nextLevel };
    unsigned lastIndex = 2, cursor = 1;
    int next = 0, alarm0 = -1, alarm1 = -1, alarm3 = -1, alarm4 = 60;
    bool visible = false, inputWait = true;
    bool create(unsigned inclusiveLastIndex) {
        if (inclusiveLastIndex >= 4) return false;
        *this = LevelClearSelection{};
        lastIndex = inclusiveLastIndex;
        cursor = lastIndex / 2;
        return true;
    }
    // Dispatch immediately after each countdown. Later alarms observe writes
    // made by this event (including synchronous effects), not a frozen mask.
    // The room scheduler can interleave other objects at each alarm index.
    template<class Emit> void advanceAlarm(unsigned index, Emit emit) {
        switch (index) {
        case 0:
            if (AlarmBank::advanceValue(alarm0)) {
                emit(Effect::dither, 0);
                alarm1 = 90;
                emit(Effect::fadeWhoosh, 0);
            }
            break;
        case 1:
            if (AlarmBank::advanceValue(alarm1)) emit(Effect::nextLevel, 0);
            break;
        case 3:
            if (AlarmBank::advanceValue(alarm3)) inputWait = false;
            break;
        case 4:
            if (AlarmBank::advanceValue(alarm4) && next <= 4) next = 4;
            break;
        }
    }
    template<class Emit> void advanceAlarms(Emit emit) {
        for (unsigned index = 0; index < AlarmBank::count; ++index)
            advanceAlarm(index, emit);
    }
    template<class Emit>
    void step(bool& confirm, bool leftPressed, bool rightPressed,
              unsigned& upgradesRemaining, Emit emit) {
        if (next != 4) return;
        if (upgradesRemaining) {
            if (!visible) {
                visible = true;
                emit(Effect::showChoices, 0);
                emit(Effect::rollOffers, 0);
                inputWait = true;
                alarm3 = 30;
            }
        } else {
            next = 7; alarm0 = 30; visible = false;
        }
        if (visible) {
            // Separate source ifs: simultaneous edges move right then left.
            // The 30-tick wait gates confirm, not navigation.
            if (rightPressed && cursor < lastIndex) { ++cursor; emit(Effect::moveSound, 0); }
            if (leftPressed && cursor > 0) { --cursor; emit(Effect::moveSound, 0); }
            if (confirm && !inputWait) {
                emit(Effect::grantChoice, cursor);
                emit(Effect::confirmSound, 0);
                emit(Effect::confirmationEffect, 0);
                if (upgradesRemaining) --upgradesRemaining;
                if (upgradesRemaining) {
                    cursor = 1;
                    emit(Effect::rollOffers, 0);
                } else visible = false;
                confirm = false;
            }
        }
        // Holding fire throughout entrance must not confirm before alarm 3.
        confirm = false;
    }
};
}
#endif

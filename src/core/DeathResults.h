#ifndef DOWNWELL_CORE_DEATH_RESULTS_H
#define DOWNWELL_CORE_DEATH_RESULTS_H
#include "DeathMenuProgress.h"
#include "Input.h"
namespace downwell {
// Shared results controller. Begin/alarms precede world updates; opening can
// occur during player processing; the presentation phase follows world updates.
// Backends consume snapshots/effects, never advance state while drawing.
struct DeathResults {
    bool active = false;
    bool failed = false;
    Input previous;
    DeathMenuSequence menu;
    DeathMenuProgress progress;
    DeathMenuSequence::Effects inputEffects;
    DeathMenuProgress::Frame frame;
    unsigned alarmSounds = 0;

    void beginStep(const Input& input) {
        inputEffects = {};
        alarmSounds = 0;
        if (active) {
            inputEffects = menu.beginStep(input.fire && !previous.fire,
                input.left && !previous.left, input.right && !previous.right);
            progress.acknowledge(inputEffects);
            alarmSounds = menu.advanceAlarms();
        }
        previous = input; // Also track before creation: held fire is not a press.
    }
    void open(bool requested, std::uint64_t lifetime, std::uint32_t runGems,
              bool unlockMax = false) {
        if (!requested || active || failed) return;
        if (!progress.initialize(lifetime, runGems, unlockMax)) {
            failed = true;
            return;
        }
        active = true;
    }
    void presentationStep() {
        if (active) frame = progress.advance(menu);
    }
    template<class Digest> void appendState(Digest& digest) const {
        digest.addBool(active); digest.addBool(failed);
        digest.addBool(previous.fire); digest.addBool(previous.left); digest.addBool(previous.right);
        digest.addI32(menu.resultShown); digest.addI32(menu.revealDelay);
        digest.addI32(menu.revealAlarm); digest.addI32(menu.noticeAlarm); digest.addI32(menu.inputAlarm);
        for (unsigned i = 0; i < 7; ++i) {
            digest.addBool(menu.visible[i]); digest.addI32(menu.rowOffset[i]);
        }
        digest.addI32(menu.cursor); digest.addBool(menu.sequenceOver);
        digest.addBool(menu.inputDisabled); digest.addBool(menu.goalPassed);
        digest.addI32(menu.goalNotice); digest.addI32(menu.titleOffset);
        digest.addI32(menu.bodyOffset); digest.addI32(menu.cursorOffset);
        digest.addI32(menu.cursorBounceTenths);
        digest.addU64(progress.total); digest.addU64(progress.animatedTotal);
        digest.addU64(progress.separatorTotal); digest.addU32(progress.goal);
        digest.addU32(progress.filling); digest.addBool(progress.maxed);
        digest.addI32(static_cast<int>(inputEffects.action));
        digest.addU32(inputEffects.navigationSounds); digest.addBool(inputEffects.acknowledgeUnlock);
        digest.addU64(frame.displayedTotal); digest.addU64(frame.nextGoal);
        digest.addU64(frame.previousGoal); digest.addI32(frame.unlockedGoal);
        digest.addBool(frame.fillSound); digest.addBool(frame.completedSound);
        digest.addU32(alarmSounds);
    }
};
} // namespace downwell
#endif

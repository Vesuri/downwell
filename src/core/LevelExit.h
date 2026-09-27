#ifndef DOWNWELL_CORE_LEVEL_EXIT_H
#define DOWNWELL_CORE_LEVEL_EXIT_H
#include "AlarmBank.h"
namespace downwell {
// objLineToResult inherits objLineToMain Create. Child creation callbacks
// must execute synchronously (their Create events may consume shared RNG).
struct LevelExit {
    enum class Effect { roomEndSetter, goalTimeField, ditherTransition, resultRoom };
    struct NoChildPublication {
        bool operator()(Effect, int, int) const { return true; }
    };
    int x = 0, y = 0;
    int alarm0 = -1, alarm1 = -1;
    bool active = false;
    // sprTouchWide (127): 800x32, origin (400,16), rectangular mask.
    // Player idle mask: x -4..+3, y -4..+7. Coordinates are already
    // rounded instance coordinates, as in the existing collision adapters.
    bool meetsPlayer(int px, int py) const {
        return px + 3 >= x - 400 && px - 4 <= x + 399 &&
            py + 7 >= y - 16 && py - 4 <= y + 15;
    }
    template<class Emit>
    void create(int roomWidth, int positionY, Emit emit) {
        *this = LevelExit{};
        x = roomWidth / 2; y = positionY;
        emit(Effect::roomEndSetter, x, y);
        emit(Effect::goalTimeField, x, y + 32);
    }
    bool collide(bool dead, bool& goalStop, bool& noControl) {
        if (dead || active) return false;
        alarm0 = 5;
        goalStop = noControl = active = true;
        return true;
    }
    // Call in the alarm phase, before that frame's collisions. Newly armed
    // alarms are not decremented again during the same dispatch.
    template<class Emit>
    void advanceAlarms(Emit emit) {
        const bool first = AlarmBank::advanceValue(alarm0);
        const bool second = AlarmBank::advanceValue(alarm1);
        if (first) {
            emit(Effect::ditherTransition, 0, 0);
            alarm1 = 90;
        }
        if (second) emit(Effect::resultRoom, 0, 0);
    }
};
}
#endif

#ifndef DOWNWELL_CORE_EXIT_DITHER_H
#define DOWNWELL_CORE_EXIT_DITHER_H
#include "AlarmBank.h"
namespace downwell {
// objTransDither: anchored once in world space, then moving eight pixels
// per ordinary Step. Its sprite is 128, with origin (160,720).
struct ExitDither {
    bool active = false;
    int x = 0, y = 0, alarm0 = -1;
    bool soundRequested = false; // soundPlayOL(187,80,0,1,"UI")
    void create(int viewX, int viewY) {
        active = true; x = viewX + 80; y = viewY - 100;
        alarm0 = 15; soundRequested = false;
    }
    void advanceAlarm() {
        if (active && AlarmBank::advanceValue(alarm0)) soundRequested = true;
    }
    void step(int viewY, int viewHeight) {
        if (active && y < viewY + viewHeight) y += 8;
    }
};
}
#endif

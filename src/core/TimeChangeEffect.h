#ifndef DOWNWELL_CORE_TIME_CHANGE_EFFECT_H
#define DOWNWELL_CORE_TIME_CHANGE_EFFECT_H
#include "AlarmBank.h"
namespace downwell {
// Object 211 has no sprite or Step event. Its flash starts false and its only
// flash assignment also clears it, so ordinary instances draw no rectangle.
struct TimeChangeEffect {
    int alarm0=-1,alarm1=-1;
    bool flash=false,destroyRequested=false;
    template<class Pause> bool create(int& roomSpeed,Pause pause){
        flash=false;destroyRequested=false;alarm0=alarm1=2;
        roomSpeed=30;
        // Source sleep(30) busy-waits on get_timer microseconds. The backend
        // implements the synchronous delay; shared logic must not busy-wait.
        return pause(30);
    }
    // Called in the global alarm phase, in source alarm-index order. A new
    // Begin-Step birth may participate in this phase; a Step birth may not.
    void advanceAlarms(int& roomSpeed){
        if(destroyRequested)return;
        if(AlarmBank::advanceValue(alarm0))flash=false;
        if(!AlarmBank::advanceValue(alarm1))return;
        if(roomSpeed<60){
            roomSpeed+=5;
            alarm1=2;
        }else{
            roomSpeed=60;
            destroyRequested=true;
        }
    }
};
}
#endif

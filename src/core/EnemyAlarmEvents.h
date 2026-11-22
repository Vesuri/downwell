#ifndef DOWNWELL_CORE_ENEMY_ALARM_EVENTS_H
#define DOWNWELL_CORE_ENEMY_ALARM_EVENTS_H

namespace downwell {
// Recovered event-presence masks for the supported area-one objects. Checked
// against ObjectPack::findEvent (including inherited events) by the scene test.
// Presence is not a claim that every event body has been implemented.
inline unsigned enemyAlarmEvents(int objectId) {
    switch (objectId) {
        case 112: return 2; // ground turtle: Alarm 1
        case 113: return 7; // frog: Alarms 0, 1, 2
        case 114: return 3; // eyefish: Alarms 0, 1 (never armed)
        case 115: return 3; // white jelly: Alarms 0, 1
        case 116: return 1; // bat: Alarm 0
        case 117: return 4; // snail: Alarm 2 only
        case 118: case 173: return 3; // red balls: Alarms 0, 1
        case 119: return 3; // crawler: Alarms 0, 1
        default: return 0;
    }
}
}
#endif

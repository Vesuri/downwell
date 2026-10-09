#ifndef DOWNWELL_CORE_MOMENT_DELAY_H
#define DOWNWELL_CORE_MOMENT_DELAY_H
namespace downwell {
// momentDelay tests room_speed once, outside the WITH. Enumeration belongs to
// the source object-membership owner, not Draw order or a singleton guess.
struct MomentDelay {
    template<class Visit> static bool request(int& roomSpeed,Visit controllers){
        if(roomSpeed!=60)return true;
        return controllers([&](auto& controller){
            roomSpeed=30;
            controller.controllerSlow=30;
            controller.alarms.set(0,1);
            return true;
        });
    }
    template<class Controller> static void alarm0(int& roomSpeed,Controller& controller){
        roomSpeed=60;controller.controllerSlow=0;
    }
};
}
#endif

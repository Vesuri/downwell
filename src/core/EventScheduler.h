#ifndef DOWNWELL_CORE_EVENT_SCHEDULER_H
#define DOWNWELL_CORE_EVENT_SCHEDULER_H

#include "InstanceStore.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace downwell {

enum class StepEventPhase {
    beginStep,
    alarm,
    step,
    endStep
};

struct StepEvent {
    StepEventPhase phase;
    std::uint8_t subtype = 0;
};

// Executes the engine-defined phase order without heap allocation. The caller
// supplies which alarm events each object implements and dispatches handlers.
// Lifetimes are snapshotted per phase so deletion is safe and newly created instances
// can participate in later phases of the same tick.
// Each snapshot uses 8 bytes per capacity slot; lookup is O(1). Keep the
// snapshot's stack cost explicit when selecting a target capacity.
template <std::size_t Capacity>
class EventScheduler {
public:
    template <typename AlarmEvents, typename Handler>
    static void runStep(InstanceStore<Capacity>& instances,
                        AlarmEvents alarmEvents,
                        Handler handler) {
        runStep(instances,alarmEvents,handler,[](StepEventPhase){});
    }
    // External fixed-capacity owners snapshot at the same global boundary,
    // including empty instance lists and births made by preceding alarms.
    template <typename AlarmEvents, typename Handler, typename BeforePhase>
    static void runStep(InstanceStore<Capacity>& instances,
                        AlarmEvents alarmEvents, Handler handler, BeforePhase beforePhase) {
        runStep(instances,alarmEvents,handler,beforePhase,
            [](InstanceHandle,const InstanceRecord&,bool){return true;});
    }
    // External collision membership follows source room reconciliation. A
    // failed publication must stop the tick before any later phase executes.
    template <typename AlarmEvents, typename Handler, typename BeforePhase, typename Publish>
    static bool runStep(InstanceStore<Capacity>& instances,
                         AlarmEvents alarmEvents, Handler handler,
                         BeforePhase beforePhase, Publish publish) {
        return runStepChecked(instances,alarmEvents,handler,beforePhase,publish,[]{return true;});
    }
    // Healthy observes the owner's latched state, including callback failures.
    // Never mutate another alarm or reconcile a partly failed phase. Completed
    // handler side effects remain committed; recovery belongs to the owner.
    template<class AlarmEvents,class Handler,class BeforePhase,class Publish,class Healthy>
    static bool runStepChecked(InstanceStore<Capacity>& instances,
        AlarmEvents alarmEvents,Handler handler,BeforePhase beforePhase,
        Publish publish,Healthy healthy) {
        return runStepChecked(instances,alarmEvents,handler,beforePhase,publish,healthy,[]{return true;});
    }
    // The normal runner has a separate collision pass before End Step. Pair
    // selection/order and both event directions belong to the callback, not
    // to an approximate storage-order overlap scan in this scheduler.
    template<class AlarmEvents,class Handler,class BeforePhase,class Publish,class Healthy,class Collisions>
    static bool runStepChecked(InstanceStore<Capacity>& instances,
        AlarmEvents alarmEvents,Handler handler,BeforePhase beforePhase,
        Publish publish,Healthy healthy,Collisions collisions) {
        if(!healthy())return false;
        const auto checkedPublish=[&](InstanceHandle h,const InstanceRecord& record,bool member){
            return healthy()&&publish(h,record,member)&&healthy();
        };
        beforePhase(StepEventPhase::beginStep);
        if(!healthy() || !dispatchPhase(instances, StepEventPhase::beginStep, handler,healthy))return false;
        if(!instances.reconcileObjectMembership(checkedPublish)||!healthy())return false;
        beforePhase(StepEventPhase::alarm);
        if(!healthy() || !dispatchAlarms(instances, alarmEvents, handler,healthy))return false;
        if(!instances.reconcileObjectMembership(checkedPublish)||!healthy())return false;
        beforePhase(StepEventPhase::step);
        if(!healthy() || !dispatchPhase(instances, StepEventPhase::step, handler,healthy))return false;
        if(!instances.reconcileObjectMembership(checkedPublish)||!healthy())return false;
        if(!collisions()||!healthy())return false;
        if(!instances.reconcileObjectMembership(checkedPublish)||!healthy())return false;
        beforePhase(StepEventPhase::endStep);
        if(!healthy() || !dispatchPhase(instances, StepEventPhase::endStep, handler,healthy))return false;
        return instances.reconcileObjectMembership(checkedPublish)&&healthy();
    }

private:
    static std::size_t captureActive(const InstanceStore<Capacity>& instances,
                                     std::array<InstanceHandle, Capacity>& handles) {
        std::size_t count = 0;
        for (std::size_t index = 0; index < instances.size(); ++index) {
            const InstanceRecord& instance = instances.at(index);
            if (instance.active && !instance.destroyRequested)
                handles[count++] = instances.handleAt(index);
        }
        return count;
    }

    template <typename Handler,typename Healthy>
    static bool dispatchPhase(InstanceStore<Capacity>& instances,
                              StepEventPhase phase,
                              Handler& handler,Healthy& healthy) {
        std::array<InstanceHandle, Capacity> handles{};
        const std::size_t count = captureActive(instances, handles);
        for (std::size_t index = 0; index < count; ++index) {
            InstanceRecord* instance = instances.resolve(handles[index]);
            if (instance != nullptr && instance->active && !instance->destroyRequested) {
                handler(*instance, StepEvent{phase, 0});
                if(!healthy())return false;
            }
        }
        return true;
    }

    template <typename AlarmEvents, typename Handler,typename Healthy>
    static bool dispatchAlarms(InstanceStore<Capacity>& instances,
                               AlarmEvents& alarmEvents,
                               Handler& handler,Healthy& healthy) {
        std::array<InstanceHandle, Capacity> handles{};
        const std::size_t count = captureActive(instances, handles);
        // Original runner 0x509d97: alarm index is the outer loop and each
        // zero is dispatched immediately. Later alarms observe handler writes.
        for (std::size_t alarm = 0; alarm < AlarmBank::count; ++alarm) {
            for (std::size_t index = 0; index < count; ++index) {
                InstanceRecord* instance = instances.resolve(handles[index]);
                if (instance == nullptr || !instance->active || instance->destroyRequested)
                    continue;
                const bool hasEvent = (alarmEvents(*instance) & (1u << alarm)) != 0;
                if(!healthy())return false;
                if (!instance->alarms.advanceOne(alarm, hasEvent)) continue;
                handler(*instance,
                        StepEvent{StepEventPhase::alarm,
                                  static_cast<std::uint8_t>(alarm)});
                if(!healthy())return false;
            }
        }
        return true;
    }
};

} // namespace downwell

#endif

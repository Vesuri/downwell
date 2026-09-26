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
        beforePhase(StepEventPhase::beginStep);
        dispatchPhase(instances, StepEventPhase::beginStep, handler);
        beforePhase(StepEventPhase::alarm);
        dispatchAlarms(instances, alarmEvents, handler);
        beforePhase(StepEventPhase::step);
        dispatchPhase(instances, StepEventPhase::step, handler);
        beforePhase(StepEventPhase::endStep);
        dispatchPhase(instances, StepEventPhase::endStep, handler);
    }

private:
    static std::size_t captureActive(const InstanceStore<Capacity>& instances,
                                     std::array<InstanceHandle, Capacity>& handles) {
        std::size_t count = 0;
        for (std::size_t index = 0; index < instances.size(); ++index) {
            const InstanceRecord& instance = instances.at(index);
            if (instance.active)
                handles[count++] = instances.handleAt(index);
        }
        return count;
    }

    template <typename Handler>
    static void dispatchPhase(InstanceStore<Capacity>& instances,
                              StepEventPhase phase,
                              Handler& handler) {
        std::array<InstanceHandle, Capacity> handles{};
        const std::size_t count = captureActive(instances, handles);
        for (std::size_t index = 0; index < count; ++index) {
            InstanceRecord* instance = instances.resolve(handles[index]);
            if (instance != nullptr && instance->active)
                handler(*instance, StepEvent{phase, 0});
        }
    }

    template <typename AlarmEvents, typename Handler>
    static void dispatchAlarms(InstanceStore<Capacity>& instances,
                               AlarmEvents& alarmEvents,
                               Handler& handler) {
        std::array<InstanceHandle, Capacity> handles{};
        const std::size_t count = captureActive(instances, handles);
        // Original runner 0x509d97: alarm index is the outer loop and each
        // zero is dispatched immediately. Later alarms observe handler writes.
        for (std::size_t alarm = 0; alarm < AlarmBank::count; ++alarm) {
            for (std::size_t index = 0; index < count; ++index) {
                InstanceRecord* instance = instances.resolve(handles[index]);
                if (instance == nullptr || !instance->active)
                    continue;
                const bool hasEvent = (alarmEvents(*instance) & (1u << alarm)) != 0;
                if (!instance->alarms.advanceOne(alarm, hasEvent)) continue;
                handler(*instance,
                        StepEvent{StepEventPhase::alarm,
                                  static_cast<std::uint8_t>(alarm)});
            }
        }
    }
};

} // namespace downwell

#endif

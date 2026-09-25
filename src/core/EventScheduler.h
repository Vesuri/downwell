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
// IDs are snapshotted per phase so deletion is safe and newly created instances
// can participate in later phases of the same tick.
template <std::size_t Capacity>
class EventScheduler {
public:
    template <typename AlarmEvents, typename Handler>
    static void runStep(InstanceStore<Capacity>& instances,
                        AlarmEvents alarmEvents,
                        Handler handler) {
        dispatchPhase(instances, StepEventPhase::beginStep, handler);
        dispatchAlarms(instances, alarmEvents, handler);
        dispatchPhase(instances, StepEventPhase::step, handler);
        dispatchPhase(instances, StepEventPhase::endStep, handler);
    }

private:
    static std::size_t captureActive(const InstanceStore<Capacity>& instances,
                                     std::array<std::uint32_t, Capacity>& ids) {
        std::size_t count = 0;
        for (std::size_t index = 0; index < instances.size(); ++index) {
            const InstanceRecord& instance = instances.at(index);
            if (instance.active)
                ids[count++] = instance.instanceId;
        }
        return count;
    }

    template <typename Handler>
    static void dispatchPhase(InstanceStore<Capacity>& instances,
                              StepEventPhase phase,
                              Handler& handler) {
        std::array<std::uint32_t, Capacity> ids{};
        const std::size_t count = captureActive(instances, ids);
        for (std::size_t index = 0; index < count; ++index) {
            InstanceRecord* instance = instances.find(ids[index]);
            if (instance != nullptr && instance->active)
                handler(*instance, StepEvent{phase, 0});
        }
    }

    template <typename AlarmEvents, typename Handler>
    static void dispatchAlarms(InstanceStore<Capacity>& instances,
                               AlarmEvents& alarmEvents,
                               Handler& handler) {
        std::array<std::uint32_t, Capacity> ids{};
        const std::size_t count = captureActive(instances, ids);
        // Original runner 0x509d97: alarm index is the outer loop and each
        // zero is dispatched immediately. Later alarms observe handler writes.
        for (std::size_t alarm = 0; alarm < AlarmBank::count; ++alarm) {
            for (std::size_t index = 0; index < count; ++index) {
                InstanceRecord* instance = instances.find(ids[index]);
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

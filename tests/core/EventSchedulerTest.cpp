#include "core/EventScheduler.h"
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

struct TraceEntry {
    std::uint32_t instanceId;
    downwell::StepEventPhase phase;
    std::uint8_t subtype;
};

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testPhaseAndAlarmOrder() {
    downwell::InstanceStore<3> instances;
    instances.create(100, 10, 7, false);
    instances.create(200, 20, 7, false);
    instances.find(100)->alarms.set(3, 1);
    instances.find(200)->alarms.set(1, 1);

    std::vector<TraceEntry> trace;
    const auto alarmEvents = [](const downwell::InstanceRecord&) {
        return downwell::AlarmBank::allEvents;
    };
    const auto handler = [&trace](downwell::InstanceRecord& instance,
                                  downwell::StepEvent event) {
        trace.push_back({instance.instanceId, event.phase, event.subtype});
    };
    downwell::EventScheduler<3>::runStep(instances, alarmEvents, handler);

    require(trace.size() == 8, "two instances receive four event phases");
    require(trace[0].phase == downwell::StepEventPhase::beginStep &&
                trace[0].instanceId == 100 && trace[1].instanceId == 200,
            "Begin Step dispatches first in stable order");
    require(trace[2].phase == downwell::StepEventPhase::alarm &&
                trace[2].instanceId == 100 && trace[2].subtype == 3,
            "first alarm dispatches after all Begin Step events");
    require(trace[3].phase == downwell::StepEventPhase::alarm &&
                trace[3].instanceId == 200 && trace[3].subtype == 1,
            "second instance alarm follows in stable order");
    require(trace[4].phase == downwell::StepEventPhase::step &&
                trace[5].phase == downwell::StepEventPhase::step,
            "Step follows alarms");
    require(trace[6].phase == downwell::StepEventPhase::endStep &&
                trace[7].phase == downwell::StepEventPhase::endStep,
            "End Step is last");
}

void testActiveStateAndMissingAlarmEvent() {
    downwell::InstanceStore<2> instances;
    instances.create(1, 10, 7, false);
    instances.create(2, 20, 7, false);
    instances.find(1)->active = false;
    instances.find(2)->alarms.set(5, 4);

    int calls = 0;
    const auto noAlarmEvents = [](const downwell::InstanceRecord&) {
        return static_cast<downwell::AlarmBank::FiredMask>(0);
    };
    const auto handler = [&calls](downwell::InstanceRecord&,
                                  downwell::StepEvent) { ++calls; };
    downwell::EventScheduler<2>::runStep(instances, noAlarmEvents, handler);

    require(calls == 3, "only active instance receives step phases");
    require(instances.find(2)->alarms.get(5) == 4,
            "alarm without an implemented event does not count down");
}

void testDeletionAndCreationAcrossPhases() {
    downwell::InstanceStore<3> instances;
    instances.create(1, 10, 7, false);
    instances.create(2, 20, 7, false);

    std::vector<TraceEntry> trace;
    const auto noAlarmEvents = [](const downwell::InstanceRecord&) {
        return static_cast<downwell::AlarmBank::FiredMask>(0);
    };
    const auto handler = [&instances, &trace](downwell::InstanceRecord& instance,
                                              downwell::StepEvent event) {
        trace.push_back({instance.instanceId, event.phase, event.subtype});
        if (event.phase == downwell::StepEventPhase::beginStep &&
            instance.instanceId == 1) {
            instances.erase(2);
            instances.create(3, 30, 7, false);
        }
    };
    downwell::EventScheduler<3>::runStep(instances, noAlarmEvents, handler);

    require(trace.size() == 5, "deleted instance is skipped and new instance joins later phases");
    require(trace[0].instanceId == 1 && trace[0].phase == downwell::StepEventPhase::beginStep,
            "original instance begins the tick");
    require(trace[1].instanceId == 1 && trace[1].phase == downwell::StepEventPhase::step,
            "survivor reaches Step");
    require(trace[2].instanceId == 3 && trace[2].phase == downwell::StepEventPhase::step,
            "new instance joins at the next phase");
}

} // namespace

int main() {
    testPhaseAndAlarmOrder();
    testActiveStateAndMissingAlarmEvent();
    testDeletionAndCreationAcrossPhases();
    std::cout << "event scheduler tests passed\n";
    return 0;
}

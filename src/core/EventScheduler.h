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
// Each ordered snapshot uses 16 bytes per capacity slot; lookup is O(1). Keep the
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
        return runStepChecked(instances,alarmEvents,handler,beforePhase,publish,healthy,collisions,
            []{return true;});
    }
    // Selected runner 0x5108d0: reconcile activation before servicing pending
    // layer depths. This boundary must also run after an empty event phase.
    template<class AlarmEvents,class Handler,class BeforePhase,class Publish,class Healthy,class Collisions,class Maintain>
    static bool runStepChecked(InstanceStore<Capacity>& instances,
        AlarmEvents alarmEvents,Handler handler,BeforePhase beforePhase,
        Publish publish,Healthy healthy,Collisions collisions,Maintain maintain) {
        return runStepChecked(instances,alarmEvents,handler,beforePhase,publish,healthy,collisions,maintain,
            []{return true;});
    }
    // Transitional owners may still have retained Step bodies outside the
    // registered population. Complete those before Step reconciliation and
    // Collision, never after End Step. This does not establish their mutual
    // object/instance order; they must ultimately join selected dispatch.
    template<class AlarmEvents,class Handler,class BeforePhase,class Publish,class Healthy,class Collisions,class Maintain,class CompleteStep>
    static bool runStepChecked(InstanceStore<Capacity>& instances,
        AlarmEvents alarmEvents,Handler handler,BeforePhase beforePhase,
        Publish publish,Healthy healthy,Collisions collisions,Maintain maintain,CompleteStep completeStep) {
        if(!healthy())return false;
        const auto checkedPublish=[&](InstanceHandle h,const InstanceRecord& record,bool member){
            return healthy()&&publish(h,record,member)&&healthy();
        };
        const auto maintenance=[&]{
            return instances.reconcileObjectMembership(checkedPublish)&&healthy()&&maintain()&&healthy();
        };
        beforePhase(StepEventPhase::beginStep);
        if(!healthy() || !dispatchPhase(instances, StepEventPhase::beginStep, handler,healthy))return false;
        if(!maintenance())return false;
        beforePhase(StepEventPhase::alarm);
        if(!healthy() || !dispatchAlarms(instances, alarmEvents, handler,healthy))return false;
        if(!maintenance())return false;
        beforePhase(StepEventPhase::step);
        if(!healthy() || !dispatchPhase(instances, StepEventPhase::step, handler,healthy))return false;
        if(!completeStep()||!healthy())return false;
        if(!maintenance())return false;
        if(!collisions()||!healthy())return false;
        if(!maintenance())return false;
        beforePhase(StepEventPhase::endStep);
        if(!healthy() || !dispatchPhase(instances, StepEventPhase::endStep, handler,healthy))return false;
        return maintenance();
    }

    // One captured room population, with alarm index outside source object /
    // registration order. Typed event adapters may own their alarm storage;
    // visit performs countdown and immediate dispatch, not a later fire mask.
    template<class Visit,class Healthy>
    static bool visitAlarmPhaseChecked(InstanceStore<Capacity>& instances,Visit visit,Healthy healthy) {
        if(!healthy())return false;
        RunnerStepSnapshot<Capacity> handles;
        if(!instances.captureStepOrder(handles))return false;
        for(unsigned alarm=0;alarm<AlarmBank::count;++alarm)
            for(std::size_t index=0;index<handles.size();++index){
                auto* instance=instances.resolve(handles.handle(index));
                if(!instance||!instance->active||instance->destroyRequested)continue;
                if(!visit(*instance,alarm)||!healthy())return false;
            }
        return true;
    }

private:
    template <typename Handler,typename Healthy>
    static bool dispatchPhase(InstanceStore<Capacity>& instances,
                              StepEventPhase phase,
                              Handler& handler,Healthy& healthy) {
        RunnerStepSnapshot<Capacity> handles;
        if(!instances.captureStepOrder(handles))return false;
        const std::size_t count = handles.size();
        for (std::size_t index = 0; index < count; ++index) {
            InstanceRecord* instance = instances.resolve(handles.handle(index));
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
        return visitAlarmPhaseChecked(instances,[&](InstanceRecord& instance,unsigned alarm){
            const bool hasEvent=(alarmEvents(instance)&(1u<<alarm))!=0;
            if(!healthy())return false;
            if(instance.alarms.advanceOne(alarm,hasEvent))
                handler(instance,StepEvent{StepEventPhase::alarm,static_cast<std::uint8_t>(alarm)});
            return healthy();
        },healthy);
    }
};

} // namespace downwell

#endif

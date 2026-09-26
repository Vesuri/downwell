#include "core/EventScheduler.h"
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

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
                trace[2].instanceId == 200 && trace[2].subtype == 1,
            "first alarm dispatches after all Begin Step events");
    require(trace[3].phase == downwell::StepEventPhase::alarm &&
                trace[3].instanceId == 100 && trace[3].subtype == 3,
            "higher alarm index follows across instances");
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

void testCrossAlarmWrites() {
    downwell::InstanceStore<2> instances;
    instances.create(1, 10, 7, false);
    instances.create(2, 20, 7, false);
    instances.find(2)->alarms.set(2, 1);
    instances.find(1)->alarms.set(3, 1);
    unsigned fired = 0;
    downwell::EventScheduler<2>::runStep(instances,
        [](const downwell::InstanceRecord&) { return downwell::AlarmBank::allEvents; },
        [&](downwell::InstanceRecord& instance, downwell::StepEvent event) {
            if (event.phase != downwell::StepEventPhase::alarm) return;
            ++fired;
            require(instance.instanceId == 2 && event.subtype == 2, "old higher alarm must not fire");
            instances.find(1)->alarms.set(3, 15);
            instances.find(1)->alarms.set(0, 5);
        });
    require(fired == 1 && instances.find(1)->alarms.get(3) == 14,
            "later slot on earlier instance sees write and decrements immediately");
    require(instances.find(1)->alarms.get(0) == 5, "visited slot waits for next phase");
}

void testDeletionDuringAlarm() {
    downwell::InstanceStore<2> instances;
    instances.create(1, 10, 7, false);
    instances.create(2, 20, 7, false);
    instances.find(1)->alarms.set(0, 1);
    instances.find(1)->alarms.set(1, 1);
    instances.find(2)->alarms.set(1, 1);
    unsigned alarms = 0;
    downwell::EventScheduler<2>::runStep(instances,
        [](const downwell::InstanceRecord&) { return downwell::AlarmBank::allEvents; },
        [&](downwell::InstanceRecord& instance, downwell::StepEvent event) {
            if (event.phase != downwell::StepEventPhase::alarm) return;
            ++alarms;
            if (instance.instanceId == 1) {
                require(event.subtype == 0, "deleted instance cannot receive a later alarm");
                instances.erase(1);
            }
        });
    require(alarms == 2 && instances.find(1) == nullptr,
            "alarm deletion preserves survivor dispatch after storage compaction");
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

void testReusedIdCannotInheritPhaseEligibility() {
    using Phase = downwell::StepEventPhase;
    for (auto replacePhase : {Phase::beginStep, Phase::alarm, Phase::step, Phase::endStep}) {
        downwell::InstanceStore<3> instances;
        instances.create(1, 10, 7, false);
        instances.create(2, 20, 7, false);
        instances.create(3, 30, 7, false);
        for (unsigned i = 0; i < 3; ++i) {
            instances.at(i).alarms.set(0, 1);
            instances.at(i).alarms.set(1, 1);
        }
        unsigned replaced = 0, survivorEvents = 0, laterBirthEvents = 0;
        downwell::EventScheduler<3>::runStep(instances,
            [](const downwell::InstanceRecord&) { return downwell::AlarmBank::allEvents; },
            [&](downwell::InstanceRecord& instance, downwell::StepEvent event) {
                if (instance.objectId == 40) {
                    require(event.phase != replacePhase,
                        "new lifetime cannot inherit old ID eligibility within any phase");
                    ++laterBirthEvents;
                }
                if (instance.objectId == 30 && event.phase == replacePhase) ++survivorEvents;
                if (instance.instanceId != 1 || event.phase != replacePhase || replaced) return;
                ++replaced;
                require(instances.erase(2), "retire not-yet-visited lifetime");
                require(instances.create(2, 40, 7, false) == downwell::CreateInstanceResult::success,
                    "reuse numeric ID and freed slot after compaction");
                instances.find(2)->alarms.set(1, 1);
            });
        require(replaced == 1 && survivorEvents == (replacePhase == Phase::alarm ? 2u : 1u),
            "compaction preserves surviving lifetime dispatch");
        require((laterBirthEvents != 0) == (replacePhase != Phase::endStep),
            "replacement joins subsequent phase snapshots only");
    }
}

void testRoomMembershipCycles() {
    constexpr unsigned capacity=16;
    downwell::RunnerRoomMembership<capacity> rooms;
    std::vector<unsigned> active,inactive;
    unsigned depths[capacity]{};
    bool dormant[capacity]{};
    std::uint32_t rng=17;
    auto next=[&](){rng=rng*1664525u+1013904223u;return rng;};
    auto remove=[](auto& list,unsigned slot){
        list.erase(std::remove(list.begin(),list.end(),slot),list.end());
    };
    auto insert=[&](unsigned slot){
        auto pos=active.begin();
        while(pos!=active.end() && depths[*pos]<=depths[slot])++pos;
        active.insert(pos,slot);
    };
    for(unsigned iteration=0;iteration<4096;++iteration){
        unsigned slot=(next()>>16)%capacity,op=(next()>>16)%5;
        if(op==0){
            remove(active,slot);remove(inactive,slot);
            depths[slot]=(next()>>16)%4; // Frequent equal-depth insertion ties.
            downwell::RunnerImageValue key;
            require(key.assign(downwell::MotionBinary64::fromInteger(depths[slot])),"depth is representable");
            require(rooms.bind(slot,key),"bind room member");insert(slot);
        }else if(op==1){
            rooms.remove(slot);remove(active,slot);remove(inactive,slot);
        }else dormant[slot]=!dormant[slot];
        // Several pending requests between publications exercise cancellation.
        if(iteration%7)continue;
        std::vector<unsigned> expected,actual;
        auto oldInactive=inactive;
        for(auto member:oldInactive)if(!dormant[member]){
            remove(inactive,member);insert(member);expected.push_back(member*2+1);
        }
        auto oldActive=active;
        for(auto member:oldActive)if(dormant[member]){
            remove(active,member);inactive.push_back(member);expected.push_back(member*2);
        }
        // Copied owners must retain links without pointers into the old owner.
        auto copied=rooms;
        copied.reconcile([&](unsigned member){return dormant[member];},
            [&](unsigned member,bool registered){actual.push_back(member*2+registered);});
        require(actual==expected,"room publication matches independent ordered lists");
        rooms=copied;
        if(iteration%91==0){rooms.clear();active.clear();inactive.clear();}
        for(unsigned member=0;member<capacity;++member)
            require(rooms.contains(member)==(std::find(active.begin(),active.end(),member)!=active.end() ||
                std::find(inactive.begin(),inactive.end(),member)!=inactive.end()),"membership survives reuse and clear");
    }
}

void testDeferredMembershipPhaseBoundary() {
    using Phase=downwell::StepEventPhase;
    downwell::InstanceStore<3> store;
    store.create(1,10,7,false,20);store.create(2,10,7,false,0);store.create(3,10,7,false,10);
    auto exact=[](int,int){return false;};
    for(unsigned i=0;i<3;++i){store.at(i).active=false;store.at(i).activationPending=true;}
    store.reconcileObjectMembership();
    require(store.registeredObjectCount(10,exact)==0,"deactivation reconciliation unbinds all dormant members");
    // Request order deliberately differs from both storage and inactive order.
    for(unsigned id:{1u,3u,2u}){store.find(id)->active=true;store.find(id)->activationPending=false;}
    unsigned beginCalls=0,publications=0;
    downwell::EventScheduler<3>::runStep(store,
        [](const downwell::InstanceRecord&){return downwell::AlarmBank::FiredMask(0);},
        [&](downwell::InstanceRecord&,downwell::StepEvent event){
            if(event.phase==Phase::beginStep){
                ++beginCalls;
                require(store.registeredObjectCount(10,exact)==0,
                    "membership stays unpublished through every Begin Step handler");
            }
        },
        [&](Phase phase){
            if(phase==Phase::beginStep)return;
            std::vector<unsigned> order;
            store.visitObject(10,exact,[&](downwell::InstanceHandle,const downwell::InstanceRecord& record){
                order.push_back(record.instanceId);return true;});
            require(order==std::vector<unsigned>({2,3,1}),
                "post-phase promotion follows inactive room-list order, not request or storage order");
            ++publications;
        });
    require(beginCalls==3 && publications==3,"publication precedes every later phase snapshot");
    store.find(3)->active=false; // Retirement, not pending activation.
    store.reconcileObjectMembership();
    require(store.registeredObjectCount(10,exact)==2,"retired diagnostic record loses membership");
}

} // namespace

int main() {
    testPhaseAndAlarmOrder();
    testCrossAlarmWrites();
    testDeletionDuringAlarm();
    testActiveStateAndMissingAlarmEvent();
    testDeletionAndCreationAcrossPhases();
    testReusedIdCannotInheritPhaseEligibility();
    testDeferredMembershipPhaseBoundary();
    testRoomMembershipCycles();
    std::cout << "event scheduler tests passed\n";
    return 0;
}

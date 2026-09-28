#include "core/EventScheduler.h"
#include "core/StoredLayerLifecycle.h"
#include "core/InstanceLifecycle.h"
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

void testObjectRegistrationOrder() {
    using namespace downwell;
    InstanceStore<8> instances;
    instances.create(1,60,7,false);
    instances.create(2,40,7,false);
    instances.create(3,60,7,false);
    instances.create(4,99,7,false);
    const auto first=instances.handleAt(0);
    // Same-object rebinding moves to the registration tail, not the storage tail.
    require(instances.bindObject(instances.handleAt(0),60),"same-object rebind");
    std::vector<unsigned> begin,step,end;
    EventScheduler<8>::runStep(instances,[](const auto&){return AlarmBank::allEvents;},
        [&](auto& r,StepEvent event){
            if(event.phase==StepEventPhase::beginStep){
                begin.push_back(r.instanceId);
                if(r.instanceId==2)instances.create(5,10,7,false);
            }
            if(event.phase==StepEventPhase::step){
                step.push_back(r.instanceId);
                if(r.instanceId==5){
                    instances.erase(3);instances.create(3,60,7,false);
                }
            }
            if(event.phase==StepEventPhase::endStep)end.push_back(r.instanceId);
        });
    require(begin==std::vector<unsigned>({2,3,1,4}),"object IDs ascending, exact-object registration order stable");
    require(step==std::vector<unsigned>({5,2,1,4}),"earlier-phase birth participates; replaced lifetime does not inherit eligibility");
    require(end==std::vector<unsigned>({5,2,1,3,4}),"replacement enters next phase at exact-object registration tail");
    require(instances.requestActivation(first,false,[](auto,const auto&,bool){return true;}) &&
            instances.reconcileObjectMembership([](auto,const auto&,bool){return true;}) &&
            instances.requestActivation(first,true,[](auto,const auto&,bool){return true;}) &&
            instances.reconcileObjectMembership([](auto,const auto&,bool){return true;}),"reactivation reconciles");
    RunnerStepSnapshot<8> snapshot;
    require(instances.captureStepOrder(snapshot),"ordered capture");
    std::vector<unsigned> ids;
    for(unsigned i=0;i<snapshot.size();++i)ids.push_back(instances.resolve(snapshot.handle(i))->instanceId);
    require(ids==std::vector<unsigned>({5,2,3,1,4}),"reactivation appends within exact object, not global dispatch order");
}

void testOrderedSnapshotSort() {
    downwell::RunnerStepSnapshot<257> snapshot;
    unsigned random=19;
    for(unsigned count=0;count<=257;++count){
        snapshot.clear();
        std::vector<std::pair<int,unsigned>> reference;
        for(unsigned i=0;i<count;++i){
            random=random*1664525u+1013904223u;
            const int object=int((random>>16)%43);
            require(snapshot.append(i,i+1,object),"snapshot append");
            reference.push_back({object,i});
        }
        std::stable_sort(reference.begin(),reference.end(),[](auto a,auto b){return a.first<b.first;});
        snapshot.sort();
        require(snapshot.objectGroupEnd(count)==count,"group end rejects past-end index");
        for(unsigned i=0;i<count;++i){
            unsigned end=i+1;
            while(end<count&&reference[end].first==reference[i].first)++end;
            require(snapshot.objectGroupEnd(i)==end,"binary group boundary matches scalar walk");
        }
        for(unsigned i=0;i<count;++i)
            require(snapshot.handle(i).slot==reference[i].second &&
                    snapshot.handle(i).generation==reference[i].second+1,"bounded heap order matches independent stable sort");
        for(unsigned repeat=0;repeat<3;++repeat){
            if(repeat==0)snapshot.sortDenseObjects<43>();
            else if(repeat==1)snapshot.sortDenseObjects<1>(); // Out-of-range fallback.
            else snapshot.sort();
            for(unsigned i=0;i<count;++i)
                require(snapshot.object(i)==reference[i].first&&snapshot.handle(i).slot==reference[i].second,
                        "dense, fallback and repeated sort retain original registration order");
        }
    }
    require(!snapshot.append(0,1,0)&&snapshot.size()==257,"capacity rejection preserves snapshot");
    snapshot.clear();
    require(!snapshot.append(0,0,1)&&!snapshot.append(0,1,-1)&&snapshot.size()==0,"invalid capture entries reject");
    require(snapshot.append(4,7,2)&&snapshot.append(1,9,1),"capture before incremental sort");
    snapshot.sort();
    require(snapshot.append(5,8,2)&&snapshot.append(2,10,1),"append after sort");
    snapshot.sort();
    const unsigned slots[]={1,2,4,5};
    for(unsigned i=0;i<4;++i)require(snapshot.handle(i).slot==slots[i],"post-sort append retains tie order");
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
    int depths[capacity]{};
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
            depths[slot]=int((next()>>16)%9)-4; // Negative/positive depths and frequent ties.
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
        std::vector<unsigned> drawOrder;
        require(rooms.visitDrawChecked([&](unsigned member){drawOrder.push_back(member);return true;}) &&
            drawOrder==std::vector<unsigned>(active.rbegin(),active.rend()),
            "Draw reverses the active list through depth ties, reuse and activation cycles");
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
    require(beginCalls==0 && publications==3,"unregistered promotions wait for reconciliation before dispatch");
    store.find(3)->active=false; // Retirement, not pending activation.
    store.reconcileObjectMembership();
    require(store.registeredObjectCount(10,exact)==2,"retired diagnostic record loses membership");
}

void testExternalMembershipPublication() {
    using namespace downwell;
    InstanceStore<3> store;
    store.create(1,10,7,false,20);store.create(2,10,7,false,0);store.create(3,10,7,false,10);
    std::vector<unsigned> publications;
    auto publish=[&](InstanceHandle h,const InstanceRecord& record,bool registered){
        require(store.resolve(h)==&record,"publication receives live handle and record");
        publications.push_back(record.instanceId+(registered?100:0));return true;
    };
    for(unsigned i=0;i<3;++i){store.at(i).active=false;store.at(i).activationPending=true;}
    require(store.reconcileObjectMembership(publish),"external deactivation succeeds");
    require(publications==std::vector<unsigned>({2,3,1}),"external deactivation follows room depth order");
    publications.clear();
    for(unsigned id:{1u,3u,2u})store.find(id)->active=true;
    unsigned phases=0;
    require(EventScheduler<3>::runStep(store,
        [](const InstanceRecord&){return AlarmBank::FiredMask(0);},
        [](InstanceRecord&,StepEvent){},
        [&](StepEventPhase phase){
            if(phase!=StepEventPhase::beginStep)
                require(publications==std::vector<unsigned>({102,103,101}),"external promotion precedes next phase");
            ++phases;
        },publish),"external promotion tick succeeds");
    require(phases==4&&publications.size()==3,"unchanged memberships are not republished every phase");
    unsigned calls=0;phases=0;
    require(!EventScheduler<3>::runStep(store,
        [](const InstanceRecord&){return AlarmBank::FiredMask(0);},
        [](InstanceRecord& record,StepEvent){record.active=false;},
        [&](StepEventPhase){++phases;},
        [&](InstanceHandle,const InstanceRecord&,bool){return ++calls<2;}),
        "external failure propagates from scheduler");
    require(phases==1&&calls==2,"failure stops later publication and later phases");
}

void testCheckedDispatch() {
    using namespace downwell;
    for(auto phase:{StepEventPhase::beginStep,StepEventPhase::alarm,StepEventPhase::step,StepEventPhase::endStep})
        for(bool boundary:{false,true}) {
            InstanceStore<2> instances;
            instances.create(1,10,7,false);instances.create(2,10,7,false);
            for(unsigned i=0;i<2;++i)instances.at(i).alarms.set(0,1);
            bool healthy=true;unsigned later=0,failedCalls=0;
            const bool result=EventScheduler<2>::runStepChecked(instances,
                [](const auto&){return AlarmBank::allEvents;},
                [&](auto& record,StepEvent event){
                    if(!healthy)++later;
                    if(!boundary&&event.phase==phase&&record.instanceId==1){healthy=false;++failedCalls;}
                },[&](StepEventPhase current){
                    if(!healthy)++later;
                    if(boundary&&current==phase){healthy=false;++failedCalls;}
                },[&](auto,const auto&,bool){if(!healthy)++later;return true;},[&]{return healthy;});
            require(!result&&failedCalls==1&&later==0,"failed callback stops current and later phases");
            if(phase==StepEventPhase::alarm)
                require(instances.at(1).alarms.get(0)==1,"failure leaves later instance alarm untouched");
            unsigned calls=0;
            require(!EventScheduler<2>::runStepChecked(instances,
                [&](const auto&){++calls;return AlarmBank::allEvents;},
                [&](auto&,auto){++calls;},[&](auto){++calls;},
                [&](auto,const auto&,bool){++calls;return true;},[&]{return healthy;})&&calls==0,
                "already-failed owner cannot dispatch another tick");
        }
    InstanceStore<1> instances;instances.create(1,10,7,false);instances.at(0).alarms.set(0,1);
    bool healthy=true;
    require(!EventScheduler<1>::runStepChecked(instances,
        [&](const auto&){healthy=false;return AlarmBank::allEvents;},
        [](auto&,auto){},[](auto){},[](auto,const auto&,bool){return true;},[&]{return healthy;}),
        "alarm eligibility failure propagates");
    require(instances.at(0).alarms.get(0)==1,"failed alarm eligibility does not decrement counter");
    InstanceStore<2> publishing;
    publishing.create(1,10,7,false);publishing.create(2,10,7,false);
    unsigned publications=0;healthy=true;
    require(!EventScheduler<2>::runStepChecked(publishing,
        [](const auto&){return AlarmBank::allEvents;},
        [](auto& record,StepEvent event){if(event.phase==StepEventPhase::beginStep)record.active=false;},
        [](auto){},[&](auto,const auto&,bool){++publications;healthy=false;return true;},[&]{return healthy;}),
        "latched publication failure stops reconciliation even when callback returns true");
    require(publications==1,"no second publication after latched failure");
}

void testDeferredDestroyEligibility() {
    using namespace downwell;
    for(const auto destructionPhase:{StepEventPhase::beginStep,StepEventPhase::alarm,
            StepEventPhase::step,StepEventPhase::endStep}){
        InstanceStore<3> instances;
        instances.create(1,10,7,false);instances.create(2,20,7,false);instances.create(3,30,7,false);
        for(unsigned i=0;i<3;++i){instances.at(i).alarms.set(0,1);instances.at(i).alarms.set(1,1);}
        instances.find(3)->destroyRequested=true; // Marked before capture, still allocated.
        bool destroyed=false,continued=false;
        unsigned forbidden=0;
        EventScheduler<3>::runStep(instances,[](const auto&){return AlarmBank::allEvents;},
            [&](auto& record,StepEvent event){
                if(record.instanceId==3||destroyed)++forbidden;
                if(record.instanceId==1&&event.phase==destructionPhase){
                    record.destroyRequested=true;
                    instances.find(2)->destroyRequested=true; // Was already snapshotted.
                    destroyed=true;
                    record.x=42;continued=true; // Destroy must not abort this handler.
                }
            });
        require(destroyed&&continued&&forbidden==0,"Destroy skips later dispatch without aborting its handler");
        require(instances.size()==3&&instances.find(1)->x==42,"all marked lifetimes remain allocated until cleanup");
        require(instances.find(3)->alarms.get(0)==1&&instances.find(3)->alarms.get(1)==1,
            "already marked records do not advance alarms");
        if(destructionPhase==StepEventPhase::beginStep||destructionPhase==StepEventPhase::alarm)
            require(instances.find(2)->alarms.get(0)==1&&instances.find(1)->alarms.get(1)==1,
                "destruction excludes later instance and later alarm indices immediately");
        unsigned nextEvents=0;
        EventScheduler<3>::runStep(instances,[](const auto&){return AlarmBank::allEvents;},
            [&](auto&,auto){++nextEvents;});
        require(nextEvents==0,"deferred storage does not reenter dispatch next frame");
    }
}

void testCollisionBoundary(){
    using namespace downwell;
    for(unsigned failure=0;failure<3;++failure){
        InstanceStore<4> instances;instances.create(1,10,11,false);instances.create(2,20,11,false);
        bool healthy=true;unsigned collisions=0,ends=0;std::vector<unsigned> steps;
        const bool ok=EventScheduler<4>::runStepChecked(instances,[](const auto&){return AlarmBank::allEvents;},
            [&](auto& r,StepEvent event){
                if(event.phase==StepEventPhase::step)steps.push_back(r.instanceId);
                if(event.phase==StepEventPhase::endStep){++ends;require(r.instanceId==3,"collision birth participates in End Step");}
            },[](auto){},[](auto,const auto&,bool){return true;},[&]{return healthy;},[&]{
                ++collisions;require(steps==std::vector<unsigned>({1,2}),"collision pass follows all Step handlers");
                instances.find(1)->active=false;instances.find(2)->destroyRequested=true;
                instances.create(3,30,11,false);instances.find(3)->alarms.set(0,2);
                if(failure==2)healthy=false;
                return failure!=1;
            });
        require(ok==(failure==0)&&collisions==1&&ends==unsigned(failure==0),"collision failure stops before End Step");
        require(instances.find(2)&&instances.find(2)->destroyRequested&&instances.find(3)->alarms.get(0)==2,
            "collision mutations survive failure; new alarms do not run retroactively");
    }
}

void testLayerMaintenance(){
    using namespace downwell;
    for(unsigned failure=0;failure<=5;++failure){
        InstanceStore<2> instances;
        require(instances.create(100,10,11,false)==CreateInstanceResult::success,"maintenance instance");
        const auto h=instances.handleAt(0);
        using Registry=RunnerLayerInstances<4,4>;
        Registry registry;Registry::Layers layers;RunnerLayerIds ids;
        RunnerLayerLifetimes<4,4,2> owner(layers,registry,ids);
        StoredLayerLifecycle<4,4,2> observer{owner};InstanceLifecycleBinding binding;
        require(!binding.publishedDepth(h,instances.at(0),{}),"unowned depth write fails");
        require(binding.observeLayers(observer)&&binding.birth(h,instances.at(0)),"maintenance layer birth");
        const int initialLayer=owner.resolve(h)->depth.layerId;
        unsigned maintained=0,memberships=0,ends=0;
        const bool ok=EventScheduler<2>::runStepChecked(instances,[](const auto&){return AlarmBank::allEvents;},
            [&](auto& r,StepEvent event){
                if(event.phase==StepEventPhase::beginStep){
                    require(binding.publishedDepth(h,r,MotionBinary64::fromInteger(-60000)),"script publishes deferred depth");
                    require(owner.pendingDepths()==1&&owner.resolve(h)->depth.layerId==initialLayer,
                        "depth write does not immediately move element");
                    r.active=false;
                    require(binding.activation(h,r,false),"publish inactive before maintenance");
                }
                if(event.phase==StepEventPhase::endStep)++ends;
            },[](auto){},[&](auto key,const auto& r,bool member){
                ++memberships;return binding.membership(key,r,member);
            },[]{return true;},[]{return true;},[&]{
                ++maintained;
                require(memberships==1&&owner.resolve(h)->inactive,"activation reconciliation precedes depth service");
                if(maintained==failure)return false;
                return owner.serviceDepths();
            });
        require(ok==(failure==0)&&maintained==(failure?failure:5)&&ends==0,
            "maintenance includes empty phases and failures stop later phases");
        require(owner.pendingDepths()==(failure==1?1u:0u),"failed boundary does not replay or roll back depth work");
        if(failure!=1)require(layers.get(layers.find(owner.resolve(h)->depth.layerId))->depth==-60000,
            "inactive instance depth migrated at maintenance boundary");
    }
    InstanceStore<2> instances;
    RunnerLayerInstances<4,4> registry;RunnerLayerInstances<4,4>::Layers layers;RunnerLayerIds ids;
    RunnerLayerLifetimes<4,4,2> owner(layers,registry,ids);
    StoredLayerLifecycle<4,4,2> observer{owner};InstanceLifecycleBinding binding;
    require(binding.observeLayers(observer),"animation-order layer observer");
    for(unsigned id:{100u,200u}){
        require(instances.create(id,10,11,false)==CreateInstanceResult::success,"animation-order birth");
        require(binding.birth(instances.handleAt(instances.size()-1),*instances.find(id)),"animation-order layer birth");
    }
    std::vector<unsigned> before,after;
    require(instances.visitAnimationChecked([&](auto,const auto& r){before.push_back(r.instanceId);return true;}),"capture animation order");
    require(EventScheduler<2>::runStepChecked(instances,[](const auto&){return AlarmBank::allEvents;},
        [&](auto& r,StepEvent event){
            if(event.phase==StepEventPhase::step&&r.instanceId==100)
                require(binding.publishedDepth(instances.handleAt(0),r,MotionBinary64::fromInteger(-60000)),"Step effect depth override");
        },[](auto){},[&](auto h,const auto& r,bool member){return binding.membership(h,r,member);},
        []{return true;},[]{return true;},[&]{return owner.serviceDepths();}),"scheduled depth migration");
    require(instances.visitAnimationChecked([&](auto,const auto& r){after.push_back(r.instanceId);return true;})&&before==after,
        "layer depth migration must not sort the animation list");
}

} // namespace

int main() {
    testObjectRegistrationOrder();
    testOrderedSnapshotSort();
    testPhaseAndAlarmOrder();
    testCrossAlarmWrites();
    testDeletionDuringAlarm();
    testActiveStateAndMissingAlarmEvent();
    testDeletionAndCreationAcrossPhases();
    testReusedIdCannotInheritPhaseEligibility();
    testDeferredMembershipPhaseBoundary();
    testRoomMembershipCycles();
    testExternalMembershipPublication();
    testCheckedDispatch();
    testDeferredDestroyEligibility();
    testCollisionBoundary();
    testLayerMaintenance();
    std::cout << "event scheduler tests passed\n";
    return 0;
}

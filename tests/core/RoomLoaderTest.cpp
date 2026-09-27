#include "core/RoomLoader.h"
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

struct TraceEntry {
    std::uint32_t instanceId;
    downwell::RoomInstancePhase phase;
};

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testFirstRoomOrder() {
    downwell::InstanceStore<4> instances;
    const downwell::RoomInstanceDefinition definitions[] = {
        {100065, 37, true},
        {100066, 36, false}
    };
    std::vector<TraceEntry> trace;
    int roomCreationCalls = 0;
    const auto instanceHandler = [&trace](downwell::InstanceRecord& instance,
                                          downwell::RoomInstancePhase phase) {
        trace.push_back({instance.instanceId, phase});
    };
    const auto roomHandler = [&roomCreationCalls](std::int32_t roomId) {
        require(roomId == 7, "room creation receives recovered room ID");
        ++roomCreationCalls;
    };

    require(downwell::RoomLoader<4>::enter(instances, 7, definitions, 2, true,
                                            instanceHandler, roomHandler) ==
                downwell::LoadRoomResult::success,
            "first room loads successfully");
    require(trace.size() == 10, "first-room event count is complete");
    require(trace[0].instanceId == 100065 &&
                trace[0].phase == downwell::RoomInstancePhase::preCreate &&
                trace[1].phase == downwell::RoomInstancePhase::create &&
                trace[2].phase == downwell::RoomInstancePhase::instanceCreation,
            "controller creation sequence runs first");
    require(trace[3].instanceId == 100066 &&
                trace[3].phase == downwell::RoomInstancePhase::preCreate &&
                trace[4].phase == downwell::RoomInstancePhase::create &&
                trace[5].phase == downwell::RoomInstancePhase::instanceCreation,
            "init object creation sequence runs second");
    require(trace[6].phase == downwell::RoomInstancePhase::gameStart &&
                trace[7].phase == downwell::RoomInstancePhase::gameStart,
            "Game Start follows all creation sequences");
    require(trace[8].phase == downwell::RoomInstancePhase::roomStart &&
                trace[9].phase == downwell::RoomInstancePhase::roomStart,
            "Room Start follows room creation code");
    require(roomCreationCalls == 1, "room creation code runs once");
}

void testDestinationRoomKeepsPersistentFirst() {
    downwell::InstanceStore<4> instances;
    instances.create(100065, 37, 7, true);
    instances.create(100066, 36, 7, false);
    const downwell::RoomInstanceDefinition definitions[] = {
        {104218, 35, false, 72, 96, 1.0F, 1.0F, 0.0F, 1.0F, 0.0F,
         -1, -1, 93, 0}
    };
    std::vector<TraceEntry> trace;
    const auto instanceHandler = [&trace](downwell::InstanceRecord& instance,
                                          downwell::RoomInstancePhase phase) {
        trace.push_back({instance.instanceId, phase});
    };
    const auto roomHandler = [](std::int32_t) {};

    require(downwell::RoomLoader<4>::enter(instances, 20, definitions, 1, false,
                                            instanceHandler, roomHandler) ==
                downwell::LoadRoomResult::success,
            "destination room loads successfully");
    require(instances.size() == 2, "persistent controller and player remain");
    require(instances.at(0).instanceId == 100065 &&
                instances.at(1).instanceId == 104218,
            "persistent instances precede newly created room instances");
    require(instances.at(1).x == 72 && instances.at(1).y == 96 &&
                instances.at(1).layerId == 93 && instances.at(1).depth == 0,
            "native room fields populate instance state");
    require(trace.size() == 5, "destination room omits Game Start");
    require(trace[0].instanceId == 104218 &&
                trace[0].phase == downwell::RoomInstancePhase::preCreate,
            "only new player receives creation sequence");
    require(trace[3].instanceId == 100065 &&
                trace[3].phase == downwell::RoomInstancePhase::roomStart &&
                trace[4].instanceId == 104218,
            "persistent and new instances receive Room Start");
}

void testValidationIsAtomic() {
    downwell::InstanceStore<2> instances;
    instances.create(1, 10, 7, true);
    instances.create(2, 20, 7, false);
    const downwell::RoomInstanceDefinition tooMany[] = {
        {3, 30, false}, {4, 40, false}
    };
    const auto noInstanceEvents = [](downwell::InstanceRecord&,
                                     downwell::RoomInstancePhase) {};
    const auto noRoomCode = [](std::int32_t) {};
    require(downwell::RoomLoader<2>::enter(instances, 20, tooMany, 2, false,
                                            noInstanceEvents, noRoomCode) ==
                downwell::LoadRoomResult::capacityExceeded,
            "capacity failure is reported");
    require(instances.size() == 2 && instances.at(1).instanceId == 2,
            "capacity failure leaves source room untouched");

    const downwell::RoomInstanceDefinition duplicate[] = {{1, 99, false}};
    require(downwell::RoomLoader<2>::enter(instances, 20, duplicate, 1, false,
                                            noInstanceEvents, noRoomCode) ==
                downwell::LoadRoomResult::duplicateInstanceId,
            "persistent ID collision is reported");
    require(instances.size() == 2 && instances.at(0).roomId == 7,
            "duplicate failure leaves source room untouched");
}

void testDeferredCreationPrecedesRoomStart() {
    downwell::InstanceStore<3> instances;
    const downwell::RoomInstanceDefinition definitions[] = {
        {1, 10, false}
    };
    std::vector<TraceEntry> trace;
    bool roomCodeRan = false;
    const auto instanceHandler = [&trace](downwell::InstanceRecord& instance,
                                          downwell::RoomInstancePhase phase) {
        trace.push_back({instance.instanceId, phase});
    };
    const auto roomHandler = [&roomCodeRan](std::int32_t) {
        roomCodeRan = true;
    };
    const auto deferredHandler = [&trace, &roomCodeRan](
                                     downwell::InstanceStore<3>& store) {
        require(roomCodeRan, "deferred commands follow room creation code");
        store.create(2, 20, 20, false);
        downwell::InstanceRecord& created = *store.find(2);
        trace.push_back({created.instanceId, downwell::RoomInstancePhase::preCreate});
        trace.push_back({created.instanceId, downwell::RoomInstancePhase::create});
    };

    require(downwell::RoomLoader<3>::enter(
                instances, 20, definitions, 1, false,
                instanceHandler, roomHandler, deferredHandler) ==
                downwell::LoadRoomResult::success,
            "room with deferred creation loads");
    require(trace.size() == 7, "deferred instance joins the lifecycle trace");
    require(trace[3].instanceId == 2 &&
                trace[3].phase == downwell::RoomInstancePhase::preCreate &&
                trace[4].phase == downwell::RoomInstancePhase::create,
            "deferred instance creation runs after room code");
    require(trace[5].instanceId == 1 && trace[6].instanceId == 2 &&
                trace[5].phase == downwell::RoomInstancePhase::roomStart &&
                trace[6].phase == downwell::RoomInstancePhase::roomStart,
            "existing and deferred instances receive Room Start afterward");
}

} // namespace

static void testCheckedRoomCallbacks(){
    using namespace downwell;
    const RoomInstanceDefinition definitions[]={{100,1,false},{101,2,false}};
    // 6 creation callbacks, 2 Game Start, room code, deferred publication,
    // and 3 Room Start callbacks (including the deferred child).
    for(unsigned failAt=1;failAt<=14;++failAt){
        InstanceStore<3> instances;
        unsigned calls=0,roomWrites=0;
        auto instance=[&](InstanceRecord& record,RoomInstancePhase){
            ++record.health;return ++calls!=failAt;
        };
        auto room=[&](std::int32_t){++roomWrites;return ++calls!=failAt;};
        auto deferred=[&](auto& store){
            require(store.create(200,3,7,false)==CreateInstanceResult::success,"deferred checked child birth");
            return ++calls!=failAt;
        };
        const auto result=RoomLoader<3>::enterChecked(instances,7,definitions,2,true,instance,room,deferred);
        const unsigned completed=failAt<14?failAt:13;
        require(result==(failAt<14?LoadRoomResult::callbackFailure:LoadRoomResult::success),
            "checked room callback failure reaches the owner");
        require(calls==completed,"room failure stops every later callback");
        require(instances.size()==(completed<=3?1u:completed<10?2u:3u),
            "room failure preserves completed births and prevents later ones");
        unsigned writes=0;
        for(unsigned i=0;i<instances.size();++i)writes+=instances.at(i).health;
        require(writes==completed-(completed>=9?1u:0u)-(completed>=10?1u:0u) &&
            roomWrites==(completed>=9?1u:0u),"room failure retains earlier callback writes");
    }
}

int main() {
    testFirstRoomOrder();
    testDestinationRoomKeepsPersistentFirst();
    testValidationIsAtomic();
    testDeferredCreationPrecedesRoomStart();
    testCheckedRoomCallbacks();
    std::cout << "room loader tests passed\n";
    return 0;
}

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
        {104218, 35, false}
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

} // namespace

int main() {
    testFirstRoomOrder();
    testDestinationRoomKeepsPersistentFirst();
    testValidationIsAtomic();
    std::cout << "room loader tests passed\n";
    return 0;
}

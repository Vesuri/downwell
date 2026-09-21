#include "core/InstanceStore.h"
#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testCapacityIdentityAndOrder() {
    downwell::InstanceStore<2> instances;
    require(instances.create(100, 10, 7, false) ==
                downwell::CreateInstanceResult::success,
            "first instance is created");
    require(instances.create(200, 20, 7, true) ==
                downwell::CreateInstanceResult::success,
            "second instance is created");
    require(instances.at(0).instanceId == 100 && instances.at(1).instanceId == 200,
            "creation order is stable");
    require(instances.create(100, 30, 7, false) ==
                downwell::CreateInstanceResult::duplicateId,
            "duplicate instance ID is rejected");
    require(instances.create(300, 30, 7, false) ==
                downwell::CreateInstanceResult::full,
            "capacity overflow is reported");
}

void testErasePreservesOrder() {
    downwell::InstanceStore<4> instances;
    instances.create(1, 10, 7, false);
    instances.create(2, 20, 7, false);
    instances.create(3, 30, 7, false);
    require(instances.erase(2), "existing instance is erased");
    require(instances.size() == 2, "erase reduces size");
    require(instances.at(0).instanceId == 1 && instances.at(1).instanceId == 3,
            "erase keeps remaining creation order");
    require(!instances.erase(99), "missing instance is not erased");
}

void testRoomTransitionKeepsPersistentInstances() {
    downwell::InstanceStore<5> instances;
    instances.create(1, 10, 7, true);
    instances.create(2, 20, 7, false);
    instances.create(3, 30, 7, true);
    instances.find(3)->alarms.set(4, 12);

    instances.enterRoom(20);
    require(instances.size() == 2, "room-owned instance is removed");
    require(instances.at(0).instanceId == 1 && instances.at(1).instanceId == 3,
            "persistent creation order survives transition");
    require(instances.at(0).roomId == 20 && instances.at(1).roomId == 20,
            "persistent instances enter the destination room");
    require(instances.at(1).alarms.get(4) == 12,
            "persistent instance state survives transition");
}

} // namespace

int main() {
    testCapacityIdentityAndOrder();
    testErasePreservesOrder();
    testRoomTransitionKeepsPersistentInstances();
    std::cout << "instance store tests passed\n";
    return 0;
}

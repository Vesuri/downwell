#include "core/DeferredInstanceQueue.h"
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void appendU16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
}

void appendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
}

void appendI32(std::vector<std::uint8_t>& bytes, std::int32_t value) {
    appendU32(bytes, static_cast<std::uint32_t>(value));
}

void appendU64(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
    appendU32(bytes, static_cast<std::uint32_t>(value));
    appendU32(bytes, static_cast<std::uint32_t>(value >> 32));
}

std::vector<std::uint8_t> makeObjectPack() {
    std::vector<std::uint8_t> bytes;
    bytes.insert(bytes.end(), {'D', 'W', 'O', 'B'});
    appendU16(bytes, downwell::ObjectPack::formatVersion);
    appendU16(bytes, downwell::ObjectPack::headerSize);
    appendU32(bytes, downwell::ObjectPack::manifestSchema);
    appendU32(bytes, 1);
    appendU32(bytes, 0);
    appendU64(bytes, downwell::ObjectPack::sourceHashPrefix);
    appendU32(bytes, 0);
    appendU64(bytes, 0);
    appendI32(bytes, 0);
    appendI32(bytes, 12);
    appendI32(bytes, -1);
    appendI32(bytes, 12);
    appendI32(bytes, -40);
    appendU32(bytes, 5); // visible and persistent
    appendU32(bytes, 0);
    appendU32(bytes, 0);
    appendU32(bytes, 0);
    return bytes;
}

void testApplyIsOrderedAndAtomic() {
    const std::vector<std::uint8_t> bytes = makeObjectPack();
    const downwell::ObjectPack objects(bytes.data(), bytes.size());
    downwell::InstanceStore<3> instances;
    downwell::DeferredInstanceQueue<2> queue;
    require(queue.enqueueCreate(0, 10, 20), "first create is queued");
    require(queue.enqueueCreate(0, 30, 40), "second create is queued");
    std::uint32_t nextId = 105186;
    std::uint32_t callbacks = 0;
    const auto onCreate = [&callbacks](downwell::InstanceRecord&) { ++callbacks; };
    require(queue.apply(instances, objects, 20, nextId, onCreate) ==
                downwell::ApplyInstanceCommandsResult::success,
            "valid queued creates apply");
    require(instances.size() == 2 && callbacks == 2 && nextId == 105188,
            "all creates apply in queue order and advance allocator");
    require(instances.at(0).instanceId == 105186 && instances.at(0).x == 10 &&
                instances.at(0).y == 20 && instances.at(0).depth == -40 &&
                instances.at(0).persistent,
            "object properties and requested position populate native state");

    downwell::DeferredInstanceQueue<2> invalid;
    invalid.enqueueCreate(7, 0, 0);
    require(invalid.apply(instances, objects, 20, nextId, onCreate) ==
                downwell::ApplyInstanceCommandsResult::unknownObject,
            "unknown object rejects the complete queue");
    require(instances.size() == 2 && nextId == 105188,
            "failed validation leaves storage and allocator unchanged");
}

void testQueueOverflowIsSticky() {
    const std::vector<std::uint8_t> bytes = makeObjectPack();
    const downwell::ObjectPack objects(bytes.data(), bytes.size());
    downwell::InstanceStore<2> instances;
    downwell::DeferredInstanceQueue<1> queue;
    require(queue.enqueueCreate(0, 0, 0), "bounded queue accepts first command");
    require(!queue.enqueueCreate(0, 0, 0), "bounded queue reports overflow");
    std::uint32_t nextId = 105186;
    const auto noCreate = [](downwell::InstanceRecord&) {};
    require(queue.apply(instances, objects, 20, nextId, noCreate) ==
                downwell::ApplyInstanceCommandsResult::queueFull,
            "overflow prevents a partial apply");
    require(instances.empty(), "overflowed queue leaves storage unchanged");
}

} // namespace

int main() {
    testApplyIsOrderedAndAtomic();
    testQueueOverflowIsSticky();
    std::cout << "deferred instance queue tests passed\n";
    return 0;
}

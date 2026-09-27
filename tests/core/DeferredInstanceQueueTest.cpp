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
    appendI32(bytes, 99); // Deliberately different collision mask and artwork.
    appendI32(bytes, -40);
    appendU32(bytes, 5); // visible and persistent
    appendU32(bytes, 0);
    appendU32(bytes, 0);
    appendI32(bytes, -70); // Compatibility depth deliberately differs from OBJT.
    return bytes;
}

void testApplyIsOrderedAndAtomic() {
    const std::vector<std::uint8_t> bytes = makeObjectPack();
    const downwell::ObjectPack objects(bytes.data(), bytes.size());
    downwell::InstanceStore<4> instances;
    downwell::DeferredInstanceQueue<2> queue;
    require(queue.enqueueCreate(0, 10, 20), "first create is queued");
    require(queue.enqueueCreate(0, 30, 40), "second create is queued");
    std::uint32_t nextId = 105186;
    std::uint32_t callbacks = 0;
    const auto onCreate = [&callbacks](downwell::InstanceRecord& instance) {
        require(instance.spriteId == 12,"serialized sprite exists before Create callback");
        require(instance.maskId == 99,"serialized collision mask exists before Create callback");
        require(instance.depth == -70,"compatibility depth exists before Create callback");
        ++callbacks;
    };
    require(queue.apply(instances, objects, 20, nextId, onCreate) ==
                downwell::ApplyInstanceCommandsResult::success,
            "valid queued creates apply");
    require(instances.size() == 2 && callbacks == 2 && nextId == 105188,
            "all creates apply in queue order and advance allocator");
    require(instances.at(0).instanceId == 105186 && instances.at(0).x == 10 &&
                instances.at(0).y == 20 && instances.at(0).depth == -70 &&
                instances.at(0).persistent,
            "object properties and requested position populate native state");

    downwell::DeferredInstanceQueue<1> configured;
    downwell::DeferredCreateInstance command{0, 50, 60};
    command.scaleX = -1.0F;
    command.imageSpeed = 0.0F;
    command.imageIndex = 22.0F;
    command.active = false;
    command.alarms.set(1, 73);
    require(configured.enqueueCreate(command),
            "configured runtime create is queued");
    require(configured.apply(instances, objects, 20, nextId, onCreate) ==
                downwell::ApplyInstanceCommandsResult::success,
            "configured runtime create applies");
    require(instances.at(2).scaleX == -1.0F &&
                instances.at(2).imageSpeed == 0.0F &&
                instances.at(2).imageIndex == 22.0F &&
                !instances.at(2).active && instances.at(2).activationPending &&
                instances.at(2).alarms.get(1) == 73,
            "synchronous Create results survive deferred insertion");

    downwell::DeferredInstanceQueue<2> invalid;
    invalid.enqueueCreate(7, 0, 0);
    require(invalid.apply(instances, objects, 20, nextId, onCreate) ==
                downwell::ApplyInstanceCommandsResult::unknownObject,
            "unknown object rejects the complete queue");
    require(instances.size() == 3 && nextId == 105189,
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

void testSynchronousChildBirthFailure() {
    const auto bytes = makeObjectPack();
    const downwell::ObjectPack objects(bytes.data(), bytes.size());
    for (bool duplicate : {false, true}) {
        downwell::InstanceStore<3> instances;
        downwell::DeferredInstanceQueue<2> queue;
        queue.enqueueCreate(0, 10, 20);
        queue.enqueueCreate(0, 30, 40);
        std::uint32_t nextId = 100, callbacks = 0;
        const auto childCreate = [&](downwell::InstanceRecord& parent) {
            ++callbacks;
            require(parent.instanceId == 100, "first parent dispatches before child births");
            require(instances.create(nextId, 0, 20, false) == downwell::CreateInstanceResult::success,
                "synchronous child consumes a live slot");
            if (!duplicate) {
                ++nextId;
                require(instances.create(nextId++, 0, 20, false) == downwell::CreateInstanceResult::success,
                    "second child consumes remaining capacity after queue preflight");
            }
        };
        const auto expected = duplicate ? downwell::ApplyInstanceCommandsResult::duplicateInstanceId :
            downwell::ApplyInstanceCommandsResult::instanceStoreFull;
        require(queue.apply(instances, objects, 20, nextId, childCreate) == expected,
            "post-callback birth rejection returns explicit failure");
        const auto afterFailure = nextId;
        require(callbacks == 1 && instances.size() == (duplicate ? 2u : 3u) &&
                nextId == (duplicate ? 101u : 103u),
            "failed parent has no callback and earlier children remain owned");
        require(queue.apply(instances, objects, 20, nextId, childCreate) == expected &&
                callbacks == 1 && nextId == afterFailure,
            "partial failure is sticky and cannot replay synchronous side effects");
        queue.clear();
        instances.clear();
        queue.enqueueCreate(0, 50, 60);
        require(queue.apply(instances, objects, 20, nextId, [](downwell::InstanceRecord&) {}) ==
                downwell::ApplyInstanceCommandsResult::success,
            "explicit queue reset permits a new creation batch");
    }
}

void testLifecycleFailureStopsCreate(){
    const auto bytes=makeObjectPack();downwell::ObjectPack objects(bytes.data(),bytes.size());
    downwell::InstanceStore<4> instances;downwell::DeferredInstanceQueue<3> queue;
    for(int i=0;i<3;++i)require(queue.enqueueCreate(0,i,20),"queue lifecycle fixture");
    unsigned births=0,creates=0;std::uint32_t next=100;
    struct Lifecycle {
        downwell::InstanceStore<4>& store;unsigned& births;
        bool birth(downwell::InstanceHandle h,const downwell::InstanceRecord& record){
            require(store.resolve(h)==&record&&record.maskId==99&&record.spriteId==12,
                "dynamic lifecycle sees initialized resources through a live handle");
            return ++births!=2;
        }
    } lifecycle{instances,births};
    auto create=[&](auto& record){
        require(births==creates+1&&record.instanceId==100,"birth publication precedes Create");++creates;
    };
    require(queue.apply(instances,objects,20,next,create,lifecycle)==downwell::ApplyInstanceCommandsResult::lifecycleFailure,
        "failed birth publication propagates explicitly");
    require(births==2&&creates==1&&instances.size()==2&&next==102,
        "failed birth runs no Create and prevents subsequent allocations");
    require(queue.apply(instances,objects,20,next,create,lifecycle)==downwell::ApplyInstanceCommandsResult::lifecycleFailure&&
        births==2&&creates==1&&next==102,"retry does not replay earlier publication or Create side effects");
}

void testCheckedCreateFailureStopsBatch(){
    const auto bytes=makeObjectPack();downwell::ObjectPack objects(bytes.data(),bytes.size());
    for(unsigned failAt:{1u,2u,3u,4u}){
        downwell::InstanceStore<8> instances;downwell::DeferredInstanceQueue<4> queue;
        for(int i=0;i<4;++i)require(queue.enqueueCreate(0,i,20),"queue checked Create fixture");
        unsigned calls=0;std::uint32_t next=100;
        auto create=[&](downwell::InstanceRecord& parent){
            ++calls;
            // A synchronous child and a write both precede the failure. They
            // must remain observable rather than being rolled back/repeated.
            parent.health=77;
            require(instances.create(next++,0,20,false)==downwell::CreateInstanceResult::success,
                "checked callback child birth");
            return calls!=failAt;
        };
        require(queue.applyChecked(instances,objects,20,next,create)==
            downwell::ApplyInstanceCommandsResult::createFailure,"Create failure propagates");
        require(calls==failAt&&instances.size()==2*failAt&&next==100+2*failAt,
            "failed Create prevents subsequent parent births but retains prior children");
        for(unsigned i=0;i<failAt;++i)
            require(instances.at(i*2).health==77,"completed Create writes remain visible");
        require(queue.applyChecked(instances,objects,20,next,create)==
            downwell::ApplyInstanceCommandsResult::createFailure&&calls==failAt,
            "retry cannot replay failed Create or earlier births");
        require(queue.apply(instances,objects,20,next,[](auto&){})==
            downwell::ApplyInstanceCommandsResult::createFailure&&next==100+2*failAt,
            "legacy apply cannot bypass a checked batch failure");
        queue.clear();instances.clear();
        require(queue.enqueueCreate(0,1,2),"fresh checked batch");
        require(queue.applyChecked(instances,objects,20,next,[](auto&){return true;})==
            downwell::ApplyInstanceCommandsResult::success,"explicit reset permits a new batch");
    }
}

} // namespace

int main() {
    testApplyIsOrderedAndAtomic();
    testQueueOverflowIsSticky();
    testSynchronousChildBirthFailure();
    testLifecycleFailureStopsCreate();
    testCheckedCreateFailureStopsBatch();
    std::cout << "deferred instance queue tests passed\n";
    return 0;
}

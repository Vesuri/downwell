#ifndef DOWNWELL_CORE_DEFERRED_INSTANCE_QUEUE_H
#define DOWNWELL_CORE_DEFERRED_INSTANCE_QUEUE_H

#include "InstanceStore.h"
#include "ObjectPack.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace downwell {

enum class ApplyInstanceCommandsResult {
    success,
    queueFull,
    instanceStoreFull,
    unknownObject,
    duplicateInstanceId,
    allocatorExhausted
};

struct DeferredCreateInstance {
    std::int32_t objectId;
    std::int32_t x;
    std::int32_t y;
};

template <std::size_t CommandCapacity>
class DeferredInstanceQueue {
public:
    void clear() {
        count_ = 0;
        overflowed_ = false;
    }

    std::size_t size() const { return count_; }

    bool enqueueCreate(std::int32_t objectId, std::int32_t x, std::int32_t y) {
        if (count_ == CommandCapacity) {
            overflowed_ = true;
            return false;
        }
        commands_[count_++] = {objectId, x, y};
        return true;
    }

    template <std::size_t InstanceCapacity, typename CreateHandler>
    ApplyInstanceCommandsResult apply(InstanceStore<InstanceCapacity>& instances,
                                      const ObjectPack& objects,
                                      std::int32_t roomId,
                                      std::uint32_t& nextInstanceId,
                                      CreateHandler createHandler) {
        if (overflowed_)
            return ApplyInstanceCommandsResult::queueFull;
        if (count_ > InstanceCapacity - instances.size())
            return ApplyInstanceCommandsResult::instanceStoreFull;
        if (static_cast<std::uint64_t>(nextInstanceId) + count_ >
            static_cast<std::uint64_t>(UINT32_MAX) + 1U)
            return ApplyInstanceCommandsResult::allocatorExhausted;

        std::array<PackedObject, CommandCapacity> definitions{};
        for (std::size_t index = 0; index < count_; ++index) {
            if (!objects.findObject(commands_[index].objectId, definitions[index]))
                return ApplyInstanceCommandsResult::unknownObject;
            const std::uint32_t instanceId = nextInstanceId +
                static_cast<std::uint32_t>(index);
            if (instanceId < nextInstanceId || instances.find(instanceId) != nullptr)
                return ApplyInstanceCommandsResult::duplicateInstanceId;
        }

        for (std::size_t index = 0; index < count_; ++index) {
            const DeferredCreateInstance& command = commands_[index];
            const PackedObject& definition = definitions[index];
            const std::uint32_t instanceId = nextInstanceId++;
            instances.create(instanceId, command.objectId, roomId,
                             definition.persistent());
            InstanceRecord& instance = *instances.find(instanceId);
            instance.x = command.x;
            instance.y = command.y;
            instance.depth = definition.depth;
            createHandler(instance);
        }
        clear();
        return ApplyInstanceCommandsResult::success;
    }

private:
    std::array<DeferredCreateInstance, CommandCapacity> commands_{};
    std::size_t count_ = 0;
    bool overflowed_ = false;
};

} // namespace downwell

#endif

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
    DeferredCreateInstance() = default;
    DeferredCreateInstance(std::int32_t requestedObjectId,
                           std::int32_t requestedX,
                           std::int32_t requestedY)
        : objectId(requestedObjectId), x(requestedX), y(requestedY) {}

    std::int32_t objectId;
    std::int32_t x;
    std::int32_t y;
    float scaleX = 1.0F;
    float scaleY = 1.0F;
    std::int32_t integerScaleX = 1;
    float imageSpeed = 1.0F;
    float imageIndex = 0.0F;
    std::int32_t integerImageIndex = 0;
    float velocityX = 0.0F;
    float velocityY = 0.0F;
    std::int32_t integerVelocityX = 0;
    std::int32_t integerVelocityY = 0;
    float direction = 0.0F;
    std::int32_t integerDirection = 0;
    float moveSpeed = 0.0F;
    std::int32_t integerMoveSpeed = 0;
    std::int32_t motionState = 0;
    std::int32_t auxiliaryState = 0;
    // Object 82 owns no motion auxiliary state. Retain its exact Create RNG
    // word in that existing payload slot, avoiding a larger command for every
    // terrain cell. Explicit arithmetic keeps all uint32 values portable.
    void setTimeFieldRadiusWord(std::uint32_t word) {
        auxiliaryState=word<=0x7fffffffu?static_cast<std::int32_t>(word):
            -1-static_cast<std::int32_t>(0xffffffffu-word);
    }
    std::uint32_t timeFieldRadiusWord() const {return static_cast<std::uint32_t>(auxiliaryState);}
    bool motionInitialized = false;
    bool grounded = false;
    bool active = true;
    AlarmBank alarms;
};

template <std::size_t CommandCapacity>
class DeferredInstanceQueue {
public:
    void clear() {
        count_ = 0;
        overflowed_ = false;
        applyFailure_ = ApplyInstanceCommandsResult::success;
    }

    std::size_t size() const { return count_; }

    bool enqueueCreate(const DeferredCreateInstance& command) {
        if (count_ == CommandCapacity) {
            overflowed_ = true;
            return false;
        }
        commands_[count_++] = command;
        return true;
    }

    bool enqueueCreate(std::int32_t objectId, std::int32_t x, std::int32_t y) {
        return enqueueCreate(DeferredCreateInstance(objectId, x, y));
    }

    template <std::size_t InstanceCapacity, typename CreateHandler>
    ApplyInstanceCommandsResult apply(InstanceStore<InstanceCapacity>& instances,
                                      const ObjectPack& objects,
                                      std::int32_t roomId,
                                      std::uint32_t& nextInstanceId,
                                      CreateHandler createHandler) {
        if (applyFailure_ != ApplyInstanceCommandsResult::success)
            return applyFailure_;
        if (overflowed_)
            return ApplyInstanceCommandsResult::queueFull;
        if (count_ > InstanceCapacity - instances.size())
            return ApplyInstanceCommandsResult::instanceStoreFull;
        if (static_cast<std::uint64_t>(nextInstanceId) + count_ >
            static_cast<std::uint64_t>(0xffffffffU) + 1U)
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
            const std::uint32_t instanceId = nextInstanceId;
            // Earlier synchronous Create callbacks can consume capacity or
            // IDs after preflight. Never dereference a rejected birth, nor
            // replay already-dispatched callbacks after partial failure.
            const auto created = instances.create(instanceId, command.objectId, roomId,
                                                  definition.persistent(),definition.creationDepth);
            if (created != CreateInstanceResult::success) {
                applyFailure_ = created == CreateInstanceResult::full ?
                    ApplyInstanceCommandsResult::instanceStoreFull :
                    ApplyInstanceCommandsResult::duplicateInstanceId;
                return applyFailure_;
            }
            ++nextInstanceId;
            InstanceRecord& instance = *instances.find(instanceId);
            instance.x = command.x;
            instance.y = command.y;
            instance.spriteId = definition.spriteId;
            instance.maskId = definition.textureMaskId;
            instance.scaleX = command.scaleX;
            instance.scaleY = command.scaleY;
            instance.imageSpeed = command.imageSpeed;
            instance.imageIndex = command.imageIndex;
            instance.preciseX = static_cast<float>(command.x);
            instance.preciseY = static_cast<float>(command.y);
            instance.velocityX = command.velocityX;
            instance.velocityY = command.velocityY;
            instance.direction = command.direction;
            instance.moveSpeed = command.moveSpeed;
            instance.motionState = command.motionState;
            instance.auxiliaryState = command.auxiliaryState;
            instance.motionInitialized = command.motionInitialized;
            instance.grounded = command.grounded;
            instance.active = command.active;
            instance.activationPending = !command.active;
            instance.alarms = command.alarms;
            createHandler(instance);
        }
        clear();
        return ApplyInstanceCommandsResult::success;
    }

private:
    ApplyInstanceCommandsResult applyFailure_ = ApplyInstanceCommandsResult::success;
    std::array<DeferredCreateInstance, CommandCapacity> commands_{};
    std::size_t count_ = 0;
    bool overflowed_ = false;
};

} // namespace downwell

#endif

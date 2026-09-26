#ifndef DOWNWELL_CORE_ROOM_LOADER_H
#define DOWNWELL_CORE_ROOM_LOADER_H

#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include <cstddef>
#include <cstdint>

namespace downwell {

struct RoomInstanceDefinition {
    std::uint32_t instanceId;
    std::int32_t objectId;
    bool persistent;
    std::int32_t x = 0;
    std::int32_t y = 0;
    float scaleX = 1.0F;
    float scaleY = 1.0F;
    float rotation = 0.0F;
    float imageSpeed = 1.0F;
    float imageIndex = 0.0F;
    std::int32_t creationCodeId = -1;
    std::int32_t preCreateCodeId = -1;
    std::uint32_t layerId = 0;
    std::int32_t depth = 0;
    std::int32_t spriteId = -1;
    std::int32_t maskId = -1;
};

enum class RoomInstancePhase {
    preCreate,
    create,
    instanceCreation,
    gameStart,
    roomStart
};

enum class LoadRoomResult {
    success,
    capacityExceeded,
    duplicateInstanceId,
    lifecycleFailure
};

// Loads ordered room definitions after validating the whole transition. Event
// Instance and room-code handlers must not mutate storage. The optional
// deferred-command handler is the one mutation boundary: it runs after room
// creation code and before Room Start.
// Lifecycle failures stop a partially applied transition; external publication
// cannot be rolled back. Callers must stop rather than retry the transition.
template <std::size_t Capacity>
class RoomLoader {
public:
    template <typename InstanceHandler, typename RoomCreationHandler>
    static LoadRoomResult enter(InstanceStore<Capacity>& instances,
                                std::int32_t roomId,
                                const RoomInstanceDefinition* definitions,
                                std::size_t definitionCount,
                                bool firstGameRoom,
                                InstanceHandler instanceHandler,
                                RoomCreationHandler roomCreationHandler) {
        const auto noDeferredCommands = [](InstanceStore<Capacity>&) {};
        return enter(instances, roomId, definitions, definitionCount,
                     firstGameRoom, instanceHandler, roomCreationHandler,
                     noDeferredCommands);
    }

    template <typename InstanceHandler,
              typename RoomCreationHandler,
              typename DeferredCommandHandler,
              typename Lifecycle = NoInstanceLifecycle>
    static LoadRoomResult enter(InstanceStore<Capacity>& instances,
                                std::int32_t roomId,
                                const RoomInstanceDefinition* definitions,
                                std::size_t definitionCount,
                                bool firstGameRoom,
                                InstanceHandler instanceHandler,
                                RoomCreationHandler roomCreationHandler,
                                DeferredCommandHandler deferredCommandHandler,
                                Lifecycle lifecycle = {}) {
        const LoadRoomResult validation =
            validate(instances, definitions, definitionCount);
        if (validation != LoadRoomResult::success)
            return validation;

        if (!instances.enterRoom(roomId, [&](InstanceHandle h, const InstanceRecord& record) {
            return lifecycle.retire(h, record);
        }))
            return LoadRoomResult::lifecycleFailure;
        for (std::size_t index = 0; index < definitionCount; ++index) {
            const RoomInstanceDefinition& definition = definitions[index];
            const auto created = instances.create(definition.instanceId,
                             definition.objectId,
                             roomId,
                             definition.persistent,definition.depth);
            if (created != CreateInstanceResult::success)
                return created == CreateInstanceResult::full ?
                    LoadRoomResult::capacityExceeded : LoadRoomResult::duplicateInstanceId;
            InstanceRecord& instance = *instances.find(definition.instanceId);
            instance.x = definition.x;
            instance.y = definition.y;
            instance.scaleX = definition.scaleX;
            instance.scaleY = definition.scaleY;
            instance.rotation = definition.rotation;
            instance.imageSpeed = definition.imageSpeed;
            instance.imageIndex = definition.imageIndex;
            instance.creationCodeId = definition.creationCodeId;
            instance.preCreateCodeId = definition.preCreateCodeId;
            instance.layerId = definition.layerId;
            instance.depth = definition.depth;
            instance.spriteId = definition.spriteId;
            instance.maskId = definition.maskId;
            if (!lifecycle.birth(instances.handleAt(instances.size() - 1), instance))
                return LoadRoomResult::lifecycleFailure;
            instanceHandler(instance, RoomInstancePhase::preCreate);
            instanceHandler(instance, RoomInstancePhase::create);
            instanceHandler(instance, RoomInstancePhase::instanceCreation);
        }

        if (firstGameRoom)
            dispatchAll(instances, RoomInstancePhase::gameStart, instanceHandler);
        roomCreationHandler(roomId);
        deferredCommandHandler(instances);
        dispatchAll(instances, RoomInstancePhase::roomStart, instanceHandler);
        return LoadRoomResult::success;
    }

private:
    static LoadRoomResult validate(const InstanceStore<Capacity>& instances,
                                   const RoomInstanceDefinition* definitions,
                                   std::size_t definitionCount) {
        std::size_t persistentCount = 0;
        for (std::size_t index = 0; index < instances.size(); ++index) {
            if (instances.at(index).persistent)
                ++persistentCount;
        }
        if (definitionCount > Capacity - persistentCount)
            return LoadRoomResult::capacityExceeded;

        for (std::size_t index = 0; index < definitionCount; ++index) {
            for (std::size_t existing = 0; existing < instances.size(); ++existing) {
                if (instances.at(existing).persistent &&
                    instances.at(existing).instanceId == definitions[index].instanceId)
                    return LoadRoomResult::duplicateInstanceId;
            }
            for (std::size_t prior = 0; prior < index; ++prior) {
                if (definitions[prior].instanceId == definitions[index].instanceId)
                    return LoadRoomResult::duplicateInstanceId;
            }
        }
        return LoadRoomResult::success;
    }

    template <typename InstanceHandler>
    static void dispatchAll(InstanceStore<Capacity>& instances,
                            RoomInstancePhase phase,
                            InstanceHandler& handler) {
        for (std::size_t index = 0; index < instances.size(); ++index) {
            InstanceRecord& instance = instances.at(index);
            if (instance.active)
                handler(instance, phase);
        }
    }
};

} // namespace downwell

#endif

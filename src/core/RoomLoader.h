#ifndef DOWNWELL_CORE_ROOM_LOADER_H
#define DOWNWELL_CORE_ROOM_LOADER_H

#include "InstanceStore.h"
#include <cstddef>
#include <cstdint>

namespace downwell {

struct RoomInstanceDefinition {
    std::uint32_t instanceId;
    std::int32_t objectId;
    bool persistent;
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
    duplicateInstanceId
};

// Loads ordered room definitions after validating the whole transition. Event
// handlers must not mutate the instance store during entry; runtime creation
// and destruction semantics will be handled by a later command queue.
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
        const LoadRoomResult validation =
            validate(instances, definitions, definitionCount);
        if (validation != LoadRoomResult::success)
            return validation;

        instances.enterRoom(roomId);
        for (std::size_t index = 0; index < definitionCount; ++index) {
            const RoomInstanceDefinition& definition = definitions[index];
            instances.create(definition.instanceId,
                             definition.objectId,
                             roomId,
                             definition.persistent);
            InstanceRecord& instance = *instances.find(definition.instanceId);
            instanceHandler(instance, RoomInstancePhase::preCreate);
            instanceHandler(instance, RoomInstancePhase::create);
            instanceHandler(instance, RoomInstancePhase::instanceCreation);
        }

        if (firstGameRoom)
            dispatchAll(instances, RoomInstancePhase::gameStart, instanceHandler);
        roomCreationHandler(roomId);
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

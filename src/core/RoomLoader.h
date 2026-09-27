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
    lifecycleFailure,
    callbackFailure
};

// Loads ordered room definitions after validating the whole transition.
// Creation handlers may synchronously create/retire instances. Resolve the
// original lifetime again between callbacks: destruction or compaction must
// not send the remaining creation phases to a replacement record. Game/Room
// Start handlers may retire themselves, but must not otherwise structurally
// mutate storage during that broadcast. Deferred commands run after room code
// and before Room Start.
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
        return enterChecked(instances,roomId,definitions,definitionCount,firstGameRoom,
            [&](InstanceRecord& instance,RoomInstancePhase phase){instanceHandler(instance,phase);return true;},
            [&](std::int32_t room){roomCreationHandler(room);return true;},
            [&](InstanceStore<Capacity>& store){deferredCommandHandler(store);return true;},lifecycle);
    }

    // Checked event boundaries preserve partial effects but stop all later
    // events. The owner must latch failure and must not retry this transition.
    template <typename InstanceHandler,typename RoomCreationHandler,
              typename DeferredCommandHandler,typename Lifecycle = NoInstanceLifecycle>
    static LoadRoomResult enterChecked(InstanceStore<Capacity>& instances,
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
            const auto handle=instances.handleAt(instances.size() - 1);
            if (!lifecycle.birth(handle, instance))
                return LoadRoomResult::lifecycleFailure;
            const RoomInstancePhase phases[]={RoomInstancePhase::preCreate,RoomInstancePhase::create,RoomInstancePhase::instanceCreation};
            for(auto phase:phases){
                auto* live=instances.resolve(handle);
                if(!live)break;
                if(!instanceHandler(*live,phase))return LoadRoomResult::callbackFailure;
            }
        }

        if (firstGameRoom && !dispatchAll(instances, RoomInstancePhase::gameStart, instanceHandler))
            return LoadRoomResult::callbackFailure;
        if(!roomCreationHandler(roomId) || !deferredCommandHandler(instances) ||
           !dispatchAll(instances, RoomInstancePhase::roomStart, instanceHandler))
            return LoadRoomResult::callbackFailure;
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
    static bool dispatchAll(InstanceStore<Capacity>& instances,
                            RoomInstancePhase phase,
                            InstanceHandler& handler) {
        for (std::size_t index = 0; index < instances.size();) {
            const auto handle=instances.handleAt(index);
            InstanceRecord& instance = instances.at(index);
            // Broadcast helper 0x444980 skips destroyed/inactive instances.
            // Their storage remains live until the room-entry cleanup sweep.
            if (instance.active && !instance.destroyRequested && !handler(instance, phase))return false;
            // Retirement can compact the next unvisited record into this
            // position. Never retain/dereference instance across a callback.
            const auto current=instances.handleAt(index);
            if(current.slot==handle.slot&&current.generation==handle.generation)++index;
        }
        return true;
    }
};

} // namespace downwell

#endif

#ifndef DOWNWELL_CORE_PACKED_ROOM_LOADER_H
#define DOWNWELL_CORE_PACKED_ROOM_LOADER_H

#include "RoomLoader.h"
#include "RoomPack.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace downwell {

enum class PackedRoomLoadResult {
    success,
    invalidPack,
    roomNotFound,
    capacityExceeded,
    duplicateInstanceId,
    invalidLayerMembership
};

template <std::size_t Capacity>
class PackedRoomLoader {
public:
    template <typename InstanceHandler, typename RoomCreationHandler>
    static PackedRoomLoadResult enter(InstanceStore<Capacity>& instances,
                                      const RoomPack& pack,
                                      std::int32_t roomId,
                                      bool firstGameRoom,
                                      InstanceHandler instanceHandler,
                                      RoomCreationHandler roomCreationHandler) {
        if (!pack.valid())
            return PackedRoomLoadResult::invalidPack;

        PackedRoom room;
        if (!pack.findRoom(roomId, room))
            return PackedRoomLoadResult::roomNotFound;
        if (room.instanceCount > Capacity)
            return PackedRoomLoadResult::capacityExceeded;

        std::array<RoomInstanceDefinition, Capacity> definitions{};
        for (std::uint32_t index = 0; index < room.instanceCount; ++index) {
            PackedRoomInstance packedInstance;
            if (!pack.roomInstance(room, index, packedInstance))
                return PackedRoomLoadResult::invalidPack;

            PackedRoomLayer packedLayer;
            if (!findLayer(pack, room, packedInstance.instanceId, packedLayer))
                return PackedRoomLoadResult::invalidLayerMembership;

            definitions[index] = {
                packedInstance.instanceId,
                packedInstance.objectId,
                packedInstance.persistent(),
                packedInstance.x,
                packedInstance.y,
                packedInstance.scaleX,
                packedInstance.scaleY,
                packedInstance.rotation,
                packedInstance.imageSpeed,
                packedInstance.imageIndex,
                packedInstance.creationCodeId,
                packedInstance.preCreateCodeId,
                packedLayer.layerId,
                packedLayer.depth
            };
        }

        const LoadRoomResult result = RoomLoader<Capacity>::enter(
            instances, roomId, definitions.data(), room.instanceCount,
            firstGameRoom, instanceHandler, roomCreationHandler);
        if (result == LoadRoomResult::capacityExceeded)
            return PackedRoomLoadResult::capacityExceeded;
        if (result == LoadRoomResult::duplicateInstanceId)
            return PackedRoomLoadResult::duplicateInstanceId;
        return PackedRoomLoadResult::success;
    }

private:
    static bool findLayer(const RoomPack& pack,
                          const PackedRoom& room,
                          std::uint32_t instanceId,
                          PackedRoomLayer& output) {
        bool found = false;
        for (std::uint32_t layerIndex = 0;
             layerIndex < room.layerCount;
             ++layerIndex) {
            PackedRoomLayer layer;
            if (!pack.roomLayer(room, layerIndex, layer))
                return false;
            for (std::uint32_t memberIndex = 0;
                 memberIndex < layer.membershipCount;
                 ++memberIndex) {
                std::uint32_t memberId = 0;
                if (!pack.membership(layer.firstMembership + memberIndex, memberId))
                    return false;
                if (memberId != instanceId)
                    continue;
                if (found)
                    return false;
                output = layer;
                found = true;
            }
        }
        return found;
    }
};

} // namespace downwell

#endif

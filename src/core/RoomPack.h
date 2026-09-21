#ifndef DOWNWELL_CORE_ROOM_PACK_H
#define DOWNWELL_CORE_ROOM_PACK_H

#include <cstddef>
#include <cstdint>

namespace downwell {

enum class RoomPackError {
    none,
    tooShort,
    badMagic,
    unsupportedVersion,
    badHeaderSize,
    badManifestSchema,
    badSourceHash,
    sizeOverflow,
    wrongSize,
    badRoomRange,
    badLayerRange,
    badInstanceIdBoundary
};

struct PackedRoom {
    std::int32_t roomId;
    std::int32_t width;
    std::int32_t height;
    std::int32_t speed;
    std::int32_t creationCodeId;
    std::uint32_t flags;
    std::uint32_t firstInstance;
    std::uint32_t instanceCount;
    std::uint32_t firstLayer;
    std::uint32_t layerCount;

    bool persistent() const { return (flags & 1U) != 0; }
};

struct PackedRoomInstance {
    std::uint32_t instanceId;
    std::int32_t objectId;
    std::int32_t x;
    std::int32_t y;
    float scaleX;
    float scaleY;
    float rotation;
    float imageSpeed;
    float imageIndex;
    std::int32_t creationCodeId;
    std::int32_t preCreateCodeId;
    std::uint32_t flags;

    bool persistent() const { return (flags & 1U) != 0; }
};

struct PackedRoomLayer {
    std::uint32_t layerId;
    std::int32_t depth;
    std::uint32_t flags;
    std::uint32_t firstMembership;
    std::uint32_t membershipCount;

    bool visible() const { return (flags & 1U) != 0; }
};

class RoomPack {
public:
    static constexpr std::uint16_t formatVersion = 2;
    static constexpr std::uint32_t manifestSchema = 3;
    static constexpr std::uint64_t sourceHashPrefix = 0xe00a94be5faf7dc0ULL;
    static constexpr std::size_t headerSize = 40;
    static constexpr std::size_t roomRecordSize = 40;
    static constexpr std::size_t instanceRecordSize = 48;
    static constexpr std::size_t layerRecordSize = 20;
    static constexpr std::size_t membershipRecordSize = 4;

    RoomPack(const std::uint8_t* bytes, std::size_t size);

    bool valid() const { return error_ == RoomPackError::none; }
    RoomPackError error() const { return error_; }
    std::uint32_t roomCount() const { return roomCount_; }
    std::uint32_t instanceCount() const { return instanceCount_; }
    std::uint32_t layerCount() const { return layerCount_; }
    std::uint32_t membershipCount() const { return membershipCount_; }
    std::uint32_t nextRuntimeInstanceId() const { return nextRuntimeInstanceId_; }

    bool room(std::uint32_t index, PackedRoom& output) const;
    bool findRoom(std::int32_t roomId, PackedRoom& output) const;
    bool instance(std::uint32_t index, PackedRoomInstance& output) const;
    bool roomInstance(const PackedRoom& room,
                      std::uint32_t index,
                      PackedRoomInstance& output) const;
    bool layer(std::uint32_t index, PackedRoomLayer& output) const;
    bool roomLayer(const PackedRoom& room,
                   std::uint32_t index,
                   PackedRoomLayer& output) const;
    bool membership(std::uint32_t index, std::uint32_t& instanceId) const;

private:
    bool validateRanges();

    const std::uint8_t* bytes_ = nullptr;
    std::size_t roomsOffset_ = 0;
    std::size_t instancesOffset_ = 0;
    std::size_t layersOffset_ = 0;
    std::size_t membershipsOffset_ = 0;
    std::uint32_t roomCount_ = 0;
    std::uint32_t instanceCount_ = 0;
    std::uint32_t layerCount_ = 0;
    std::uint32_t membershipCount_ = 0;
    std::uint32_t nextRuntimeInstanceId_ = 0;
    RoomPackError error_ = RoomPackError::tooShort;
};

} // namespace downwell

#endif

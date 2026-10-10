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
    badInstanceIdBoundary,
    badLayerType,
    badCatalogMaximum
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

// Serialized camera inputs, before following an instance or script setters.
struct PackedRoomView {
    bool enabled;
    std::int32_t x, y, width, height;
    std::int32_t portX, portY, portWidth, portHeight;
    std::int32_t borderX, borderY, speedX, speedY, objectId;
};

struct PackedRoomLayer {
    std::uint32_t layerId;
    std::int32_t depth;
    std::uint32_t flags;
    std::uint32_t firstMembership;
    std::uint32_t membershipCount;
    std::uint32_t type; // Serialized layer type: 1 background, 2 instances.
    std::int32_t backgroundSpriteId;
    std::uint32_t backgroundColor;
    std::uint32_t backgroundFlags; // visible, foreground, tile X/Y, stretch.
    float backgroundFirstFrame;
    float backgroundAnimationSpeed;
    std::uint32_t backgroundAnimationSpeedType;

    bool visible() const { return (flags & 1U) != 0; }
};

class RoomPack {
public:
    static constexpr std::uint16_t formatVersion = 4;
    // Projected packs retain the process-wide source catalogue ID seed even
    // when room payloads not admitted by this executable are absent.
    static constexpr std::uint16_t projectedFormatVersion = 5;
    static constexpr std::uint32_t manifestSchema = 3;
    static constexpr std::uint64_t sourceHashPrefix = 0xe00a94be5faf7dc0ULL;
    static constexpr std::size_t headerSize = 40;
    static constexpr std::size_t viewCount = 8;
    static constexpr std::size_t viewRecordSize = 56;
    static constexpr std::size_t roomRecordSize = 40 + viewCount * viewRecordSize;
    static constexpr std::size_t instanceRecordSize = 48;
    static constexpr std::size_t layerRecordSize = 48;
    static constexpr std::size_t membershipRecordSize = 4;

    RoomPack(const std::uint8_t* bytes, std::size_t size);

    bool valid() const { return error_ == RoomPackError::none; }
    RoomPackError error() const { return error_; }
    std::uint32_t roomCount() const { return roomCount_; }
    std::uint32_t instanceCount() const { return instanceCount_; }
    std::uint32_t layerCount() const { return layerCount_; }
    std::uint32_t membershipCount() const { return membershipCount_; }
    std::uint32_t nextRuntimeInstanceId() const { return nextRuntimeInstanceId_; }
    std::uint32_t catalogLayerMaximum() const { return catalogLayerMaximum_; }

    bool room(std::uint32_t index, PackedRoom& output) const;
    bool findRoom(std::int32_t roomId, PackedRoom& output) const;
    bool roomView(std::uint32_t roomIndex, std::uint32_t viewIndex,
                  PackedRoomView& output) const;
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
    std::uint32_t catalogLayerMaximum_ = 0;
    bool projected_ = false;
    std::uint32_t instanceCount_ = 0;
    std::uint32_t layerCount_ = 0;
    std::uint32_t membershipCount_ = 0;
    std::uint32_t nextRuntimeInstanceId_ = 0;
    RoomPackError error_ = RoomPackError::tooShort;
};

} // namespace downwell

#endif

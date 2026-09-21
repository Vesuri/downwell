#include "RoomPack.h"
#include <cstring>
#include <limits>

namespace downwell {
namespace {

std::uint16_t readU16(const std::uint8_t* bytes) {
    return static_cast<std::uint16_t>(bytes[0]) |
           static_cast<std::uint16_t>(bytes[1] << 8);
}

std::uint32_t readU32(const std::uint8_t* bytes) {
    return static_cast<std::uint32_t>(bytes[0]) |
           (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) |
           (static_cast<std::uint32_t>(bytes[3]) << 24);
}

std::int32_t readI32(const std::uint8_t* bytes) {
    const std::uint32_t value = readU32(bytes);
    if (value <= 0x7fffffffU)
        return static_cast<std::int32_t>(value);
    return -static_cast<std::int32_t>(0xffffffffU - value) - 1;
}

std::uint64_t readU64(const std::uint8_t* bytes) {
    return static_cast<std::uint64_t>(readU32(bytes)) |
           (static_cast<std::uint64_t>(readU32(bytes + 4)) << 32);
}

float readF32(const std::uint8_t* bytes) {
    const std::uint32_t bits = readU32(bytes);
    float value;
    static_assert(sizeof(value) == sizeof(bits), "32-bit float required");
    static_assert(std::numeric_limits<float>::is_iec559, "IEEE-754 float required");
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

bool addRegion(std::size_t& offset,
               std::uint32_t count,
               std::size_t recordSize,
               std::size_t totalSize) {
    if (offset > totalSize || count > (totalSize - offset) / recordSize)
        return false;
    offset += static_cast<std::size_t>(count) * recordSize;
    return true;
}

bool validRange(std::uint32_t first,
                std::uint32_t count,
                std::uint32_t limit) {
    return first <= limit && count <= limit - first;
}

} // namespace

RoomPack::RoomPack(const std::uint8_t* bytes, std::size_t size)
    : bytes_(bytes) {
    if (bytes == nullptr || size < headerSize) {
        error_ = RoomPackError::tooShort;
        return;
    }
    if (bytes[0] != 'D' || bytes[1] != 'W' || bytes[2] != 'R' || bytes[3] != 'M') {
        error_ = RoomPackError::badMagic;
        return;
    }
    if (readU16(bytes + 4) != formatVersion) {
        error_ = RoomPackError::unsupportedVersion;
        return;
    }
    if (readU16(bytes + 6) != headerSize) {
        error_ = RoomPackError::badHeaderSize;
        return;
    }
    if (readU32(bytes + 8) != manifestSchema) {
        error_ = RoomPackError::badManifestSchema;
        return;
    }
    if (readU64(bytes + 28) != sourceHashPrefix) {
        error_ = RoomPackError::badSourceHash;
        return;
    }

    roomCount_ = readU32(bytes + 12);
    instanceCount_ = readU32(bytes + 16);
    layerCount_ = readU32(bytes + 20);
    membershipCount_ = readU32(bytes + 24);

    roomsOffset_ = headerSize;
    std::size_t offset = roomsOffset_;
    if (!addRegion(offset, roomCount_, roomRecordSize, size)) {
        error_ = RoomPackError::sizeOverflow;
        return;
    }
    instancesOffset_ = offset;
    if (!addRegion(offset, instanceCount_, instanceRecordSize, size)) {
        error_ = RoomPackError::sizeOverflow;
        return;
    }
    layersOffset_ = offset;
    if (!addRegion(offset, layerCount_, layerRecordSize, size)) {
        error_ = RoomPackError::sizeOverflow;
        return;
    }
    membershipsOffset_ = offset;
    if (!addRegion(offset, membershipCount_, membershipRecordSize, size)) {
        error_ = RoomPackError::sizeOverflow;
        return;
    }
    if (offset != size) {
        error_ = RoomPackError::wrongSize;
        return;
    }

    error_ = RoomPackError::none;
    validateRanges();
}

bool RoomPack::validateRanges() {
    for (std::uint32_t index = 0; index < roomCount_; ++index) {
        PackedRoom value;
        room(index, value);
        if (!validRange(value.firstInstance, value.instanceCount, instanceCount_) ||
            !validRange(value.firstLayer, value.layerCount, layerCount_)) {
            error_ = RoomPackError::badRoomRange;
            return false;
        }
    }
    for (std::uint32_t index = 0; index < layerCount_; ++index) {
        PackedRoomLayer value;
        layer(index, value);
        if (!validRange(value.firstMembership,
                        value.membershipCount,
                        membershipCount_)) {
            error_ = RoomPackError::badLayerRange;
            return false;
        }
    }
    return true;
}

bool RoomPack::room(std::uint32_t index, PackedRoom& output) const {
    if (!valid() || index >= roomCount_)
        return false;
    const std::uint8_t* data = bytes_ + roomsOffset_ + index * roomRecordSize;
    output.roomId = readI32(data + 0);
    output.width = readI32(data + 4);
    output.height = readI32(data + 8);
    output.speed = readI32(data + 12);
    output.creationCodeId = readI32(data + 16);
    output.flags = readU32(data + 20);
    output.firstInstance = readU32(data + 24);
    output.instanceCount = readU32(data + 28);
    output.firstLayer = readU32(data + 32);
    output.layerCount = readU32(data + 36);
    return true;
}

bool RoomPack::findRoom(std::int32_t roomId, PackedRoom& output) const {
    for (std::uint32_t index = 0; index < roomCount_; ++index) {
        if (room(index, output) && output.roomId == roomId)
            return true;
    }
    return false;
}

bool RoomPack::instance(std::uint32_t index, PackedRoomInstance& output) const {
    if (!valid() || index >= instanceCount_)
        return false;
    const std::uint8_t* data = bytes_ + instancesOffset_ + index * instanceRecordSize;
    output.instanceId = readU32(data + 0);
    output.objectId = readI32(data + 4);
    output.x = readI32(data + 8);
    output.y = readI32(data + 12);
    output.scaleX = readF32(data + 16);
    output.scaleY = readF32(data + 20);
    output.rotation = readF32(data + 24);
    output.imageSpeed = readF32(data + 28);
    output.imageIndex = readF32(data + 32);
    output.creationCodeId = readI32(data + 36);
    output.preCreateCodeId = readI32(data + 40);
    output.flags = readU32(data + 44);
    return true;
}

bool RoomPack::roomInstance(const PackedRoom& roomValue,
                            std::uint32_t index,
                            PackedRoomInstance& output) const {
    return index < roomValue.instanceCount &&
           instance(roomValue.firstInstance + index, output);
}

bool RoomPack::layer(std::uint32_t index, PackedRoomLayer& output) const {
    if (!valid() || index >= layerCount_)
        return false;
    const std::uint8_t* data = bytes_ + layersOffset_ + index * layerRecordSize;
    output.layerId = readU32(data + 0);
    output.depth = readI32(data + 4);
    output.flags = readU32(data + 8);
    output.firstMembership = readU32(data + 12);
    output.membershipCount = readU32(data + 16);
    return true;
}

bool RoomPack::roomLayer(const PackedRoom& roomValue,
                         std::uint32_t index,
                         PackedRoomLayer& output) const {
    return index < roomValue.layerCount &&
           layer(roomValue.firstLayer + index, output);
}

bool RoomPack::membership(std::uint32_t index, std::uint32_t& instanceId) const {
    if (!valid() || index >= membershipCount_)
        return false;
    instanceId = readU32(bytes_ + membershipsOffset_ + index * membershipRecordSize);
    return true;
}

} // namespace downwell

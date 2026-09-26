#ifndef DOWNWELL_CORE_OBJECT_PACK_H
#define DOWNWELL_CORE_OBJECT_PACK_H

#include <cstddef>
#include <cstdint>

namespace downwell {

enum class ObjectPackError {
    none,
    tooShort,
    badMagic,
    unsupportedVersion,
    badHeaderSize,
    badManifestSchema,
    badSourceHash,
    sizeOverflow,
    wrongSize,
    badObjectId,
    badBindingRange,
    badParentId,
    inheritanceCycle
};

struct PackedObject {
    std::int32_t objectId;
    std::int32_t spriteId;
    std::int32_t parentId;
    std::int32_t textureMaskId;
    std::int32_t depth;
    std::uint32_t flags;
    std::uint32_t firstBinding;
    std::uint32_t bindingCount;
    std::int32_t creationDepth; // Compatibility instance_create, not OBJT depth.

    bool visible() const { return (flags & 1U) != 0; }
    bool solid() const { return (flags & 2U) != 0; }
    bool persistent() const { return (flags & 4U) != 0; }
    bool usesPhysics() const { return (flags & 8U) != 0; }
    bool managed() const { return (flags & 16U) != 0; }
};

struct PackedObjectEvent {
    std::int32_t ownerObjectId;
    std::int32_t eventType;
    std::int32_t eventSubtype;
    std::int32_t codeId;
};

class ObjectPack {
public:
    static constexpr std::uint16_t formatVersion = 3;
    static constexpr std::uint32_t manifestSchema = 3;
    static constexpr std::uint64_t sourceHashPrefix = 0xe00a94be5faf7dc0ULL;
    static constexpr std::size_t headerSize = 40;
    static constexpr std::size_t objectRecordSize = 36;
    static constexpr std::size_t bindingRecordSize = 16;

    ObjectPack(const std::uint8_t* bytes, std::size_t size);

    bool valid() const { return error_ == ObjectPackError::none; }
    ObjectPackError error() const { return error_; }
    std::uint32_t objectCount() const { return objectCount_; }
    std::uint32_t bindingCount() const { return bindingCount_; }

    bool object(std::uint32_t index, PackedObject& output) const;
    bool findObject(std::int32_t objectId, PackedObject& output) const;
    bool binding(std::uint32_t index, PackedObjectEvent& output) const;
    bool directEvent(const PackedObject& object,
                     std::int32_t eventType,
                     std::int32_t eventSubtype,
                     PackedObjectEvent& output) const;
    bool findEvent(std::int32_t objectId,
                   std::int32_t eventType,
                   std::int32_t eventSubtype,
                   PackedObjectEvent& output) const;

private:
    bool validateObjects();

    const std::uint8_t* bytes_ = nullptr;
    std::size_t objectsOffset_ = 0;
    std::size_t bindingsOffset_ = 0;
    std::uint32_t objectCount_ = 0;
    std::uint32_t bindingCount_ = 0;
    ObjectPackError error_ = ObjectPackError::tooShort;
};

} // namespace downwell

#endif

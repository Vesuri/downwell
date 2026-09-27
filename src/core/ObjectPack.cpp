#include "ObjectPack.h"

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

ObjectPack::ObjectPack(const std::uint8_t* bytes, std::size_t size)
    : bytes_(bytes) {
    if (bytes == nullptr || size < headerSize) {
        error_ = ObjectPackError::tooShort;
        return;
    }
    if (bytes[0] != 'D' || bytes[1] != 'W' || bytes[2] != 'O' || bytes[3] != 'B') {
        error_ = ObjectPackError::badMagic;
        return;
    }
    if (readU16(bytes + 4) != formatVersion) {
        error_ = ObjectPackError::unsupportedVersion;
        return;
    }
    if (readU16(bytes + 6) != headerSize) {
        error_ = ObjectPackError::badHeaderSize;
        return;
    }
    if (readU32(bytes + 8) != manifestSchema) {
        error_ = ObjectPackError::badManifestSchema;
        return;
    }
    if (readU64(bytes + 20) != sourceHashPrefix) {
        error_ = ObjectPackError::badSourceHash;
        return;
    }

    objectCount_ = readU32(bytes + 12);
    bindingCount_ = readU32(bytes + 16);
    objectsOffset_ = headerSize;
    std::size_t offset = objectsOffset_;
    if (!addRegion(offset, objectCount_, objectRecordSize, size)) {
        error_ = ObjectPackError::sizeOverflow;
        return;
    }
    bindingsOffset_ = offset;
    if (!addRegion(offset, bindingCount_, bindingRecordSize, size)) {
        error_ = ObjectPackError::sizeOverflow;
        return;
    }
    if (offset != size) {
        error_ = ObjectPackError::wrongSize;
        return;
    }

    error_ = ObjectPackError::none;
    validateObjects();
}

bool ObjectPack::validateObjects() {
    for (std::uint32_t index = 0; index < objectCount_; ++index) {
        PackedObject value;
        object(index, value);
        if (value.objectId != static_cast<std::int32_t>(index)) {
            error_ = ObjectPackError::badObjectId;
            return false;
        }
        if (!validRange(value.firstBinding, value.bindingCount, bindingCount_)) {
            error_ = ObjectPackError::badBindingRange;
            return false;
        }
        if (value.parentId < -1 ||
            value.parentId >= static_cast<std::int32_t>(objectCount_)) {
            error_ = ObjectPackError::badParentId;
            return false;
        }

        std::int32_t ancestor = value.parentId;
        std::uint32_t hops = 0;
        while (ancestor != -1) {
            if (++hops > objectCount_) {
                error_ = ObjectPackError::inheritanceCycle;
                return false;
            }
            PackedObject parent;
            object(static_cast<std::uint32_t>(ancestor), parent);
            ancestor = parent.parentId;
        }
    }
    return true;
}

bool ObjectPack::object(std::uint32_t index, PackedObject& output) const {
    if (!valid() || index >= objectCount_)
        return false;
    const std::uint8_t* data = bytes_ + objectsOffset_ + index * objectRecordSize;
    output.objectId = readI32(data + 0);
    output.spriteId = readI32(data + 4);
    output.parentId = readI32(data + 8);
    output.textureMaskId = readI32(data + 12);
    output.depth = readI32(data + 16);
    output.flags = readU32(data + 20);
    output.firstBinding = readU32(data + 24);
    output.bindingCount = readU32(data + 28);
    output.creationDepth = readI32(data + 32);
    return true;
}

bool ObjectPack::findObject(std::int32_t objectId, PackedObject& output) const {
    return objectId >= 0 && object(static_cast<std::uint32_t>(objectId), output);
}

bool ObjectPack::binding(std::uint32_t index, PackedObjectEvent& output) const {
    if (!valid() || index >= bindingCount_)
        return false;
    const std::uint8_t* data = bytes_ + bindingsOffset_ + index * bindingRecordSize;
    output.ownerObjectId = -1;
    output.eventType = readI32(data + 0);
    output.eventSubtype = readI32(data + 4);
    output.codeId = readI32(data + 8);
    return true;
}

bool ObjectPack::directEvent(const PackedObject& objectValue,
                             std::int32_t eventType,
                             std::int32_t eventSubtype,
                             PackedObjectEvent& output) const {
    for (std::uint32_t index = 0; index < objectValue.bindingCount; ++index) {
        if (!binding(objectValue.firstBinding + index, output))
            return false;
        if (output.eventType == eventType && output.eventSubtype == eventSubtype) {
            output.ownerObjectId = objectValue.objectId;
            return true;
        }
    }
    return false;
}

bool ObjectPack::collisionEventExists(std::int32_t selfObject,std::int32_t otherObject,bool& found) const {
    PackedObjectEvent event;
    return collisionEvent(selfObject,otherObject,found,event);
}

bool ObjectPack::collisionEvent(std::int32_t selfObject,std::int32_t otherObject,
                                bool& found,PackedObjectEvent& output) const {
    PackedObject self,other;
    if(!findObject(selfObject,self)||!findObject(otherObject,other))return false;
    // 0x478830 copies absent ancestor entries; 0x478b60/0x4782a0
    // expand collision targets and retain inherited collision entries.
    // Exact entries are inherited BEFORE target expansion (0x47b812/0x47b893).
    // Expansion stops at existing descendant entries (0x478341), so the
    // nearest seeded target wins regardless of hash-table traversal order.
    for(int target=otherObject;target>=0;target=other.parentId){
        if(!findObject(target,other))return false;
        PackedObjectEvent event;
        if(findEvent(selfObject,4,target,event)){output=event;found=true;return true;}
    }
    found=false;return true;
}

bool ObjectPack::findEvent(std::int32_t objectId,
                           std::int32_t eventType,
                           std::int32_t eventSubtype,
                           PackedObjectEvent& output) const {
    PackedObject current;
    while (findObject(objectId, current)) {
        if (directEvent(current, eventType, eventSubtype, output))
            return true;
        objectId = current.parentId;
    }
    return false;
}

} // namespace downwell

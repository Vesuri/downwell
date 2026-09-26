#include "SpritePack.h"

namespace downwell {
namespace {
std::uint16_t u16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>(p[0]) |
           static_cast<std::uint16_t>(p[1] << 8);
}
std::uint32_t u32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) |
           static_cast<std::uint32_t>(p[1]) << 8 |
           static_cast<std::uint32_t>(p[2]) << 16 |
           static_cast<std::uint32_t>(p[3]) << 24;
}
std::int32_t i32(const std::uint8_t* p) {
    const std::uint32_t v = u32(p);
    return v <= 0x7fffffffU ? static_cast<std::int32_t>(v)
                            : -static_cast<std::int32_t>(0xffffffffU - v) - 1;
}
std::uint64_t u64(const std::uint8_t* p) {
    return static_cast<std::uint64_t>(u32(p)) |
           static_cast<std::uint64_t>(u32(p + 4)) << 32;
}
bool region(std::size_t& offset, std::uint32_t count,
            std::size_t recordSize, std::size_t size) {
    if (offset > size || count > (size - offset) / recordSize)
        return false;
    offset += static_cast<std::size_t>(count) * recordSize;
    return true;
}
bool range(std::uint32_t first, std::uint32_t count, std::uint32_t limit) {
    return first <= limit && count <= limit - first;
}
} // namespace

SpritePack::SpritePack(const std::uint8_t* bytes, std::size_t size) : bytes_(bytes) {
    if (bytes == nullptr || size < headerSize) return;
    if (bytes[0] != 'D' || bytes[1] != 'W' || bytes[2] != 'S' || bytes[3] != 'P') {
        error_ = SpritePackError::badMagic; return;
    }
    if (u16(bytes + 4) != formatVersion) {
        error_ = SpritePackError::unsupportedVersion; return;
    }
    if (u16(bytes + 6) != headerSize || u32(bytes + 8) != manifestSchema) {
        error_ = SpritePackError::badHeader; return;
    }
    if (u64(bytes + 24) != sourceHashPrefix) {
        error_ = SpritePackError::badSourceHash; return;
    }
    spriteCount_ = u32(bytes + 12);
    frameCount_ = u32(bytes + 16);
    pageItemCount_ = u32(bytes + 20);
    texturePageCount_ = u32(bytes + 32);
    spritesOffset_ = headerSize;
    std::size_t offset = spritesOffset_;
    if (!region(offset, spriteCount_, spriteRecordSize, size)) return;
    framesOffset_ = offset;
    if (!region(offset, frameCount_, frameRecordSize, size)) return;
    pageItemsOffset_ = offset;
    if (!region(offset, pageItemCount_, pageItemRecordSize, size)) return;
    if (offset != size) { error_ = SpritePackError::wrongSize; return; }
    error_ = SpritePackError::none;
    for (std::uint32_t id = 0; id < spriteCount_; ++id) {
        PackedSprite value;
        sprite(id, value);
        if (value.spriteId != static_cast<std::int32_t>(id) ||
            !range(value.firstFrame, value.frameCount, frameCount_) || value.collisionMode>1) {
            error_ = SpritePackError::badSprite; return;
        }
    }
    for (std::uint32_t id = 0; id < frameCount_; ++id) {
        if (u32(bytes_ + framesOffset_ + id * frameRecordSize) >= pageItemCount_) {
            error_ = SpritePackError::badFrame; return;
        }
    }
    for (std::uint32_t id = 0; id < pageItemCount_; ++id) {
        PackedTexturePageItem value;
        pageItem(id, value);
        if (value.itemId != id || value.texturePageId < 0 ||
            static_cast<std::uint32_t>(value.texturePageId) >= texturePageCount_) {
            error_ = SpritePackError::badPageItem; return;
        }
    }
}

bool SpritePack::sprite(std::uint32_t id, PackedSprite& out) const {
    if (!valid() || id >= spriteCount_) return false;
    const std::uint8_t* p = bytes_ + spritesOffset_ + id * spriteRecordSize;
    out = {i32(p), i32(p + 4), i32(p + 8), i32(p + 12), i32(p + 16),
           u32(p + 20), u32(p + 24), u32(p + 28),
           i32(p + 32), i32(p + 36), i32(p + 40), i32(p + 44),u32(p + 48),u32(p + 52)};
    return true;
}

bool SpritePack::pageItem(std::uint32_t id, PackedTexturePageItem& out) const {
    if (!valid() || id >= pageItemCount_) return false;
    const std::uint8_t* p = bytes_ + pageItemsOffset_ + id * pageItemRecordSize;
    out = {u32(p), i32(p + 4), i32(p + 8), i32(p + 12), i32(p + 16),
           i32(p + 20), i32(p + 24), i32(p + 28), i32(p + 32), i32(p + 36),
           i32(p + 40), i32(p + 44)};
    return true;
}

bool SpritePack::spriteFrame(const PackedSprite& value, std::uint32_t frame,
                             PackedTexturePageItem& out) const {
    if (frame >= value.frameCount) return false;
    const std::uint32_t item = u32(bytes_ + framesOffset_ +
                                   (value.firstFrame + frame) * frameRecordSize);
    return pageItem(item, out);
}
} // namespace downwell

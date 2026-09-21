#ifndef DOWNWELL_CORE_SPRITE_PACK_H
#define DOWNWELL_CORE_SPRITE_PACK_H

#include <cstddef>
#include <cstdint>

namespace downwell {

enum class SpritePackError { none, tooShort, badMagic, unsupportedVersion,
    badHeader, badSourceHash, wrongSize, badSprite, badFrame, badPageItem };

struct PackedSprite {
    std::int32_t spriteId, width, height, originX, originY;
    std::uint32_t flags, firstFrame, frameCount;
};

struct PackedTexturePageItem {
    std::uint32_t itemId;
    std::int32_t texturePageId, sourceX, sourceY, sourceWidth, sourceHeight;
    std::int32_t targetX, targetY, targetWidth, targetHeight;
    std::int32_t boundingWidth, boundingHeight;
};

class SpritePack {
public:
    static constexpr std::uint16_t formatVersion = 1;
    static constexpr std::uint32_t manifestSchema = 3;
    static constexpr std::uint64_t sourceHashPrefix = 0xe00a94be5faf7dc0ULL;
    static constexpr std::size_t headerSize = 40;
    static constexpr std::size_t spriteRecordSize = 32;
    static constexpr std::size_t frameRecordSize = 4;
    static constexpr std::size_t pageItemRecordSize = 48;

    SpritePack(const std::uint8_t* bytes, std::size_t size);
    bool valid() const { return error_ == SpritePackError::none; }
    SpritePackError error() const { return error_; }
    std::uint32_t spriteCount() const { return spriteCount_; }
    std::uint32_t frameCount() const { return frameCount_; }
    std::uint32_t pageItemCount() const { return pageItemCount_; }
    std::uint32_t texturePageCount() const { return texturePageCount_; }
    bool sprite(std::uint32_t id, PackedSprite& output) const;
    bool pageItem(std::uint32_t id, PackedTexturePageItem& output) const;
    bool spriteFrame(const PackedSprite& sprite,
                     std::uint32_t frame,
                     PackedTexturePageItem& output) const;

private:
    const std::uint8_t* bytes_ = nullptr;
    std::size_t spritesOffset_ = 0, framesOffset_ = 0, pageItemsOffset_ = 0;
    std::uint32_t spriteCount_ = 0, frameCount_ = 0;
    std::uint32_t pageItemCount_ = 0, texturePageCount_ = 0;
    SpritePackError error_ = SpritePackError::tooShort;
};

} // namespace downwell
#endif

#ifndef DOWNWELL_CORE_FONT_PACK_H
#define DOWNWELL_CORE_FONT_PACK_H

#include <cstddef>
#include <cstdint>

namespace downwell {

enum class FontPackError { none, tooShort, badMagic, unsupportedVersion,
    badHeader, badSourceHash, wrongSize, badFont, badGlyph, badKerning };

struct PackedFont {
    std::int32_t fontId;
    float emSize;
    std::int32_t texturePageItemId;
    std::uint32_t flags, firstGlyph, glyphCount, rangeStart, rangeEnd;
};

struct PackedGlyph {
    std::uint32_t character;
    std::int32_t sourceX, sourceY, sourceWidth, sourceHeight, shift, offset;
    std::uint32_t firstKerning, kerningCount;
};

struct PackedKerning {
    std::uint32_t character;
    std::int32_t shiftModifier;
};

class FontPack {
public:
    static constexpr std::uint16_t formatVersion = 1;
    static constexpr std::uint32_t manifestSchema = 3;
    static constexpr std::uint64_t sourceHashPrefix = 0xe00a94be5faf7dc0ULL;
    static constexpr std::size_t headerSize = 40;
    static constexpr std::size_t fontRecordSize = 36;
    static constexpr std::size_t glyphRecordSize = 36;
    static constexpr std::size_t kerningRecordSize = 8;

    FontPack(const std::uint8_t* bytes, std::size_t size);
    bool valid() const { return error_ == FontPackError::none; }
    FontPackError error() const { return error_; }
    std::uint32_t fontCount() const { return fontCount_; }
    std::uint32_t glyphCount() const { return glyphCount_; }
    std::uint32_t kerningCount() const { return kerningCount_; }
    bool font(std::uint32_t id, PackedFont& output) const;
    bool glyph(std::uint32_t id, PackedGlyph& output) const;
    bool kerning(std::uint32_t id, PackedKerning& output) const;
    bool findGlyph(const PackedFont& font, std::uint32_t character,
                   PackedGlyph& output) const;
    std::int32_t kerningShift(const PackedGlyph& glyph,
                              std::uint32_t nextCharacter) const;

private:
    const std::uint8_t* bytes_ = nullptr;
    std::size_t fontsOffset_ = 0, glyphsOffset_ = 0, kerningsOffset_ = 0;
    std::uint32_t fontCount_ = 0, glyphCount_ = 0, kerningCount_ = 0;
    FontPackError error_ = FontPackError::tooShort;
};

} // namespace downwell
#endif

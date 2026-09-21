#include "FontPack.h"
#include <cstring>

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
float f32(const std::uint8_t* p) {
    const std::uint32_t bits = u32(p);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
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

FontPack::FontPack(const std::uint8_t* bytes, std::size_t size) : bytes_(bytes) {
    if (bytes == nullptr || size < headerSize) return;
    if (bytes[0] != 'D' || bytes[1] != 'W' || bytes[2] != 'F' || bytes[3] != 'N') {
        error_ = FontPackError::badMagic; return;
    }
    if (u16(bytes + 4) != formatVersion) {
        error_ = FontPackError::unsupportedVersion; return;
    }
    if (u16(bytes + 6) != headerSize || u32(bytes + 8) != manifestSchema) {
        error_ = FontPackError::badHeader; return;
    }
    if (u64(bytes + 24) != sourceHashPrefix) {
        error_ = FontPackError::badSourceHash; return;
    }
    fontCount_ = u32(bytes + 12);
    glyphCount_ = u32(bytes + 16);
    kerningCount_ = u32(bytes + 20);
    fontsOffset_ = headerSize;
    std::size_t offset = fontsOffset_;
    if (!region(offset, fontCount_, fontRecordSize, size)) return;
    glyphsOffset_ = offset;
    if (!region(offset, glyphCount_, glyphRecordSize, size)) return;
    kerningsOffset_ = offset;
    if (!region(offset, kerningCount_, kerningRecordSize, size)) return;
    if (offset != size) { error_ = FontPackError::wrongSize; return; }
    error_ = FontPackError::none;
    for (std::uint32_t id = 0; id < fontCount_; ++id) {
        PackedFont value;
        font(id, value);
        if (value.fontId != static_cast<std::int32_t>(id) ||
            !range(value.firstGlyph, value.glyphCount, glyphCount_)) {
            error_ = FontPackError::badFont; return;
        }
        std::uint32_t previous = 0;
        for (std::uint32_t index = 0; index < value.glyphCount; ++index) {
            PackedGlyph current;
            glyph(value.firstGlyph + index, current);
            if ((index != 0 && current.character <= previous) ||
                !range(current.firstKerning, current.kerningCount, kerningCount_)) {
                error_ = FontPackError::badGlyph; return;
            }
            previous = current.character;
        }
    }
}

bool FontPack::font(std::uint32_t id, PackedFont& out) const {
    if (!valid() || id >= fontCount_) return false;
    const std::uint8_t* p = bytes_ + fontsOffset_ + id * fontRecordSize;
    out = {i32(p), f32(p + 4), i32(p + 8), u32(p + 12), u32(p + 16),
           u32(p + 20), u32(p + 24), u32(p + 28)};
    return true;
}
bool FontPack::glyph(std::uint32_t id, PackedGlyph& out) const {
    if (!valid() || id >= glyphCount_) return false;
    const std::uint8_t* p = bytes_ + glyphsOffset_ + id * glyphRecordSize;
    out = {u32(p), i32(p + 4), i32(p + 8), i32(p + 12), i32(p + 16),
           i32(p + 20), i32(p + 24), u32(p + 28), u32(p + 32)};
    return true;
}
bool FontPack::kerning(std::uint32_t id, PackedKerning& out) const {
    if (!valid() || id >= kerningCount_) return false;
    const std::uint8_t* p = bytes_ + kerningsOffset_ + id * kerningRecordSize;
    out = {u32(p), i32(p + 4)};
    return true;
}
bool FontPack::findGlyph(const PackedFont& value, std::uint32_t character,
                         PackedGlyph& out) const {
    std::uint32_t low = 0, high = value.glyphCount;
    while (low < high) {
        const std::uint32_t mid = low + (high - low) / 2;
        glyph(value.firstGlyph + mid, out);
        if (out.character < character) low = mid + 1;
        else high = mid;
    }
    return low < value.glyphCount &&
           glyph(value.firstGlyph + low, out) && out.character == character;
}
std::int32_t FontPack::kerningShift(const PackedGlyph& value,
                                    std::uint32_t nextCharacter) const {
    PackedKerning item;
    for (std::uint32_t index = 0; index < value.kerningCount; ++index) {
        if (kerning(value.firstKerning + index, item) &&
            item.character == nextCharacter)
            return item.shiftModifier;
    }
    return 0;
}
} // namespace downwell

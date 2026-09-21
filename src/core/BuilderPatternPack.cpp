#include "BuilderPatternPack.h"

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
std::uint64_t u64(const std::uint8_t* p) {
    return static_cast<std::uint64_t>(u32(p)) |
           static_cast<std::uint64_t>(u32(p + 4)) << 32;
}
bool range(std::uint32_t first, std::uint32_t count, std::uint32_t limit) {
    return first <= limit && count <= limit - first;
}
} // namespace

BuilderPatternPack::BuilderPatternPack(const std::uint8_t* bytes,
                                       std::size_t size) : bytes_(bytes) {
    if (bytes == nullptr || size < headerSize) return;
    if (bytes[0] != 'D' || bytes[1] != 'W' || bytes[2] != 'B' || bytes[3] != 'P') {
        error_ = BuilderPatternPackError::badMagic; return;
    }
    if (u16(bytes + 4) != formatVersion) {
        error_ = BuilderPatternPackError::unsupportedVersion; return;
    }
    if (u16(bytes + 6) != headerSize || u32(bytes + 8) != manifestSchema) {
        error_ = BuilderPatternPackError::badHeader; return;
    }
    if (u64(bytes + 20) != sourceHashPrefix) {
        error_ = BuilderPatternPackError::badSourceHash; return;
    }
    patternCount_ = u32(bytes + 12);
    cellCount_ = u32(bytes + 16);
    rowCount_ = u32(bytes + 28);
    patternsOffset_ = headerSize;
    if (patternCount_ > (size - patternsOffset_) / patternRecordSize) return;
    rowsOffset_ = patternsOffset_ + patternCount_ * patternRecordSize;
    if (rowCount_ > (size - rowsOffset_) / rowRecordSize) return;
    cellsOffset_ = rowsOffset_ + rowCount_ * rowRecordSize;
    if (cellCount_ != size - cellsOffset_) {
        error_ = BuilderPatternPackError::wrongSize; return;
    }
    error_ = BuilderPatternPackError::none;
    std::uint16_t nextStartIndex = 0;
    std::uint16_t nextMainIndex = 0;
    std::uint16_t nextRightIndex = 0;
    std::uint16_t nextShopIndex = 0;
    std::uint16_t nextEndIndex = 0;
    for (std::uint32_t id = 0; id < patternCount_; ++id) {
        PackedBuilderPattern value;
        pattern(id, value);
        bool ordered = false;
        if (value.family == BuilderPatternFamily::areaOneStart)
            ordered = value.sourceIndex == nextStartIndex++;
        if (value.family == BuilderPatternFamily::areaOneMain)
            ordered = value.sourceIndex == nextMainIndex++;
        if (value.family == BuilderPatternFamily::areaOneRight)
            ordered = value.sourceIndex == nextRightIndex++;
        if (value.family == BuilderPatternFamily::areaOneShop)
            ordered = value.sourceIndex == nextShopIndex++;
        if (value.family == BuilderPatternFamily::areaOneEnd)
            ordered = value.sourceIndex == nextEndIndex++;
        if (!ordered || value.width == 0 || value.rowCount == 0 ||
            !range(value.firstRow, value.rowCount, rowCount_) ||
            !range(value.firstCell, value.cellCount, cellCount_)) {
            error_ = BuilderPatternPackError::badPattern; return;
        }
        std::uint32_t summedCells = 0;
        for (std::uint32_t row = 0; row < value.rowCount; ++row) {
            const std::uint8_t* r = bytes_ + rowsOffset_ +
                (value.firstRow + row) * rowRecordSize;
            const std::uint32_t first = u32(r), count = u32(r + 4);
            if (count == 0 || count > value.width ||
                !range(first, count, cellCount_) ||
                first != value.firstCell + summedCells) {
                error_ = BuilderPatternPackError::badPattern; return;
            }
            summedCells += count;
        }
        if (summedCells != value.cellCount) {
            error_ = BuilderPatternPackError::badPattern; return;
        }
        for (std::uint32_t offset = 0; offset < value.cellCount; ++offset) {
            const std::uint8_t item = bytes_[cellsOffset_ + value.firstCell + offset];
            if (item < 0x20 || item > 0x7e) {
                error_ = BuilderPatternPackError::badPattern; return;
            }
        }
    }
}

bool BuilderPatternPack::pattern(std::uint32_t id,
                                 PackedBuilderPattern& out) const {
    if (!valid() || id >= patternCount_) return false;
    const std::uint8_t* p = bytes_ + patternsOffset_ + id * patternRecordSize;
    out = {static_cast<BuilderPatternFamily>(u16(p)), u16(p + 2), u16(p + 4),
           u16(p + 6), u32(p + 8), u32(p + 12), u32(p + 16)};
    return true;
}

bool BuilderPatternPack::findPattern(BuilderPatternFamily family,
                                     std::uint16_t sourceIndex,
                                     PackedBuilderPattern& out) const {
    for (std::uint32_t id = 0; id < patternCount_; ++id) {
        if (pattern(id, out) && out.family == family &&
            out.sourceIndex == sourceIndex)
            return true;
    }
    return false;
}

bool BuilderPatternPack::cell(const PackedBuilderPattern& value,
                              std::uint16_t x, std::uint16_t y,
                              char& out) const {
    if (!valid() || y >= value.rowCount ||
        !range(value.firstCell, value.cellCount, cellCount_))
        return false;
    const std::uint8_t* row = bytes_ + rowsOffset_ +
        (value.firstRow + y) * rowRecordSize;
    const std::uint32_t first = u32(row), width = u32(row + 4);
    if (x >= width) return false;
    out = static_cast<char>(bytes_[cellsOffset_ + first + x]);
    return true;
}

bool BuilderPatternPack::rowWidth(const PackedBuilderPattern& value,
                                  std::uint16_t y,
                                  std::uint32_t& out) const {
    if (!valid() || y >= value.rowCount ||
        !range(value.firstRow, value.rowCount, rowCount_)) return false;
    out = u32(bytes_ + rowsOffset_ +
              (value.firstRow + y) * rowRecordSize + 4);
    return true;
}

} // namespace downwell

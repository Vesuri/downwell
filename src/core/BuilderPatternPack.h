#ifndef DOWNWELL_CORE_BUILDER_PATTERN_PACK_H
#define DOWNWELL_CORE_BUILDER_PATTERN_PACK_H

#include <cstddef>
#include <cstdint>

namespace downwell {

enum class BuilderPatternPackError {
    none, tooShort, badMagic, unsupportedVersion, badHeader,
    badSourceHash, wrongSize, badPattern
};

enum class BuilderPatternFamily : std::uint16_t {
    areaOneStart = 1,
    areaOneMain = 2,
    areaOneRight = 3,
    areaOneShop = 4
};

struct PackedBuilderPattern {
    BuilderPatternFamily family;
    std::uint16_t sourceIndex, width, rowCount;
    std::uint32_t firstRow, firstCell, cellCount;
};

class BuilderPatternPack {
public:
    static constexpr std::uint16_t formatVersion = 3;
    static constexpr std::uint32_t manifestSchema = 3;
    static constexpr std::uint64_t sourceHashPrefix = 0xe00a94be5faf7dc0ULL;
    static constexpr std::size_t headerSize = 32;
    static constexpr std::size_t patternRecordSize = 20;
    static constexpr std::size_t rowRecordSize = 8;

    BuilderPatternPack(const std::uint8_t* bytes, std::size_t size);
    bool valid() const { return error_ == BuilderPatternPackError::none; }
    BuilderPatternPackError error() const { return error_; }
    std::uint32_t patternCount() const { return patternCount_; }
    std::uint32_t cellCount() const { return cellCount_; }
    std::uint32_t rowCount() const { return rowCount_; }
    bool pattern(std::uint32_t id, PackedBuilderPattern& output) const;
    bool findPattern(BuilderPatternFamily family, std::uint16_t sourceIndex,
                     PackedBuilderPattern& output) const;
    bool cell(const PackedBuilderPattern& pattern, std::uint16_t x,
              std::uint16_t y, char& output) const;
    bool rowWidth(const PackedBuilderPattern& pattern, std::uint16_t y,
                  std::uint32_t& output) const;

private:
    const std::uint8_t* bytes_ = nullptr;
    std::size_t patternsOffset_ = 0, rowsOffset_ = 0, cellsOffset_ = 0;
    std::uint32_t patternCount_ = 0, rowCount_ = 0, cellCount_ = 0;
    BuilderPatternPackError error_ = BuilderPatternPackError::tooShort;
};

} // namespace downwell
#endif

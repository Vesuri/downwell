#include "core/BuilderPatternPack.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
}

int main(int argc, char** argv) {
    require(argc == 2, "builder-pattern pack path is required");
    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    downwell::BuilderPatternPack pack(bytes.data(), bytes.size());
    require(pack.valid(), "generated builder-pattern pack validates");
    require(pack.patternCount() == 62 && pack.rowCount() == 737 &&
                pack.cellCount() == 8435,
            "recovered area-one pattern counts are pinned");

    downwell::PackedBuilderPattern pattern;
    require(pack.findPattern(downwell::BuilderPatternFamily::areaOneStart, 0,
                             pattern) &&
                pattern.width == 9 && pattern.rowCount == 7,
            "first start pattern metadata is exact");
    char cell = 0;
    require(pack.cell(pattern, 4, 0, cell) && cell == 'd',
            "first start pattern entrance marker is exact");
    require(pack.cell(pattern, 3, 1, cell) && cell == '-',
            "first start pattern platform marker is exact");
    require(!pack.cell(pattern, 9, 0, cell), "out-of-range columns are rejected");
    require(pack.findPattern(downwell::BuilderPatternFamily::areaOneStart, 4,
                             pattern) && pattern.rowCount == 6,
            "last recovered start pattern is addressable");
    require(!pack.findPattern(downwell::BuilderPatternFamily::areaOneStart, 5,
                              pattern),
            "unrecovered pattern indices are rejected");
    require(pack.findPattern(downwell::BuilderPatternFamily::areaOneMain, 0,
                             pattern) && pattern.width == 9 &&
                pattern.rowCount == 8,
            "first regular area-one fragment metadata is exact");
    require(pack.cell(pattern, 8, 0, cell) && cell == '1' &&
                pack.cell(pattern, 1, 2, cell) && cell == '%',
            "regular fragment wall and enemy markers are exact");
    require(pack.findPattern(downwell::BuilderPatternFamily::areaOneMain, 40,
                             pattern) && pattern.rowCount == 10,
            "last regular area-one fragment is addressable");
    require(!pack.findPattern(downwell::BuilderPatternFamily::areaOneMain, 41,
                              pattern),
            "out-of-range regular fragment indices are rejected");
    require(pack.findPattern(downwell::BuilderPatternFamily::areaOneRight, 0,
                             pattern) && pattern.width == 19 &&
                pattern.rowCount == 12,
            "first recovered right room preserves its wider layout");
    require(pack.cell(pattern, 10, 0, cell) && cell == '{' &&
                pack.cell(pattern, 9, 7, cell) && cell == 'O' &&
                pack.cell(pattern, 17, 6, cell) && cell == '@',
            "right-room doorway, opening, and occupant markers are exact");
    require(pack.findPattern(downwell::BuilderPatternFamily::areaOneRight, 9,
                             pattern) && pattern.rowCount == 18,
            "last recovered right room is addressable");
    require(pack.findPattern(downwell::BuilderPatternFamily::areaOneShop, 1,
                             pattern) && pattern.width == 21 &&
                pattern.rowCount == 18,
            "irregular recovered shop metadata is exact");
    std::uint32_t rowWidth = 0;
    require(pack.rowWidth(pattern, 13, rowWidth) && rowWidth == 21 &&
                pack.cell(pattern, 20, 13, cell) && cell == '.',
            "the anomalous long shop row is preserved verbatim");
    require(pack.rowWidth(pattern, 12, rowWidth) && rowWidth == 19 &&
                !pack.cell(pattern, 19, 12, cell),
            "shop row bounds follow each recovered row");
    require(pack.findPattern(downwell::BuilderPatternFamily::areaOneEnd, 0,
                             pattern) && pattern.width == 9 &&
                pattern.rowCount == 16 &&
                pack.cell(pattern, 4, 15, cell) && cell == '=',
            "first recovered area-one ending preserves its result marker");
    require(pack.findPattern(downwell::BuilderPatternFamily::areaOneEnd, 3,
                             pattern) && pattern.rowCount == 10,
            "last recovered area-one ending is addressable");

    for (unsigned id = 0; id < pack.patternCount(); ++id) {
        require(pack.pattern(id, pattern), "enumerate row-view pattern");
        for (unsigned y = 0; y < pattern.rowCount; ++y) {
            const std::uint8_t* row = nullptr;
            std::uint32_t width = 0, expectedWidth = 0;
            require(pack.rowData(pattern, y, row, width) && pack.rowWidth(pattern, y, expectedWidth) &&
                    width == expectedWidth, "row view preserves each serialized row width");
            for (unsigned x = 0; x < width; ++x)
                require(pack.cell(pattern, x, y, cell) && row[x] == static_cast<std::uint8_t>(cell),
                        "row view equals individual cell reads");
        }
        const std::uint8_t* row = nullptr;
        std::uint32_t width = 123;
        require(!pack.rowData(pattern, pattern.rowCount, row, width) && !row && width == 123,
                "invalid row leaves outputs untouched");
        auto invalid = pattern; invalid.firstRow = 0xffffffffu;
        require(!pack.rowData(invalid, 0, row, width), "row view rejects invalid row range");
        invalid = pattern; invalid.firstCell = 0xffffffffu;
        require(!pack.rowData(invalid, 0, row, width), "row view rejects invalid cell range");
        invalid = pattern; invalid.cellCount = 0;
        require(!pack.rowData(invalid, 0, row, width), "row view rejects cells outside pattern");
        invalid = pattern; invalid.width = 0;
        require(!pack.rowData(invalid, 0, row, width), "row view rejects excessive row width");
    }

    std::vector<std::uint8_t> damaged = bytes;
    damaged[40] = 0xff;
    damaged[41] = 0xff;
    damaged[42] = 0xff;
    damaged[43] = 0xff;
    downwell::BuilderPatternPack badRange(damaged.data(), damaged.size());
    require(badRange.error() == downwell::BuilderPatternPackError::badPattern,
            "cell ranges are validated before access");
    damaged = bytes;
    damaged.push_back(0);
    downwell::BuilderPatternPack trailing(damaged.data(), damaged.size());
    require(trailing.error() == downwell::BuilderPatternPackError::wrongSize,
            "trailing payload bytes are rejected");

    std::cout << "builder-pattern pack tests passed\n";
    return 0;
}

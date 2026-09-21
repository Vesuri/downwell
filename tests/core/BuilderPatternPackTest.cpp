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
    require(pack.patternCount() == 56 && pack.cellCount() == 7411,
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

#include "core/BuilderPatternPack.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

static void require(bool value) { if (!value) std::exit(1); }

int main(int argc, char** argv) {
    require(argc == 2);
    std::ifstream stream(argv[1], std::ios::binary);
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(stream)), {});
    using namespace downwell;
    const BuilderPatternPack pack(bytes.data(), bytes.size());
    require(pack.valid() && pack.patternCount() == 50 && pack.rowCount() == 722 && pack.cellCount() == 8078);
    require(!pack.preparedPillars(false) && !pack.preparedPillars(true));
    for (unsigned id = 0; id < pack.patternCount(); ++id) {
        PackedBuilderPattern p{}, found{};
        require(pack.pattern(id, p));
        require(pack.findPattern(p.family, p.sourceIndex, found) && found.firstRow == p.firstRow);
        std::cout << unsigned(p.family) << ' ' << p.sourceIndex << ' ' << p.width << ' ' << p.rowCount << '\n';
        for (unsigned y = 0; y < p.rowCount; ++y) {
            const std::uint8_t* row = nullptr;
            std::uint32_t width = 0, expectedWidth = 0;
            require(pack.rowData(p, y, row, width) && pack.rowWidth(p, y, expectedWidth) && width == expectedWidth);
            require(!pack.preparedRow(p, y, false) && !pack.preparedRow(p, y, true));
            for (unsigned x = 0; x < width; ++x) {
                char cell = 0;
                require(pack.cell(p, x, y, cell) && static_cast<unsigned char>(cell) == row[x]);
                std::cout << cell;
            }
            std::cout << '\n';
        }
        const std::uint8_t* row = nullptr;
        std::uint32_t width = 0;
        require(!pack.rowData(p, p.rowCount, row, width));
    }
    PackedBuilderPattern p{};
    require(!pack.findPattern(BuilderPatternFamily::areaOneMain, 0, p));
    require(!pack.findPattern(BuilderPatternFamily::areaTwoMain, 29, p));
    // Every truncation, as well as appended prepared data, must fail closed.
    for (std::size_t n = 0; n < bytes.size(); ++n)
        require(!BuilderPatternPack(bytes.data(), n).valid());
    auto invalid = bytes;
    invalid.push_back(255);
    require(!BuilderPatternPack(invalid.data(), invalid.size()).valid());
    for (auto change : {std::pair<unsigned, unsigned>{4, 4}, {32, 1}, {32, 12}, {34, 1}, {40, 255}}) {
        invalid = bytes;
        invalid[change.first] = change.second;
        require(!BuilderPatternPack(invalid.data(), invalid.size()).valid());
    }
}

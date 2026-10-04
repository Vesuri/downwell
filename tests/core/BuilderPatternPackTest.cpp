#include "core/BuilderPatternPack.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>
#include <string>

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

    auto checkLookup = [](const downwell::BuilderPatternPack& source) {
        for (unsigned family = 0; family <= 6; ++family) {
            for (unsigned index = 0; index <= 63; ++index) {
                downwell::PackedBuilderPattern expected{}, actual{};
                bool found = false;
                for (unsigned id = 0; id < source.patternCount(); ++id) {
                    if (source.pattern(id, expected) &&
                        static_cast<unsigned>(expected.family) == family &&
                        expected.sourceIndex == index) {
                        found = true;
                        break;
                    }
                }
                require(source.findPattern(static_cast<downwell::BuilderPatternFamily>(family),
                    index, actual) == found, "lookup agrees with full record enumeration");
                require(actual.family == expected.family && actual.sourceIndex == expected.sourceIndex &&
                    actual.width == expected.width && actual.rowCount == expected.rowCount &&
                    actual.firstRow == expected.firstRow && actual.firstCell == expected.firstCell &&
                    actual.cellCount == expected.cellCount,
                    "lookup preserves every output field on success and failure");
            }
        }
    };
    checkLookup(pack);
    checkLookup(downwell::BuilderPatternPack(nullptr, 0));

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
            for(bool reversed:{false,true}){
                // Expand the offline program to the original ordered draws,
                // Creates and event cells; validate every prepared grid word.
                std::vector<unsigned> expected,actual;
                auto create=[&](int x,bool platform,bool border){
                    expected.push_back(0x20000u|unsigned((x+512)/16)|(platform?128u:0u));
                    if(border)expected.push_back(0x30000u);
                };
                for(unsigned x=0;x<width;++x){
                    const char symbol=char(row[x]);
                    const int worldX=reversed?320-int(x+1)*16:160+int(x+1)*16;
                    if(std::string("abcdet%IJ^2B3vO_$\\=@").find(symbol)!=std::string::npos){
                        expected.push_back(0x10000u|(unsigned(symbol)<<8)|unsigned((worldX+512)/16));continue;
                    }
                    expected.push_back(0);
                    if(symbol=='1'||symbol=='|'||symbol=='-')create(worldX,symbol=='-',false);
                    if(symbol=='{'||symbol=='}')create(reversed?16:464,false,true);
                }
                if(y+1<pattern.rowCount){
                    expected.push_back(0);
                    const bool side=pattern.family==downwell::BuilderPatternFamily::areaOneRight||
                        pattern.family==downwell::BuilderPatternFamily::areaOneShop;
                    create(side?(reversed?0:480):(reversed?160:320),false,false);
                    create(reversed?320:160,false,false);
                }
                const auto* p=pack.preparedRow(pattern,y,reversed);
                require(p,"prepared row exists");
                while(*p!=255){
                    if(*p){actual.push_back(0x10000u|(unsigned(p[0])<<8)|p[1]);p+=2;continue;}
                    unsigned consumed=0, flags[128]={},walls[128]={},seen[128]={};
                    for(unsigned i=0;i<p[3];++i){
                        const unsigned item=p[6+i*2],n=p[7+i*2]&127;
                        actual.insert(actual.end(),n,0);consumed+=n;
                        actual.push_back(0x20000u|item);
                        if(p[7+i*2]&128)actual.push_back(0x30000u);
                        flags[item&127]|=(item&128)?34:19;
                        walls[item&127]+=(item&128)?0:1;
                    }
                    actual.insert(actual.end(),p[1]-consumed,0);
                    const auto* words=p+6+p[3]*2;
                    for(unsigned i=0;i<p[4];++i){
                        for(unsigned half=0;half<2;++half){
                            const unsigned col=words[i*4]+half,value=words[i*4+2+half];
                            if(!value)continue;
                            require(!seen[col]++ && flags[col]==(value&63)&&walls[col]==(value>>6),
                                    "prepared occupancy/contact words exactly match ordered Creates");
                        }
                    }
                    for(unsigned col=0;col<128;++col)require(bool(flags[col])==bool(seen[col]),"every landing column prepared once");
                    p=words+p[4]*4;
                }
                require(actual==expected,"prepared row preserves source order, draw counts and border effects");
            }
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
    checkLookup(badRange);
    damaged = bytes;
    damaged.push_back(0);
    downwell::BuilderPatternPack trailing(damaged.data(), damaged.size());
    require(trailing.error() == downwell::BuilderPatternPackError::wrongSize,
            "trailing payload bytes are rejected");

    std::cout << "builder-pattern pack tests passed\n";
    return 0;
}

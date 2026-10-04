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
    if (cellCount_ > size - cellsOffset_) {
        error_ = BuilderPatternPackError::wrongSize; return;
    }
    const std::size_t extension = cellsOffset_ + cellCount_;
    const std::size_t tableBytes = (std::size_t(rowCount_) * 2 + 3) * 4;
    if (size-extension < 4 || tableBytes > size-extension-4 ||
        bytes[extension]!='D' || bytes[extension+1]!='W' ||
        bytes[extension+2]!='P' || bytes[extension+3]!='R') {
        error_ = BuilderPatternPackError::wrongSize; return;
    }
    preparedOffsets_ = bytes + extension + 4;
    preparedPrograms_ = preparedOffsets_ + tableBytes;
    const std::size_t programBytes = size - (preparedPrograms_ - bytes);
    if (u32(preparedOffsets_ + tableBytes - 4) != programBytes) {
        error_ = BuilderPatternPackError::wrongSize; return;
    }
    if (u32(preparedOffsets_)) {
        error_ = BuilderPatternPackError::badPattern; return;
    }
    for (unsigned row=0; row<rowCount_*2+2; ++row) {
        const unsigned start=u32(preparedOffsets_+row*4), end=u32(preparedOffsets_+(row+1)*4);
        if (start>=end || end>programBytes) { error_=BuilderPatternPackError::badPattern; return; }
        const auto* p=preparedPrograms_+start;
        const auto* limit=preparedPrograms_+end;
        while(p<limit && *p!=255) {
            if (*p) {
                if (limit-p<2 || p[0]<32 || p[0]>126 || p[1]>=128) break;
                p+=2;
            } else {
                if (limit-p<6) break;
                const unsigned creates=p[3], words=p[4], length=6+creates*2+words*4;
                if (unsigned(limit-p)<length) break;
                unsigned draws=0, borders=0, platforms=0;
                for(unsigned i=0;i<creates;++i){
                    draws+=p[6+i*2+1]&127;
                    borders+=p[6+i*2+1]>>7;
                    platforms+=p[6+i*2]>>7;
                }
                if(draws>p[1] || borders!=p[2] || platforms!=p[5]) break;
                bool good=true;
                for(unsigned i=0;i<words;++i){
                    const auto* word=p+6+creates*2+i*4;
                    if(word[0]>=128 || (word[0]&1) || !word[1] || word[1]>3)good=false;
                    for(unsigned half=0;half<2;++half){
                        const unsigned value=word[2+half],flags=value&63,walls=value>>6;
                        if(bool(value)!=bool(word[1]&(1<<half)) ||
                           (value && flags!=19 && flags!=34 && flags!=51) ||
                           ((walls!=0)!=((flags&1)!=0)) || walls>creates)good=false;
                    }
                }
                if(!good)break;
                p+=length;
            }
        }
        if(p!=limit-1 || *p!=255){error_=BuilderPatternPackError::badPattern;return;}
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
    if (!valid()) return false;
    // Lookup only needs the two identifiers. Avoid decoding all row/cell
    // ranges for every preceding record on the 68000. The constructor has
    // already validated the immutable record table.
    const auto* record = bytes_ + patternsOffset_;
    for (std::uint32_t id = 0; id < patternCount_; ++id, record += patternRecordSize) {
        if (u16(record) == static_cast<std::uint16_t>(family) &&
            u16(record + 2) == sourceIndex)
            return pattern(id, out);
    }
    // Preserve the previous enumeration API's output on a failed lookup.
    if (patternCount_) pattern(patternCount_ - 1, out);
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

bool BuilderPatternPack::rowData(const PackedBuilderPattern& value,
                                 std::uint16_t y, const std::uint8_t*& out,
                                 std::uint32_t& width) const {
    if (!valid() || y >= value.rowCount ||
        !range(value.firstRow, value.rowCount, rowCount_) ||
        !range(value.firstCell, value.cellCount, cellCount_)) return false;
    const std::uint8_t* row = bytes_ + rowsOffset_ + (value.firstRow+y)*rowRecordSize;
    const std::uint32_t first = u32(row), count = u32(row+4);
    if (!count || count > value.width || first < value.firstCell ||
        !range(first-value.firstCell, count, value.cellCount)) return false;
    out = bytes_ + cellsOffset_ + first;
    width = count;
    return true;
}

const std::uint8_t* BuilderPatternPack::preparedRow(const PackedBuilderPattern& value,
                                                   unsigned row, bool reversed) const {
    if (!valid() || row>=value.rowCount || !range(value.firstRow,value.rowCount,rowCount_)) return nullptr;
    return preparedPrograms_+u32(preparedOffsets_+((value.firstRow+row)*2+unsigned(reversed))*4);
}

const std::uint8_t* BuilderPatternPack::preparedPillars(bool reversed) const {
    return valid()?preparedPrograms_+u32(preparedOffsets_+(rowCount_*2+unsigned(reversed))*4):nullptr;
}

} // namespace downwell

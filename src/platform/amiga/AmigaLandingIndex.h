#ifndef DOWNWELL_AMIGA_LANDING_INDEX_H
#define DOWNWELL_AMIGA_LANDING_INDEX_H
#include <cstdint>

// Candidate occupancy, not active-breakable state. Queries retain the old
// row+1/zero encoding; the world still checks whether a candidate is active.
class AmigaLandingIndex {
    std::uint16_t words_[128][32]{};
public:
    void clear() { for (auto& column : words_) for (auto& word : column) word=0; }
    void insert(unsigned column, unsigned row) {
        if (column<128 && row<512) words_[column][row>>4] |= 0x8000u>>(row&15);
    }
    // Prepared row publication: one address/mask setup, not one checked
    // insertion per terrain cell. Each record is even column, landing bits,
    // then two occupancy bytes (unused here).
    void insertPreparedWords(const std::uint8_t* source,unsigned count,unsigned row){
        const unsigned group=row>>4;
        const std::uint16_t bit=std::uint16_t(0x8000u>>(row&15));
        for(unsigned i=0;i<count;++i,source+=4){
            auto* word=&words_[source[0]][group];
            if(source[1]&1)*word|=bit;
            if(source[1]&2)word[32]|=bit;
        }
    }
    // One row across count columns stepping by step (±1); all must be valid.
    void insertSpan(unsigned column, int step, unsigned count, unsigned row) {
        if (row>=512) return;
        std::uint16_t* word=&words_[column][row>>4];
        const std::uint16_t bit=std::uint16_t(0x8000u>>(row&15));
        const int stride=step<0?-32:32;
        for (; count; --count, word+=stride) *word|=bit;
    }
    void remove(unsigned column, unsigned row) {
        if (column<128 && row<512) words_[column][row>>4] &= ~(0x8000u>>(row&15));
    }
    unsigned next(unsigned column, unsigned row) const {
        if (column>=128 || row>=512) return 0;
        unsigned word=row>>4;
        std::uint16_t bits=words_[column][word] & (0xffffu>>(row&15));
        while (!bits) {
            if (++word==32) return 0;
            bits=words_[column][word];
        }
        unsigned bit=0;
        if (!(bits&0xff00u)) { bits<<=8; bit+=8; }
        if (!(bits&0xf000u)) { bits<<=4; bit+=4; }
        if (!(bits&0xc000u)) { bits<<=2; bit+=2; }
        if (!(bits&0x8000u)) ++bit;
        return word*16+bit+1;
    }
    unsigned operator[](unsigned cell) const { return next(cell%128,cell/128); }
};
static_assert(sizeof(AmigaLandingIndex)==8192,"landing candidate storage budget");
#endif

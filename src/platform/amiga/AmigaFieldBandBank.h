#pragma once
#include <cstdint>

// Scene-owned immutable DMA source. Read directly into the existing Chip
// range; no permanently linked or temporary CPU copy. Invalidate before any
// other scene reuses the range, including when that scene's read fails.
class AmigaFieldBandBank {
    unsigned char* resident_=nullptr;
    unsigned bytes_=0;std::uint32_t checksum_=0;
public:
    void invalidate(){resident_=nullptr;}
    template<class Read> bool load(unsigned char* target,unsigned capacity,
        unsigned bytes,std::uint32_t expected,Read read){
        if(!target||!bytes||(bytes&1)||capacity<2||bytes>capacity-2)return false;
        if(resident_==target&&bytes_==bytes&&checksum_==expected)return true;
        invalidate();
        if(!read(target,bytes))return false;
        std::uint32_t checksum=0;
        for(unsigned i=0;i<bytes;i+=2)
            checksum=((checksum<<5)|(checksum>>27))^(unsigned(target[i])*256+target[i+1]);
        if(checksum!=expected)return false;
        target[bytes]=target[bytes+1]=0; // shifted blitter's extra fetch
        resident_=target;bytes_=bytes;checksum_=expected;return true;
    }
};

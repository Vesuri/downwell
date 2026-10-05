#ifndef DOWNWELL_AMIGA_INITIAL_TERRAIN_H
#define DOWNWELL_AMIGA_INITIAL_TERRAIN_H
#include <cstdint>
#include <cstring>

// Immutable host-composed rows. This owns no gameplay geometry or instances.
// Destination words use the retained raster's row/plane/word ordering.
class AmigaInitialTerrain {
    const std::uint8_t* references_=nullptr;
    const std::uint8_t* dictionary_=nullptr;
    unsigned rows_=0;
    static unsigned word(const std::uint8_t* p){return unsigned(p[0])*256+p[1];}
public:
    static constexpr unsigned columns=3,words=22,rowBytes=44;
    bool bind(const std::uint8_t* data,unsigned bytes){
        references_=dictionary_=nullptr;rows_=0;
        if(!data||bytes<12||data[0]!='D'||data[1]!='W'||data[2]!='I'||data[3]!='T'||
           word(data+4)!=1||word(data+10))return false;
        const unsigned rows=word(data+6),count=word(data+8);
        if(!rows||rows>8192||!count||bytes!=12+rows*6+count*rowBytes)return false;
        for(unsigned i=0;i<rows*columns;++i)if(word(data+12+i*2)>=count)return false;
        references_=data+12;dictionary_=references_+rows*6;rows_=rows;return true;
    }
    unsigned rows()const{return rows_;}
    bool copy(unsigned column,std::uint16_t* destination)const{
        if(!references_||column>=columns||!destination)return false;
        const auto* reference=references_+column*rows_*2;
        for(unsigned row=0;row<rows_;++row,reference+=2,destination+=words){
            const auto* source=dictionary_+word(reference)*rowBytes;
#ifdef DOWNWELL_PLATFORM_AMIGA
            std::memcpy(destination,source,rowBytes);
#else
            for(unsigned i=0;i<words;++i)destination[i]=std::uint16_t(word(source+i*2));
#endif
        }
        return true;
    }
};
#endif

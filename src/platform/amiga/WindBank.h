#ifndef DOWNWELL_AMIGA_WIND_BANK_H
#define DOWNWELL_AMIGA_WIND_BANK_H
#include <cstddef>
#include <cstdint>

namespace downwell {
// Borrowed Fast-memory PCM. Only the streaming driver's small DMA buffers may
// enter Chip RAM. Parse bytes explicitly: target alignment and endianness must
// not depend on a packed host structure.
class WindBank {
public:
    struct Sample { const std::uint8_t* bytes=nullptr;std::uint32_t length=0; };
    bool bind(const void* bytes,std::size_t size){
        data_=nullptr;size_=0;
        const auto* p=static_cast<const std::uint8_t*>(bytes);
        if(!p||size<104||size>2u*1024u*1024u||
           p[0]!='D'||p[1]!='W'||p[2]!='W'||p[3]!='A'||
           u16(p+4)!=1||u16(p+6)!=7||u32(p+8)!=22030||
           u32(p+12)!=161||u32(p+16)!=size)return false;
        std::uint32_t next=104;
        for(unsigned i=0;i<7;++i){
            const auto* entry=p+20+12*i;
            const auto offset=u32(entry+4),length=u32(entry+8);
            if(u32(entry)!=349+i||offset!=next||!length||
               offset>size||length>size-offset)return false;
            next=offset+length;
            if(length&1){if(next>=size||p[next]!=0)return false;++next;}
        }
        if(next!=size)return false;
        data_=p;size_=size;return true;
    }
    void reset(){data_=nullptr;size_=0;}
    bool ready()const{return data_!=nullptr;}
    std::size_t size()const{return size_;}
    bool sample(int resource,Sample& result)const{
        result={};
        if(!data_||resource<349||resource>355)return false;
        const auto* entry=data_+20+12*(resource-349);
        result={data_+u32(entry+4),u32(entry+8)};return true;
    }
    static unsigned period(){return 161;}
private:
    static std::uint16_t u16(const std::uint8_t* p){return (std::uint16_t(p[0])<<8)|p[1];}
    static std::uint32_t u32(const std::uint8_t* p){return (std::uint32_t(u16(p))<<16)|u16(p+2);}
    const std::uint8_t* data_=nullptr;
    std::size_t size_=0;
};
}
#endif

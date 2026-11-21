#ifndef DOWNWELL_AMIGA_MEMORY_COPY_H
#define DOWNWELL_AMIGA_MEMORY_COPY_H
#include <cstdint>
#include <cstddef>

// memcpy contract: non-overlapping ordinary memory. The 68000 requires even,
// not four-byte, alignment for longword accesses. Volatile accesses prevent
// a compiler from replacing this runtime primitive with a call to itself.
inline void* downwellMemoryCopy(void* destination,const void* source,std::size_t length){
    using Address=__UINTPTR_TYPE__;
    auto* out=static_cast<volatile unsigned char*>(destination);
    const auto* in=static_cast<const volatile unsigned char*>(source);
    if(!length)return destination;
    if((reinterpret_cast<Address>(out)^reinterpret_cast<Address>(in))&1u){
        do{*out++=*in++;}while(--length);
        return destination;
    }
    if(reinterpret_cast<Address>(out)&1u){*out++=*in++;--length;}
    typedef std::uint32_t Long __attribute__((aligned(2),may_alias));
    typedef std::uint16_t Word __attribute__((aligned(2),may_alias));
    while(length>=4){
        *reinterpret_cast<volatile Long*>(out)=*reinterpret_cast<const volatile Long*>(in);
        out+=4;in+=4;length-=4;
    }
    if(length>=2){
        *reinterpret_cast<volatile Word*>(out)=*reinterpret_cast<const volatile Word*>(in);
        out+=2;in+=2;length-=2;
    }
    if(length)*out=*in;
    return destination;
}
// memset contract. The toolchain's byte loop costs about 12 cycles a byte;
// -Os lowers every struct value-initialisation to a call, so longword stores
// (four per iteration) are what keeps cleared state cheap.
inline void* downwellMemoryFill(void* destination,int value,std::size_t length){
    using Address=__UINTPTR_TYPE__;
    auto* out=static_cast<volatile unsigned char*>(destination);
    const unsigned char byte=static_cast<unsigned char>(value);
    if(!length)return destination;
    if(reinterpret_cast<Address>(out)&1u){*out++=byte;--length;}
    typedef std::uint32_t Long __attribute__((aligned(2),may_alias));
    typedef std::uint16_t Word __attribute__((aligned(2),may_alias));
    const std::uint32_t pattern=0x01010101u*byte;
    while(length>=16){
        auto* longs=reinterpret_cast<volatile Long*>(out);
        longs[0]=pattern;longs[1]=pattern;longs[2]=pattern;longs[3]=pattern;
        out+=16;length-=16;
    }
    while(length>=4){*reinterpret_cast<volatile Long*>(out)=pattern;out+=4;length-=4;}
    if(length>=2){*reinterpret_cast<volatile Word*>(out)=static_cast<std::uint16_t>(pattern);out+=2;length-=2;}
    if(length)*out=byte;
    return destination;
}
#endif

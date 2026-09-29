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
#endif

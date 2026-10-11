#pragma once
#include <cstdint>
#include "../../../generated/downwell-field-columns.h"

// Borrow the scene's ordinary-memory lease. Publish only an exact, validated
// offline stream; no render-time loading or fallback to a linked catalogue.
class AmigaFieldColumnBank {
    const downwell::FieldColumnWord* words_=nullptr;
public:
    void invalidate(){words_=nullptr;}
    const downwell::FieldColumnWord* data()const{return words_;}
    template<class Read> bool load(unsigned char* target,unsigned capacity,Read read){
        if(!target||capacity!=downwell::fieldColumnBytes||
           (reinterpret_cast<__UINTPTR_TYPE__>(target)%alignof(downwell::FieldColumnWord)))return false;
        if(words_==reinterpret_cast<const downwell::FieldColumnWord*>(target))return true;
        invalidate();
        if(!read(target,capacity))return false;
        std::uint32_t checksum=0;
        for(unsigned i=0;i<capacity;i+=2)
            checksum=((checksum<<5)|(checksum>>27))^(unsigned(target[i])*256+target[i+1]);
        if(checksum!=downwell::fieldColumnChecksum)return false;
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        // Host fixtures consume the same serialized stream. The 68000 reads
        // native big-endian words directly, without this conversion.
        for(unsigned i=0;i<capacity;i+=2){const auto byte=target[i];target[i]=target[i+1];target[i+1]=byte;}
#endif
        words_=reinterpret_cast<const downwell::FieldColumnWord*>(target);return true;
    }
};

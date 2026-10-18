#ifndef DOWNWELL_AMIGA_INPUT_H
#define DOWNWELL_AMIGA_INPUT_H

#include "core/Input.h"
#include <cstdint>

struct Library;

class AmigaInput {
public:
    AmigaInput();
    ~AmigaInput();
    AmigaInput(const AmigaInput&)=delete;
    AmigaInput& operator=(const AmigaInput&)=delete;
    bool ready()const{return (allocated_&0x4000u)!=0;}
    downwell::Input sample()const;
    // RKM controller port 2: quadrature left/right, CIA fire, POTGOR pin 9.
    static downwell::Input decode(std::uint16_t joy,std::uint8_t cia,std::uint16_t pot){
        downwell::Input input;
        input.left=(joy&0x0200u)!=0;
        input.right=(joy&0x0002u)!=0;
        input.fire=(cia&0x80u)==0;
        input.cancel=(pot&0x4000u)==0;
        return input;
    }
private:
    Library* resource_=nullptr;
    std::uint16_t allocated_=0;
};

#endif

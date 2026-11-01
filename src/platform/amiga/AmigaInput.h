#ifndef DOWNWELL_AMIGA_INPUT_H
#define DOWNWELL_AMIGA_INPUT_H

#include "core/Input.h"
#include <cstdint>

class AmigaInput {
public:
    // Out of line: native route verifiers break on its entry.
    __attribute__((noinline)) static downwell::Input sample();
    // Controller port 2: quadrature left/right, CIA-A PRA bit 7 fire,
    // POTINP bit 14 (pin 9) second button, all active low.
    static downwell::Input decode(std::uint16_t joy,std::uint8_t cia,std::uint16_t pot){
        downwell::Input input;
        input.left=(joy&0x0200u)!=0;
        input.right=(joy&0x0002u)!=0;
        input.fire=(cia&0x80u)==0;
        input.cancel=(pot&0x4000u)==0;
        return input;
    }
};

#endif

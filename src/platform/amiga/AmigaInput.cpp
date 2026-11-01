#include "AmigaInput.h"

#include <exec/types.h>

namespace {
volatile UWORD* const joy1dat = (volatile UWORD*)0xdff00c;
volatile UBYTE* const ciaaPra = (volatile UBYTE*)0xbfe001;
volatile UWORD* const potinp = (volatile UWORD*)0xdff016;
}

downwell::Input AmigaInput::sample()
{
    // Read-only, as in Rescue on Fractalus: no POTGO writes, no OS resource.
    return decode(*joy1dat,*ciaaPra,*potinp);
}

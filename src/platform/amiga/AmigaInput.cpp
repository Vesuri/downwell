#include "AmigaInput.h"

#include <exec/types.h>
#include <proto/exec.h>
#define __NOLIBBASE__
#include <proto/potgo.h>
#undef __NOLIBBASE__

namespace {
volatile UWORD* const joy1dat = (volatile UWORD*)0xdff00c;
volatile UBYTE* const ciaaPra = (volatile UBYTE*)0xbfe001;
volatile UWORD* const potgor = (volatile UWORD*)0xdff016;
}

AmigaInput::AmigaInput()
{
    resource_=(Library*)OpenResource((CONST_STRPTR)"potgo.resource");
    if(!resource_)return;
    Library* const PotgoBase=resource_;
    // Lease only port 2 pin 9. Output-high provides the RKM button pull-up;
    // never infer a previous output latch from the button's readback value.
    allocated_=AllocPotBits(0xc000u);
    if(ready())WritePotgo(0xc000u,0xc000u);
}

AmigaInput::~AmigaInput()
{
    if(!ready())return;
    Library* const PotgoBase=resource_;
    WritePotgo(0,0xc000u);
    FreePotBits(allocated_);
}

downwell::Input AmigaInput::sample()const
{
    return decode(*joy1dat,*ciaaPra,*potgor);
}

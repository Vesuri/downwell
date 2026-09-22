#include "AmigaInput.h"

#include <exec/types.h>

namespace {
volatile UWORD* const joy1dat = (volatile UWORD*)0xdff00c;
volatile UBYTE* const ciaaPra = (volatile UBYTE*)0xbfe001;
}

downwell::Input AmigaInput::sample()
{
    const UWORD value = *joy1dat;
    const UWORD y1 = (value >> 9) & 1u;
    const UWORD x1 = (value >> 1) & 1u;
    downwell::Input input;

    /* JOY1DAT is a quadrature pair; these are its left/right switch levels. */
    input.left = y1 != 0;
    input.right = x1 != 0;
    input.fire = (*ciaaPra & 0x80u) == 0;
    return input;
}

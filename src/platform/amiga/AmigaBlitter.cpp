#include "framework/AmigaHardware.h"
#include "framework/Bitmap.h"

#include <exec/types.h>

extern "C" int amigaBlitterDrawPlanarMasked(
    const UBYTE* source, const UBYTE* mask,
    UWORD sourceWidth, UWORD sourceHeight, UWORD sourceDataWidth,
    UBYTE* destination, UWORD destinationHeight,
    UWORD destinationX, UWORD destinationY,
    UWORD sourceX, UWORD sourceY, UWORD width, UWORD height)
{
    if ((ULONG)source >= 0x200000UL ||
        (ULONG)mask >= 0x200000UL ||
        (ULONG)destination >= 0x200000UL)
        return 0;

    Bitmap sourceBitmap((void*)source, sourceWidth, sourceHeight, 2,
                        false, false, sourceDataWidth);
    Bitmap maskBitmap((void*)mask, sourceWidth, sourceHeight, 1,
                      false, false, sourceDataWidth);
    Bitmap destinationBitmap(destination, 380, destinationHeight, 2,
                             false, false, 384);
    destinationBitmap.copyWithMask(
        sourceBitmap, maskBitmap,
        destinationX, destinationY,
        sourceX, sourceY, sourceX, sourceY,
        width, height, false);
    while (AmigaHardware::isBlitterBusy() || AmigaHardware::hasQueuedBlits) {
    }
    return 1;
}

extern "C" int amigaBlitterClearPlanar(UBYTE* destination,
                                         UWORD height, UWORD planes)
{
    if ((ULONG)destination >= 0x200000UL || height == 0 || planes == 0)
        return 0;
    AmigaHardware::blitterClear(
        (UWORD*)destination, 384 / 16, (UWORD)(height * planes), 0);
    AmigaHardware::blitterWait();
    return 1;
}

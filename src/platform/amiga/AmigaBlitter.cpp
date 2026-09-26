#include "framework/AmigaHardware.h"
#include "framework/Bitmap.h"

#include <exec/types.h>
inline void* operator new(__SIZE_TYPE__, void* address) noexcept { return address; }

namespace {
struct PreparedBitmap {
    Bitmap source, mask;
    PreparedBitmap(const UBYTE* data, const UBYTE* maskData, UWORD width, UWORD height, UWORD stride)
        : source((void*)data, width, height, 2, false, false, stride),
          mask((void*)maskData, width, height, 1, false, false, stride) {}
};
alignas(PreparedBitmap) UBYTE artStorage[64 * sizeof(PreparedBitmap)];
bool artReady[64];
}

extern "C" int amigaBlitterDrawPlanarMasked(
    const UBYTE* source, const UBYTE* mask,
    UWORD sourceWidth, UWORD sourceHeight, UWORD sourceDataWidth,
    UBYTE* destination, UWORD destinationHeight,
    UWORD destinationX, UWORD destinationY,
    UWORD sourceX, UWORD sourceY, UWORD width, UWORD height, int outputPen)
{
    if ((ULONG)source >= 0x200000UL ||
        (ULONG)mask >= 0x200000UL ||
        (ULONG)destination >= 0x200000UL)
        return 0;

    const unsigned slot = ((ULONG)source >> 2) & 63;
    PreparedBitmap* art = reinterpret_cast<PreparedBitmap*>(artStorage + slot * sizeof(PreparedBitmap));
    if (!artReady[slot] || art->source.data != source || art->mask.data != mask ||
        art->source.width != sourceWidth || art->source.height != sourceHeight ||
        art->source.dataWidth != sourceDataWidth) {
        if (artReady[slot]) art->~PreparedBitmap();
        new (art) PreparedBitmap(source, mask, sourceWidth, sourceHeight, sourceDataWidth);
        artReady[slot] = true;
    }
    Bitmap destinationBitmap(destination, 380, destinationHeight, 2,
                             false, false, 384);
    if (outputPen >= 0) {
        for (unsigned plane = 0; plane < 2; ++plane) {
            Bitmap target(destination + plane * destinationBitmap.bitplaneSizeInBytes,
                380, destinationHeight, 1, false, false, 384);
            target.patternWithMask(art->mask, destinationX, destinationY,
                sourceX, sourceY, width, height, outputPen & (1u << plane) ? 0xffff : 0);
        }
    } else destinationBitmap.copyWithMask(
        art->source, art->mask,
        destinationX, destinationY,
        sourceX, sourceY, sourceX, sourceY,
        width, height, false);
    AmigaHardware::blitterDrain();
    return 1;
}

extern "C" int amigaBlitterClearPlanar(UBYTE* destination,
                                         UWORD height, UWORD planes)
{
    if ((ULONG)destination >= 0x200000UL || height == 0 || planes == 0)
        return 0;
    AmigaHardware::blitterClear(
        (UWORD*)destination, 384 / 16, (UWORD)(height * planes), 0);
    // Clear may be queued behind a terrain/display copy. Waiting only for
    // the active DMA does not finish queued work; subsequent CPU tile writes
    // would then be erased when the pending clear finally executes.
    AmigaHardware::blitterDrain();
    return 1;
}

extern "C" int amigaBlitterSolidRect(UBYTE* destination, UWORD targetHeight,
    UWORD x, UWORD y, UWORD width, UWORD height, UWORD pen)
{
    if ((ULONG)destination >= 0x200000UL) return 0;
    Bitmap bitmap(destination, 380, targetHeight, 2, false, false, 384);
    bitmap.solidRect(x, y, width, height, pen);
    AmigaHardware::blitterDrain();
    return 1;
}

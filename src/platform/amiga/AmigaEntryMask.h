#ifndef DOWNWELL_AMIGA_ENTRY_MASK_H
#define DOWNWELL_AMIGA_ENTRY_MASK_H
#include "core/EntryDitherMask.h"
// Offline opacity of sprEnterDither: select an opaque-black playfield pen,
// not a white sprite. One immutable 384-wide plane; the scrolling gameplay
// copper owns the DMA crop/modulo just as it does for the exit cover.
using AmigaEntryMask = downwell::EntryDitherMask;
#endif

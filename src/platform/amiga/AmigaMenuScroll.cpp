#include "AmigaMenuScroll.h"

namespace {

int clamp(int value, int minimum, int maximum) {
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return value;
}

std::uint32_t planeOffset(int viewX, int viewY) {
    return static_cast<std::uint32_t>(viewY - amigaMenuCacheWorldY) *
               amigaMenuCacheBytesPerRow +
           static_cast<std::uint32_t>((viewX >> 4) * 2);
}

std::uint32_t backgroundOffset(int viewX, int viewY) {
    const int top = clamp(viewY / 11, 0, amigaMenuCacheHeight - 284);
    return static_cast<std::uint32_t>(top) * amigaMenuCacheBytesPerRow +
           static_cast<std::uint32_t>((viewX >> 4) * 2);
}

} // namespace

AmigaMenuScrollState amigaMenuScrollState(int viewX, int viewY) {
    const int maximumX = amigaMenuCacheWidth - amigaMenuFetchWords * 16;
    const int maximumY = amigaMenuCacheWorldY + amigaMenuCacheHeight - 284;
    const int foregroundX = clamp(viewX, 0, maximumX);
    const int backgroundX = clamp(viewX / 6, 0, maximumX);
    const int top = clamp(viewY, amigaMenuCacheWorldY, maximumY);
    AmigaMenuScrollState state;
    state.backgroundVisibleLines = 284;
    state.foregroundOffset = planeOffset(foregroundX, top);
    state.backgroundOffset = backgroundOffset(backgroundX, viewY);
    /*
     * PFxH is a display delay, not a source-pixel offset.  With the extra
     * fetch word selected by DDFSTRT=$30, delay 15 is the word-aligned
     * position.  Decrease the delay while the view advances through a word,
     * then advance the pointer and wrap the delay back to 15.
     */
    state.scrollControl = static_cast<std::uint16_t>(
        (15 - (foregroundX & 15)) |
        ((15 - (backgroundX & 15)) << 4));
    return state;
}

std::uint32_t amigaMenuEntryOffset(int viewY) {
    const int top = clamp(viewY, 0,
                          amigaMenuEntryHeight - 284);
    return static_cast<std::uint32_t>(top) *
           amigaMenuEntryBytesPerRow;
}

AmigaMenuScrollState amigaMenuEntryScrollState(int viewX, int viewY) {
    AmigaMenuScrollState state = amigaMenuScrollState(viewX, viewY);
    // rmMenu objWell at (272,512): Draw paints black from y+64 down.
    state.backgroundVisibleLines = (unsigned)clamp(576 - viewY, 0, 284);
    const int x = clamp(viewX - amigaMenuEntryWorldX, 0,
                        amigaMenuEntryWidth - amigaMenuFetchWords * 16);
    state.foregroundOffset = amigaMenuEntryOffset(viewY) + (x >> 4) * 2;
    state.scrollControl = (state.scrollControl & 0xf0) | (15 - (x & 15));
    return state;
}

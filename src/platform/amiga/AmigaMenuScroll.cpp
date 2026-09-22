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
    state.foregroundOffset = planeOffset(foregroundX, top);
    state.backgroundOffset = backgroundOffset(backgroundX, viewY);
    state.scrollControl = static_cast<std::uint16_t>(
        (foregroundX & 15) | ((backgroundX & 15) << 4));
    return state;
}

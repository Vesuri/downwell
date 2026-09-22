#ifndef DOWNWELL_PLATFORM_AMIGA_MENU_SCROLL_H
#define DOWNWELL_PLATFORM_AMIGA_MENU_SCROLL_H

#include <cstdint>

enum {
    amigaMenuCacheWidth = 512,
    amigaMenuCacheHeight = 384,
    amigaMenuCacheWorldY = 284,
    amigaMenuCacheBytesPerRow = amigaMenuCacheWidth / 8,
    amigaMenuDisplayWidth = 160,
    amigaMenuFetchWords = 11,
    amigaMenuFetchBytes = amigaMenuFetchWords * 2,
    amigaMenuDisplayHStart = 0xd1,
    amigaMenuDisplayHStop = 0x171,
    amigaMenuFetchStart = 0x60,
    amigaMenuFetchStop = 0xb0,
    amigaMenuModulo = amigaMenuCacheBytesPerRow - amigaMenuFetchBytes
};

struct AmigaMenuScrollState {
    std::uint32_t foregroundOffset;
    std::uint32_t backgroundOffset;
    std::uint16_t bplcon1;
};

AmigaMenuScrollState amigaMenuScrollState(int viewX, int viewY);

#endif

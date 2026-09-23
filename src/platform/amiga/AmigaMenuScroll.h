#ifndef DOWNWELL_PLATFORM_AMIGA_MENU_SCROLL_H
#define DOWNWELL_PLATFORM_AMIGA_MENU_SCROLL_H

#include <cstdint>

enum {
    /*
     * The garden intro is deliberately not a dual playfield.  These are two
     * normal one-bit color planes: foreground on odd BPL1, background on even
     * BPL2.  BPLCON1 delays and bitplane pointers scroll them independently
     * inside a centered 160-pixel window, with one extra fetched word for
     * smooth scrolling.
     */
    amigaMenuCacheWidth = 512,
    amigaMenuCacheHeight = 384,
    amigaMenuCacheWorldY = 284,
    amigaMenuCacheBytesPerRow = amigaMenuCacheWidth / 8,
    amigaMenuDisplayWidth = 160,
    amigaMenuFetchWords = 11,
    amigaMenuFetchBytes = amigaMenuFetchWords * 2,
    amigaMenuDisplayHStart = 0xd1,
    amigaMenuDisplayHStop = 0x171,
    /*
     * The unscrolled 160-pixel fetch is $60..$a8.  Smooth scrolling keeps
     * DDFSTOP unchanged and starts one word earlier, placing the eleventh
     * fetched word to the left of the 160-pixel DIW.
     */
    amigaMenuFetchStart = 0x58,
    amigaMenuFetchStop = 0xa8,
    amigaMenuModulo = amigaMenuCacheBytesPerRow - amigaMenuFetchBytes,
    amigaMenuEntryWidth = 192,
    amigaMenuEntryHeight = 1152,
    amigaMenuEntryBytesPerRow = amigaMenuEntryWidth / 8,
    amigaMenuEntryPlaneBytes = amigaMenuEntryBytesPerRow *
                               amigaMenuEntryHeight,
    amigaMenuEntryWorldX = 240
};

struct AmigaMenuScrollState {
    std::uint32_t foregroundOffset;
    std::uint32_t backgroundOffset;
    std::uint16_t scrollControl;
};

AmigaMenuScrollState amigaMenuScrollState(int viewX, int viewY);
AmigaMenuScrollState amigaMenuEntryScrollState(int viewX, int viewY);
std::uint32_t amigaMenuEntryOffset(int viewY);

#endif

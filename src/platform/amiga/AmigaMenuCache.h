#ifndef DOWNWELL_PLATFORM_AMIGA_MENU_CACHE_H
#define DOWNWELL_PLATFORM_AMIGA_MENU_CACHE_H

#include <exec/types.h>

struct AmigaMenuDrawList;

UWORD* amigaMenuHardwareSprite(const UBYTE* pack, const UBYTE* end,
                              UWORD sprite, UWORD frame, bool flip,
                              unsigned buffer, int x, int y);

bool amigaRenderMenuCache(const AmigaMenuDrawList& list,
                          const UBYTE* pack, const UBYTE* packEnd,
                          UBYTE* foreground, UBYTE* background,
                          UBYTE* entry);

bool amigaRenderMenuTitle(UWORD frame,
                          const UBYTE* pack, const UBYTE* packEnd,
                          UBYTE* foreground, UBYTE* entry);

#endif

#ifndef DOWNWELL_PLATFORM_AMIGA_MENU_CACHE_H
#define DOWNWELL_PLATFORM_AMIGA_MENU_CACHE_H

#include <exec/types.h>

struct AmigaMenuDrawList;

bool amigaRenderMenuCache(const AmigaMenuDrawList& list,
                          const UBYTE* pack, const UBYTE* packEnd,
                          UBYTE* foreground, UBYTE* background);

#endif

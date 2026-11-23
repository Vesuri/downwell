#ifndef DOWNWELL_AMIGA_HEALTH_ART_H
#define DOWNWELL_AMIGA_HEALTH_ART_H
#include "AmigaRenderer.h"
#include "core/InstalledTables.h"

struct AmigaHealthPair { UWORD low,high; };
namespace AmigaHealthArt {
#ifdef DOWNWELL_INSTALLED_TABLES
#include "../../../generated/installed/amiga-health-art.h" // WI.3: bound from Downwell.tables
#else
#include "../../../generated/amiga-health-art.h"
#endif

// Immutable packs are admitted once on binding. Compare the exact source
// directory records and payloads, not a collision-prone hash or resource IDs.
inline bool matches(const UBYTE* pack,const UBYTE* end){
#ifdef DOWNWELL_REFERENCE_HEALTH_ART
    (void)pack;(void)end;return false;
#else
    if(!pack||!end||end<pack||unsigned(end-pack)<minimumBytes)return false;
    for(unsigned s=0;s<DOWNWELL_TABLE_COUNT(spans);++s){
        const auto& span=spans[s];
        for(unsigned i=0;i<span.size;++i)
            if(pack[span.offset+i]!=span.bytes[i])return false;
    }
    return true;
#endif
}
}
#endif

#ifndef DOWNWELL_AMIGA_HEALTH_ART_H
#define DOWNWELL_AMIGA_HEALTH_ART_H
#include "AmigaRenderer.h"

struct AmigaHealthPair { UWORD low,high; };
namespace AmigaHealthArt {
#include "../../../generated/amiga-health-art.h"

// Immutable packs are admitted once on binding. Compare the exact source
// directory records and payloads, not a collision-prone hash or resource IDs.
inline bool matches(const UBYTE* pack,const UBYTE* end){
#ifdef DOWNWELL_REFERENCE_HEALTH_ART
    (void)pack;(void)end;return false;
#else
    if(!pack||!end||end<pack||unsigned(end-pack)<minimumBytes)return false;
    for(const auto& span:spans)
        for(unsigned i=0;i<span.size;++i)
            if(pack[span.offset+i]!=span.bytes[i])return false;
    return true;
#endif
}
}
#endif

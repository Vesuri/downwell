#ifndef DOWNWELL_AMIGA_OVERFLOW_PROFILE_ENTRY_H
#define DOWNWELL_AMIGA_OVERFLOW_PROFILE_ENTRY_H
#include "AmigaTerrain.h"

// Profiling-only seed-8 entry, before the entrance Create consumes its radius.
// Verified against ordinary decoded startup by the currency-promotion test.
inline bool amigaOverflowProfileEntry(AmigaAreaOneTerrainBuilder& builder,
                                      downwell::GameMakerRandom& random){
    random.setSeed(8);
    for(unsigned i=0;i<334;++i)random.nextU32();
    downwell::AreaOneBuilder initial;
    initial.restoreInitialCheckpoint(240,704,-30,13,4);
    return builder.resetFromRoomEntry(initial);
}
#endif

#ifndef DOWNWELL_AMIGA_OVERFLOW_PROFILE_ENTRY_H
#define DOWNWELL_AMIGA_OVERFLOW_PROFILE_ENTRY_H
#include "AmigaTerrain.h"

constexpr unsigned amigaOverflowSeed=40;
constexpr unsigned amigaOverflowRoute=21;
// Profiling-only entry, before the entrance Create consumes its radius.
// Verified against ordinary decoded startup by the currency-promotion test.
inline bool amigaOverflowProfileEntry(AmigaAreaOneTerrainBuilder& builder,
                                      downwell::GameMakerRandom& random){
    random.setSeed(amigaOverflowSeed);
    for(unsigned i=0;i<338;++i)random.nextU32();
    downwell::AreaOneBuilder initial;
    initial.restoreInitialCheckpoint(-16,704,-30,3,12);
    return builder.resetFromRoomEntry(initial);
}
#endif

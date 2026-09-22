#ifndef DOWNWELL_AMIGA_TERRAIN_H
#define DOWNWELL_AMIGA_TERRAIN_H

#include "core/BuilderPatternPack.h"
#include "core/RoomPack.h"

class AmigaPortRuntime;

enum {
    amigaInitialTerrainFragmentCount = 5,
    amigaTerrainBreakableCapacity = 96
};

struct AmigaTerrainFragment {
    downwell::PackedBuilderPattern pattern;
    int y;
    bool reversed;
    bool sideRoom;
    bool appendPillars;
};

struct AmigaTerrainCollisionContext {
    const downwell::RoomPack* rooms;
    const downwell::PackedRoom* room;
    const downwell::BuilderPatternPack* builders;
    const AmigaTerrainFragment* fragments;
    unsigned fragmentCount;
    const AmigaPortRuntime* runtime;
};

bool amigaBuildInitialAreaOneTerrain(
    const downwell::BuilderPatternPack& builders,
    AmigaTerrainFragment* fragments, unsigned capacity);

unsigned amigaCollectTerrainBreakables(
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments, unsigned fragmentCount,
    int* x, int* y, unsigned capacity);

bool amigaIsLandingTerrainSymbol(char symbol);
int amigaResolveTerrainLanding(void* opaque, int offsetX,
                               int minimumOffsetY);
bool amigaResolveTerrainSolid(void* opaque, int offsetX, int offsetY);

#endif

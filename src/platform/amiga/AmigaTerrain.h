#ifndef DOWNWELL_AMIGA_TERRAIN_H
#define DOWNWELL_AMIGA_TERRAIN_H

#include "core/BuilderPatternPack.h"
#include "core/GameMakerRandom.h"
#include "core/RoomPack.h"

class AmigaPortRuntime;

enum {
    amigaInitialTerrainFragmentCount = 19,
    amigaTerrainBreakableCapacity = 128,
    amigaTerrainDecorationCapacity = 128
};

struct AmigaTerrainFragment {
    downwell::PackedBuilderPattern pattern;
    int y;
    bool reversed;
    bool sideRoom;
    bool appendPillars;
};

struct AmigaTerrainEnemy {
    int objectId;
    int x;
    int y;
    int speedX;
    int speedY;
    int direction;
    int moveSpeed;
    int motionState;
    int auxiliaryState;
    int alarm0;
    int alarm1;
};

struct AmigaTerrainDecoration {
    int objectId;
    int spriteId;
    int x;
    int y;
    int frame;
    bool flipX;
};

struct AmigaTerrainCollisionContext {
    const downwell::RoomPack* rooms;
    const downwell::PackedRoom* room;
    const downwell::BuilderPatternPack* builders;
    const AmigaTerrainFragment* fragments;
    unsigned fragmentCount;
    const AmigaPortRuntime* runtime;
    const struct AmigaTerrainIndex* index = nullptr;
};

struct AmigaTerrainIndex {
    enum { columns = 128, rows = 512, minX = -512 };
    unsigned char cells[columns * rows];
    std::uint16_t nextLanding[columns * rows]; // row+1; zero means no candidate
};
bool amigaBuildTerrainIndex(const AmigaTerrainCollisionContext& context,
                           AmigaTerrainIndex& index);
bool amigaIndexedRoomWall(const AmigaTerrainIndex& index, int x, int y);

bool amigaBuildInitialAreaOneTerrain(
    const downwell::BuilderPatternPack& builders,
    AmigaTerrainFragment* fragments, unsigned capacity);
bool amigaBuildInitialAreaOneTerrain(
    const downwell::BuilderPatternPack& builders,
    AmigaTerrainFragment* fragments, unsigned capacity,
    AmigaTerrainEnemy* enemies, unsigned enemyCapacity,
    unsigned& enemyCount,
    downwell::GameMakerRandom* randomAfter = 0);
bool amigaBuildInitialAreaOneTerrain(
    const downwell::BuilderPatternPack& builders,
    AmigaTerrainFragment* fragments, unsigned capacity,
    AmigaTerrainEnemy* enemies, unsigned enemyCapacity,
    unsigned& enemyCount,
    downwell::GameMakerRandom* randomAfter,
    AmigaTerrainDecoration* decorations,
    unsigned decorationCapacity,
    unsigned* decorationCount);

int amigaEnemyHealth(int objectId);
int amigaEnemyReward(int objectId);

unsigned amigaCollectTerrainBreakables(
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments, unsigned fragmentCount,
    int* x, int* y, unsigned capacity);

bool amigaIsLandingTerrainSymbol(char symbol);
int amigaResolveTerrainLanding(void* opaque, int offsetX,
                               int minimumOffsetY);
bool amigaResolveTerrainSolid(void* opaque, int offsetX, int offsetY);

#endif

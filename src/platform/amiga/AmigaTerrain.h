#ifndef DOWNWELL_AMIGA_TERRAIN_H
#define DOWNWELL_AMIGA_TERRAIN_H

#include "core/BuilderPatternPack.h"
#include "core/AreaOneBuilder.h"
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
    bool grounded;
    bool active;
};

struct AmigaTerrainDecoration {
    int objectId;
    int spriteId;
    int x;
    int y;
    int frame;
    bool flipX;
};

// Allocation-free live generator. Storage is owned by the caller; the random
// stream is supplied on every tick so enemy events and generation share it.
// A failed emission latches failure: partial Create events must not be retried.
class AmigaAreaOneTerrainBuilder {
public:
    AmigaAreaOneTerrainBuilder(
        AmigaTerrainFragment* fragments, unsigned fragmentCapacity,
        AmigaTerrainEnemy* enemies = nullptr, unsigned enemyCapacity = 0,
        AmigaTerrainDecoration* decorations = nullptr,
        unsigned decorationCapacity = 0);
    void resetInitialCheckpoint(downwell::GameMakerRandom& random);
    bool stepVisible(const downwell::BuilderPatternPack& patterns,
                     downwell::GameMakerRandom& random, int viewBottomCeiling,
                     downwell::AreaOneBuilderStep& result);
    unsigned fragmentCount() const { return fragmentCount_; }
    unsigned enemyCount() const { return enemyCount_; }
    unsigned decorationCount() const { return decorationCount_; }
    int frontierY() const { return builder_.y(); }
    bool endReached() const { return builder_.endReached(); }
    bool failed() const { return failed_; }

private:
    static bool collect(void*, const downwell::DeferredCreateInstance&);
    downwell::AreaOneBuilder builder_;
    AmigaTerrainFragment* fragments_;
    unsigned fragmentCapacity_;
    AmigaTerrainEnemy* enemies_;
    unsigned enemyCapacity_;
    AmigaTerrainDecoration* decorations_;
    unsigned decorationCapacity_;
    unsigned fragmentCount_ = 0;
    unsigned enemyCount_ = 0;
    unsigned decorationCount_ = 0;
    bool failed_ = false;
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

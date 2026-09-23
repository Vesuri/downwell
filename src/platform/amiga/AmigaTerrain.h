#ifndef DOWNWELL_AMIGA_TERRAIN_H
#define DOWNWELL_AMIGA_TERRAIN_H

#include "core/BuilderPatternPack.h"
#include "core/AreaOneBuilder.h"
#include "core/GameMakerRandom.h"
#include "core/RoomPack.h"

class AmigaPortRuntime;

enum {
    amigaInitialTerrainFragmentCount = 19,
    amigaTerrainBreakableCapacity = 512,
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

// Non-wall tile presentation from actual Create events. Row lists preserve
// emission order without scanning the whole level for each newly exposed row.
class AmigaTerrainTiles {
public:
    enum { capacity = 2048, rows = 512 };
    struct Tile { std::int16_t x; std::uint16_t objectId, frame, next; };
    void clear();
    bool append(const downwell::DeferredCreateInstance& command);
    unsigned first(int worldY) const {
        return worldY >= 0 && worldY < rows * 16 && !(worldY & 15)
                   ? heads_[worldY / 16] : 0;
    }
    const Tile& at(unsigned id) const { return tiles_[id - 1]; }
    unsigned size() const { return count_; }
private:
    Tile tiles_[capacity];
    std::uint16_t heads_[rows] = {}, tails_[rows] = {};
    unsigned count_ = 0;
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
    void setCreateObserver(downwell::AreaOneBuilder::EmitFunction observer,
                           void* context) {
        observer_ = observer;
        observerContext_ = context;
    }
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
    downwell::AreaOneBuilder::EmitFunction observer_ = nullptr;
    void* observerContext_ = nullptr;
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
// Add an actual recovered Create event, not a probabilistic pattern symbol.
// Requires an initialized index. Only links above the new landing are patched.
bool amigaAddTerrainObject(AmigaTerrainIndex& index, int objectId, int x, int y,
                           bool initialRoomWall = false);
bool amigaIndexedRoomWall(const AmigaTerrainIndex& index, int x, int y);
bool amigaIndexedWall(const AmigaTerrainIndex& index, int x, int y);

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
    unsigned* decorationCount,
    downwell::AreaOneBuilder::EmitFunction observer = nullptr,
    void* observerContext = nullptr);

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

#ifndef DOWNWELL_AMIGA_TERRAIN_H
#define DOWNWELL_AMIGA_TERRAIN_H

#include "core/BuilderPatternPack.h"
#include "core/AreaOneBuilder.h"
#include "core/GameMakerRandom.h"
#include "core/RoomPack.h"
#include "core/TerrainWallContacts.h"
#include "AmigaLandingIndex.h"

class GameplayWorld;
using AmigaPortRuntime = GameplayWorld;
namespace downwell { struct ProjectileSegment; }

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
    bool appendCreatedTile(int objectId,int x,int y,unsigned frame);
    unsigned first(int worldY) const {
        return worldY >= 0 && worldY < rows * 16 && !(worldY & 15)
                   ? heads_[worldY / 16] : 0;
    }
    const Tile& at(unsigned id) const { return tiles_[id - 1]; }
    unsigned size() const { return count_; }
    // Same records and row order as appending each platform (93) Create.
    bool appendPlatforms(int x, int y, int stepX, unsigned count);
    bool appendPrepared(const downwell::AreaOneBuilder::PreparedTerrain&);
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
        runObserver_ = nullptr;
        preparedObserver_ = nullptr;
        preparedPublisher_ = nullptr;
        breakablePublisher_ = nullptr;
    }
    using RunObserver = bool (*)(void*,const downwell::AreaOneBuilder::TerrainRun&);
    void setTerrainRunObserver(RunObserver observer){runObserver_=observer;}
    using PreparedObserver=bool (*)(void*,const downwell::AreaOneBuilder::PreparedTerrain&);
    void setPreparedTerrainObserver(PreparedObserver observer){preparedObserver_=observer;}
    // Fixed terrain has no collected actor/decor outputs. A directly bound
    // publisher bypasses the collection adapter; stepVisible still latches
    // any failed publication before another generation step can run.
    void setPreparedTerrainPublisher(downwell::AreaOneBuilder::PreparedTerrainFunction publisher){preparedPublisher_=publisher;}
    void setBreakablePublisher(downwell::AreaOneBuilder::BreakableEmitter publisher){breakablePublisher_=publisher;}
    void resetInitialCheckpoint(downwell::GameMakerRandom& random);
    void resetInitialCheckpointBeforeEntrance(downwell::GameMakerRandom& random);
    // Import simulation state without replaying Create or consuming RNG.
    // The caller transfers the matching RNG separately. Existing terrain is
    // not imported; use only before the first generated pattern in a room.
    bool resetFromRoomEntry(const downwell::AreaOneBuilder& builder);
    // New-run entry uses the source Create logic, not the seed-2 startup capture.
    void resetRoom(int x, int y, downwell::GameMakerRandom& random);
    void resetRoom(int x, int y, downwell::GameMakerRandom& random,
                   const downwell::AreaOneBuilder::Setup& setup);
    bool stepVisible(const downwell::BuilderPatternPack& patterns,
                     downwell::GameMakerRandom& random, int viewBottomCeiling,
                     downwell::AreaOneBuilderStep& result);
    unsigned fragmentCount() const { return fragmentCount_; }
    void setPlayerHealth(int health) { builder_.setPlayerHealth(health); }
    void setPositionPublisher(downwell::AreaOneBuilder::PositionPublisher publish,void* context){
        builder_.setPositionPublisher(publish,context);
    }
    unsigned enemyCount() const { return enemyCount_; }
    unsigned decorationCount() const { return decorationCount_; }
    int frontierY() const { return builder_.y(); }
    bool endReached() const { return builder_.endReached(); }
    bool failed() const { return failed_; }

private:
    static bool collect(void*, const downwell::DeferredCreateInstance&);
    static bool collectRun(void*,const downwell::AreaOneBuilder::TerrainRun&,downwell::AreaOneBuilder::EmitFunction);
    static bool collectPrepared(void*,const downwell::AreaOneBuilder::PreparedTerrain&,downwell::AreaOneBuilder::EmitFunction);
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
    RunObserver runObserver_ = nullptr;
    PreparedObserver preparedObserver_ = nullptr;
    downwell::AreaOneBuilder::PreparedTerrainFunction preparedPublisher_=nullptr;
    downwell::AreaOneBuilder::BreakableEmitter breakablePublisher_=nullptr;
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
    enum { columns = 128, rows = 512, minX = -512, wallAppearanceFlags = 24 };
    // Occupancy flags in bits 0-5. Bits 6-7 count coincident wall (60)
    // Creates up to three; the breakable flag stands for the first box (92).
    unsigned char cells[columns * rows];
    AmigaLandingIndex nextLanding; // candidate bits; row+1/zero query encoding
    // Contacts a cell cannot count: further coincident walls and boxes, boxes
    // without a breakable flag, and every object 293.
    downwell::TerrainWallContacts<256> extraContacts;
    bool wallContactsComplete = false;
    // Subtraction is safe only when occupancy came from actual Create events.
    bool mutableWallOccupancy = false;
};
// Borrowed same-row backend setup, with no publication during admission.
// Each append still publishes exactly one cell before that cell's core Create.
class AmigaTerrainRunIndex {
public:
    AmigaTerrainRunIndex(AmigaTerrainIndex&,int object,int x,int y,int stepX,unsigned count);
    bool ready()const{return remaining_!=0;}
    bool append();
    // Whether every remaining cell can publish (overflow contacts included),
    // so publishSpan cannot fail part-way. Platforms also need tile capacity.
    bool admitSpan(const AmigaTerrainTiles& tiles)const;
    // Publishes all remaining cells in order; requires admitSpan.
    bool publishSpan(AmigaTerrainTiles& tiles);
private:
    AmigaTerrainIndex& index_;
    int object_,x_,y_,step_;
    unsigned row_=0,column_=0,flags_=0,remaining_=0;
};
bool amigaBuildTerrainIndex(const AmigaTerrainCollisionContext& context,
                           AmigaTerrainIndex& index);
// Retained generic reconstruction oracle, including empty-world scans.
bool amigaReconstructTerrainIndex(const AmigaTerrainCollisionContext& context,
                                 AmigaTerrainIndex& index);
// Add an actual recovered Create event, not a probabilistic pattern symbol.
// Requires an initialized index. Candidate insertion does not scan earlier rows.
bool amigaAddTerrainObject(AmigaTerrainIndex& index, int objectId, int x, int y,
                           bool initialRoomWall = false);
bool amigaAdmitPreparedTerrain(const AmigaTerrainIndex&,const AmigaTerrainTiles&,
                              const downwell::AreaOneBuilder::PreparedTerrain&);
bool amigaPublishPreparedTerrain(AmigaTerrainIndex&,AmigaTerrainTiles&,
                                const downwell::AreaOneBuilder::PreparedTerrain&);
bool amigaIndexedRoomWall(const AmigaTerrainIndex& index, int x, int y);
// Remove one collision contribution, not the authoritative instance lifetime.
bool amigaRemoveTerrainWall(AmigaTerrainIndex& index, int x, int y);
bool amigaIndexedWall(const AmigaTerrainIndex& index, int x, int y);
// Contact multiplicity at one Create position, and over the whole index.
// Diagnostic views that scan; gameplay queries use the contact resolver.
unsigned amigaTerrainContacts(const AmigaTerrainIndex& index, int objectId, int x, int y);
unsigned amigaTerrainContactCount(const AmigaTerrainIndex& index);

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
bool amigaResolveTerrainPlayerMask(void* opaque, int offsetX, int offsetY);
bool amigaResolveTerrainSegment(void* opaque, const downwell::ProjectileSegment& path);
bool amigaResolveTerrainWallContacts(void* opaque,const downwell::ContactBounds& query,unsigned& count);
// Object 93 only, not every member of parentThinwall (87).
bool amigaResolveTerrainPlatformContact(void* opaque,const downwell::ContactBounds& query,bool& hit);

#endif

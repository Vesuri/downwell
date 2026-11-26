#include "AmigaTerrain.h"
#include "AmigaProfile.h"

#include "AmigaPortRuntime.h"
#include "core/AreaOneBuilder.h"
#include "core/GameMakerRandom.h"
#include "core/GameplayLimits.h"
#include "core/EnemyCreateProfile.h"

void AmigaTerrainTiles::clear() {
    for (unsigned row = 0; row < rows; ++row) heads_[row] = tails_[row] = 0;
    count_ = 0;
}

bool AmigaTerrainTiles::append(const downwell::DeferredCreateInstance& command) {
    DW_GENERATION_SCOPE(profileGeneratedTiles);
    if (command.objectId != 92 && command.objectId != 93) return true;
    return appendCreatedTile(command.objectId,command.x,command.y,
        command.objectId==93?23:command.integerImageIndex);
}

bool AmigaTerrainTiles::appendCreatedTile(int objectId,int x,int y,unsigned frame){
    if (count_ == capacity || y < 0 || y >= rows * 16 ||
        (y & 15) || x < -512 || x >= 1536 || (x & 15))
        return false;
    const unsigned row = y / 16;
    const unsigned id = ++count_;
    tiles_[id - 1] = Tile{static_cast<std::int16_t>(x),
        static_cast<std::uint16_t>(objectId),static_cast<std::uint16_t>(frame),0};
    if (tails_[row]) tiles_[tails_[row] - 1].next = id;
    else heads_[row] = id;
    tails_[row] = id;
    return true;
}

bool amigaIsLandingTerrainSymbol(char symbol) {
    return symbol == '1' || symbol == '2' || symbol == '3' ||
           symbol == 'B' || symbol == '-' || symbol == '|';
}

namespace {
bool isSolidTerrainSymbol(char symbol) {
    return symbol != '-' && amigaIsLandingTerrainSymbol(symbol);
}
}

bool amigaBuildInitialAreaOneTerrain(
    const downwell::BuilderPatternPack& builders,
    AmigaTerrainFragment* fragments, unsigned capacity) {
    unsigned ignoredEnemyCount = 0;
    return amigaBuildInitialAreaOneTerrain(
        builders, fragments, capacity, 0, 0, ignoredEnemyCount);
}

namespace {
int decorationSprite(int objectId) {
    if (objectId == 289) return 550;
    if (objectId == 293) return 569;
    if (objectId == 296) return 568;
    // W1.4: GunModule is a falling compact record drawn by the draw list;
    // only fixtures predating it keep the static painted battery.
    if (objectId == 109) return GameplayWorld::compactGunsAdmitted ? -1 : 139;
    // objLineToResult's sprite supplies a mask, but Visible is false.
    return -1;
}
}

bool AmigaTerrainTiles::appendPlatforms(int x, int y, int stepX, unsigned count) {
    if (count > capacity - count_ || y < 0 || y >= rows * 16 || (y & 15)) return false;
    const unsigned row = unsigned(y) >> 4;
    for (unsigned cell = 0; cell < count; ++cell, x += stepX) {
        if (x < -512 || x >= 1536 || (x & 15)) return false;
        const unsigned id = ++count_;
        tiles_[id - 1] = Tile{static_cast<std::int16_t>(x), 93, 23, 0};
        if (tails_[row]) tiles_[tails_[row] - 1].next = id;
        else heads_[row] = id;
        tails_[row] = id;
    }
    return true;
}
bool AmigaTerrainTiles::appendPrepared(const downwell::AreaOneBuilder::PreparedTerrain& batch){
    if(!batch.platforms())return true;
    if(batch.platforms()>capacity-count_ || batch.y<0 || batch.y>=rows*16 || (batch.y&15))return false;
    const unsigned row=unsigned(batch.y)>>4;
    unsigned tail=tails_[row];
    const auto* p=batch.creates();
    for(unsigned i=0;i<batch.count();++i,p+=2)if(p[0]&128){
        const unsigned id=++count_;
        tiles_[id-1]=Tile{std::int16_t((int(p[0]&127)-32)*16),93,23,0};
        if(tail)tiles_[tail-1].next=id;else heads_[row]=id;
        tail=id;
    }
    tails_[row]=tail;
    return true;
}
bool AmigaAreaOneTerrainBuilder::collect(
    void* opaque, const downwell::DeferredCreateInstance& command) {
    auto& context = *static_cast<AmigaAreaOneTerrainBuilder*>(opaque);
    if (context.observer_ && !context.observer_(context.observerContext_, command)) {
        context.failed_ = true;
        return false;
    }
    const bool enemy = command.objectId == 111 || command.objectId == 112 ||
                       command.objectId == 113 || command.objectId == 116 ||
                       command.objectId == 117 || command.objectId == 119;
    if (enemy && context.enemies_ != 0) {
        if (context.enemyCount_ >= context.enemyCapacity_) {
            context.failed_ = true;
            return false;
        }
        AmigaTerrainEnemy& output = context.enemies_[context.enemyCount_++];
        output.objectId = command.objectId;
        output.x = command.x - 240;
        output.y = command.y - 176;
        output.speedX = command.integerVelocityX;
        output.speedY = command.integerVelocityY;
        output.direction = command.integerDirection;
        output.moveSpeed = command.integerMoveSpeed;
        output.motionState = command.motionState;
        output.auxiliaryState = command.auxiliaryState;
        output.alarm0 = command.alarms.get(0);
        output.alarm1 = command.alarms.get(1);
        output.grounded = command.grounded;
        output.active = command.active;
    }
    const int spriteId = decorationSprite(command.objectId);
    if (spriteId >= 0 && context.decorations_ != 0) {
        if (context.decorationCount_ >= context.decorationCapacity_) {
            context.failed_ = true;
            return false;
        }
        AmigaTerrainDecoration& output =
            context.decorations_[context.decorationCount_++];
        output.objectId = command.objectId;
        output.spriteId = spriteId;
        output.x = command.x;
        output.y = command.y;
        output.frame = command.integerImageIndex;
        output.flipX = command.integerScaleX < 0;
    }
    return true;
}

AmigaAreaOneTerrainBuilder::AmigaAreaOneTerrainBuilder(
    AmigaTerrainFragment* fragments, unsigned fragmentCapacity,
    AmigaTerrainEnemy* enemies, unsigned enemyCapacity,
    AmigaTerrainDecoration* decorations, unsigned decorationCapacity)
    : fragments_(fragments), fragmentCapacity_(fragmentCapacity),
      enemies_(enemies), enemyCapacity_(enemyCapacity),
      decorations_(decorations), decorationCapacity_(decorationCapacity) {}

bool AmigaAreaOneTerrainBuilder::collectRun(void* opaque,
    const downwell::AreaOneBuilder::TerrainRun& run,downwell::AreaOneBuilder::EmitFunction scalar){
    auto& self=*static_cast<AmigaAreaOneTerrainBuilder*>(opaque);
    // Runs contain only walls/platforms: collect's enemy and decoration
    // outputs do not apply. The observer must execute all synchronous births.
    const bool ok=self.runObserver_?self.runObserver_(self.observerContext_,run):run.execute(scalar,opaque);
    if(!ok)self.failed_=true;
    return ok;
}
bool AmigaAreaOneTerrainBuilder::collectPrepared(void* opaque,
    const downwell::AreaOneBuilder::PreparedTerrain& batch,downwell::AreaOneBuilder::EmitFunction scalar){
    auto& self=*static_cast<AmigaAreaOneTerrainBuilder*>(opaque);
    const bool ok=self.preparedObserver_?self.preparedObserver_(self.observerContext_,batch):batch.executeReference(scalar,opaque);
    if(!ok)self.failed_=true;
    return ok;
}

void AmigaAreaOneTerrainBuilder::resetInitialCheckpoint(
    downwell::GameMakerRandom& random) {
    resetInitialCheckpointBeforeEntrance(random);
    random.nextU32(); // Forecast-only compatibility: entrance Create's radius.
}
void AmigaAreaOneTerrainBuilder::resetInitialCheckpointBeforeEntrance(
    downwell::GameMakerRandom& random) {
    // Captured at the initial main-room entry, before any builder Step event.
    random.setSeed(2);
    for (unsigned draw = 0; draw < 330; ++draw) random.nextU32();
    // rmMain's objBuilder (100856) is at -16,704, not the player's x=240.
    // scrBuilder uses this x for the synchronous objSpawner birth (x-100).
    builder_.restoreInitialCheckpoint(-16, 704, -30, 8, 3);
    fragmentCount_ = enemyCount_ = decorationCount_ = 0;
    failed_ = false;
}

bool AmigaAreaOneTerrainBuilder::resetFromRoomEntry(
    const downwell::AreaOneBuilder& builder) {
    if (builder.rowsGenerated() != 0 || builder.spawnerMade() ||
        builder.endReached()) return false;
    builder_ = builder;
    fragmentCount_ = enemyCount_ = decorationCount_ = 0;
    failed_ = false;
    return true;
}

void AmigaAreaOneTerrainBuilder::resetRoom(int x, int y,
        downwell::GameMakerRandom& random) {
    resetRoom(x, y, random, downwell::AreaOneBuilder::Setup{});
}

void AmigaAreaOneTerrainBuilder::resetRoom(int x, int y,
        downwell::GameMakerRandom& random, const downwell::AreaOneBuilder::Setup& setup) {
    builder_.reset(x, y, random, setup);
    fragmentCount_ = enemyCount_ = decorationCount_ = 0;
    failed_ = false;
}

bool AmigaAreaOneTerrainBuilder::stepVisible(
    const downwell::BuilderPatternPack& patterns,
    downwell::GameMakerRandom& random, int viewBottomCeiling,
    downwell::AreaOneBuilderStep& result) {
    result = downwell::AreaOneBuilderStep{};
    if (failed_) return false;
    // reset/room-state import replaces the core builder, not backend bindings.
    builder_.enableTerrainRuns(runObserver_!=nullptr);
    builder_.setBreakableEmitter(breakablePublisher_,observerContext_);
    builder_.setTerrainRunEmitter(runObserver_?collectRun:nullptr);
    builder_.setPreparedTerrainEmitter(preparedPublisher_?preparedPublisher_:(preparedObserver_?collectPrepared:nullptr),
                                      preparedPublisher_?observerContext_:nullptr);
    if (!builder_.stepVisible(patterns, random, viewBottomCeiling,
                              collect, this, result)) {
        failed_ = true;
        return false;
    }
    if (!result.patternGenerated) return true;
    downwell::PackedBuilderPattern pattern;
    if (!fragments_ || fragmentCount_ >= fragmentCapacity_ ||
        !patterns.findPattern(result.family, result.patternIndex, pattern)) {
        failed_ = true;
        return false;
    }
    fragments_[fragmentCount_++] = AmigaTerrainFragment{
        pattern, result.patternY, result.reversed, result.sideRoom,
        result.appendPillars};
    return true;
}

int amigaEnemyHealth(int objectId) {
    return downwell::enemyCreateProfile(objectId).health;
}

int amigaEnemyReward(int objectId) {
    return downwell::enemyCreateProfile(objectId).reward;
}

bool amigaBuildInitialAreaOneTerrain(
    const downwell::BuilderPatternPack& builders,
    AmigaTerrainFragment* fragments, unsigned capacity,
    AmigaTerrainEnemy* enemies, unsigned enemyCapacity,
    unsigned& enemyCount, downwell::GameMakerRandom* randomAfter) {
    return amigaBuildInitialAreaOneTerrain(
        builders, fragments, capacity, enemies, enemyCapacity, enemyCount,
        randomAfter, 0, 0, 0);
}

bool amigaBuildInitialAreaOneTerrain(
    const downwell::BuilderPatternPack& builders,
    AmigaTerrainFragment* fragments, unsigned capacity,
    AmigaTerrainEnemy* enemies, unsigned enemyCapacity,
    unsigned& enemyCount, downwell::GameMakerRandom* randomAfter,
    AmigaTerrainDecoration* decorations, unsigned decorationCapacity,
    unsigned* decorationCount, downwell::AreaOneBuilder::EmitFunction observer,
    void* observerContext) {
    enemyCount = 0;
    if (decorationCount != 0)
        *decorationCount = 0;
    if (fragments == 0 || capacity < amigaInitialTerrainFragmentCount)
        return false;
    downwell::GameMakerRandom random;
    AmigaAreaOneTerrainBuilder builder(fragments, capacity, enemies,
        enemyCapacity, decorations, decorationCapacity);
    builder.resetInitialCheckpoint(random);
    builder.setCreateObserver(observer, observerContext);
    for (unsigned step = 0; step < 64 && !builder.endReached(); ++step) {
        downwell::AreaOneBuilderStep result;
        // Forecast only: force one event at a time without live enemy events.
        if (!builder.stepVisible(builders, random, builder.frontierY() - 127,
                                 result))
            return false;
    }
    enemyCount = builder.enemyCount();
    if (decorationCount != 0)
        *decorationCount = builder.decorationCount();
    // Success is structural, not a historical RNG draw total. The forecast
    // regression compares ordered Create output and complete RNG state with
    // incremental generation under the current source-backed choice rules.
    const bool valid = !builder.failed() && builder.endReached() &&
        builder.fragmentCount() == amigaInitialTerrainFragmentCount;
    if (valid && randomAfter != 0)
        *randomAfter = random;
    return valid;
}

unsigned amigaCollectTerrainBreakables(
    const downwell::BuilderPatternPack& builders,
    const AmigaTerrainFragment* fragments, unsigned fragmentCount,
    int* x, int* y, unsigned capacity) {
    unsigned count = 0;
    if (fragments == 0 || x == 0 || y == 0)
        return 0;
    for (unsigned fragmentIndex = 0;
         fragmentIndex < fragmentCount && count < capacity;
         ++fragmentIndex) {
        const AmigaTerrainFragment& fragment = fragments[fragmentIndex];
        for (std::uint16_t row = 0;
             row < fragment.pattern.rowCount && count < capacity; ++row) {
            std::uint32_t width = 0;
            if (!builders.rowWidth(fragment.pattern, row, width))
                break;
            for (unsigned column = 0;
                 column < width && count < capacity; ++column) {
                char symbol = 0;
                const int ordinal = static_cast<int>(column) + 1;
                if (!builders.cell(fragment.pattern,
                                   static_cast<std::uint16_t>(column),
                                   row, symbol))
                    break;
                if (symbol != 'B')
                    continue;
                x[count] = (fragment.reversed
                                ? 320 - ordinal * 16
                                : 160 + ordinal * 16) - 240;
                y[count] = fragment.y + static_cast<int>(row) * 16 - 176;
                ++count;
            }
        }
    }
    return count;
}

namespace {
enum { solidCell = 1, landingCell = 2, breakableCell = 4, roomWallCell = 8, wallCell = 16,
       platformCell = 32, wallContactUnit = 64, wallContactMask = 192 };
bool contactCell(int x, int y, unsigned& cell) {
    if ((x & 15) || (y & 15) || x < AmigaTerrainIndex::minX ||
        x >= AmigaTerrainIndex::minX + AmigaTerrainIndex::columns * 16 ||
        y < 0 || y >= AmigaTerrainIndex::rows * 16) return false;
    cell = (unsigned(y) >> 4) * AmigaTerrainIndex::columns +
        (unsigned(x - AmigaTerrainIndex::minX) >> 4);
    return true;
}
// One contact per actual Create. A box is counted by the breakable flag its
// own Create sets, unless an earlier box already holds that flag. Fails only
// when the overflow records are full, before any occupancy changes.
bool addContact(AmigaTerrainIndex& index, int objectId, int x, int y, bool boxFlag) {
    if (objectId != 60 && objectId != 92 && objectId != 293) return true;
    unsigned cell;
    if (!contactCell(x, y, cell)) return false;
    auto& flags = index.cells[cell];
    if (objectId == 60 && (flags & wallContactMask) != wallContactMask) {
        flags += wallContactUnit;
        return true;
    }
    if (objectId == 92 && boxFlag && !(flags & breakableCell)) return true;
    return index.extraContacts.append(objectId, x, y);
}
bool indexAdd(AmigaTerrainIndex& index, int x, int y, unsigned flags,
              unsigned* addedCell = nullptr) {
    if ((x & 15) || (y & 15) || x < AmigaTerrainIndex::minX ||
        x >= AmigaTerrainIndex::minX + AmigaTerrainIndex::columns * 16 ||
        y < 0 || y >= AmigaTerrainIndex::rows * 16) return false;
    const unsigned cell = (unsigned(y) >> 4) * AmigaTerrainIndex::columns +
        (unsigned(x - AmigaTerrainIndex::minX) >> 4);
    index.cells[cell] |= flags;
    if (addedCell) *addedCell = cell;
    return true;
}
unsigned indexCell(const AmigaTerrainIndex& index, int x, int y) {
    const int shiftedX = x + 8 - AmigaTerrainIndex::minX;
    const int shiftedY = y + 8;
    if (shiftedX < 0 || shiftedY < 0 || shiftedX >= AmigaTerrainIndex::columns * 16 ||
        shiftedY >= AmigaTerrainIndex::rows * 16) return 0;
    return index.cells[(shiftedY >> 4) * AmigaTerrainIndex::columns + (shiftedX >> 4)];
}
bool indexActive(const AmigaTerrainCollisionContext& context, unsigned cell,
                 unsigned flag, int x, int y) {
    if (cell & flag) return true;
    return (cell & breakableCell) && (!context.runtime ||
        context.runtime->breakableActiveAt(((x + 8 - AmigaTerrainIndex::minX) >> 4) * 16 +
                AmigaTerrainIndex::minX - 240, ((y + 8) >> 4) * 16 - 176));
}
}

bool amigaBuildTerrainIndex(const AmigaTerrainCollisionContext& context,
                           AmigaTerrainIndex& index) {
#ifndef DOWNWELL_REFERENCE_EMPTY_TERRAIN
    if(!context.fragmentCount && !(context.rooms && context.room)){
        // Empty-world entry has no candidates to reconstruct. Reset the two
        // owners directly; room Creates will publish their candidates later.
        index.extraContacts.clear();
        index.wallContactsComplete=true;
        index.mutableWallOccupancy=true;
        for(auto& cell:index.cells)cell=0;
        index.nextLanding.clear();
        return true;
    }
#endif
    return amigaReconstructTerrainIndex(context,index);
}

bool amigaReconstructTerrainIndex(const AmigaTerrainCollisionContext& context,
                           AmigaTerrainIndex& index) {
    index.extraContacts.clear();
    // Reconstructed pattern occupancy is not evidence of actual Create events.
    index.wallContactsComplete = context.fragmentCount==0;
    index.mutableWallOccupancy = context.fragmentCount==0 && !(context.rooms && context.room);
    for (unsigned i = 0; i < sizeof(index.cells); ++i) index.cells[i] = 0;
    if (context.rooms && context.room) {
        for (unsigned i = 0; i < context.room->instanceCount; ++i) {
            downwell::PackedRoomInstance instance;
            if (!context.rooms->roomInstance(*context.room, i, instance)) return false;
            // Room boxes are solid here, not breakable cells; keep their records.
            if (!addContact(index, instance.objectId, instance.x, instance.y, false)) return false;
            if ((instance.objectId == 60 || instance.objectId == 92) &&
                !indexAdd(index, instance.x, instance.y, solidCell |
                    (instance.objectId == 60 ? roomWallCell : 0))) return false;
        }
    }
    for (unsigned f = 0; f < context.fragmentCount; ++f) {
        const auto& fragment = context.fragments[f];
        const int firstX = fragment.sideRoom ? (fragment.reversed ? 0 : 480)
                                             : (fragment.reversed ? 160 : 320);
        const int secondX = fragment.reversed ? 320 : 160;
        for (unsigned row = 0; row < fragment.pattern.rowCount; ++row) {
            const int y = fragment.y + row * 16;
            if (row + 1 < fragment.pattern.rowCount &&
                (!indexAdd(index, firstX, y, solidCell) ||
                 !indexAdd(index, secondX, y, solidCell))) return false;
            std::uint32_t width;
            if (!context.builders->rowWidth(fragment.pattern, row, width)) return false;
            for (unsigned col = 0; col < width; ++col) {
                char symbol;
                if (!context.builders->cell(fragment.pattern, col, row, symbol)) return false;
                const unsigned flags = symbol == 'B' ? breakableCell :
                    (isSolidTerrainSymbol(symbol) ? solidCell : 0) |
                    (amigaIsLandingTerrainSymbol(symbol) ? landingCell : 0);
                const int x = fragment.reversed ? 320 - (col + 1) * 16 : 160 + (col + 1) * 16;
                if (flags && !indexAdd(index, x, y, flags)) return false;
            }
        }
        if (fragment.appendPillars)
            for (unsigned row = 0; row < 3; ++row) {
                const int y = fragment.y + (fragment.pattern.rowCount - 1 + row) * 16;
                if (!indexAdd(index, firstX, y, solidCell) ||
                    !indexAdd(index, secondX, y, solidCell)) return false;
            }
    }
    index.nextLanding.clear();
    for (unsigned column = 0; column < AmigaTerrainIndex::columns; ++column) {
        for (int row = AmigaTerrainIndex::rows - 1; row >= 0; --row) {
            const unsigned cell = row * AmigaTerrainIndex::columns + column;
            if (index.cells[cell] & (landingCell | breakableCell)) index.nextLanding.insert(column,row);
        }
    }
    return true;
}

bool amigaIndexedRoomWall(const AmigaTerrainIndex& index, int x, int y) {
    return (indexCell(index, x, y) & roomWallCell) != 0;
}

// Counts every overlapping parentWall instance, up to limit: each cell's
// coincident walls and flagged box, then the overflow records. Callers need
// multiplicity only, and place_meeting callers only whether there is one.
bool amigaResolveTerrainWallContacts(void* opaque,const downwell::ContactBounds& query,unsigned limit,unsigned& count){
    count=0;
    const auto& context=*static_cast<const AmigaTerrainCollisionContext*>(opaque);
    if(!context.index || !context.index->wallContactsComplete)return false;
    const auto& index=*context.index;
    auto active=[&](unsigned object,int x,int y){
        if(!context.runtime)return true;
        if(object==293)return context.runtime->gemChunkSolidAt(x,y);
        return object!=92 || context.runtime->breakableActiveAt(x-240,y-176);
    };
    if(!limit||query.left>=query.right || query.top>=query.bottom || query.right<=-521 ||
       query.left>=1546 || query.bottom<=-8 || query.top>=8201)return true;
#ifndef DOWNWELL_REFERENCE_WALL_CONTACT_SCAN
    // The cells whose 16x16 contact box (mask 585: -8..8 about the cell
    // origin) overlaps, exactly: no per-cell bounds test. A cell at column c
    // spans [minX+16c-8, minX+16c+8), so it overlaps when
    // left-8-minX < 16c < right+8-minX; rows likewise from y 0.
    static_assert(AmigaTerrainIndex::minX==-512,"column origin");
    int firstRow=((query.top-8)>>4)+1,lastRow=(query.bottom+7)>>4;
    int firstColumn=((query.left+504)>>4)+1,lastColumn=(query.right+519)>>4;
    if(firstRow<0)firstRow=0;
    if(lastRow>=AmigaTerrainIndex::rows)lastRow=AmigaTerrainIndex::rows-1;
    if(firstColumn<0)firstColumn=0;
    if(lastColumn>=AmigaTerrainIndex::columns)lastColumn=AmigaTerrainIndex::columns-1;
    for(int row=firstRow;row<=lastRow;++row){
        const unsigned char* cell=index.cells+row*AmigaTerrainIndex::columns+firstColumn;
        for(int column=firstColumn;column<=lastColumn;++column,++cell){
            const unsigned flags=*cell;
            if(!(flags&(wallContactMask|breakableCell)))continue;
            count+=flags/wallContactUnit;
            if((flags&breakableCell) && active(92,AmigaTerrainIndex::minX+column*16,row*16))++count;
            if(count>=limit)return true;
        }
    }
#else
    // Superset of the cells whose 16x16 contact box can overlap; each is tested exactly.
    const int firstRow=query.top<=9?0:(query.top-9)/16;
    const int lastRow=query.bottom>=8184?AmigaTerrainIndex::rows-1:(query.bottom+8)/16;
    const int firstColumn=query.left<=-520?0:(query.left+504)/16;
    int lastColumn=query.right>=1544?AmigaTerrainIndex::columns-1:(query.right+519)/16;
    if(lastColumn>=AmigaTerrainIndex::columns)lastColumn=AmigaTerrainIndex::columns-1;
    const auto local=downwell::contactSpriteBounds(585);
    for(int row=firstRow;row<=lastRow&&row<AmigaTerrainIndex::rows;++row)
        for(int column=firstColumn;column<=lastColumn;++column){
            const unsigned flags=index.cells[row*AmigaTerrainIndex::columns+column];
            if(!(flags&(wallContactMask|breakableCell)))continue;
            const int x=AmigaTerrainIndex::minX+column*16,y=row*16;
            if(!downwell::contactBoundsOverlap(query,{x+local.left,x+local.right,y+local.top,y+local.bottom}))continue;
            count+=flags/wallContactUnit;
            if((flags&breakableCell) && active(92,x,y))++count;
            if(count>=limit)return true;
        }
#endif
    if(!index.extraContacts.size())return true;
    bool full=false;
    return index.extraContacts.visit(query,active,[&](){full=++count>=limit;return !full;})||full;
}

unsigned amigaTerrainContacts(const AmigaTerrainIndex& index, int objectId, int x, int y) {
    unsigned cell;
    if (!contactCell(x, y, cell)) return 0;
    const unsigned flags = index.cells[cell];
    return index.extraContacts.count(objectId, x, y) +
        (objectId == 60 ? flags / wallContactUnit : objectId == 92 ? (flags & breakableCell) != 0 : 0);
}

bool amigaResolveTerrainPlatformContact(void* opaque,const downwell::ContactBounds& query,bool& hit){
    const auto& context=*static_cast<const AmigaTerrainCollisionContext*>(opaque);
    if(!context.index||!context.index->wallContactsComplete)return false;
    const auto& index=*context.index;
    bool found=false;
    if(query.left<query.right&&query.top<query.bottom&&query.right>-521&&
        query.left<1546&&query.bottom>-8&&query.top<8201){
#ifndef DOWNWELL_REFERENCE_WALL_CONTACT_SCAN
        // Exactly the cells whose thin platform box (sprThinPlatform mask 49:
        // origin 12,12; margins 4,4..19,5, so x-8..x+8, y-8..y-6 about the
        // cell origin) overlaps. Columns as for walls; a row at y = 16r
        // overlaps when top+6 < 16r < bottom+8.
        int firstRow=((query.top+6)>>4)+1,lastRow=(query.bottom+7)>>4;
        int firstColumn=((query.left+504)>>4)+1,lastColumn=(query.right+519)>>4;
        if(firstRow<0)firstRow=0;
        if(lastRow>=AmigaTerrainIndex::rows)lastRow=AmigaTerrainIndex::rows-1;
        if(firstColumn<0)firstColumn=0;
        if(lastColumn>=AmigaTerrainIndex::columns)lastColumn=AmigaTerrainIndex::columns-1;
        for(int row=firstRow;row<=lastRow&&!found;++row){
            const unsigned char* cell=index.cells+row*AmigaTerrainIndex::columns+firstColumn;
            for(int column=firstColumn;column<=lastColumn;++column,++cell)
                if(*cell&platformCell){found=true;break;}
        }
#else
        const int firstRow=query.top<=9?0:(query.top-9)/16;
        const int lastRow=query.bottom>=8184?AmigaTerrainIndex::rows-1:(query.bottom+8)/16;
        const int firstColumn=query.left<=-520?0:(query.left+504)/16;
        int lastColumn=query.right>=1544?AmigaTerrainIndex::columns-1:(query.right+519)/16;
        if(lastColumn>=AmigaTerrainIndex::columns)lastColumn=AmigaTerrainIndex::columns-1;
        for(int row=firstRow;row<=lastRow&&row<AmigaTerrainIndex::rows&&!found;++row)
            for(int column=firstColumn;column<=lastColumn;++column){
                if(!(index.cells[row*AmigaTerrainIndex::columns+column]&platformCell))continue;
                const int x=AmigaTerrainIndex::minX+column*16,y=row*16;
                if(downwell::contactBoundsOverlap(query,{x-8,x+8,y-8,y-6})){found=true;break;}
            }
#endif
    }
    hit=found;return true;
}

unsigned amigaTerrainContactCount(const AmigaTerrainIndex& index) {
    unsigned count = index.extraContacts.size();
    for (unsigned flags : index.cells) count += flags / wallContactUnit + ((flags & breakableCell) != 0);
    return count;
}

bool amigaIndexedWall(const AmigaTerrainIndex& index, int x, int y) {
    static_assert((wallCell|roomWallCell)==AmigaTerrainIndex::wallAppearanceFlags,
        "appearance row mask follows indexed-wall occupancy");
    return (indexCell(index, x, y) & (wallCell | roomWallCell)) != 0;
}

bool amigaAddTerrainObject(AmigaTerrainIndex& index, int objectId, int x, int y,
                           bool initialRoomWall) {
    if (!addContact(index, objectId, x, y, true)) return false;
    const unsigned flags = objectId == 60
        ? solidCell | landingCell | wallCell | (initialRoomWall ? roomWallCell : 0)
        : objectId == 92 ? breakableCell : objectId == 93 ? landingCell | platformCell : 0;
    if (!flags) return true;
    unsigned cell;
    {
        DW_GENERATION_SCOPE(profileGeneratedCell);
        if (!indexAdd(index, x, y, flags, &cell)) return false;
    }
    DW_GENERATION_SCOPE(profileGeneratedLanding);
    index.nextLanding.insert(cell%AmigaTerrainIndex::columns,cell/AmigaTerrainIndex::columns);
    return true;
}

bool amigaAdmitPreparedTerrain(const AmigaTerrainIndex& index,const AmigaTerrainTiles& tiles,
    const downwell::AreaOneBuilder::PreparedTerrain& batch){
    // Conservative admission avoids a cell scan. Even if every wall overlaps
    // a saturated cell, the reserved contact space suffices. Otherwise retain
    // the scalar prefix/failure behavior rather than partially publishing.
    return batch.y>=0 && batch.y<AmigaTerrainIndex::rows*16 && !(batch.y&15) &&
        batch.platforms()<=AmigaTerrainTiles::capacity-tiles.size() &&
        batch.count()-batch.platforms()<=index.extraContacts.capacity()-index.extraContacts.size();
}

bool amigaPublishPreparedTerrain(AmigaTerrainIndex& index,AmigaTerrainTiles& tiles,
    const downwell::AreaOneBuilder::PreparedTerrain& batch){
    static_assert(solidCell==1 && landingCell==2 && wallCell==16 && platformCell==32,
                  "prepared occupancy resource contract");
    const unsigned row=unsigned(batch.y)>>4;
    auto* destination=index.cells+row*AmigaTerrainIndex::columns;
    const auto* word=batch.words();
    for(unsigned i=0;i<batch.wordCount();++i,word+=4){
        auto* target=destination+word[0];
        if(!(target[0]|target[1])){
            // Newly generated space already has its occupancy and contact
            // counts composed offline. No runtime cell classification/merge.
            target[0]=word[2];target[1]=word[3];
        }else for(unsigned half=0;half<2;++half){
            const unsigned old=target[half], prepared=word[2+half];
            unsigned walls=(old>>6)+(prepared>>6);
            while(walls>3){
                if(!index.extraContacts.append(60,(int(word[0]+half)-32)*16,batch.y))return false;
                --walls;
            }
            target[half]=((old|prepared)&63)|(walls<<6);
        }
    }
    index.nextLanding.insertPreparedWords(batch.words(),batch.wordCount(),row);
    return tiles.appendPrepared(batch);
}

AmigaTerrainRunIndex::AmigaTerrainRunIndex(AmigaTerrainIndex& index,int object,
    int x,int y,int stepX,unsigned count):index_(index),object_(object),x_(x),y_(y),step_(stepX<0?-1:1){
    if((object!=60&&object!=93)||!count||count>AmigaTerrainIndex::columns||
        (stepX!=16&&stepX!=-16)||(x&15)||(y&15)||x<AmigaTerrainIndex::minX||
        x>=AmigaTerrainIndex::minX+AmigaTerrainIndex::columns*16||
        y<0||y>=AmigaTerrainIndex::rows*16)return;
    const int column=(x-AmigaTerrainIndex::minX)>>4;
    const int last=step_>0?column+int(count-1):column-int(count-1);
    if(last<0||last>=AmigaTerrainIndex::columns)return;
    column_=unsigned(column);row_=unsigned(y)>>4;
    flags_=object==60?solidCell|landingCell|wallCell:landingCell|platformCell;
    remaining_=count;
}
bool AmigaTerrainRunIndex::append(){
    if(!remaining_)return false;
    auto& cell=index_.cells[row_*AmigaTerrainIndex::columns+column_];
    if(object_==60){
        if((cell&wallContactMask)!=wallContactMask)cell+=wallContactUnit;
        else if(!index_.extraContacts.append(60,x_,y_))return false;
    }
    cell|=flags_;
    index_.nextLanding.insert(column_,row_);
    --remaining_;x_+=step_*16;column_=unsigned(int(column_)+step_);
    return true;
}

bool AmigaTerrainRunIndex::admitSpan(const AmigaTerrainTiles& tiles)const{
    if(!remaining_)return false;
    if(object_==93)return remaining_<=AmigaTerrainTiles::capacity-tiles.size();
    unsigned saturated=0;
    const unsigned char* cell=&index_.cells[row_*AmigaTerrainIndex::columns+column_];
    for(unsigned i=0;i<remaining_;++i,cell+=step_)saturated+=(*cell&wallContactMask)==wallContactMask;
    return saturated<=index_.extraContacts.capacity()-index_.extraContacts.size();
}
bool AmigaTerrainRunIndex::publishSpan(AmigaTerrainTiles& tiles){
    if(!admitSpan(tiles))return false;
    if(object_==93&&!tiles.appendPlatforms(x_,y_,step_*16,remaining_))return false;
    // Locals: byte stores through the cell pointer would otherwise reload members.
    const unsigned count=remaining_;
    const int step=step_;
    unsigned char* cell=&index_.cells[row_*AmigaTerrainIndex::columns+column_];
    const unsigned char flags=static_cast<unsigned char>(flags_);
    if(object_==60){
        int x=x_;
        for(unsigned i=0;i<count;++i,cell+=step,x+=step*16){
            unsigned char value=*cell;
            if((value&wallContactMask)!=wallContactMask)value+=wallContactUnit;
            else index_.extraContacts.append(60,x,y_);
            *cell=value|flags;
        }
    }else for(unsigned i=0;i<count;++i,cell+=step)*cell|=flags;
    index_.nextLanding.insertSpan(column_,step,count,row_);
    const int span=step<0?-int(count):int(count); // step_ is -1 or 1
    x_+=span*16;column_=unsigned(int(column_)+span);remaining_=0;
    return true;
}

bool amigaRemoveTerrainWall(AmigaTerrainIndex& index, int x, int y) {
    unsigned cell;
    if (!index.mutableWallOccupancy || !index.wallContactsComplete ||
        !contactCell(x, y, cell)) return false;
    // Overflow walls exist only while the cell count is saturated; take them first.
    if (index.extraContacts.removeOne(60,x,y)) return true;
    auto& flags=index.cells[cell];
    if (!(flags & wallContactMask)) return false;
    flags -= wallContactUnit;
    if (flags & wallContactMask) return true;
    flags &= ~(solidCell | wallCell | roomWallCell);
    if (!(flags & platformCell)) flags &= ~landingCell;
    if (!(flags & (landingCell | breakableCell)))
        index.nextLanding.remove(cell%AmigaTerrainIndex::columns,cell/AmigaTerrainIndex::columns);
    return true;
}

int amigaResolveTerrainLanding(void* opaque, int offsetX,
                               int minimumOffsetY) {
    DW_PROFILE_SCOPE(profileTerrainCollision);
    const AmigaTerrainCollisionContext& context =
        *static_cast<const AmigaTerrainCollisionContext*>(opaque);
    const int worldX = 240 + offsetX;
    int landing = downwell::gameplayNoLanding;
    if (context.index) {
        DW_PROFILE_COUNT(counterLandingIndexed);
        int row = (minimumOffsetY + 192 + 15) / 16;
        if (row < 0) row = 0;
        const int firstColumn = (worldX - 4 + 8 - AmigaTerrainIndex::minX) >> 4;
        const int lastColumn = (worldX + 3 + 8 - AmigaTerrainIndex::minX) >> 4;
        for (int column = firstColumn; column <= lastColumn; ++column) {
            if (column < 0 || column >= AmigaTerrainIndex::columns) continue;
            int candidate = row;
            while (candidate < AmigaTerrainIndex::rows && candidate * 16 - 192 < landing) {
                candidate = context.index->nextLanding[candidate * AmigaTerrainIndex::columns + column] - 1;
                if (candidate < 0 || candidate * 16 - 192 >= landing) break;
                const int y = candidate * 16;
                const unsigned cell = context.index->cells[candidate * AmigaTerrainIndex::columns + column];
                if ((cell & landingCell) || ((cell & breakableCell) &&
                    (!context.runtime || context.runtime->breakableActiveAt(
                        column * 16 + AmigaTerrainIndex::minX - 240, y - 176)))) {
                    landing = y - 192;
                    break;
                }
                ++candidate;
            }
        }
        if (context.runtime)
            landing = context.runtime->gemChunkLanding(worldX, minimumOffsetY, landing);
        return landing;
    }
    DW_PROFILE_COUNT(counterLandingFallback);
    for (unsigned fragmentIndex = 0;
         fragmentIndex < context.fragmentCount; ++fragmentIndex) {
        const AmigaTerrainFragment& fragment =
            context.fragments[fragmentIndex];
        for (std::uint16_t row = 0; row < fragment.pattern.rowCount; ++row) {
            std::uint32_t width = 0;
            const int candidate = fragment.y +
                static_cast<int>(row) * 16 - 16 - 176;
            if (candidate < minimumOffsetY || candidate >= landing)
                continue;
            if (!context.builders->rowWidth(fragment.pattern, row, width))
                break;
            for (unsigned column = 0; column < width; ++column) {
                char symbol = 0;
                const int ordinal = static_cast<int>(column) + 1;
                const int tileX = fragment.reversed
                                      ? 320 - ordinal * 16
                                      : 160 + ordinal * 16;
                if (!context.builders->cell(
                        fragment.pattern,
                        static_cast<std::uint16_t>(column), row, symbol))
                    break;
                const bool active = symbol != 'B' || context.runtime == 0 ||
                    context.runtime->breakableActiveAt(
                        tileX - 240,
                        fragment.y + static_cast<int>(row) * 16 - 176);
                /*
                 * The recovered player mask spans X - 4 through X + 3;
                 * landing is an overlap test against the tile's -8..+7
                 * mask, not a point sample at the player's origin.
                 */
                if (active && amigaIsLandingTerrainSymbol(symbol) &&
                    worldX + 3 >= tileX - 8 &&
                    worldX - 4 <= tileX + 7) {
                    landing = candidate;
                    break;
                }
            }
        }
    }
    return landing;
}

bool amigaResolveTerrainSegment(void* opaque, const downwell::ProjectileSegment& path) {
    const auto& context = *static_cast<const AmigaTerrainCollisionContext*>(opaque);
    const int left = path.x.pixel < path.endX.pixel ? path.x.pixel : path.endX.pixel;
    const int right = path.x.pixel > path.endX.pixel ? path.x.pixel : path.endX.pixel;
    const int top = path.y.pixel < path.endY.pixel ? path.y.pixel : path.endY.pixel;
    const int bottom = path.y.pixel > path.endY.pixel ? path.y.pixel : path.endY.pixel;
    if (!context.index) {
        DW_PROFILE_COUNT(counterSegmentFallback);
        for (int y = top; y <= bottom; ++y)
            for (int x = left; x <= right; ++x)
                if (amigaResolveTerrainSolid(opaque, x, y) && path.intersects(x, x+1, y, y+1)) return true;
        return false;
    }
    DW_PROFILE_SCOPE(profileTerrainCollision);
    DW_PROFILE_COUNT(counterSegmentIndexed);
    // Each indexed cell represents the union of its 16x16 half-open pixel
    // rectangles. Test that union once instead of querying each crossed pixel.
    int firstColumn = (left + 248 - AmigaTerrainIndex::minX) >> 4;
    int lastColumn = (right + 248 - AmigaTerrainIndex::minX) >> 4;
    int firstRow = (top + 184) >> 4, lastRow = (bottom + 184) >> 4;
    if (firstColumn < 0) firstColumn = 0;
    if (lastColumn >= AmigaTerrainIndex::columns) lastColumn = AmigaTerrainIndex::columns-1;
    if (firstRow < 0) firstRow = 0;
    if (lastRow >= AmigaTerrainIndex::rows) lastRow = AmigaTerrainIndex::rows-1;
    for (int row = firstRow; row <= lastRow; ++row)
        for (int column = firstColumn; column <= lastColumn; ++column) {
            DW_PROFILE_COUNT(counterSegmentCells);
            const int x = column*16 + AmigaTerrainIndex::minX, y = row*16;
            const unsigned cell = context.index->cells[row*AmigaTerrainIndex::columns+column];
            if (indexActive(context, cell, solidCell, x, y) &&
                path.intersects(x-248, x-232, y-184, y-168)) return true;
        }
    return false;
}

bool amigaResolveTerrainPlayerMask(void* opaque, int offsetX, int offsetY) {
    const auto& context = *static_cast<const AmigaTerrainCollisionContext*>(opaque);
    if (!context.index)
        return amigaResolveTerrainSolid(opaque, offsetX-4, offsetY-4) ||
            amigaResolveTerrainSolid(opaque, offsetX+3, offsetY-4) ||
            amigaResolveTerrainSolid(opaque, offsetX-4, offsetY+7) ||
            amigaResolveTerrainSolid(opaque, offsetX+3, offsetY+7);
    DW_PROFILE_SCOPE(profileTerrainCollision);
    const int xs[] = {offsetX+236, offsetX+243};
    const int ys[] = {offsetY+172, offsetY+183};
    const unsigned columns = ((xs[0]+8) >> 4) == ((xs[1]+8) >> 4) ? 1 : 2;
    const unsigned rows = ((ys[0]+8) >> 4) == ((ys[1]+8) >> 4) ? 1 : 2;
    for (unsigned row = 0; row < rows; ++row)
        for (unsigned column = 0; column < columns; ++column)
            if (indexActive(context, indexCell(*context.index, xs[column], ys[row]),
                            solidCell, xs[column], ys[row])) return true;
    // Unopened gem chunks are parentWall too (unaligned 19x16 masks).
    if (context.runtime)
        for (unsigned row = 0; row < 2; ++row)
            for (unsigned column = 0; column < 2; ++column)
                if (context.runtime->gemChunkBlocks(xs[column], ys[row])) return true;
    return false;
}

bool amigaResolveTerrainCorners(void* opaque,int left,int top,int right,int bottom) {
    const auto& context=*static_cast<const AmigaTerrainCollisionContext*>(opaque);
    if(!context.index)return amigaResolveTerrainSolid(opaque,left,top)||
        amigaResolveTerrainSolid(opaque,right,top)||amigaResolveTerrainSolid(opaque,left,bottom)||
        amigaResolveTerrainSolid(opaque,right,bottom);
    // The aligned occupancy cell determines both static and live-box contact.
    // No retained cache: destruction/generation is visible to the next query.
    const int xs[2]={left+248-AmigaTerrainIndex::minX,right+248-AmigaTerrainIndex::minX};
    const int ys[2]={top+184,bottom+184};
    const int columns[2]={xs[0]>>4,xs[1]>>4},rows[2]={ys[0]>>4,ys[1]>>4};
    for(unsigned y=0;y<2;++y){
        if((y&&rows[1]==rows[0])||ys[y]<0||ys[y]>=AmigaTerrainIndex::rows*16)continue;
        for(unsigned x=0;x<2;++x){
            if((x&&columns[1]==columns[0])||xs[x]<0||xs[x]>=AmigaTerrainIndex::columns*16)continue;
            const auto cell=context.index->cells[unsigned(rows[y])*AmigaTerrainIndex::columns+unsigned(columns[x])];
            if(cell&solidCell)return true;
            if((cell&breakableCell)&&(!context.runtime||context.runtime->breakableActiveAt(
                columns[x]*16+AmigaTerrainIndex::minX-240,rows[y]*16-176)))return true;
        }
    }
    return context.runtime&&(context.runtime->gemChunkBlocks(left+240,top+176)||
        context.runtime->gemChunkBlocks(right+240,top+176)||context.runtime->gemChunkBlocks(left+240,bottom+176)||
        context.runtime->gemChunkBlocks(right+240,bottom+176));
}

downwell::SolidCellGrid amigaTerrainSolidCells(const AmigaTerrainCollisionContext& context) {
    static_assert(AmigaTerrainIndex::columns == 1 << 7, "grid rows are shifted");
    downwell::SolidCellGrid grid;
    if (!context.index || !context.runtime) return grid;
    grid.cells = context.index->cells;
    grid.originX = 240 + 8 - AmigaTerrainIndex::minX;
    grid.originY = 176 + 8;
    grid.columnShift = 7;
    grid.width = AmigaTerrainIndex::columns * 16;
    grid.height = AmigaTerrainIndex::rows * 16;
    grid.solid = solidCell;
    grid.breakable = breakableCell;
    return grid;
}

bool amigaResolveTerrainSolid(void* opaque, int offsetX, int offsetY) {
    DW_PROFILE_SCOPE(profileTerrainCollision);
    const AmigaTerrainCollisionContext& context =
        *static_cast<const AmigaTerrainCollisionContext*>(opaque);
    const int worldX = 240 + offsetX;
    const int worldY = 176 + offsetY;
    if (context.index) {
        DW_PROFILE_COUNT(counterSolidIndexed);
        return indexActive(context, indexCell(*context.index, worldX, worldY),
                           solidCell, worldX, worldY) ||
            (context.runtime && context.runtime->gemChunkBlocks(worldX, worldY));
    }
    DW_PROFILE_COUNT(counterSolidFallback);
    if (context.rooms != 0 && context.room != 0) {
        for (unsigned index = 0; index < context.room->instanceCount; ++index) {
            downwell::PackedRoomInstance instance;
            if (!context.rooms->roomInstance(*context.room, index, instance) ||
                (instance.objectId != 60 && instance.objectId != 92))
                continue;
            if (worldX >= instance.x - 8 && worldX < instance.x + 8 &&
                worldY >= instance.y - 8 && worldY < instance.y + 8)
                return true;
        }
    }
    for (unsigned fragmentIndex = 0;
         fragmentIndex < context.fragmentCount; ++fragmentIndex) {
        const AmigaTerrainFragment& fragment =
            context.fragments[fragmentIndex];
        for (std::uint16_t row = 0; row < fragment.pattern.rowCount; ++row) {
            const int tileY = fragment.y + static_cast<int>(row) * 16;
            std::uint32_t width = 0;
            if (worldY < tileY - 8 || worldY >= tileY + 8)
                continue;
            if (row + 1 < fragment.pattern.rowCount) {
                const int firstX = fragment.sideRoom
                                       ? (fragment.reversed ? 0 : 480)
                                       : (fragment.reversed ? 160 : 320);
                const int secondX = fragment.reversed ? 320 : 160;
                if ((worldX >= firstX - 8 && worldX < firstX + 8) ||
                    (worldX >= secondX - 8 && worldX < secondX + 8))
                    return true;
            }
            if (!context.builders->rowWidth(fragment.pattern, row, width))
                break;
            for (unsigned column = 0; column < width; ++column) {
                char symbol = 0;
                const int ordinal = static_cast<int>(column) + 1;
                const int tileX = fragment.reversed
                                      ? 320 - ordinal * 16
                                      : 160 + ordinal * 16;
                if (!context.builders->cell(
                        fragment.pattern,
                        static_cast<std::uint16_t>(column), row, symbol))
                    break;
                if (worldX < tileX - 8 || worldX >= tileX + 8 ||
                    !isSolidTerrainSymbol(symbol))
                    continue;
                if (symbol != 'B' || context.runtime == 0 ||
                    context.runtime->breakableActiveAt(
                        tileX - 240, tileY - 176))
                    return true;
            }
        }
        if (fragment.appendPillars) {
            const int finalRowY = fragment.y +
                (static_cast<int>(fragment.pattern.rowCount) - 1) * 16;
            if (worldY >= finalRowY - 8 &&
                worldY < finalRowY + 3 * 16 - 8) {
                const int firstX = fragment.sideRoom
                                       ? (fragment.reversed ? 0 : 480)
                                       : (fragment.reversed ? 160 : 320);
                const int secondX = fragment.reversed ? 320 : 160;
                if ((worldX >= firstX - 8 && worldX < firstX + 8) ||
                    (worldX >= secondX - 8 && worldX < secondX + 8))
                    return true;
            }
        }
    }
    return false;
}

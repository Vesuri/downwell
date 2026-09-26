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
    if (count_ == capacity || command.y < 0 || command.y >= rows * 16 ||
        (command.y & 15) || command.x < -512 || command.x >= 1536 || (command.x & 15))
        return false;
    const unsigned row = command.y / 16;
    const unsigned id = ++count_;
    tiles_[id - 1] = Tile{static_cast<std::int16_t>(command.x),
        static_cast<std::uint16_t>(command.objectId),
        static_cast<std::uint16_t>(command.objectId == 93 ? 23 : command.integerImageIndex), 0};
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
    if (objectId == 109) return 139;
    // objLineToResult's sprite supplies a mask, but Visible is false.
    return -1;
}
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

void AmigaAreaOneTerrainBuilder::resetInitialCheckpoint(
    downwell::GameMakerRandom& random) {
    // Captured at the initial main-room entry, before any builder Step event.
    random.setSeed(2);
    for (unsigned draw = 0; draw < 330; ++draw) random.nextU32();
    builder_.restoreInitialCheckpoint(240, 704, -30, 8, 3);
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
enum { solidCell = 1, landingCell = 2, breakableCell = 4, roomWallCell = 8, wallCell = 16 };
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
    index.wallContacts.clear();
    // Reconstructed pattern occupancy is not evidence of actual Create events.
    index.wallContactsComplete = context.fragmentCount==0;
    for (unsigned i = 0; i < sizeof(index.cells); ++i) index.cells[i] = 0;
    if (context.rooms && context.room) {
        for (unsigned i = 0; i < context.room->instanceCount; ++i) {
            downwell::PackedRoomInstance instance;
            if (!context.rooms->roomInstance(*context.room, i, instance)) return false;
            if ((instance.objectId == 60 || instance.objectId == 92) &&
                !indexAdd(index, instance.x, instance.y, solidCell |
                    (instance.objectId == 60 ? roomWallCell : 0))) return false;
            if (!index.wallContacts.append(instance.objectId,instance.x,instance.y))return false;
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
    for (unsigned column = 0; column < AmigaTerrainIndex::columns; ++column) {
        unsigned next = 0;
        for (int row = AmigaTerrainIndex::rows - 1; row >= 0; --row) {
            const unsigned cell = row * AmigaTerrainIndex::columns + column;
            if (index.cells[cell] & (landingCell | breakableCell)) next = row + 1;
            index.nextLanding[cell] = next;
        }
    }
    return true;
}

bool amigaIndexedRoomWall(const AmigaTerrainIndex& index, int x, int y) {
    return (indexCell(index, x, y) & roomWallCell) != 0;
}

bool amigaResolveTerrainWallContacts(void* opaque,const downwell::ContactBounds& query,unsigned& count){
    count=0;
    const auto& context=*static_cast<const AmigaTerrainCollisionContext*>(opaque);
    if(!context.index || !context.index->wallContactsComplete)return false;
    return context.index->wallContacts.visit(query,[&](unsigned object,int x,int y){
        return object!=92 || !context.runtime || context.runtime->breakableActiveAt(x-240,y-176);
    },[&](){++count;return true;});
}

bool amigaIndexedWall(const AmigaTerrainIndex& index, int x, int y) {
    return (indexCell(index, x, y) & (wallCell | roomWallCell)) != 0;
}

bool amigaAddTerrainObject(AmigaTerrainIndex& index, int objectId, int x, int y,
                           bool initialRoomWall) {
    if (!index.wallContacts.append(objectId,x,y))return false;
    const unsigned flags = objectId == 60
        ? solidCell | landingCell | wallCell | (initialRoomWall ? roomWallCell : 0)
        : objectId == 92 ? breakableCell : objectId == 93 ? landingCell : 0;
    if (!flags) return true;
    unsigned cell;
    {
        DW_GENERATION_SCOPE(profileGeneratedCell);
        if (!indexAdd(index, x, y, flags, &cell)) return false;
    }
    DW_GENERATION_SCOPE(profileGeneratedLanding);
    const std::uint16_t landing = cell / AmigaTerrainIndex::columns + 1;
    // An existing closer landing terminates the update. No clearing, full-map
    // scan or allocation is required when a new section becomes live.
    for (;;) {
        auto& next = index.nextLanding[cell];
        if (next && next <= landing) break;
        next = landing;
        if (cell < AmigaTerrainIndex::columns) break;
        cell -= AmigaTerrainIndex::columns;
    }
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
    return false;
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
                           solidCell, worldX, worldY);
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

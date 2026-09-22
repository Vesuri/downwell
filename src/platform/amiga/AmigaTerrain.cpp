#include "AmigaTerrain.h"

#include "AmigaPortRuntime.h"
#include "core/AreaOneBuilder.h"
#include "core/GameMakerRandom.h"

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
struct BuildContext {
    AmigaTerrainEnemy* enemies;
    unsigned capacity;
    unsigned count;
    bool overflowed;
};

bool collectGeneratedObject(
    void* opaque, const downwell::DeferredCreateInstance& command) {
    BuildContext& context = *static_cast<BuildContext*>(opaque);
    const bool enemy = command.objectId == 111 || command.objectId == 112 ||
                       command.objectId == 113 || command.objectId == 116 ||
                       command.objectId == 117 || command.objectId == 119;
    if (!enemy || context.enemies == 0)
        return true;
    if (context.count >= context.capacity) {
        context.overflowed = true;
        return false;
    }
    AmigaTerrainEnemy& output = context.enemies[context.count++];
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
    return true;
}
}

int amigaEnemyHealth(int objectId) {
    if (objectId == 111 || objectId == 112) return 5;
    if (objectId == 113) return 35;
    if (objectId == 116) return 15;
    if (objectId == 117) return 20;
    if (objectId == 119) return 3;
    return 0;
}

int amigaEnemyReward(int objectId) {
    if (objectId == 111 || objectId == 119) return 6;
    if (objectId == 112) return 14;
    if (objectId == 113) return 20;
    if (objectId == 116) return 12;
    if (objectId == 117) return 24;
    return 0;
}

bool amigaBuildInitialAreaOneTerrain(
    const downwell::BuilderPatternPack& builders,
    AmigaTerrainFragment* fragments, unsigned capacity,
    AmigaTerrainEnemy* enemies, unsigned enemyCapacity,
    unsigned& enemyCount, downwell::GameMakerRandom* randomAfter) {
    enemyCount = 0;
    if (fragments == 0 || capacity < amigaInitialTerrainFragmentCount)
        return false;
    enum {
        recoveredSeed = 2,
        entranceRandomDraws = 470,
        builderX = 240,
        builderY = 704,
        firstRightRoom = 3,
        secondRightRoom = 7
    };
    downwell::GameMakerRandom random(recoveredSeed);
    for (unsigned draw = 0; draw < entranceRandomDraws; ++draw)
        random.nextU32();
    downwell::AreaOneBuilder builder;
    builder.restoreInitialCheckpoint(builderX, builderY, -30,
                                     firstRightRoom, secondRightRoom);
    BuildContext context = {enemies, enemyCapacity, 0, false};
    unsigned fragmentCount = 0;
    for (unsigned step = 0; step < 64 && !builder.endReached(); ++step) {
        downwell::AreaOneBuilderStep result;
        if (!builder.step(builders, random, collectGeneratedObject,
                          &context, result))
            return false;
        if (!result.patternGenerated)
            continue;
        if (fragmentCount >= capacity)
            return false;
        downwell::PackedBuilderPattern pattern;
        if (!builders.findPattern(result.family, result.patternIndex, pattern))
            return false;
        fragments[fragmentCount++] = AmigaTerrainFragment{
            pattern, result.patternY, result.reversed, result.sideRoom,
            result.appendPillars};
    }
    enemyCount = context.count;
    const bool valid = !context.overflowed && builder.endReached() &&
        fragmentCount == amigaInitialTerrainFragmentCount &&
        random.drawCount() == 4770;
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

int amigaResolveTerrainLanding(void* opaque, int offsetX,
                               int minimumOffsetY) {
    const AmigaTerrainCollisionContext& context =
        *static_cast<const AmigaTerrainCollisionContext*>(opaque);
    const int worldX = 240 + offsetX;
    int landing = 4096 - 176;
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

bool amigaResolveTerrainSolid(void* opaque, int offsetX, int offsetY) {
    const AmigaTerrainCollisionContext& context =
        *static_cast<const AmigaTerrainCollisionContext*>(opaque);
    const int worldX = 240 + offsetX;
    const int worldY = 176 + offsetY;
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

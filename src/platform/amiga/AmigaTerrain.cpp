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
    unsigned fragmentCount = 0;
    for (unsigned step = 0; step < 64 && !builder.endReached(); ++step) {
        downwell::AreaOneBuilderStep result;
        if (!builder.step(
                builders, random,
                [](void*, const downwell::DeferredCreateInstance&) {
                    return true;
                },
                0, result))
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
    return builder.endReached() &&
           fragmentCount == amigaInitialTerrainFragmentCount &&
           random.drawCount() == 4770;
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

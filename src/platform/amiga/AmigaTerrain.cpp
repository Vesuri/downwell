#include "AmigaTerrain.h"

#include "AmigaPortRuntime.h"

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
    downwell::PackedBuilderPattern entrance;
    downwell::PackedBuilderPattern main32;
    downwell::PackedBuilderPattern main30;
    downwell::PackedBuilderPattern main27;
    downwell::PackedBuilderPattern right3;
    if (!builders.findPattern(downwell::BuilderPatternFamily::areaOneStart,
                              3, entrance) ||
        !builders.findPattern(downwell::BuilderPatternFamily::areaOneMain,
                              32, main32) ||
        !builders.findPattern(downwell::BuilderPatternFamily::areaOneMain,
                              30, main30) ||
        !builders.findPattern(downwell::BuilderPatternFamily::areaOneMain,
                              27, main27) ||
        !builders.findPattern(downwell::BuilderPatternFamily::areaOneRight,
                              3, right3))
        return false;
    fragments[0] = AmigaTerrainFragment{entrance, 1168, true, false, false};
    fragments[1] = AmigaTerrainFragment{main32, 1248, true, false, true};
    fragments[2] = AmigaTerrainFragment{main30, 1472, true, false, true};
    fragments[3] = AmigaTerrainFragment{main27, 1776, true, false, true};
    fragments[4] = AmigaTerrainFragment{right3, 1968, false, true, true};
    return true;
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

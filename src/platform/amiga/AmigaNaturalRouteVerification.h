#ifndef DOWNWELL_AMIGA_NATURAL_ROUTE_VERIFICATION_H
#define DOWNWELL_AMIGA_NATURAL_ROUTE_VERIFICATION_H
#include "AmigaPortRuntime.h"
#include "AmigaTerrain.h"

// Test-only seed-4 ordinary-input route. Entry checkpoint is checked against
// decoded startup by the paired host test; no terrain/exit/health is injected.
struct AmigaNaturalRouteVerification {
    int target = 240;
    static void entry(AmigaAreaOneTerrainBuilder& builder, downwell::GameMakerRandom& random) {
        random.setSeed(4);
        for (unsigned i = 0; i < 437; ++i) random.nextU32();
        downwell::AreaOneBuilder initial;
        initial.restoreInitialCheckpoint(240, 704, -30, 8, 13);
        builder.resetFromRoomEntry(initial);
    }
    downwell::Input input(const AmigaPortRuntime& game, AmigaTerrainCollisionContext& collision) {
        downwell::Input result;
        if (game.levelResultRequested() || game.levelClearActive()) { result.fire = true; return result; }
        if (game.runStage().level > 1) { result.fire = game.tick() < 4; return result; }
        const int x = 240 + game.player().offsetX(), y = 176 + game.player().offsetY();
        if (game.tick() % 16 == 0) {
            int best = -1000000;
            for (int candidate = 180; candidate <= 300; candidate += 8) {
                int floor = amigaResolveTerrainLanding(&collision, candidate - 240, y - 176 + 8);
                if (floor == downwell::gameplayNoLanding) floor = y + 400 - 176;
                int depth = floor + 176 - y;
                if (depth > 400) depth = 400;
                const int distance = candidate > x ? candidate - x : x - candidate;
                const int score = depth * 4 - distance;
                if (score > best) { best = score; target = candidate; }
            }
        }
        result.left = x > target + 2; result.right = x < target - 2;
        result.fire = game.tick() % 40 == 0 || game.tick() % 40 == 3;
        return result;
    }
};
#endif

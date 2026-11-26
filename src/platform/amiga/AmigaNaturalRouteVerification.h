#ifndef DOWNWELL_AMIGA_NATURAL_ROUTE_VERIFICATION_H
#define DOWNWELL_AMIGA_NATURAL_ROUTE_VERIFICATION_H
#include "AmigaPortRuntime.h"
#include "AmigaTerrain.h"

// Test-only seed-4 ordinary-input route. Entry checkpoint is checked against
// decoded startup by the paired host test; no terrain/exit/health is injected.
struct AmigaNaturalRouteVerification {
    int target = 240;
    int deepestY = 0;
    unsigned noDescent = 0;
    unsigned menuFrame = 0, returnFrames = 0, failures = 0;
    unsigned money = 0, stock = 0, price = 0;
    bool done = false;
    static void entry(AmigaAreaOneTerrainBuilder& builder, downwell::GameMakerRandom& random) {
        random.setSeed(4);
        for (unsigned i = 0; i < 325; ++i) random.nextU32();
        downwell::AreaOneBuilder initial;
        initial.restoreInitialCheckpoint(-16, 704, -30, 8, 4);
        builder.resetFromRoomEntry(initial);
    }
    downwell::Input input(const AmigaPortRuntime& game, AmigaTerrainCollisionContext& collision,
                         unsigned secondUpdates = 0) {
        downwell::Input result;
        if (game.levelResultRequested() || game.levelClearActive()) { result.fire = true; return result; }
        const bool second = game.runStage().level > 1;
        if (second && secondUpdates < 120) { result.fire = secondUpdates < 4; return result; }
        if (second && (menuFrame || game.shops().interactable)) {
            if (menuFrame < 7) {
                ++menuFrame;
                result.fire = menuFrame == 2 || menuFrame == 3 || menuFrame == 5 || menuFrame == 7;
                result.left = menuFrame == 6;
            } else {
                result.fire = returnFrames % 40 == 0;
                result.left = 240 + game.player().offsetX() > 240;
                ++returnFrames;
            }
            return result;
        }
        const unsigned tick = second ? secondUpdates - 120 : game.tick();
        const int x = 240 + game.player().offsetX(), y = 176 + game.player().offsetY();
        if (tick % 16 == 0) {
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
        {
            if (tick == 0) { deepestY = y; noDescent = 0; }
            if (y > deepestY + 8) { deepestY = y; noDescent = 0; }
            else ++noDescent;
            if (noDescent >= 120) target = (tick / 120) % 2 ? 180 : 300;
        }
        if (second) for (const auto& shop : game.shops().instances)
            if (shop.present && y < shop.y + 12 && y > shop.y - 200) target = shop.x - 6;
        result.left = x > target + 2; result.right = x < target - 2;
        result.fire = tick % 40 == 0 || tick % 40 == 3;
        if (noDescent >= 120) result.fire = tick % 12 == 0;
        return result;
    }
    void check(const AmigaPortRuntime& game) {
        if (game.dead()) ++failures;
        if (!menuFrame) return;
        const auto& shop = game.shops().instances[0];
        if (menuFrame == 2) {
            money = game.currency(); stock = shop.stock.items[0].remaining; price = shop.stock.items[0].price;
            if (!shop.menu.active || !game.shops().noControl) ++failures;
        }
        if (menuFrame == 3 && (money != game.currency() || stock != shop.stock.items[0].remaining)) ++failures;
        // Preserve the insufficient-funds scenario without pinning incidental
        // currency/stock values from the superseded two-word choose stream.
        if (menuFrame == 5 && (money >= price || game.currency() != money ||
                shop.stock.items[0].remaining != stock)) ++failures;
        if (menuFrame == 7 && game.shops().noControl) ++failures;
        if (returnFrames && 240 + game.player().offsetX() < 280 && game.cameraSteadyOffsetX() == 0) done = true;
    }
};
#endif

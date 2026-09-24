#include "core/RunSession.h"
#include "core/PlayerMotion.h"
#include "core/ShopFixtures.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::abort(); }
}
int main() {
    downwell::InteractionPrompt prompt;
    double sourcePhase = 0;
    for (unsigned tick = 0; tick < 10000; ++tick) {
        const bool eligible = (tick % 71) < 30;
        if (eligible) {
            if (sourcePhase < 2) sourcePhase = 2;
            if (sourcePhase <= 5.1) sourcePhase += 0.3;
        } else {
            sourcePhase += 0.1;
            if (sourcePhase > 2) sourcePhase -= 2;
        }
        prompt.step(eligible);
        std::uint64_t bits = 0;
        std::memcpy(&bits, &sourcePhase, sizeof(bits));
        require(prompt.phase.mantissa == ((bits & 0xfffffffffffffULL) | 0x10000000000000ULL) &&
            prompt.phase.exponent == int((bits >> 52) & 0x7ff) - 1023,
            "interaction prompt matches source binary64 phase exactly");
        require(prompt.frame() == unsigned(sourcePhase),
            "interaction prompt preserves binary64 frame boundaries through approach/leave cycles");
    }
    for (unsigned farewell = 0; farewell < 3; ++farewell) {
        downwell::ShopWorld world;
        unsigned call = 0;
        const unsigned lower[] = {100, 100, 102, 102, 0};
        const unsigned upper[] = {101, 103, 105, 105, 2};
        const unsigned values[] = {100, 101, 104, 105, farewell};
        auto* created = world.create(0, 0, [&](unsigned lo, unsigned hi) {
            require(call < 5 && lo == lower[call] && hi == upper[call],
                "source stock ranges precede scrShopAssignText's three-way choice");
            return values[call++];
        }, 4, 1);
        require(call == 5 && created->stock.items[2].upgrade == 105 &&
            created->farewellVariant == farewell, "all three creation-time farewell choices are retained");
    }
    using Feedback = downwell::ShopFeedback;
    using Effect = downwell::ShopInteraction::Effect;
    Feedback feedback;
    downwell::ShopInventory stock;
    const Effect replies[] = {Effect::welcome, Effect::hello, Effect::hoho,
        Effect::poor, Effect::question, Effect::what, Effect::huh, Effect::thank};
    const unsigned sounds[] = {101, 95, 96, 100, 98, 102, 97, 99};
    for (unsigned i = 0; i < 8; ++i) {
        feedback.beginStep();
        feedback.apply(replies[i], 1, stock);
        require(feedback.soundCount == 1 && feedback.sounds[0].resource == sounds[i] &&
            feedback.sounds[0].priority == 90 && feedback.sounds[0].channel == Feedback::Channel::merchant &&
            feedback.message != Feedback::Message::none, "source merchant reply resource/channel mapping");
    }
    feedback.confused = 2;
    feedback.beginStep(); feedback.apply(Effect::opened, 1, stock);
    require(feedback.confused == 2 && !feedback.thankful && !feedback.goodbye &&
        !feedback.resetKeeperImage && feedback.soundCount == 0,
        "opening must not invent a confusion clear or image reset");
    feedback.apply(Effect::purchased, 1, stock);
    require(feedback.itemPresent[0], "only exhausted stock removes case contents");
    feedback.beginStep();
    feedback.apply(Effect::moveRight, 1, stock); feedback.apply(Effect::moveLeft, 1, stock);
    feedback.apply(Effect::browse, 1, stock); feedback.apply(Effect::cancel, 1, stock);
    require(feedback.soundCount == 4 && feedback.sounds[0].resource == 109 &&
        feedback.sounds[1].resource == 109 && feedback.sounds[2].resource == 107 &&
        feedback.sounds[3].resource == 108 && feedback.sounds[3].channel == Feedback::Channel::ui,
        "direction, browse and cancel sounds retain order on UI channel");
    for (unsigned i = 0; i < Feedback::soundCapacity; ++i)
        feedback.apply(Effect::moveRight, 1, stock);
    require(feedback.overflow && feedback.soundCount == Feedback::soundCapacity,
        "feedback capacity failure is explicit, never out-of-bounds");
    downwell::RunSession run;
    downwell::CurrencyAccount money;
    money.currency = 1000;
    unsigned draws = 0;
    auto range = [&](unsigned lo, unsigned) { ++draws; return lo + (draws % 4 == 2 ? 1 : 0); };
    auto* shop = run.shops.create(5, 0, range, 4, 1);
    require(shop && draws == 5 && shop->farewellVariant == 0,
        "registration consumes stock rolls followed by one farewell choice");
    unsigned fixtures = 0;
    downwell::drawShopFixtures(run.shops, [&](int sprite, int frame, int x, int y) {
        const int expectedSprite[] = {480, 481, 482, 482, 482, 152, 143, 151};
        const int expectedX[] = {5, 69, 50, 69, 88, 50, 69, 88};
        require(fixtures < 8 && sprite == expectedSprite[fixtures] && frame == (fixtures < 5 ? 0 : 1) &&
            x == expectedX[fixtures] && y == (fixtures < 2 ? 0 : fixtures < 5 ? -9 : -8),
            "fixture draw stream preserves original IDs, positions and creation order");
        ++fixtures;
    });
    require(fixtures == 8 && run.shops.fixtureRevision == 1, "registration invalidates fixture rows once");
    fixtures = 0;
    downwell::drawShopKeepers(run.shops, [&](int sprite, unsigned frame, int x, int y) {
        require(sprite == 485 && frame == 0 && x == 23 && y == 0,
            "merchant drawing uses recovered child anchor and current animation");
        ++fixtures;
    });
    require(fixtures == 1, "merchant is a separate moving-object draw");
    require(run.shops.playerSolidAt(39, 0) && run.shops.playerSolidAt(98, 7) &&
        !run.shops.playerSolidAt(38, 0) && !run.shops.playerSolidAt(99, 7),
        "table uses cropped solid mask, not full artwork bounds");
    require(run.shops.playerSolidAt(41, -18) && run.shops.playerSolidAt(58, -1) &&
        !run.shops.playerSolidAt(40, -18) && !run.shops.playerSolidAt(59, -1),
        "glass case uses 18x18 mask and leaves one-pixel gaps between cases");
    require(run.shops.playerSolidAt(-3, 0) && run.shops.playerSolidAt(12, 15) &&
        !run.shops.playerSolidAt(-4, 0), "counter child uses full invisible-wall mask");
    require(run.shops.playerLanding(50, -100, 1000) == -26 &&
        run.shops.playerLanding(50, -25, 1000) == -8,
        "landing chooses case top before table, skipping tops already above the player");
    for (int shopX : {0, -45}) {
        downwell::ShopWorld fallingWorld;
        fallingWorld.create(shopX, 100, range, 4, 1);
        PlayerMotion falling(false, 1000);
        falling.configureLandingResolver([](void* opaque, int x, int minimumY) {
            return static_cast<downwell::ShopWorld*>(opaque)->playerLanding(x, minimumY, 1000);
        }, &fallingWorld);
        for (unsigned i = 0; i < 120 && !falling.grounded(); ++i) falling.step({});
        require(falling.grounded() && falling.offsetY() == (shopX == 0 ? 92 : 74),
            "shared player lands on invisible wall or case at recovered top");
    }
    PlayerMotion player;
    unsigned greetings = 0;
    auto tick = [&](bool fire, bool left = false, bool right = false) {
        downwell::Input input; input.fire = fire; input.left = left; input.right = right;
        player.step(input, run.shops.noControl, false, run.shops.interactable);
        run.shops.observePlayer(player.offsetX(), player.offsetY(),
            player.grounded(), player.speedXFixed());
        int facing = player.facing();
        run.shops.step(input, player.offsetX(), player.offsetY(), false, false,
            facing, money, [&] { ++greetings; return 1u; },
            [&](unsigned id) { run.consumables.grant(id, money); },
            [](auto&, auto) {});
    };
    tick(false); // Publish eligibility for the next player Begin Step.
    require(run.shops.interactable, "stationary grounded mask overlap is eligible");
    tick(true);
    require(shop->menu.active && run.shops.noControl && player.grounded() &&
        money.currency == 1000 && greetings == 1, "opening consumes press without jumping or buying");
    tick(true);
    require(money.currency == 1000, "held confirmation does not purchase");
    require(shop->feedback.soundCount == 0 && !shop->feedback.resetKeeperImage,
        "presentation requests expire instead of replaying while input is held");
    tick(false); tick(true);
    require(money.currency == 700 && run.consumables.pending[0] == 1 &&
        shop->stock.items[0].remaining == 0, "fresh confirmation purchases into run queue");
    require(!shop->feedback.storedItem[0] && !shop->feedback.itemPresent[0] &&
        shop->feedback.itemPresent[1] && shop->feedback.thankful == 1 &&
        shop->feedback.resetKeeperImage && shop->feedback.soundCount == 2 &&
        shop->feedback.sounds[0].resource == 104 && shop->feedback.sounds[1].resource == 105 &&
        shop->feedback.sounds[1].priority == 50,
        "purchase removes only purchased contents and retains ordered merchant/UI feedback");
    require(run.shops.fixtureRevision == 1, "purchase must not dirty unchanged furniture");
    require(shop->artRevision == 2, "purchase invalidates cached contents once");
    fixtures = 0;
    downwell::drawShopFixtures(run.shops, [&](int sprite, int, int, int) {
        require(sprite != 152, "purchased item disappears from shared drawing stream");
        ++fixtures;
    });
    require(fixtures == 7, "only purchased item disappears, not its case");
    require(run.shops.playerSolidAt(50, -10), "sold case remains physically solid");
    tick(false, true); // First item -> Leave; locked player stays still.
    require(shop->menu.cursor == 0 && player.offsetX() == 0, "menu navigation does not move player");
    tick(true);
    require(!run.shops.noControl && !shop->menu.active, "Leave unlocks world controls");
    require(shop->feedback.goodbye == 1 && shop->feedback.resetKeeperImage &&
        shop->feedback.message == downwell::ShopFeedback::Message::thank,
        "thanked exit requests keeper goodbye and rising text");
    tick(false); tick(true);
    require(shop->menu.active && shop->stock.items[0].remaining == 0, "reentry retains sold stock");
    require(shop->farewellVariant == 0, "exit and reentry do not reroll farewell text");
    require(shop->feedback.goodbye == 0 && shop->feedback.thankful == 0 &&
        !shop->feedback.itemPresent[0], "opening clears keeper thanks/goodbye without respawning contents");
    using World = downwell::ShopWorld;
    require(World::meets(*shop, -8, -8) && !World::meets(*shop, -9, -8) &&
        World::meets(*shop, 8, 12) && !World::meets(*shop, 9, 12), "inclusive source mask edges");
    run.shops.observePlayer(0, 0, true, 1);
    require(!run.shops.interactable, "horizontal movement disallows interaction");
    for (unsigned i = 1; i < World::capacity; ++i)
        require(run.shops.create(100, 0, range, 4, 1), "bounded registration succeeds");
    const unsigned before = draws;
    require(!run.shops.create(100, 0, range, 4, 1) && draws == before,
        "exhaustion is explicit and consumes no RNG");
    run.shops.endStep(320);
    require(shop->present, "retirement boundary is strict");
    run.shops.endStep(321);
    require(!shop->present, "offscreen shop retires at source threshold");
    require(!shop->wallPresent && run.shops.playerSolidAt(50, -10) && shop->allocated,
        "invisible wall retires separately; tables/cases survive counter retirement");
    run.reset();
    require(!run.shops.noControl && !run.shops.interactable &&
        !run.shops.previous.fire && !run.shops.instances[0].present &&
        run.consumables.pending[0] == 0, "new run clears shops, edges, locks and deferred grants");
    std::puts("Shop world ownership, player input ordering and purchase lifecycle passed");
}

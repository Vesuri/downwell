#include "core/RunSession.h"
#include "core/PlayerMotion.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::abort(); }
}
int main() {
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
            feedback.sounds[0].volume == 90 && feedback.sounds[0].channel == Feedback::Channel::merchant &&
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
    require(shop && draws == 4, "registration consumes stock RNG once");
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
        shop->feedback.sounds[1].volume == 50,
        "purchase removes only purchased contents and retains ordered merchant/UI feedback");
    tick(false, true); // First item -> Leave; locked player stays still.
    require(shop->menu.cursor == 0 && player.offsetX() == 0, "menu navigation does not move player");
    tick(true);
    require(!run.shops.noControl && !shop->menu.active, "Leave unlocks world controls");
    require(shop->feedback.goodbye == 1 && shop->feedback.resetKeeperImage &&
        shop->feedback.message == downwell::ShopFeedback::Message::thank,
        "thanked exit requests keeper goodbye and rising text");
    tick(false); tick(true);
    require(shop->menu.active && shop->stock.items[0].remaining == 0, "reentry retains sold stock");
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
    run.reset();
    require(!run.shops.noControl && !run.shops.interactable &&
        !run.shops.previous.fire && !run.shops.instances[0].present &&
        run.consumables.pending[0] == 0, "new run clears shops, edges, locks and deferred grants");
    std::puts("Shop world ownership, player input ordering and purchase lifecycle passed");
}

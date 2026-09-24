#include "core/LevelClearSelection.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
    using Menu = downwell::LevelClearSelection;
    using Effect = Menu::Effect;
    Menu menu;
    require(menu.create(2) && menu.cursor == 1 && menu.alarm4 == 60, "default Result entrance");
    unsigned remaining = 1, grants = 0, rolls = 0, moves = 0, dither = 0, next = 0;
    unsigned tick = 0;
    auto emit = [&](Effect effect, unsigned value) {
        if (effect == Effect::rollOffers) ++rolls;
        if (effect == Effect::moveSound) ++moves;
        if (effect == Effect::grantChoice) {
            require(tick == 90 && remaining == 1 && value == 0,
                "grant occurs before decrement with the selected cursor"); ++grants;
        }
        if (effect == Effect::dither) { require(tick == 121, "exit dither follows thirty alarm phases"); ++dither; }
        if (effect == Effect::nextLevel) { require(tick == 211, "next level follows ninety further phases"); ++next; }
    };
    for (tick = 1; tick <= 220; ++tick) {
        menu.advanceAlarms(emit);
        bool confirm = true;
        menu.step(confirm, tick == 60 || tick == 61, tick == 60, remaining, emit);
        if (tick < 60) require(confirm && !menu.visible, "entrance ignores rather than consumes confirm");
        if (tick == 60) require(menu.visible && menu.cursor == 1 && moves == 2 && rolls == 1 && menu.alarm3 == 30,
            "both direction edges apply during entrance wait and preserve newly armed alarm");
        if (tick >= 60 && tick <= 90) require(!confirm, "selection consumes held confirm including while waiting");
        if (tick == 89) require(grants == 0 && menu.inputWait, "confirm locked for thirty full alarm phases");
        if (tick == 90) require(!menu.visible && menu.next == 4 && remaining == 0,
            "last grant hides options but defers departure to next Step");
        if (tick == 91) require(menu.next == 7 && menu.alarm0 == 30, "departure arms thirty-tick alarm once");
    }
    require(grants == 1 && rolls == 1 && dither == 1 && next == 1, "selection and transitions are one-shot");
    menu.create(1); remaining = 0;
    menu.next = 4;
    bool held = true;
    menu.step(held, false, false, remaining, [](Effect, unsigned) { std::abort(); });
    require(menu.next == 7 && menu.alarm0 == 30 && !held && !menu.visible,
        "zero-upgrade styles depart without generating offers");
    menu.create(3); menu.next = 4; remaining = 2;
    auto ignore = [](Effect, unsigned) {};
    held = false; menu.step(held, false, false, remaining, ignore);
    menu.inputWait = false; menu.cursor = 3; held = true;
    unsigned order = 0;
    menu.step(held, false, false, remaining, [&](Effect effect, unsigned value) {
        const Effect expected[] = {Effect::grantChoice, Effect::confirmSound, Effect::confirmationEffect, Effect::rollOffers};
        require(order < 4 && effect == expected[order++], "grant and feedback precede reroll");
        if (effect == Effect::grantChoice) require(value == 3 && remaining == 2, "grant receives prior cursor/count");
        if (effect == Effect::rollOffers) require(menu.cursor == 1 && remaining == 1, "reroll resets cursor after decrement");
    });
    require(order == 4 && menu.visible && !menu.inputWait && !held, "additional award does not invent another input wait");
    std::puts("Level-clear selection timing, input and effect-order tests passed");
}

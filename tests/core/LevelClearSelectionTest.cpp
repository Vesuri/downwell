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
        if (effect == Effect::nextLevel) { require(tick == 210, "alarm 1 includes its arming phase in the ninety countdowns"); ++next; }
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
    menu.create(2); menu.alarm0 = menu.alarm1 = 1;
    unsigned departures = 0;
    menu.advanceAlarms([&](Effect effect, unsigned) {
        if (effect == Effect::nextLevel) ++departures;
    });
    require(menu.alarm0 == 0 && menu.alarm1 == 89 && departures == 0,
        "Alarm 0 overwrites the formerly due Alarm 1 before its countdown");

    menu.create(2); menu.alarm0 = 1;
    menu.advanceAlarms([&](Effect effect, unsigned) {
        if (effect == Effect::dither) menu.alarm1 = 1;
        if (effect == Effect::fadeWhoosh) {
            require(menu.alarm1 == 90, "post-Create source assignment overwrites child mutation");
            menu.alarm1 = 1; menu.alarm3 = 1; menu.alarm4 = 1;
            menu.alarm0 = 2;
        }
        if (effect == Effect::nextLevel) ++departures;
    });
    require(departures == 1 && menu.alarm1 == 0 && !menu.inputWait && menu.next == 4,
        "synchronous event writes reach every later alarm in the same phase");
    require(menu.alarm0 == 2, "earlier alarm is not revisited after a callback writes it");
    menu.alarm0 = -1;
    menu.advanceAlarms([&](Effect, unsigned) { std::abort(); });
    require(menu.alarm1 == -1 && menu.alarm3 == -1 && menu.alarm4 == -1,
        "zero alarms retire without dispatch on the following phase");
    for(auto failed:{Effect::dither,Effect::fadeWhoosh,Effect::nextLevel}){
        menu.create(2);menu.alarm0=failed==Effect::nextLevel?-1:1;menu.alarm1=1;
        unsigned calls=0;
        require(!menu.advanceAlarmsChecked([&](Effect effect,unsigned){++calls;return effect!=failed;}),
            "checked alarm failure stops the phase");
        require(calls==(failed==Effect::fadeWhoosh?2u:1u)&&
            menu.alarm1==(failed==Effect::dither?1:failed==Effect::fadeWhoosh?90:0)&&
            menu.alarm4==60&&menu.next==0,
            "failure preserves exactly the completed alarm prefix without later countdowns");
    }
    std::puts("Level-clear selection timing, input and effect-order tests passed");
}

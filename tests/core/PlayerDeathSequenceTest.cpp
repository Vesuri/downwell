#include "core/PlayerDeathSequence.h"
#include <cstdlib>
#include <iostream>
static void require(bool value, const char* message) {
    if (!value) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    downwell::PlayerDeathSequence death;
    death.beginStep(true);
    require(!death.advanceAlarms(true) && !death.active && death.frames == 0,
            "living input does not start death alarms");
    death.enter();
    death.enter();
    require(death.animation == 1 && death.frames == 0 && death.alarm == -1,
            "depletion latches once without inventing a contact-time alarm");
    for (unsigned tick = 1; tick <= 150; ++tick) {
        death.beginStep(true);
        const bool effect = death.advanceAlarms(true);
        require(death.frames == tick && death.skipPresses == 0,
                "holding fire across death does not generate skip edges");
        require(effect == (tick == 30), "grounded landing effect occurs once at first alarm");
        require(death.menuReady == (tick == 150), "menu opens after 30 then 120 updates");
        if (tick == 1) require(death.alarm == 29, "flash Create changes speed before same-frame countdown");
        if (tick == 30) require(death.animation == -1 && death.alarm == 120,
                               "alarm callback replacement is not decremented twice");
    }
    require(death.animation == -2, "menu publication uses source terminal animation state");
    downwell::PlayerDeathSequence skip;
    skip.enter();
    for (int edge = 1; edge <= 3; ++edge) {
        skip.beginStep(false); skip.advanceAlarms(false);
        skip.beginStep(true); skip.advanceAlarms(false);
        require(skip.menuReady == (edge == 3), "three press edges open menu, not three held updates");
    }
    for (unsigned i = 0; i < 240; ++i) {
        skip.beginStep(true);
        require(!skip.advanceAlarms(true), "terminal negative state does not restart positive death branch");
    }
    require(skip.animation == -2 && skip.menuReady, "skip remains terminal after old alarm expires");
    downwell::PlayerDeathSequence alternate;
    alternate.enter(); alternate.beginStep(false, 40); alternate.advanceAlarms(false);
    require(alternate.alarm == 59, "first delay derives from supplied post-Create room speed");
    std::cout << "shared player death sequence tests passed\n";
}

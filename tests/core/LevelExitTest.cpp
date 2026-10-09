#include "core/LevelExit.h"
#include "core/ExitDither.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
    downwell::ExitDither dither;
    dither.create(160, 1000);
    require(dither.x == 240 && dither.y == 900 && dither.alarm0 == 15,
        "transition captures world anchor from creation-time view");
    dither.step(1000, 284);
    require(dither.y == 908 && dither.alarm0 == 15,
        "creation-frame ordinary Step moves without ticking new alarm");
    for (unsigned tick = 1; tick <= 60; ++tick) {
        dither.advanceAlarm();
        dither.step(1000, 284);
        require(dither.soundRequested == (tick >= 15), "transition sound fires fifteen alarm phases later");
    }
    require(dither.y == 1284 && dither.x == 240, "transition stops at view bottom without horizontal following");
    dither.step(1001, 284);
    require(dither.y == 1292, "moving camera can resume eight-pixel motion; no invented clamping");
    using Exit = downwell::LevelExit;
    Exit exit;
    unsigned creates = 0;
    exit.create(480, 4000, [&](Exit::Effect effect, int x, int y) {
        require(x == 240 && y == 4000 + int(creates)*32, "inherited centered child anchors");
        require(effect == (creates ? Exit::Effect::goalTimeField : Exit::Effect::roomEndSetter),
            "inherited Create precedes derived goal field Create");
        ++creates;
    });
    require(creates == 2, "both children created synchronously");
    require(exit.meetsPlayer(240, 3977) && !exit.meetsPlayer(240, 3976) &&
        exit.meetsPlayer(240, 4019) && !exit.meetsPlayer(240, 4020),
        "trigger collision includes exact top and bottom player-mask contacts");
    require(exit.meetsPlayer(-163, 4000) && !exit.meetsPlayer(-164, 4000) &&
        exit.meetsPlayer(643, 4000) && !exit.meetsPlayer(644, 4000),
        "wide trigger collision includes exact horizontal mask contacts");
    bool stop = false, locked = false;
    require(!exit.collide(true, stop, locked) && !stop && !locked && exit.alarm0 == -1,
        "dead player cannot begin level exit");
    require(exit.collide(false, stop, locked) && stop && locked && exit.active && exit.alarm0 == 5,
        "first living collision locks and arms the source countdown");
    unsigned ditherEvents = 0, result = 0;
    for (unsigned tick = 1; tick <= 100; ++tick) {
        exit.advanceAlarms([&](Exit::Effect effect, int x, int y) {
            require(x == 0 && y == 0, "alarm transition origin");
            if (effect == Exit::Effect::ditherTransition) {
                require(tick == 5, "dither starts on fifth alarm phase"); ++ditherEvents;
            } else {
                require(effect == Exit::Effect::resultRoom && tick == 94,
                    "result-room request counts Alarm 1 in the arming phase"); ++result;
            }
        });
        require(!exit.collide(tick > 10, stop, locked), "later living/dead collisions cannot rearm exit");
        if (tick == 5) require(exit.alarm1 == 89, "later alarm decrements after being armed");
    }
    require(ditherEvents == 1 && result == 1 && exit.alarm0 == -1 && exit.alarm1 == -1,
        "alarms are one-shot and later death does not invent cancellation");
    exit.alarm0=exit.alarm1=1;
    exit.advanceAlarms([&](Exit::Effect effect,int,int){
        require(effect==Exit::Effect::ditherTransition,"overwritten due alarm must not request a room");
        exit.alarm1=1; // Synchronous child Create, before the source assigns 90.
    });
    require(exit.alarm0==0&&exit.alarm1==89,"source post-Create assignment wins before later countdown");
    std::puts("Shared level-exit Create, collision and alarm sequence passed");
}

#include "core/LevelExit.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main() {
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
    bool stop = false, locked = false;
    require(!exit.collide(true, stop, locked) && !stop && !locked && exit.alarm0 == -1,
        "dead player cannot begin level exit");
    require(exit.collide(false, stop, locked) && stop && locked && exit.active && exit.alarm0 == 5,
        "first living collision locks and arms the source countdown");
    unsigned dither = 0, result = 0;
    for (unsigned tick = 1; tick <= 100; ++tick) {
        exit.advanceAlarms([&](Exit::Effect effect, int x, int y) {
            require(x == 0 && y == 0, "alarm transition origin");
            if (effect == Exit::Effect::ditherTransition) {
                require(tick == 5, "dither starts on fifth alarm phase"); ++dither;
            } else {
                require(effect == Exit::Effect::resultRoom && tick == 95,
                    "result-room request follows ninety further alarm phases"); ++result;
            }
        });
        require(!exit.collide(tick > 10, stop, locked), "later living/dead collisions cannot rearm exit");
        if (tick == 5) require(exit.alarm1 == 90, "new alarm retains full duration in dispatch frame");
    }
    require(dither == 1 && result == 1 && exit.alarm0 == -1 && exit.alarm1 == -1,
        "alarms are one-shot and later death does not invent cancellation");
    std::puts("Shared level-exit Create, collision and alarm sequence passed");
}

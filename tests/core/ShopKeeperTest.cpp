#include "core/ShopKeeper.h"
#include <cstdio>
#include <cstdlib>
static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::abort(); }
}
int main() {
    downwell::ShopKeeper keeper;
    downwell::ShopFeedback feedback;
    unsigned rolls = 0;
    auto random = [&](unsigned lo, unsigned hi) { require(lo == 20 && hi == 60, "source alarm range"); ++rolls; return 20; };
    auto tick = [&](int x, unsigned area = 1) {
        keeper.alarms(feedback, random);
        keeper.step(feedback, x, 200, false, area, random);
        keeper.advanceAnimation();
        feedback.beginStep();
    };
    volatile double reference = 0;
    for (int i = 0; i < 10000; ++i) {
        tick(190); reference = reference + 0.09;
        if (reference >= 4) reference = reference - 4;
        require(keeper.frame() == static_cast<unsigned>(reference), "idle frame matches binary64 runner arithmetic");
    }
    require(rolls == 0, "idle consumes no random draws");
    tick(200);
    require(keeper.sprite == 487 && keeper.frame() == 0 && keeper.alarm0 == 20 && rolls == 1,
        "equal X enters angry state and rolls once");
    for (int i = 0; i < 19; ++i) tick(200);
    require(keeper.frame() == 0, "angry alarm does not fire early");
    tick(200);
    require(keeper.frame() == 1 && keeper.alarm1 == 5, "angry blink starts at alarm zero");
    for (int i = 0; i < 5; ++i) tick(200);
    require(keeper.frame() == 0 && rolls == 2 && keeper.alarm0 == 20, "blink rearms random wait after five updates");
    feedback.thankful = 1; feedback.resetKeeperImage = true;
    for (int i = 0; i < 12; ++i) tick(190);
    require(keeper.sprite == 486 && keeper.frame() == 4 && feedback.thankful == 2 && keeper.alarm2 == 60,
        "thank animation holds its final frame for sixty updates");
    for (int i = 0; i < 60; ++i) tick(190);
    require(feedback.thankful == 0 && keeper.sprite == 485, "alarm two returns keeper to idle");
    feedback.goodbye = 1; feedback.resetKeeperImage = true;
    tick(190);
    require(feedback.goodbye == 2 && keeper.alarm2 == 240 && keeper.sprite == 496,
        "goodbye starts a 240-update looping animation");
    tick(201);
    require(!feedback.goodbye, "moving right of keeper clears reaction flags at Step tail");
    tick(201);
    require(keeper.sprite == 487, "angry sprite follows on next Step");
    keeper = {}; feedback = {}; feedback.confused = 1; feedback.resetKeeperImage = true;
    for (int i = 0; i < 7; ++i) tick(190);
    require(keeper.sprite == 497 && keeper.frame() == 1 && feedback.confused == 2 && keeper.alarm2 == 60,
        "confused reaction freezes after passing image index one");
    feedback.thankful = feedback.goodbye = feedback.confused = 1;
    feedback.resetKeeperImage = true;
    tick(190);
    require(keeper.sprite == 486 && feedback.confused == 0 && feedback.goodbye == 1 &&
        keeper.speed.equal(downwell::HudReal{0x1999999999999aULL, -3}),
        "thank branch wins, but goodbye tail still overrides its animation speed");
    keeper = {}; feedback = {}; reference = 0;
    for (int i = 0; i < 1000; ++i) {
        tick(190, 3); reference = reference + 0.045;
        if (reference >= 4) reference = reference - 4;
        require(keeper.frame() == static_cast<unsigned>(reference), "area three halves animation rate");
    }
    keeper.step(feedback, 201, 200, true, 1, random);
    require(!keeper.present && keeper.spawnEnemy, "criminal branch requests enemy replacement");
    std::puts("Recovered merchant Step, alarms and binary64 animation tests passed");
}

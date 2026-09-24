#include "core/FrogBehavior.h"
#include "core/FractionalMotion.h"
#include "core/FrogHorizontalMotion.h"
#include "core/FrogVerticalMotion.h"
#include "core/FrogHitState.h"
#include "core/FrogMotion.h"
#include "core/FrogPresentation.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}

int main() {
    {
        downwell::FrogPresentation presentation;
        unsigned appliedRate = presentation.beginStepRate();
        auto visual = presentation.finishStep(false, true, false, false);
        require(visual.sprite == 166 && visual.forcedFrame == -1 &&
                    appliedRate == 0 && presentation.desiredRate == 4,
                "first landing chooses idle and schedules imgSp without changing this Step's rate");
        appliedRate = presentation.beginStepRate();
        visual = presentation.finishStep(false, false, false, true);
        require(appliedRate == 4 && presentation.desiredRate == 0 &&
                    visual.sprite == 168 && visual.forcedFrame == 0,
                "takeoff forces rising pose but only schedules the animation stop");
        require(presentation.beginStepRate() == 0, "next Step applies the airborne zero rate");
        visual = presentation.finishStep(false, false, false, false);
        require(visual.sprite == 168 && visual.forcedFrame == 1,
                "zero/downward vertical speed selects the falling pose");
        visual = presentation.finishStep(false, true, true, false);
        require(visual.sprite == 170 && visual.forcedFrame == -1 && presentation.desiredRate == 0,
                "preparation preserves imgSp instead of inventing an idle rate");
        visual = presentation.finishStep(true, true, false, false);
        require(visual.sprite == 173 && visual.forcedFrame == -1 && presentation.desiredRate == 0,
                "damage takes priority over landing and preserves imgSp and pose phase");
        presentation.finishStep(false, true, false, false);
        visual = presentation.finishStep(true, false, false, true);
        require(visual.sprite == 173 && visual.forcedFrame == -1 && presentation.desiredRate == 4,
                "airborne stun must not execute the flight rate/pose branch");
    }
    for (int direction : {-1, 1}) {
        const auto prepare = downwell::frogPreparationAlarm(direction);
        require(prepare.preparation == 1 && prepare.setX && !prepare.setY &&
                prepare.setJumpSpeed && prepare.xQ16 == direction * 65536L &&
                prepare.jumpSpeed == direction * 2 && prepare.alarm == 0 &&
                prepare.ticks == 60 && prepare.sound == 113,
                "source preparation produces shared velocity, alarm and sound effects");
        const auto jump = downwell::frogJumpAlarm(true, prepare.jumpSpeed);
        require(jump.preparation == 0 && jump.setX && jump.setY &&
                !jump.setJumpSpeed && jump.xQ16 == direction * 2 * 65536L &&
                jump.yFifthsQ8 == -4 * 1280L && jump.alarm == 1 &&
                jump.ticks == 60 && jump.sound == 116,
                "source jump uses stored signed launch speed and resets preparation");
    }
    const auto retry = downwell::frogPreparationAlarm(0);
    require(retry.preparation == -1 && !retry.setX && !retry.setY &&
            !retry.setJumpSpeed && retry.alarm == 1 && retry.ticks == 60 && retry.sound == -1,
            "failed preparation preserves state and retries without sound");
    const auto airborne = downwell::frogJumpAlarm(false, 2);
    require(airborne.preparation == 0 && !airborne.setX && !airborne.setY &&
            airborne.alarm == 1 && airborne.ticks == 60 && airborne.sound == -1,
            "airborne jump alarm cancels preparation without changing velocity");
    bool landed = false;
    require(!downwell::frogLandingEdge(false, landed) && !landed &&
            downwell::frogLandingEdge(true, landed) && landed &&
            !downwell::frogLandingEdge(true, landed) && landed &&
            !downwell::frogLandingEdge(false, landed) && !landed,
            "shared landing latch rearms only on an actual support edge");
    downwell::FrogHorizontalMotion sharedX{0, 2 * 65536L};
    downwell::FrogVerticalMotion sharedY{1000, 3 * 1280L, false};
    downwell::FrogHitState sharedHit;
    sharedHit.pending = true;
    sharedHit.advanceFrame();
    require(downwell::stepFrogMotion(sharedX, sharedY, sharedHit, true,
                [](int, int) { return false; }) &&
            sharedX.position == 104858 && sharedX.velocity == 104858 &&
            sharedY.position == 1000 && sharedY.velocity == 0,
            "shared step consumes hit, damps X and clears Y before movement");
    downwell::FrogHitState hit;
    hit.pending = true;
    require(!hit.consume(), "queued bullet hit is not consumed before the next frame");
    hit.advanceFrame();
    require(hit.consume() && hit.remaining == 5 && !hit.consume(),
            "hit is consumed once and starts the recovered five-tick stun");
    for (unsigned tick = 1; tick <= 5; ++tick) {
        hit.advanceFrame();
        require(hit.remaining == 5 - tick, "hit alarm expires after five following alarm phases");
    }
    require(downwell::FrogHitState::dampHorizontal(2 * 65536L) == 104858 &&
            downwell::FrogHitState::dampHorizontal(-2 * 65536L) == -104858,
            "hit damps horizontal speed by source .8 in either direction");
    downwell::FrogVerticalMotion vertical;
    vertical = {1000, 3 * 1280, false};
    vertical.step([](int) { return false; }, true, true);
    require(vertical.velocity == 0 && vertical.position == 1000,
            "hit resets vertical velocity after gravity and before integration");
    vertical = {};
    for (long tick = 1; tick <= 40; ++tick) {
        vertical.step([](int) { return false; });
        require(vertical.velocity == 192 * tick && vertical.position == 96 * tick * (tick + 1),
                "shared dry gravity and displacement retain exact fifth-Q8 motion");
    }
    for (unsigned i = 0; i < 20; ++i) vertical.step([](int) { return false; });
    require(vertical.velocity == 7 * 1280, "shared frog terminal speed is seven pixels");
    vertical = {};
    for (unsigned i = 0; i < 4; ++i) vertical.step([](int y) { return y >= 9; });
    require(vertical.grounded && vertical.velocity == 0 && vertical.position == 1152,
            "provisional support query retains complete fractional landing position");
    vertical.step([](int) { return false; });
    require(!vertical.grounded && vertical.position == 1344 && vertical.velocity == 192,
            "removing support makes the shared frog fall immediately");
    vertical = {0, -4 * 1280, true};
    vertical.step([](int y) { return y >= 9; });
    require(!vertical.grounded && vertical.position == -4928 && vertical.pixel() == -4,
            "jump alarm launch retains negative fractional state and floor query coordinates");
    downwell::FrogHorizontalMotion horizontal;
    horizontal.velocity = -2 * 65536L;
    horizontal.step([](int) { return true; });
    require(horizontal.position == 0 && horizontal.velocity == 39322,
            "source -0.3 rebound retains Q16 precision and does not advance blocked X");
    for (unsigned i = 0; i < 100; ++i) horizontal.step([](int) { return false; });
    require(std::fabs(horizontal.position / 65536.0 - 60.0) < 0.001,
            "one hundred rebound steps retain fractional movement rather than rounded Q8 speed");
    for (int sign : {-1, 1}) {
        horizontal = {};
        horizontal.velocity = sign * 2 * 65536L;
        double expected = sign * 2.0;
        for (unsigned i = 0; i < 8; ++i) {
            horizontal.step([](int) { return true; });
            expected *= -0.3;
            require(std::fabs(horizontal.velocity / 65536.0 - expected) < 1.0 / 65536.0,
                    "repeated signed rebounds remain within one Q16 unit of source multiplier");
        }
    }
    horizontal = {};
    horizontal.velocity = -1;
    int probe = 0;
    horizontal.step([&probe](int edge) { probe = edge; return false; });
    require(probe == -7 && horizontal.position == -1,
            "negative fractional X uses floor for the provisional terrain point query");
    long position = 0;
    unsigned char remainder = 0;
    downwell::integrateFifthsQ8(position, remainder, -1, 1);
    require(position == -1 && remainder == 1,
            "negative displacement preserves a nonnegative residual");
    downwell::integrateFifthsQ8(position, remainder, 0, 4);
    require(position == 0 && remainder == 0,
            "opposite fractional displacements cancel exactly");
    using downwell::frogPreparationDirection;
    using downwell::frogMotionVisible;
    require(frogMotionVisible(0, 173 * 256L, -80 * 256L, -142 * 256L, 80 * 256L, 142 * 256L),
            "motion includes the source's extra thirty-two-pixel bottom margin");
    require(!frogMotionVisible(0, 174 * 256L, -80 * 256L, -142 * 256L, 80 * 256L, 142 * 256L),
            "expanded motion bottom edge is strict");
    require(!frogMotionVisible(80 * 256L, 0, -80 * 256L, -142 * 256L, 80 * 256L, 142 * 256L) &&
            !frogMotionVisible(0, -142 * 256L, -80 * 256L, -142 * 256L, 80 * 256L, 142 * 256L),
            "movement adds no horizontal or top margin");
    vertical = {};
    for (unsigned i = 0; i < 60; ++i) vertical.step([](int) { return false; }, false);
    require(vertical.position == 0 && vertical.velocity == 7 * 1280,
            "offscreen gravity continues to terminal speed without displacement");
    vertical.step([](int) { return false; }, true);
    require(vertical.position == 7 * 1280,
            "entering view integrates accumulated offscreen velocity");
    horizontal = {};
    horizontal.velocity = 2 * 65536L;
    horizontal.step([](int) { return true; }, false);
    require(horizontal.velocity == -39322 && horizontal.position == 0,
            "offscreen collision still rebounds without integrating position");
    auto direction = [](long x, long y, long playerX, bool grounded = true) {
        return frogPreparationDirection(grounded, x, y, playerX,
            -80 * 256L, -142 * 256L, 80 * 256L, 142 * 256L);
    };
    require(direction(0, 0, 0) == -1, "source GT equality prepares left");
    require(direction(0, 0, 1) == 1, "subpixel player X is retained for aim");
    require(direction(0, 0, -1) == -1, "player left prepares left");
    require(direction(0, 0, 1, false) == 0, "airborne frog retries instead of preparing");
    require(direction(-80 * 256L, 0, 0) == 0 && direction(80 * 256L, 0, 0) == 0 &&
            direction(0, -142 * 256L, 0) == 0 && direction(0, 142 * 256L, 0) == 0,
            "all four view boundaries are strict");
    require(direction(-80 * 256L + 1, 0, 0) == 1 &&
            direction(0, 142 * 256L - 1, 0) == -1,
            "inside boundary is eligible, with no added margin");
    require(frogPreparationDirection(true, 0, 0, 1,
                -80 * 256L, 1, 80 * 256L, 284 * 256L + 1) == 0,
            "fractional camera top must not round the frog into view");
    require(frogPreparationDirection(true, 240 * 256L, 176 * 256L, 240 * 256L + 1,
                160 * 256L, 34 * 256L, 320 * 256L, 318 * 256L) == direction(0, 0, 1),
            "SDL world and native relative coordinate adapters agree");
    std::cout << "frog behavior tests passed\n";
}

#include "core/FrogBehavior.h"
#include "core/FractionalMotion.h"
#include "core/FrogHorizontalMotion.h"
#include "core/FrogVerticalMotion.h"
#include "core/FrogHitState.h"
#include "core/FrogMotion.h"
#include "core/GameMakerRounding.h"
#include "core/FrogPresentation.h"
#include "core/EnemyStepPresentation.h"
#include "core/EnemyCreateProfile.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}

int main() {
    // Source round(xx/yy), not round(the lossy public coordinate mirror).
    for(long unit:{65536L,1280L})for(long whole=-512;whole<=512;++whole)
        for(long delta=-4;delta<=4;++delta){
            const long value=whole*unit+unit/2+delta;
            require(downwell::frogPublishedPixel(value,unit)==
                downwell::roundGameMakerCoordinate(double(value)/unit),
                "frog publication retains sub-Q8 fractions around signed half-pixels");
        }
    require(240+downwell::frogPublishedPixel(-64*65536L+32768+9,65536)==177,
        "seed 8 route 50 publishes 176.500137 as 177 rather than 176");
    for(long speed:{-65536L,-1L,0L,1L,65536L})for(bool visible:{false,true})for(bool accept:{false,true}){
        downwell::FrogHorizontalMotion x{0,speed};
        downwell::FrogVerticalMotion y;
        downwell::FrogHitState hit;
        unsigned scales=0,landings=0;
        const auto effect=downwell::stepFrogMotion(x,y,hit,visible,[](int,int){return false;},
            []{return true;},[&](int scale){
                ++scales;require(scale==(speed<0?-1:1)&&x.position==0&&y.position==0&&landings==0,
                        "frog scale precedes landing and both integrations, retaining Q16 sign");
                return accept;
            },[&](bool grounded){require(!grounded,"clear-space frog remains airborne");++landings;});
        const bool ok=!speed||accept;
        require(effect.succeeded==ok&&!effect.hit&&scales==unsigned(speed!=0)&&landings==unsigned(ok)&&
                x.position==(ok&&visible?speed:0)&&y.position==(ok&&visible?192:0),
                "offscreen and zero-speed scale guards do not bypass source callback order");
    }
    {
        downwell::FrogHorizontalMotion x{0,65536};downwell::FrogVerticalMotion y;
        downwell::FrogHitState hit;
        const auto effect=downwell::stepFrogMotion(x,y,hit,true,[](int px,int){return px>=6;},
            []{return true;},[&](int scale){return scale==-1&&x.position==0&&x.velocity==-19661;});
        require(effect.succeeded&&x.position==-19661&&y.position==192,
                "collision rebound is published before integrating the reversed velocity");
    }
    {
        downwell::FrogHorizontalMotion x{0,2*65536};downwell::FrogVerticalMotion y{0,1280,false};
        downwell::FrogHitState hit;hit.ready=true;unsigned queries=0,scales=0,landings=0;
        const auto effect=downwell::stepFrogMotion(x,y,hit,true,[&](int,int){++queries;return false;},
            []{return false;},[&](int){++scales;return true;},[&](bool){++landings;});
        require(effect.hit&&!effect.succeeded&&queries==0&&scales==0&&landings==0&&
                x.position==0&&y.position==0&&x.velocity==104858&&y.velocity==0,
                "rejected hit publication stops terrain queries and later scale, landing and motion work");
    }
    {
        // Integer rational oracle: no repeated floating-point additions. The
        // initial remainder and changing rates survive without quantization.
        for (unsigned initial = 0; initial < 5; ++initial) {
            unsigned phase = 319, oldPhase = phase;
            unsigned char remainder = initial, oldRemainder = initial;
            unsigned long long numerator = phase * 5u + initial;
            for (unsigned step = 0; step < 10000; ++step) {
                const unsigned rate = (step * 7u) % 21u;
                downwell::advanceEnemyImageQ8(phase, remainder, rate);
                // Previous production implementation, kept as an equivalence check.
                const unsigned oldNumerator = rate * 64u + oldRemainder;
                oldPhase += oldNumerator / 5u;
                oldRemainder = oldNumerator % 5u;
                numerator += rate * 64u;
                require(phase == oldPhase && remainder == oldRemainder &&
                        phase == numerator / 5u && remainder == numerator % 5u,
                        "twentieth-frame callers retain exact previous animation state");
            }
            phase = 319; remainder = initial; numerator = phase * 5u + initial;
            for (unsigned step = 0; step < 10000; ++step) {
                // Skull (7), stopped (0), spider (10), skeleton throw (12),
                // then skull again: no remainder loss at a rate transition.
                const unsigned rates[] = {7, 0, 10, 12, 7};
                const unsigned rate = rates[(step / 13) % 5];
                downwell::advanceEnemyImageFortiethsQ8(phase, remainder, rate);
                numerator += rate * 32u;
                require(phase == numerator / 5u && remainder == numerator % 5u,
                        "fortieth-frame rates preserve exact fractional phase across changes");
            }
        }
        unsigned phase = 0; unsigned char remainder = 0;
        for (unsigned step = 0; step < 40; ++step)
            downwell::advanceEnemyImageFortiethsQ8(phase, remainder, 7);
        require(phase == 7u * 256u && remainder == 0,
                "skull 0.175 rate advances exactly seven frames in forty updates");
    }
    {
        struct Instance { int health = -1, reward = -1; float imageSpeed = -1; int untouched = 37; };
        for (int object : {111, 112, 113, 114, 116, 117, 119}) {
            const auto profile = downwell::enemyCreateProfile(object);
            Instance instance;
            downwell::applyEnemyCreateStats(instance, object);
            require(instance.health == profile.health && instance.reward == profile.reward &&
                    std::lround(instance.imageSpeed * 20) == profile.animationRate && instance.untouched == 37,
                    "SDL creation adapter preserves shared health/reward/rate without resetting other state");
        }
        for (int object : {-1, 0, 110, 115, 118, 120}) { // 115, 118: steering, not this profile
            const auto profile = downwell::enemyCreateProfile(object);
            require(profile.health == 0 && profile.reward == 0 && profile.animationRate == 0,
                    "unsupported objects are not misclassified as supported enemies");
        }
    }
    {
        struct Case {
            downwell::EnemyStepPose pose;
            bool flying, batStunned;
            int hang, sprite, forcedFrame;
        };
        const Case cases[] = {
            {{111, 0, true, false, false, false}, true, false, 0, 163, -1},
            {{111, 0, false, false, false, false}, true, false, 0, 164, -1},
            {{112, -1, true, false, false, false}, true, false, 0, 186, -1},
            {{112, 1, true, false, false, false}, true, false, 0, 188, -1},
            {{112, 1, false, false, false, false}, true, false, 0, 187, -1},
            {{113, 0, true, false, false, false}, true, false, 0, 166, -1},
            {{113, 1, true, false, false, false}, true, false, 0, 170, -1},
            {{113, 0, false, true, false, false}, true, false, 0, 168, 0},
            {{113, 0, false, false, false, false}, true, false, 0, 168, 1},
            {{113, 1, true, false, true, false}, true, false, 0, 173, -1},
            {{113, 0, false, true, true, false}, true, false, 0, 173, -1},
            {{116, 0, false, false, false, false}, true, false, 0, 153, -1},
            {{116, 0, false, false, false, false}, true, true, 0, 154, -1},
            {{116, 0, false, false, false, false}, false, true, -9, 162, -1},
            {{116, 0, false, false, false, false}, false, true, -10, 157, -1},
            {{117, 0, false, false, false, false}, true, false, 0, 177, -1},
            {{117, 0, false, false, false, true}, true, false, 0, 178, -1},
            {{119, 0, false, false, false, false}, true, false, 0, 278, -1}
        };
        for (const auto& test : cases) for (unsigned initial : {0u, 255u, 256u, 1023u, 4097u}) {
            downwell::FrogPresentation frog;
            frog.desiredRate = 3;
            downwell::BatActivity bat;
            bat.flying = test.flying; bat.hang = test.hang;
            downwell::BatHitState hit;
            hit.stunned = test.batStunned;
            unsigned phase = initial;
            unsigned char remainder = 4;
            const int sprite = downwell::finishEnemyPresentation(test.pose, frog, bat, hit, phase, remainder);
            require(sprite == test.sprite, "shared end-of-Step selection keeps specialized enemy poses");
            require(phase == (test.forcedFrame >= 0 ? static_cast<unsigned>(test.forcedFrame)*256u
                : initial % (downwell::enemySpriteFrameCount(sprite)*256u)),
                "selected sprite owns frame wrapping; airborne frogs force the source frame");
            require(remainder == (test.forcedFrame >= 0 ? 0 : 4),
                "only forced frames discard the fractional animation remainder");
            const unsigned rate = test.pose.object != 113 || test.pose.frogStunned ? 3
                : !test.pose.grounded ? 0 : test.pose.auxiliary ? 3 : 4;
            require(frog.desiredRate == rate, "only frog presentation changes next Step's frog rate");
        }
    }
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
                [](int, int) { return false; }).hit &&
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
    require(horizontal.position == 39322 && horizontal.velocity == 39322,
            "source -0.3 rebound integrates reversed speed in the same Step");
    for (unsigned i = 0; i < 100; ++i) horizontal.step([](int) { return false; });
    require(std::fabs(horizontal.position / 65536.0 - 60.6) < 0.001,
            "initial rebound plus one hundred steps retain fractional movement rather than rounded Q8 speed");
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

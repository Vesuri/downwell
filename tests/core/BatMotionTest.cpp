#include "core/BatMotion.h"
#include "core/BatBehavior.h"
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
}

int main() {
    using namespace downwell;
    BatMotion rightWall{0, 0, 2 * 256, 128, 0, 50};
    resolveBatMotion(rightWall, [](int x, int) { return x >= 8; });
    require(rightWall.x == 256 && rightWall.y == 128 &&
                rightWall.speedX == 0 && rightWall.speedY == 128,
            "right wing contacts before the origin; only blocked X velocity is cleared");
    BatMotion leftWall{0, 0, -3 * 256, 0, 180, 50, -1};
    resolveBatMotion(leftWall, [](int x, int) { return x <= -9; });
    require(leftWall.x == -256 && leftWall.speedX == 0 && leftWall.facing == -1,
            "mirrored asymmetric mask stops at the left wall and retains facing");
    BatMotion floorContact{0, 0, 0, 5 * 256, 270, 50};
    resolveBatMotion(floorContact, [](int, int y) { return y >= 8; });
    require(floorContact.y == 4 * 256 && floorContact.speedY == 0,
            "bat resolves to its last clear mask row before zeroing downward velocity");
    BatMotion corner{0, 0, 2 * 256, 2 * 256, 315, 50};
    resolveBatMotion(corner, [](int x, int y) { return x >= 8 && y >= 5; });
    require(corner.x == 2 * 256 && corner.y == 256 &&
                corner.speedX == 0 && corner.speedY == 0,
            "source diagonal fallback walks X then Y and cancels both velocity components");
    struct RandomProbe {
        unsigned calls = 0;
        std::uint32_t nextU32() { ++calls; return 0x80000000u; }
    } random;
    {
        RandomProbe draws;
        BatMotion bat{0, 0, 0, 0, 45, 50};
        BatHitState hit;
        BatActivity activity;
        const auto ceiling = [](int, int y) { return y <= -8; };
        auto effect = stepBatBehavior(bat, hit, activity, 0, 0, 153, 0, -20 * 256, draws, ceiling);
        require(!activity.flying && activity.hang == -10 && effect.sprite == 157 &&
                    bat.direction == 270 && bat.speedFortieths == 80 && draws.calls == 0,
                "ceiling attachment selects hanging art without consuming flight RNG");
        effect = stepBatBehavior(bat, hit, activity, 0, 0, 157, 0, -4 * 256, draws, ceiling);
        require(!activity.flying, "player at exact y-minus-four does not wake a hanging bat");
        effect = stepBatBehavior(bat, hit, activity, 0, 0, 157, 0, -3 * 256, draws, ceiling);
        require(activity.flying && effect.sprite == 153 && effect.wakeSounds == 1 &&
                    bat.x == 0 && bat.y == 0 && draws.calls == 0,
                "player wake changes art immediately but movement starts on the next Step");
        stepBatBehavior(bat, hit, activity, 0, 0, 153, 0, 100 * 256, draws, ceiling);
        require(draws.calls == 1 && bat.y > 0, "awakened bat begins shared pursuit");
        BatMotion hangingHit{0, 0, 0, 0, 270, 80};
        BatActivity attached{true, false, -10};
        BatHitState pendingHit;
        pendingHit.queue(270);
        pendingHit.advanceFrame();
        const auto drawsBeforeHit = draws.calls;
        const auto hitEffect = stepBatBehavior(hangingHit, pendingHit, attached, 0, 0, 157,
            0, -20 * 256, draws, ceiling);
        require(attached.flying && hitEffect.hit && hitEffect.sprite == 154 &&
                    hangingHit.x == 0 && hangingHit.y == 0 && draws.calls == drawsBeforeHit,
                "a surviving hit wakes an attached bat into stationary stun without pursuit RNG");
        for (int side : {-1, 1}) {
            BatMotion wallBat{0, 0, 0, 0, 0, 50};
            BatActivity wallState;
            BatHitState wallHit;
            const auto wall = [side](int x, int) { return side < 0 ? x <= -8 : x >= 8; };
            auto wallEffect = stepBatBehavior(wallBat, wallHit, wallState, 0, 0, 153,
                0, -20 * 256, draws, wall);
            require(!wallState.flying && wallState.hang == side && wallEffect.sprite == 162 &&
                        wallBat.facing == -side, "left/right support sets wall art and source facing");
            wallEffect = stepBatBehavior(wallBat, wallHit, wallState, 0, 0, 162,
                0, -20 * 256, draws, wall);
            require(!wallState.flying, "negative hang flag queries the left support, not the right");
            wallEffect = stepBatBehavior(wallBat, wallHit, wallState, 0, 0, 162,
                0, -20 * 256, draws, [](int, int) { return false; });
            require(wallState.flying && wallEffect.wakeSounds == 1,
                    "destroying either supporting wall wakes its bat");
        }
    }
    BatMotion injured{256, 512, 20, 30, 90, 50};
    BatHitState hit;
    hit.queue(270);
    hit.advanceFrame();
    require(stepActiveBat(injured, hit, 1000, 2000, random, [](int, int) { return false; }) &&
            hit.stunned && injured.direction == 270 && injured.speedFortieths == 120 &&
            injured.x == 256 && injured.y == 512 && random.calls == 0,
            "bat hit resets direction/speed but neither moves nor consumes pursuit RNG");
    hit.advanceFrame();
    require(!stepActiveBat(injured, hit, 1000, 2000, random, [](int, int) { return false; }) &&
            injured.x == 256 && injured.y == 512 && random.calls == 0,
            "stunned bat remains stationary without re-consuming the hit");
    hit.alarm();
    stepActiveBat(injured, hit, 1000, 2000, random, [](int, int) { return false; });
    require(!hit.stunned && random.calls == 1 && (injured.x != 256 || injured.y != 512),
            "Alarm 0 enables pursuit and its one RNG draw again");
    constexpr double pi = 3.14159265358979323846;
    double maximumAngleError = 0;
    for (int radius : {1, 3, 127, 4096, 262144}) {
        for (int angle = 0; angle < 360; ++angle) {
            const long x = std::lround(std::cos(angle * pi / 180) * radius);
            const long y = std::lround(-std::sin(angle * pi / 180) * radius);
            if (x == 0 && y == 0) continue;
            double expected = std::atan2(-static_cast<double>(y), static_cast<double>(x)) * 180 / pi;
            if (expected < 0) expected += 360;
            double error = std::abs(batPointDirection(x, y) / 65536.0 - expected);
            if (error > 180) error = 360 - error;
            if (error > maximumAngleError) maximumAngleError = error;
            require(error < 0.004, "integer point_direction stays within the sixteen-iteration angle bound");
        }
    }
    for (int angle = -360; angle < 720; ++angle)
        require(std::abs(batSine(angle) / 32768.0 - std::sin(angle * pi / 180)) < 0.000016,
                "one-degree lengthdir table rounds within half a Q15 unit");
    auto clear = [](int, int) { return false; };
    BatMotion aligned{0, 0, 0, 0, 0, 50};
    stepBatMotion(aligned, 25600, 0, 0x80000000u, clear);
    require(aligned.direction == 0 && aligned.speedFortieths == 50 &&
            aligned.speedX == 320 && aligned.speedY == 0,
            "zero angular difference does not rotate and source maximum speed is preserved");
    BatMotion overshoot{0, 0, 0, 0, 0, 49};
    stepBatMotion(overshoot, 25600, 0, 0x80000000u, clear);
    require(overshoot.speedFortieths == 43,
            "source acceleration 49+2 is followed by overspeed subtraction 8, not a clamp");
    BatMotion turning{0, 0, 0, 0, 0, 50};
    stepBatMotion(turning, 0, -25600, 0x80000000u, clear);
    require(turning.direction == 10 && turning.speedFortieths == 49,
            "outside the cone the source turns ten degrees and decelerates by .025");
    BatMotion stopped{0, 0, 0, 0, 0, 40};
    stepBatMotion(stopped, -25600, 0, 0x80000000u, [](int, int) { return true; });
    require(stopped.x == 0 && stopped.y == 0 && stopped.speedFortieths == 40,
            "solid adapter rejects displacement and speed does not decelerate below one");
    for (int speed = 40; speed <= 120; ++speed) {
        const int q8 = (speed * 256 + 20) / 40;
        require((q8 * 40 + 128) / 256 == speed,
                "native speed adapter preserves exact fortieths between updates");
    }
    BatMotion relative{-20 * 256, -30 * 256, 0, 0, 93, 50};
    BatMotion world = relative;
    world.x += 240 * 256; world.y += 176 * 256;
    std::uint32_t sample = 7;
    for (unsigned tick = 0; tick < 1000; ++tick) {
        sample = sample * 1664525u + 1013904223u;
        stepBatMotion(relative, (long)(tick % 120) * 256, (long)tick * 16, sample, clear);
        stepBatMotion(world, (long)(tick % 120 + 240) * 256, (long)tick * 16 + 176 * 256, sample, clear);
        require(world.x == relative.x + 240 * 256 && world.y == relative.y + 176 * 256 &&
                world.direction == relative.direction && world.speedFortieths == relative.speedFortieths,
                "shared bat simulation is identical in world and player-relative coordinate systems");
    }
    std::cout << "bat motion tests passed; maximum angle error " << maximumAngleError << " degrees\n";
}

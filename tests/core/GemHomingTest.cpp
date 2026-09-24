#include "core/GemHoming.h"
#include "core/GemMotion.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
void require(bool value, const char* text) {
    if (!value) { std::cerr << text << '\n'; std::exit(1); }
}
int main() {
    downwell::GemMotion contact;
    contact.lifetime.age = 10;
    require(contact.collectible(7 * 256, 0) && !contact.collectible(8 * 256, 0) &&
            contact.collectible(-7 * 256, 0) && !contact.collectible(-8 * 256, 0),
            "small gem and player rectangular masks overlap but edge-touch does not collect");
    require(contact.collectible(0, -11 * 256) && !contact.collectible(0, -12 * 256) &&
            contact.collectible(0, 7 * 256) && !contact.collectible(0, 8 * 256),
            "collection preserves the source player's asymmetric vertical mask");
    require(contact.collectible(9 * 256, 0, true) && !contact.collectible(10 * 256, 0, true) &&
            contact.collectible(0, -13 * 256, true) && !contact.collectible(0, -14 * 256, true) &&
            contact.collectible(0, 9 * 256, true) && !contact.collectible(0, 10 * 256, true),
            "large gem uses its own twelve-pixel source mask");
    contact.lifetime.age = 9;
    require(!contact.collectible(0, 0, true), "collision cannot collect before obtainable alarm");
    contact.lifetime.age = 10;
    contact.x = 6401;
    require(!contact.collectible(-7 * 256, 0), "rounding above a half does not lose precision via Q8");
    require(downwell::GemMotion::rounded(6400) == 0 && downwell::GemMotion::rounded(19200) == 2 &&
            downwell::GemMotion::rounded(-6400) == 0 && downwell::GemMotion::rounded(-19200) == -2 &&
            downwell::GemMotion::rounded(-6401) == -1, "authoritative gem rounding uses signed ties-to-even");
    for (unsigned choice = 0; choice < 4; ++choice) {
        struct Choice {
            unsigned value, calls = 0;
            unsigned irandomSmall(unsigned upper) {
                require(upper == 3, "animation uses the recovered four-way choice");
                ++calls; return value;
            }
        } choiceRandom{choice};
        downwell::GemAnimation animation;
        animation.initialize(choiceRandom);
        const int rates[] = {-5, -3, 3, 5};
        require(choiceRandom.calls == 1 && animation.rateTenths == rates[choice] && animation.frame() == 0,
                "each source animation choice consumes one draw and starts frame zero");
        for (int tick = 1; tick <= 400; ++tick) {
            animation.advance();
            const int phase = ((tick * rates[choice]) % 40 + 40) % 40;
            require(animation.phaseTenths == static_cast<unsigned>(phase) &&
                    animation.frame() == static_cast<unsigned>(phase / 10),
                    "signed animation wraps correctly with no accumulated rounding drift");
        }
    }
    downwell::GemLifetime loose, attracted;
    for (unsigned age = 1; age <= 400; ++age) {
        require(attracted.advance(true), "attracted gem survives its source expiry alarm");
        require(attracted.obtainable() == (age >= 10) &&
                attracted.suckable() == (age >= 15) &&
                attracted.disappearing() == (age >= 190),
                "default currency alarm boundaries match Create and Alarm 0/1/3");
        if (age <= 310)
            require(loose.advance(false) == (age < 310), "loose gem expires exactly at Alarm 2");
    }
    require(attracted.advance(false), "expired Alarm 2 is not repeated if attraction later clears");
    attracted.age = 65535;
    require(attracted.advance(true) && attracted.age == 65536 && attracted.obtainable(),
            "long-lived attracted gems do not wrap the former SDL 16-bit age counter");
    struct Random {
        unsigned call = 0;
        unsigned irandomSmall(unsigned) {
            const unsigned values[] = {0, 10, 60, 1};
            return values[call++ % 4];
        }
        unsigned randomIntegerSmall(unsigned) { return 25600; }
    } random;
    downwell::GemMotion motion;
    motion.initialize(10, 20, true, random);
    require(motion.homing.direction == 180L * 65536 && random.call == 4 &&
            motion.vx == 0 && motion.vy == 0 && motion.attracted,
            "VM evaluates second heading candidate first; stomp preserves Create homing defaults");
    motion = {};
    const auto empty = [](int, int) { return false; };
    for (unsigned tick = 1; tick <= 20; ++tick) {
        motion.step(10000 * 256L, 0, empty, random);
        require(motion.vy == 448L * tick && motion.y == 224L * tick * (tick + 1) && motion.vx == 0,
                "free gravity is exact .035 and sign(0) does not invent horizontal motion");
    }
    motion = {};
    motion.lifetime.age = 14;
    motion.step(10 * 256L, 0, empty, random);
    require(motion.attracted && motion.homing.speedEighths == 32 && motion.vy == 448,
            "acquisition update still performs free motion; homing begins next update");
    motion = {};
    motion.lifetime.age = 309;
    require(!motion.step(10 * 256L, 0, empty, random) && !motion.attracted && motion.y == 0,
            "expiry alarm runs before motion and cannot acquire a nearby player on its last tick");
    motion = {};
    motion.attracted = true;
    motion.lifetime.age = 309;
    require(motion.step(10 * 256L, 0, empty, random) && motion.x != 0,
            "already attracted gem survives expiry and still moves that update");
    downwell::GemHoming gem;
    long vx, vy;
    gem.step(0, -25600, vx, vy);
    require(gem.direction == 5L * 65536 && gem.turnFifths == 28 &&
            gem.speedEighths == 9 && vx > 280 && vy < 0,
            "stomp gem begins at speed 1, turns only five degrees, then accelerates");
    gem.acquire(25600, 0);
    require(gem.direction == 0 && gem.speedEighths == 32 && gem.turnFifths == 160,
            "proximity acquisition aims at the player and starts speed four");
    gem.step(25600, 0, vx, vy);
    require(vx == 1056 && vy == 0, "first attracted update accelerates to 4.125");
    for (int i = 0; i < 100; ++i) gem.step(25600, 0, vx, vy);
    require(vx == 2304 && gem.speedEighths == 72 && gem.turnFifths == 160,
            "source speed and turn caps are independent of gem age");
    gem.direction = 359L * 65536;
    gem.turnFifths = 25;
    gem.step(25600, -447, vx, vy);
    require(gem.direction < 2L * 65536, "turn takes short arc across zero degrees");
    double maximumError = 0;
    for (int initial = 0; initial < 360; initial += 13) {
        gem = {};
        gem.direction = initial * 65536L;
        double angle = initial, turn = 5, speed = 1;
        for (int tick = 0; tick < 80; ++tick) {
            const long dx = 25600 - tick * 37, dy = -8700 + tick * 91;
            double target = std::atan2(-double(dy), double(dx)) * 180 / std::acos(-1.0);
            if (target < 0) target += 360;
            double delta = angle - target;
            if (delta > 180) delta -= 360;
            else if (delta < -180) delta += 360;
            if (delta > turn) angle -= turn;
            else if (delta < -turn) angle += turn;
            else angle = target;
            angle = std::fmod(angle + 360, 360);
            turn = std::fmin(32, turn + .6);
            speed = std::fmin(9, speed + .125);
            gem.step(dx, dy, vx, vy);
            const double radians = angle * std::acos(-1.0) / 180;
            const double error = std::fmax(std::fabs(vx - std::cos(radians) * speed * 256),
                                          std::fabs(vy + std::sin(radians) * speed * 256));
            maximumError = std::fmax(maximumError, error);
        }
    }
    require(maximumError < 1.5, "fixed-point homing stays within 1.5 Q8 velocity units of source math");
    std::cout << "gem homing tests passed; max Q8 velocity error " << maximumError << '\n';
}

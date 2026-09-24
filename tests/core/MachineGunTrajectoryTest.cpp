#include "core/MachineGunTrajectory.h"
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
namespace {
void require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
double position(downwell::MachineGunTrajectory::Coordinate value) {
    return value.pixel + value.fraction / 1048576.0;
}
}
int main() {
    using downwell::MachineGunTrajectory;
    struct Sample {
        std::uint32_t value; unsigned draws = 0;
        std::uint32_t nextU32() { ++draws; return value; }
    } sample{0};
    double maximumError = 0;
    for (unsigned i = 0; i <= 65536; ++i) {
        sample.value = i == 65536 ? 0xffffffffu : i << 16;
        sample.draws = 0;
        MachineGunTrajectory motion;
        motion.reset(-3, 100000);
        motion.initialize(sample);
        motion.initialize(sample);
        require(sample.draws == 1, "spread consumes exactly one sample at first initialization");
        const double degrees = 270.0 + (-3.0 + sample.value / 4294967296.0 * 6.0);
        const double radians = degrees * std::acos(-1.0) / 180.0;
        const double dx = 8.0 * std::cos(radians), dy = -8.0 * std::sin(radians);
        const double ex = std::abs(motion.velocityX / 1048576.0 - dx);
        const double ey = std::abs(motion.velocityY / 1048576.0 - dy);
        maximumError = std::fmax(maximumError, std::fmax(ex, ey));
        require(ex < 0.000001 && ey < 0.000001,
                "fixed velocity differs from source-formula double by less than one micro-pixel");
        require(motion.velocityY > 0, "every spread sample travels downward");
        motion.advance();
        require(std::abs(position(motion.x) - (-3 + dx)) < 0.000001 &&
                std::abs(position(motion.y) - (100000 + dy)) < 0.000001,
                "split coordinates preserve fraction without limiting level coordinate range");
    }
    for (std::uint32_t raw : {0u, 0x80000000u, 0xffffffffu}) {
        sample = {raw};
        MachineGunTrajectory motion; motion.reset(-3, -8); motion.initialize(sample);
        const double dx = motion.velocityX / 1048576.0, dy = motion.velocityY / 1048576.0;
        for (unsigned tick = 1; tick <= 1000; ++tick) {
            motion.advance();
            require(position(motion.x) == -3 + tick * dx && position(motion.y) == -8 + tick * dy,
                    "movement accumulates its fixed velocity without losing fractional remainders");
            require(motion.x.fraction < MachineGunTrajectory::unit && motion.y.fraction < MachineGunTrajectory::unit,
                    "negative coordinates retain normalized fractions");
        }
        motion.reset(1, 2);
        require(!motion.initialized && motion.velocityX == 0 && motion.velocityY == 0,
                "reused projectile resets its first-Step initialization");
        motion.initialize(sample);
        require(sample.draws == 2, "reused projectile consumes a new spread sample");
    }
    downwell::GameMakerRandom random(2), reference(2);
    MachineGunTrajectory motion;
    motion.initialize(random); reference.nextU32();
    for (unsigned i = 0; i < 16; ++i)
        require(random.stateWord(i) == reference.stateWord(i), "spread preserves the shared runner RNG stream");
    require(random.stateIndex() == reference.stateIndex() && random.drawCount() == 1,
            "spread advances exactly one runner RNG word");
    std::printf("Machine-gun trajectory tests passed; maximum sampled velocity error %.9g pixels/update\n", maximumError);
}

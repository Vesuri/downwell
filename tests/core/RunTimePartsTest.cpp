#include "core/RunTimeParts.h"
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
static void check(std::uint32_t ticks) {
    // Explicit stores preserve the separate binary64 VM arithmetic operations.
    volatile double divided = static_cast<double>(ticks) / 60.0;
    const double whole = std::floor(divided);
    volatile double fraction = divided - whole;
    volatile double scaled = fraction * 100.0;
    const auto parts = downwell::RunTimeParts::fromTicks(ticks);
    if (parts.minutes != static_cast<std::uint32_t>(whole / 60) ||
        parts.seconds != static_cast<std::uint32_t>(whole) % 60 ||
        parts.centiseconds != static_cast<std::uint32_t>(std::floor(scaled))) {
        std::cerr << "timer binary64 mismatch at " << ticks << '\n';
        std::exit(1);
    }
}
int main() {
    for (std::uint32_t ticks = 0; ticks < 1000000; ++ticks) check(ticks);
    std::uint32_t random = 12345;
    for (unsigned i = 0; i < 100000; ++i) {
        random = random * 1664525u + 1013904223u;
        check(random);
    }
    for (std::uint32_t seconds = 1; seconds <= (1u << 26); seconds <<= 1) {
        const std::uint32_t boundary = seconds * 60;
        for (int offset = -60; offset <= 60; ++offset) check(boundary + offset);
    }
    check(0xffffffffu);
    std::cout << "run time binary64 arithmetic tests passed\n";
}

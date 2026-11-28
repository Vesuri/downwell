#include "core/PlayerBoxContact.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
// PlayerBoxContact::coordinate against the runner's binary64 expression
// float(double(value)/1280 + origin): every int32 value at the gameplay
// origins (240, 176), and a stride at other origins: the integer path up to
// |origin| 16384, the binary64 form beyond it (outside world one).
using downwell::PlayerBoxContact;
using downwell::RunnerImageValue;
static std::uint64_t checked = 0;
static bool same(long value, int origin, bool reference) {
    RunnerImageValue actual;
    if (!PlayerBoxContact::coordinate(value, origin, actual)) {
        std::fprintf(stderr, "coordinate rejected %ld at %d\n", value, origin); return false;
    }
    const float expected = static_cast<float>(double(value) / 1280.0 + double(origin));
    std::uint32_t bits; std::memcpy(&bits, &expected, 4);
    if (actual.bits != bits) {
        std::fprintf(stderr, "coordinate %ld at %d: %08x, expected %08x\n",
            value, origin, unsigned(actual.bits), unsigned(bits));
        return false;
    }
    if (reference) {
        RunnerImageValue old;
        if (!PlayerBoxContact::coordinateBinary64(value, origin, old) || old.bits != bits) {
            std::fprintf(stderr, "coordinateBinary64 %ld at %d differs\n", value, origin); return false;
        }
    }
    ++checked;
    return true;
}
int main() {
    for (const int origin : {240, 176}) {
        std::uint32_t word = 0;
        do {
            const long value = static_cast<std::int32_t>(word);
            if (!same(value, origin, !(word & 0xfffffu))) return 1;
        } while (++word);
    }
    for (const int origin : {0, 1, -1, -240, 16384, -16384, 16385, 999999, -1000000}) {
        for (std::uint32_t word = 0x12345u; ; word += 65521u) {
            if (!same(static_cast<std::int32_t>(word), origin, !(word & 0xff))) return 1;
            if (word > 0xffffffffu - 65521u) break;
        }
        // Around the cancellation point and the int32 ends.
        for (long delta = -5000; delta <= 5000; ++delta) {
            const long centre = -1280L * origin;
            if (centre + delta >= -2147483648L && centre + delta <= 2147483647L &&
                !same(centre + delta, origin, true)) return 1;
            if (!same(2147483647L - (delta + 5000), origin, false) ||
                !same(-2147483647L - 1 + (delta + 5000), origin, false)) return 1;
        }
    }
    std::printf("Player box coordinate: %llu values match binary64\n",
        static_cast<unsigned long long>(checked));
}

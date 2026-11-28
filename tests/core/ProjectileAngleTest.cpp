#include "core/ProjectilePublication.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
// ProjectilePublication::angle against the runner's binary64 degrees
// (turns/2^32 * 360, then binary32) on every 32-bit turn count.
int main() {
    using downwell::ProjectilePublication;
    using downwell::RunnerImageValue;
    std::uint64_t checked = 0;
    std::uint32_t turns = 0;
    do {
        RunnerImageValue actual;
        if (!ProjectilePublication::angle(turns, actual)) {
            std::fprintf(stderr, "angle rejected %08x\n", unsigned(turns)); return 1;
        }
        const float expected = static_cast<float>(double(turns) * 360.0 / 4294967296.0);
        std::uint32_t bits; std::memcpy(&bits, &expected, 4);
        if (actual.bits != bits) {
            std::fprintf(stderr, "angle %08x: %08x, expected %08x\n",
                unsigned(turns), unsigned(actual.bits), unsigned(bits));
            return 1;
        }
        // The binary64 reference on a sample, including every 2^16th word.
        if (!(turns & 0xffffu) || (turns & 0xfffffu) == 0x5a5a5u) {
            RunnerImageValue reference;
            if (!ProjectilePublication::angleBinary64(turns, reference) || reference.bits != bits) {
                std::fprintf(stderr, "angleBinary64 %08x differs\n", unsigned(turns)); return 1;
            }
        }
        ++checked;
    } while (++turns);
    std::printf("Projectile angle: %llu turn counts match binary64 degrees\n",
        static_cast<unsigned long long>(checked));
}

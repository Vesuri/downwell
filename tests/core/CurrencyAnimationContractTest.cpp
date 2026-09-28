#include "core/GemAnimation.h"
#include "core/UnitSequenceFrames.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

// Independent host arithmetic for the selected runner's four-key sequence.
// The existing live helper is compared diagnostically, never used as oracle.
static unsigned bits(float value) {
    unsigned result; std::memcpy(&result, &value, sizeof(result)); return result;
}
int main() {
    const float rates[] = {-.5f, -.3f, .3f, .5f};
    for (unsigned choice = 0; choice < 4; ++choice) {
        struct Random {
            unsigned choice, calls = 0;
            unsigned chooseIndexSmall(unsigned count) {
                assert(count == 4); ++calls; return choice;
            }
        } random{choice};
        downwell::GemAnimation live;
        live.initialize(random);
        downwell::RunnerImageValue timeline{}, image{};
        float reference = 0;
        unsigned firstDifference = 0, differences = 0;
        for (unsigned tick = 1; tick <= 4096; ++tick) {
            reference = float(reference + rates[choice]);
            const bool boundary = reference < 0 || reference >= 4;
            if (boundary)
                reference = std::fmod(float(std::fmod(reference, 4.f) + 4.f), 4.f);
            bool crossed = false;
            assert(downwell::UnitSequenceFrames::advance(
                timeline, {bits(rates[choice])}, 4, image, crossed));
            assert(crossed == boundary && timeline.bits == bits(reference));
            assert(image.bits == (bits(reference) & 0x7fffffffu));
            live.advance();
            if (live.frame() != unsigned(reference)) {
                if (!firstDifference) {
                    firstDifference = tick;
                    std::printf("rate %+.1f: first live frame difference at update %u "
                                "(live %u, runner %u, image bits %08x)\n",
                                double(rates[choice]), tick, live.frame(),
                                unsigned(reference), bits(reference));
                }
                ++differences;
            }
        }
        assert(random.calls == 1);
        std::printf("rate %+.1f: %u differing visible frames in 4096 updates\n",
                    double(rates[choice]), differences);
    }
    std::puts("Currency animation contract: 16384 source-float sequence updates passed; "
              "live comparison is diagnostic, not a gameplay parity claim");
}

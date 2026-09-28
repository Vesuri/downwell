#include "core/GemAnimation.h"
#include "core/UnitSequenceFrames.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>

// Independent host arithmetic for the selected runner's four-key sequence.
// Neither the live helper nor the integer-only evaluator is the scalar oracle.
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
            assert(live.phaseBits() == image.bits && live.frame() == unsigned(reference));
            assert(live.speedBits() == bits(rates[choice]));
            if(choice == 2 && tick == 10)
                assert(live.phaseBits() == 0x403fffffu && live.frame() == 2);
        }
        assert(random.calls == 1);
        live.initialize(random);
        assert(random.calls == 2 && live.phaseBits() == 0 && live.frame() == 0);
    }
    std::puts("Currency animation contract: 16384 source-float sequence updates passed; "
              "including live frames, state, RNG count and reuse; not a full gameplay parity claim");
}

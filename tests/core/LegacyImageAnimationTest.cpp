#include "core/LegacyImageAnimation.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
static void check(bool ok) { if (!ok) std::abort(); }
static downwell::RunnerImageValue stored(float value) {
    downwell::RunnerImageValue out; std::memcpy(&out.bits, &value, 4); return out;
}
int main() {
    unsigned updates = 0;
    for (unsigned frames : {1u,2u,7u,8u,16u})
        for (float speed : {-0.45f,-0.3f,-0.05f,0.0f,0.1f,0.2f,0.3f,20.0f}) {
            float expected = 0;
            auto index = stored(expected);
            for (unsigned i = 0; i < 1000; ++i) {
                // Independent scalar transcription of legacy x87 add/store,
                // upper comparison, single subtract/store, event dispatch.
                expected = static_cast<float>(double(expected) + double(speed));
                bool expectedEnd = expected >= float(frames);
                if (expectedEnd) expected = static_cast<float>(double(expected) - double(frames));
                bool ended = !expectedEnd;
                check(downwell::advanceLegacyImage(index,stored(speed),frames,ended));
                check(index.bits == stored(expected).bits && ended == expectedEnd);
                ++updates;
            }
        }
    auto index = stored(7.0f);
    bool ended = true;
    check(!downwell::advanceLegacyImage(index,stored(1),0,ended) && index.bits == stored(7).bits && ended);
    downwell::RunnerImageValue invalid; invalid.bits = 0x7f800000u;
    check(!downwell::advanceLegacyImage(index,invalid,8,ended) && index.bits == stored(7).bits);
    std::printf("Legacy animation: %u frame/negative-speed/overflow updates match scalar runner path\n", updates);
}

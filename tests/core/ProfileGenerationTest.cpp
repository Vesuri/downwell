#include "core/Profile.h"
#include <cassert>
#include <cstdio>

static unsigned active = profileGeneration, calls = 0;
unsigned downwellProfileEnter(unsigned phase) {
    const unsigned previous = active;
    active = phase; ++calls;
    return previous;
}
void downwellProfilePhase(unsigned phase) { active = phase; ++calls; }

int main() {
    {
        DW_GENERATION_SCOPE(profileBuilderCommand);
        DW_GENERATION_PHASE(profileBuilderInitialize);
        DW_GENERATION_PHASE(profileBuilderCommand);
    }
    assert(active == profileGeneration);
#if defined(DOWNWELL_PROBES) && defined(DOWNWELL_GENERATION_DETAIL)
    assert(calls == 4);
#else
    assert(calls == 0);
#endif
    std::puts("Generation probe opt-in and enclosing phase restoration passed");
}

#include "core/RunSession.h"
#include <cassert>
#include <cstdio>
extern "C" { volatile unsigned long gDownwellProfileDamageBirths=0; }
static unsigned active=profileEnemyContact, calls[profilePhaseCount]{};
unsigned downwellProfileEnter(unsigned phase){unsigned previous=active;active=phase;++calls[phase];return previous;}
void downwellProfilePhase(unsigned phase){active=phase;}
int main(){
    downwell::RunSession run;
    assert(run.contactDamageNumber(128,-128,0,0,240,176));
    assert(run.damageNumbers.atNewest(0)->direction.high==0x40468000u); // 45 degrees
    assert(run.stepDamageNumbers(run.damageNumbers.beginPhase()));
    assert(active==profileEnemyContact);
    for(unsigned i=1;i<8;++i)assert(run.createDamageNumber({}, {}, {}, {}));
    assert(!run.createDamageNumber({}, {}, {}, {})&&!run.effectsSucceeded);
    assert(active==profileEnemyContact);
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
    assert(gDownwellProfileDamageBirths==8); // Failed allocation is not a birth.
#else
    assert(gDownwellProfileDamageBirths==0);
#endif
#ifdef DOWNWELL_PROBES
    assert(calls[profileContactDirection]==1&&calls[profileDamageNumberCreate]==9&&calls[profileDamageNumberStep]==1);
#else
    for(auto count:calls)assert(count==0);
#endif
    std::puts("Damage probes: unchanged state, successful births only, enclosing phase restored");
}

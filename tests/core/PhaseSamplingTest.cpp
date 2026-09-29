#include "core/Profile.h"
#include <cassert>
#include <cstdio>
extern "C" { volatile unsigned short gDownwellSamplePhase=profileOther; }
static void nested(bool early){
    DW_PROFILE_SCOPE(profileInstanceBirth);
    assert(gDownwellSamplePhase==profileInstanceBirth);
    {DW_PROFILE_SCOPE(profileBirthMetadata);assert(gDownwellSamplePhase==profileBirthMetadata);}
    assert(gDownwellSamplePhase==profileInstanceBirth);
    if(early)return;
    {DW_PROFILE_SCOPE(profileInstanceBirth);assert(gDownwellSamplePhase==profileInstanceBirth);}
}
int main(){
    {
        DW_PROFILE_SCOPE(profileGeneration);
        nested(false);assert(gDownwellSamplePhase==profileGeneration);
        nested(true);assert(gDownwellSamplePhase==profileGeneration);
    }
    assert(gDownwellSamplePhase==profileOther);
    DW_PROFILE_PHASE(profileWait);
    nested(true);assert(gDownwellSamplePhase==profileWait);
    DW_PROFILE_PHASE(profileSimulation);
    {
        DW_PROFILE_SCOPE(profileGeneration);
        DW_PROFILE_PHASE(profileGeneratedRegistration);
        nested(false);assert(gDownwellSamplePhase==profileGeneratedRegistration);
    }
    assert(gDownwellSamplePhase==profileSimulation);
    DW_PROFILE_PHASE(profileOther);
    assert(gDownwellSamplePhase==profileOther);
    std::puts("Phase sampling: manual phases, nested publication and early-return restoration passed");
}

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
    std::puts("Phase sampling: nested phase publication and early-return restoration passed");
}

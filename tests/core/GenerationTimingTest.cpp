#include "core/Profile.h"
#include <cassert>
#include <cstdio>

static unsigned entries=0,leaves=0,attempts=0;
static bool active=true,inScope=false;
bool downwellProfileGenerationEnter(unsigned long& start){
    ++attempts;
    if(!active||inScope)return false;
    inScope=true;
    ++entries;
    start=12345;return true;
}
void downwellProfileGenerationLeave(unsigned long start){assert(start==12345&&inScope);inScope=false;++leaves;}

static void generate(bool earlyReturn){
    DW_PROFILE_SCOPE(profileGeneration);
    {DW_PROFILE_SCOPE(profileGeneration);}
    {DW_PROFILE_SCOPE(profileInstanceBirth);{DW_PROFILE_SCOPE(profileInstanceBirth);}}
    {DW_PROFILE_SCOPE(profileBoundsGeometry);}
    if(earlyReturn)return;
    {DW_PROFILE_SCOPE(profileInstanceLayerAttach);{DW_PROFILE_SCOPE(profileInstanceLayerAttach);}}
}
int main(){
    {DW_PROFILE_SCOPE(profileInput);}
    assert(entries==0&&leaves==0);
    generate(false);generate(true);
    active=false;generate(false);
#ifdef DOWNWELL_GENERATION_TIMING
    constexpr bool late=DOWNWELL_GENERATION_TIMING_PHASE==profileInstanceLayerAttach;
    assert(entries==(late?1u:2u)&&leaves==entries&&attempts==(late?4u:6u)&&!inScope);
#else
    assert(entries==0&&leaves==0&&attempts==0&&!inScope);
#endif
    std::puts("Coarse timing: selected stage only, nesting, early return and inactive frame passed");
}

#include "core/Profile.h"
#include <cassert>
#include <cstdio>

static unsigned entries=0,leaves=0;
static bool active=true;
bool downwellProfileGenerationEnter(unsigned long& start){
    ++entries;
    if(!active)return false;
    start=12345;return true;
}
void downwellProfileGenerationLeave(unsigned long start){assert(start==12345);++leaves;}

static void generate(bool earlyReturn){
    DW_PROFILE_SCOPE(profileGeneration);
    {DW_PROFILE_SCOPE(profileInstanceBirth);}
    {DW_PROFILE_SCOPE(profileBoundsGeometry);}
    if(earlyReturn)return;
    {DW_PROFILE_SCOPE(profileInstanceLayerAttach);}
}
int main(){
    {DW_PROFILE_SCOPE(profileInput);}
    assert(entries==0&&leaves==0);
    generate(false);generate(true);
    active=false;generate(false);
#ifdef DOWNWELL_GENERATION_TIMING
    assert(entries==3&&leaves==2);
#else
    assert(entries==0&&leaves==0);
#endif
    std::puts("Generation timing: outer scope only, early return and inactive frame passed");
}

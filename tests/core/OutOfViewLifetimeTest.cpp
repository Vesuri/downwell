#include "core/OutOfViewLifetime.h"
#include "core/CorpseLifecycle.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
static MotionBinary64 real(double value){std::uint64_t bits;std::memcpy(&bits,&value,8);return {std::uint32_t(bits>>32),std::uint32_t(bits)};}
struct Retire {unsigned count=0;void destroy(){++count;}};
int main(){
    unsigned cases=0;
    for(double y:{-160.,0.,.25,200.,300000.})for(bool main:{false,true})
    for(bool upper:{false,true})for(double offset:{-1.,-.00002,-.000001,0.,.000001,.00002,1.}){
        const double camera=y+(upper?160.:-3000.)+offset;
        const bool expected=main&&(y-(camera-160)<-1e-5||y-(camera+3000)>1e-5);
        bool retire=!expected;
        assert(outOfViewRetirement(real(y),real(camera),main,retire)&&retire==expected);
        CorpseLifecycle corpse;assert(corpse.y.assign(real(y)));corpse.yy=real(123456);
        corpse.active=false;corpse.health=0;corpse.disappearing=true;
        corpse.alarms.set(0,1);Retire events;
        assert(corpse.endStep(main,real(camera),events));
        assert(corpse.destroyRequested==expected&&events.count==unsigned(expected));
        assert(corpse.alarms.get(0)==1&&!corpse.myExplosion&&corpse.flash==-1);
        assert(corpse.endStep(main,real(camera),events)&&events.count==unsigned(expected));++cases;
    }
    bool retire=true;
    assert(!outOfViewRetirement({}, {0x7ff00000u,0},true,retire)&&retire);
    CorpseLifecycle corpse;Retire events;
    assert(!corpse.endStep(true,{0x7ff00000u,0},events)&&!corpse.valid&&!events.count);
    assert(!corpse.endStep(true,{},events));
    std::printf("Out-of-view lifetime: %u boundary/room cases, published coordinates and failure retirement passed\n",cases);
}

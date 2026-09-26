#include "core/TimeChangeEffect.h"
// UTMT 0.9.2.0 Assembly exports; data.win SHA-256
// c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8.
// gml_Object_fxTimeChange_Create_0.asm:
// 570678f181b0142af4d420bf1067ceafb430ae3bb4bfe89b1c548b4c5e8c0ac6
// gml_Object_fxTimeChange_Alarm_1.asm:
// 8f3f69db55a52caffc5aa99846b87744e4aa6b301fabe909da41970c75846c0e
// gml_GlobalScript_sleep.asm:
// dd01a506b9c826c319f293a8fa87407fa01f3cfbacbe9d4bf558858dce36bc38
#include <cassert>
#include <cstdio>
using downwell::TimeChangeEffect;
int main(){
    TimeChangeEffect effect;int speed=60;unsigned pauses=0;
    assert(effect.create(speed,[&](unsigned milliseconds){
        assert(milliseconds==30 && speed==30 && effect.alarm0==2 && effect.alarm1==2);
        ++pauses;return true;
    }));
    for(unsigned tick=1;tick<=14;++tick){
        effect.advanceAlarms(speed);
        assert(speed==int(tick<12?30+5*(tick/2):60));
        assert(effect.destroyRequested==(tick==14) && !effect.flash);
    }
    assert(pauses==1 && effect.alarm0==-1);
    speed=25;effect.advanceAlarms(speed);assert(speed==25); // Retired actor cannot write global speed.
    assert(effect.create(speed,[](unsigned){return true;}));
    speed=58;effect.advanceAlarms(speed);effect.advanceAlarms(speed);
    assert(speed==63 && !effect.destroyRequested); // Source does not clamp the addition.
    effect.advanceAlarms(speed);effect.advanceAlarms(speed);
    assert(speed==60 && effect.destroyRequested);
    // Distinct owners act on the shared room speed, not private ramp counters.
    TimeChangeEffect a,b;assert(a.create(speed,[](unsigned){return true;}));
    assert(b.create(speed,[](unsigned){return true;}));
    for(unsigned tick=0;tick<8;++tick){b.advanceAlarms(speed);a.advanceAlarms(speed);}
    assert(speed==60 && a.destroyRequested && b.destroyRequested);
    TimeChangeEffect failed;assert(!failed.create(speed,[](unsigned){return false;}));
    assert(speed==30 && failed.alarm1==2); // Failure is reported after source-visible assignments.
    std::puts("Time-change Create/pause order, alarm pacing, shared speed and retirement passed");
}

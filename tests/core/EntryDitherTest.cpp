#include "core/EntryDither.h"
using namespace downwell;
// Source hashes and resource identity are pinned by verify-runner-instance-ids.
// This probe also compiles without host floating point for the 68000 target.
bool nativeEntryDitherProbe(){
    EntryDither effect;unsigned reads=0;
    auto camera=[&](MotionBinary64& value){
        ++reads;value=RunnerImageValue{0x43704000u}.promoted();return true; // 240.25
    };
    if(!effect.create(camera,[&](MotionBinary64& value){
        if(reads!=1||effect.x.bits!=0x43704000u||effect.alarm0!=-1)return false;
        value=RunnerImageValue{0xc1a60000u}.promoted();return true; // view -20.75
    }))return false;
    if(effect.y.bits!=0xc1a60000u||effect.alarm0!=120)return false;
    for(unsigned tick=1;tick<=120;++tick){
        if(!effect.advanceAlarm()||effect.destroyRequested!=(tick==120))return false;
        if(tick==120)break; // Alarm phase excludes destroyed instance from Step.
        if(!effect.step(camera))return false;
    }
    auto expected=RunnerImageValue{0xc1a60000u}.promoted();
    if(!expected.add(MotionBinary64::fromInteger(119*16)))return false;
    RunnerImageValue result;if(!result.assign(expected)||effect.y.bits!=result.bits)return false;
    const auto count=reads;
    if(effect.step(camera)||effect.advanceAlarm()||reads!=count)return false;
    return effect.succeeded()&&effect.alarm0==0;
}
#ifndef DOWNWELL_NATIVE_QUERY_PROBE
#include <cassert>
#include <cstdio>
#include <cstring>
static unsigned bits(float value){unsigned b;std::memcpy(&b,&value,sizeof b);return b;}
int main(){
    assert(nativeEntryDitherProbe());
    // Independent native-float oracle: both signs, fractional positions and
    // values large enough for binary32 narrowing to lose an increment.
    for(float initial:{-4096.25F,-20.75F,0.0F,176.5F,16000.5F,30000.0F}){ // y + 16 runs in Q16 (W3.20)
        EntryDither e;float y=initial;
        assert(e.create([](auto& out){out={};return true;},[&](auto& out){
            out=RunnerImageValue{bits(initial)};return true;
        }));
        for(unsigned i=0;i<120;++i){
            y=static_cast<float>(static_cast<double>(y)+16.0);
            assert(e.step([&](auto& out){
                assert(e.y.bits==bits(y)); // y setter precedes camera lookup.
                return out.assignInteger(int(i)-60);
            }));
            assert(e.y.bits==bits(y));
            assert(e.x.bits==bits(float(int(i)-60)));
        }
    }
    // A failed second read retains the first assignment, doesn't arm the
    // alarm, and latches failure so a caller cannot replay the prefix.
    EntryDither failed;unsigned reads=0;
    auto camera=[&](auto& out){++reads;return out.assignInteger(241);};
    assert(!failed.create(camera,[](auto&){return false;}));
    assert(failed.x.bits==bits(241.0F)&&failed.alarm0==-1&&!failed.succeeded());
    assert(!failed.create(camera,[](auto& out){out={};return true;})&&reads==1);
    EntryDither stepFailure;
    assert(stepFailure.create(camera,[](auto& out){out={};return true;}));
    assert(!stepFailure.step([](auto&){return false;}));
    assert(stepFailure.y.bits==bits(16.0F)&&!stepFailure.succeeded());
    EntryDither invalid;
    assert(!invalid.create([](auto& out){out={0x7f800000u};return true;},
                          [](auto& out){out={};return true;}));
    // Explicit alarm dispatch must not decrement a second time.
    EntryDither dispatched;dispatched.alarm0=0;
    assert(dispatched.fireAlarm(0)&&dispatched.destroyRequested&&dispatched.alarm0==0);
    std::puts("Entry dither: ordered view reads, binary32 motion, alarm retirement and failure prefixes passed");
}
#endif

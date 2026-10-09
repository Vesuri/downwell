#ifndef DOWNWELL_CORE_RUNNER_AUDIO_GAIN_H
#define DOWNWELL_CORE_RUNNER_AUDIO_GAIN_H
#include "RunnerImageValue.h"
#include <cstring>

namespace downwell {
// Original resource/voice gain controls (runner 4a22d2 / 4a23f9).
// The owner advances these once per main-loop audio update, not per PCM sample.
// Explicit float stores preserve the SSE subss/mulss/addss rounding boundaries.
struct RunnerAudioGain {
    RunnerImageValue current{}, start{}, target{};
    std::int64_t startMicros=0, endMicros=0;
    static float value(RunnerImageValue v){
        float f;static_assert(sizeof(f)==sizeof(v.bits));
        std::memcpy(&f,&v.bits,sizeof(f));return f;
    }
    bool set(MotionBinary64 gain,int milliseconds,std::int64_t now){
        if(!gain.valid()||milliseconds<0||milliseconds>2147483||now<0||
           now>0x7fffffffffffffffLL-2147483000)return false;
        if((gain.high&0x80000000u)&&!gain.zero())gain={};
        RunnerImageValue narrowed;if(!narrowed.assign(gain))return false;
        if(!milliseconds){current=narrowed;endMicros=0;return true;}
        start=current;target=narrowed;startMicros=now;
        endMicros=now+std::int64_t(milliseconds)*1000;return true;
    }
    void update(std::int64_t now){
        if(endMicros<=0)return;
        if(now>endMicros){current=target;endMicros=0;return;}
        const volatile float fraction=float(double(now-startMicros)/double(endMicros-startMicros));
        const volatile float difference=value(target)-value(start);
        const volatile float product=fraction*difference;
        const float result=value(start)+product;
        std::memcpy(&current.bits,&result,sizeof(result));
    }
};
}
#endif

#include "core/RunnerAudioGain.h"
// Compile the shared control without SDL or a hardware floating-point unit.
bool compileAudioGain(downwell::RunnerAudioGain& gain,downwell::AudioLevel value,
                     int milliseconds,std::int64_t begin,std::int64_t update){
    if(!gain.set(value,milliseconds,begin))return false;
    gain.update(update);return true;
}

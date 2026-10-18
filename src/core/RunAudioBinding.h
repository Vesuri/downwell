#ifndef DOWNWELL_CORE_RUN_AUDIO_BINDING_H
#define DOWNWELL_CORE_RUN_AUDIO_BINDING_H
namespace downwell {
// ResetPlayer/scrInitialize stop all device voices before resetting the run's
// sound guards. No device lifetime or sound selection belongs in this binding.
// An unbound legacy/constructor world has not admitted device audio yet.
struct RunAudioBinding {
    bool (*stopAll)(void*)=nullptr;
    void* context=nullptr;
    bool reset()const{return !stopAll||stopAll(context);}
    template<class Audio> static RunAudioBinding bind(Audio& audio){
        return {[](void* p){return static_cast<Audio*>(p)->stopAll();},&audio};
    }
};
}
#endif

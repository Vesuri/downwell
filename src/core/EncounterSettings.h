#ifndef DOWNWELL_CORE_ENCOUNTER_SETTINGS_H
#define DOWNWELL_CORE_ENCOUNTER_SETTINGS_H
#include "AudioLevel.h"
namespace downwell {
// Defaults from scrGlobalStuff and scrLoadSave. Loading saved settings is a
// separate admission; master gain must not replace the per-script gameVolume.
struct EncounterSettings {
    AudioLevel gameVolume=audio_level::one;
    AudioLevel masterGain=audio_level::one; // 1.0 (the original ini default is 0.6)
    bool lowSpec=false,ending=false;
};
}
#endif

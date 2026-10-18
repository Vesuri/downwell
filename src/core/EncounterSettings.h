#ifndef DOWNWELL_CORE_ENCOUNTER_SETTINGS_H
#define DOWNWELL_CORE_ENCOUNTER_SETTINGS_H
#include "MotionBinary64.h"
namespace downwell {
// Defaults from scrGlobalStuff and scrLoadSave. Loading saved settings is a
// separate admission; master gain must not replace the per-script gameVolume.
struct EncounterSettings {
    MotionBinary64 gameVolume=MotionBinary64::fromInteger(1);
    MotionBinary64 masterGain{0x3fe33333u,0x33333333u}; // ini default 0.6
    bool lowSpec=false,ending=false;
};
}
#endif

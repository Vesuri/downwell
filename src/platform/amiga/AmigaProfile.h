#ifndef DOWNWELL_AMIGA_PROFILE_H
#define DOWNWELL_AMIGA_PROFILE_H

#include <exec/types.h>

enum DownwellProfilePhase {
    profileOther, profileWait, profileInput, profileSimulation,
    profileTitle, profileSprite, profileScroll, profileCopper,
    profilePublish, profileStyleCopy, profileStyleActors, profileStyleText,
    profileStyleDrain, profileStyleMirror, profilePhaseCount
};

#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
void downwellProfileVblank();
void downwellProfileBegin(unsigned scene);
void downwellProfileEnd();
void downwellProfilePhase(unsigned phase);
class DownwellProfileFrame {
public:
    explicit DownwellProfileFrame(unsigned scene) { downwellProfileBegin(scene); }
    ~DownwellProfileFrame() { downwellProfileEnd(); }
};
#define DW_PROFILE_FRAME(scene) DownwellProfileFrame profileFrame(scene)
#ifdef DOWNWELL_PROBES
#define DW_PROFILE_PHASE(phase) downwellProfilePhase(phase)
#else
#define DW_PROFILE_PHASE(phase) ((void)0)
#endif
#else
#define DW_PROFILE_FRAME(scene) ((void)0)
#define DW_PROFILE_PHASE(phase) ((void)0)
#endif

#endif

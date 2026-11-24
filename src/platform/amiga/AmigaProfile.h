#ifndef DOWNWELL_AMIGA_PROFILE_H
#define DOWNWELL_AMIGA_PROFILE_H
#include "core/Profile.h"
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
// Called with interrupts disabled after restoring our VERTB handler.
void downwellProfileOsFields(unsigned long fields);
#endif
#ifdef DOWNWELL_PROBES
unsigned long downwellAmigaProfileTimestamp();
#endif
#ifdef DOWNWELL_CENSUS
// Per-update work census (W3.5). Call once per main-loop iteration, before
// DW_PROFILE_FRAME: it closes the previous update's record and opens this
// one's. Gameplay state is ignored outside gameplay.
enum DownwellCensusFlag : unsigned char {
    censusSlowed = 1,      // room speed below 60 (fxTimeChange, momentDelay, damage)
    censusDeliberate = 2,  // the loop waited fields on purpose (room speed, sleep)
    censusHandoff = 4,     // scene or level changed in or at this update
    censusOverlay = 8,     // level clear, results or upgrade screen
    censusSideCamera = 16, // the camera changed column (side room, shop)
    censusWarmup = 32,     // first gameplay update, outside the profile window
};
void downwellCensusUpdate(unsigned scene, int roomSpeed, unsigned stage, bool overlay, int cameraX);
void downwellCensusMark(unsigned flags);
#endif
#ifdef DOWNWELL_EFFECT_BLIT_CENSUS
// Effect-surface blits in the current census update (section 5 C): every
// queued blit's words and kind, and the CPU's waits for the queue.
// EFFECT_BLIT_CENSUS=2 serializes them: the backend drains before the clear
// and around each blit, and reports each one's blitter time (update totals
// then stop describing the overlapped game).
enum DownwellEffectBlit : unsigned char {
    effectBlitSmoke, effectBlitCurrency, effectBlitOther, effectBlitShade,
    effectBlitClear, effectBlitPublish };
// The publish copy's words (two planes).
void downwellEffectPublishCensus(unsigned long words);
void downwellEffectBlitCensus(DownwellEffectBlit kind, unsigned long words);
void downwellEffectWaitCensus(bool begin);
unsigned long downwellEffectCensusTick();
void downwellEffectSerialCensus(DownwellEffectBlit kind, unsigned long ticks);
#endif
#endif

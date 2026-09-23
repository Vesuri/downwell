#include "AmigaProfile.h"

#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
// Revs-style emulated clock: never use host time or stop GDB inside a sample.
static const ULONG ticksPerField = 313UL * 256UL;
extern "C" {
#ifdef DOWNWELL_SKIP_TO_GAME
volatile ULONG gDownwellProfileTargetScene = 3;
volatile ULONG gDownwellProfileTargetFrames = 30;
#elif defined(DOWNWELL_SKIP_INTRO)
volatile ULONG gDownwellProfileTargetScene = 2;
volatile ULONG gDownwellProfileTargetFrames = 180;
#else
volatile ULONG gDownwellProfileTargetScene = 1;
volatile ULONG gDownwellProfileTargetFrames = 180;
#endif
volatile ULONG gDownwellProfileVbi;
volatile ULONG gDownwellProfileEpoch;
volatile ULONG gDownwellProfileFrames[4];
volatile ULONG gDownwellProfileFields[4];
volatile ULONG gDownwellProfileMaxGap[4];
volatile ULONG gDownwellProfileTicks[4][profilePhaseCount];
volatile ULONG gDownwellProfileCalls[4][profilePhaseCount];
volatile ULONG gDownwellProfileMax[4][profilePhaseCount];
volatile ULONG gDownwellProfileFrameMax[4];
volatile ULONG gDownwellProfileFrozen;
volatile ULONG gDownwellProfileClockClamps;
__attribute__((noinline, used)) void downwellProfileReady()
{
    __asm__ volatile("" ::: "memory");
}
}

void downwellProfileVblank()
{
    ++gDownwellProfileVbi;
#ifdef DOWNWELL_PROBES
    gDownwellProfileEpoch += ticksPerField;
#endif
}

#ifdef DOWNWELL_PROBES
static ULONG beamTick()
{
    volatile UWORD* const vp = (volatile UWORD*)0xdff004;
    volatile UWORD* const vh = (volatile UWORD*)0xdff006;
    ULONG epoch;
    UWORD high, low, highAgain;
    do {
        epoch = gDownwellProfileEpoch;
        high = *vp;
        low = *vh;
        highAgain = *vp;
    } while (epoch != gDownwellProfileEpoch || ((high ^ highAgain) & 1));
    ULONG now = epoch + ((ULONG)(high & 1) << 16) + low;
    static ULONG previous;
    // Beam wrap precedes the VERTB handler by a small latency interval.
    if ((long)(now - previous) < 0) {
        now = previous;
        ++gDownwellProfileClockClamps;
    }
    previous = now;
    return now;
}
#endif

static unsigned sceneId;
static ULONG startField;
#ifdef DOWNWELL_PROBES
static unsigned phaseId;
static ULONG phaseStart, frameStart;
#endif

void downwellProfileBegin(unsigned scene)
{
    sceneId = scene < 4 ? scene : 0;
    startField = gDownwellProfileVbi;
#ifdef DOWNWELL_PROBES
    frameStart = phaseStart = beamTick();
    phaseId = profileOther;
#endif
}

void downwellProfilePhase(unsigned phase)
{
#ifdef DOWNWELL_PROBES
    const ULONG now = beamTick();
    const ULONG elapsed = now - phaseStart;
    gDownwellProfileTicks[sceneId][phaseId] += elapsed;
    ++gDownwellProfileCalls[sceneId][phaseId];
    if (elapsed > gDownwellProfileMax[sceneId][phaseId])
        gDownwellProfileMax[sceneId][phaseId] = elapsed;
    phaseStart = now;
    phaseId = phase;
#else
    (void)phase;
#endif
}

void downwellProfileEnd()
{
#ifdef DOWNWELL_PROBES
    downwellProfilePhase(profileOther);
    const ULONG elapsed = phaseStart - frameStart;
    if (elapsed > gDownwellProfileFrameMax[sceneId])
        gDownwellProfileFrameMax[sceneId] = elapsed;
#endif
    // The first gameplay frame builds the scene; measure steady-state work
    // separately rather than averaging this one-time setup into frame cost.
    static bool gameplayWarmed = false;
    if (sceneId == 3 && !gameplayWarmed) {
        gameplayWarmed = true;
        for (unsigned phase = 0; phase < profilePhaseCount; ++phase) {
            gDownwellProfileTicks[3][phase] = 0;
            gDownwellProfileCalls[3][phase] = 0;
            gDownwellProfileMax[3][phase] = 0;
        }
        gDownwellProfileFrameMax[3] = 0;
        return;
    }
    const ULONG fields = gDownwellProfileVbi - startField;
    ++gDownwellProfileFrames[sceneId];
    gDownwellProfileFields[sceneId] += fields;
    if (fields > gDownwellProfileMaxGap[sceneId])
        gDownwellProfileMaxGap[sceneId] = fields;
#ifdef DOWNWELL_PROFILE_AUTORUN
    // Freeze only at a completed frame boundary, outside the measured window.
    if (sceneId == gDownwellProfileTargetScene &&
        gDownwellProfileFrames[sceneId] == gDownwellProfileTargetFrames) {
        gDownwellProfileFrozen = 1;
        downwellProfileReady();
        for (;;) { }
    }
#endif
}
#endif

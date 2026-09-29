#include "AmigaProfile.h"
#include "AmigaTerrainProfile.h"
#include "core/GameplayContactProfileInput.h"
#include <exec/types.h>

#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
// Revs-style emulated clock: never use host time or stop GDB inside a sample.
static const ULONG ticksPerField = 313UL * 256UL;
extern "C" {
#ifdef DOWNWELL_PROFILE_TRANSITION
volatile ULONG gDownwellProfileTargetScene = 2;
volatile ULONG gDownwellProfileTargetFrames = 60;
#elif defined(DOWNWELL_SKIP_TO_GAME)
volatile ULONG gDownwellProfileTargetScene = 3;
volatile ULONG gDownwellProfileTargetFrames =
#ifdef DOWNWELL_PROFILE_FRAMES
#if DOWNWELL_PROFILE_FRAMES < 1
#error PROFILE_FRAMES must be positive
#endif
    DOWNWELL_PROFILE_FRAMES;
#else
#ifdef DOWNWELL_PROFILE_CONTACT
    downwell::gameplayContactProfileFrames;
#else
#ifdef DOWNWELL_SKIP_LEVEL
    480;
#else
    600;
#endif
#endif
#endif
#elif defined(DOWNWELL_SKIP_INTRO)
volatile ULONG gDownwellProfileTargetScene = 2;
volatile ULONG gDownwellProfileTargetFrames = 180;
#else
volatile ULONG gDownwellProfileTargetScene = 1;
volatile ULONG gDownwellProfileTargetFrames = 180;
#endif
volatile ULONG gDownwellProfileVbi;
volatile ULONG gDownwellProfileDamageBirths;
// Read-only, pre-Step coverage samples. Enemy slots are allocated records,
// not an active/on-screen count. Dead samples must not masquerade as combat.
volatile ULONG gDownwellProfileGameSamples, gDownwellProfileFirstTick, gDownwellProfileLastTick;
volatile ULONG gDownwellProfileDeadSamples, gDownwellProfileMaxEnemySlots;
volatile long gDownwellProfileFirstViewY, gDownwellProfileLastViewY, gDownwellProfileMaxViewY;
volatile long gDownwellProfileMaxPlayerYQ8, gDownwellProfileLastHealth;
volatile ULONG gDownwellProfileRequireDamage =
#ifdef DOWNWELL_PROFILE_CONTACT
    1;
#else
    0;
#endif
volatile ULONG gDownwellProfileEpoch;
volatile ULONG gDownwellProfileFrames[4];
volatile ULONG gDownwellProfileFields[4];
volatile ULONG gDownwellProfileMaxGap[4];
volatile ULONG gDownwellProfileMaxGapFrame[4];
volatile ULONG gDownwellProfileMissCount;
volatile ULONG gDownwellProfileMissFrame[32];
volatile ULONG gDownwellProfileMissFields[32];
volatile long gDownwellProfileMissScroll[32];
volatile long gDownwellProfileMissCamera[32];
volatile long gDownwellProfileScroll;
volatile long gDownwellProfileCameraChange;
volatile long gDownwellProfileStagedHits;
volatile long gDownwellProfileStageShortfall;
volatile ULONG gDownwellProfileTicks[4][profilePhaseCount];
volatile ULONG gDownwellProfileCalls[4][profilePhaseCount];
volatile ULONG gDownwellProfileMax[4][profilePhaseCount];
volatile ULONG gDownwellProfileFrameMax[4];
volatile ULONG gDownwellProfileFrozen;
volatile ULONG gDownwellProfileClockClamps;
volatile ULONG gDownwellProfileSection;
volatile ULONG gDownwellSectionFrames[7], gDownwellSectionFields[7], gDownwellSectionMaxGap[7];
volatile ULONG gDownwellSectionTicks[7][profilePhaseCount], gDownwellSectionMax[7][profilePhaseCount];
#ifdef DOWNWELL_PROBES
volatile unsigned long gDownwellProfileCounters[profileCounterCount];
volatile ULONG gDownwellProfileWorstTicks[4][profilePhaseCount];
volatile ULONG gDownwellProfileScopeEntries[4][profilePhaseCount];
volatile ULONG gDownwellProfileWorstScopeEntries[4][profilePhaseCount];
volatile ULONG gDownwellProfileWorstFrame[4];
volatile ULONG gDownwellProfileWorstSpanCount[4];
volatile AmigaTerrainProfileSpan gDownwellProfileWorstSpans[4][16];
#endif
__attribute__((noinline, used)) void downwellProfileReady()
{
    // Keep this debugger gate reachable through LTO/section collection.
    (void)gDownwellProfileRequireDamage;
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

void downwellProfileGameplay(unsigned tick,long playerYQ8,long viewY,int health,unsigned enemySlots){
    if(gDownwellProfileFrozen)return;
    if(!gDownwellProfileGameSamples){
        gDownwellProfileFirstTick=tick;
        gDownwellProfileFirstViewY=gDownwellProfileMaxViewY=viewY;
        gDownwellProfileMaxPlayerYQ8=playerYQ8;
    }
    ++gDownwellProfileGameSamples;
    gDownwellProfileLastTick=tick;gDownwellProfileLastViewY=viewY;
    gDownwellProfileLastHealth=health;
    if(health<=0)++gDownwellProfileDeadSamples;
    if(viewY>gDownwellProfileMaxViewY)gDownwellProfileMaxViewY=viewY;
    if(playerYQ8>gDownwellProfileMaxPlayerYQ8)gDownwellProfileMaxPlayerYQ8=playerYQ8;
    if(enemySlots>gDownwellProfileMaxEnemySlots)gDownwellProfileMaxEnemySlots=enemySlots;
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
static bool frameActive;
static ULONG phaseStart, frameStart;
static ULONG frameTicks[profilePhaseCount];
static ULONG frameScopeEntries[profilePhaseCount];
static unsigned terrainSpanCount;
static AmigaTerrainProfileSpan terrainSpans[16];
void downwellProfileTerrainSpan(int viewX, int firstY, int height,
    int clipX, int width, unsigned kind, unsigned cause) {
    const unsigned slot = terrainSpanCount++;
    if (slot < 16) terrainSpans[slot] = {viewX, firstY, height, clipX, width,
        static_cast<long>(kind), static_cast<long>(cause)};
}
#endif

void downwellProfileBegin(unsigned scene)
{
    sceneId = scene < 4 ? scene : 0;
    startField = gDownwellProfileVbi;
#ifdef DOWNWELL_PROBES
    frameActive = true;
    terrainSpanCount = 0;
    for (unsigned phase = 0; phase < profilePhaseCount; ++phase) {
        frameTicks[phase] = 0;
        frameScopeEntries[phase] = 0;
    }
    frameStart = phaseStart = beamTick();
    phaseId = profileOther;
#endif
}

void downwellProfilePhase(unsigned phase)
{
#ifdef DOWNWELL_PROBES
    if (!frameActive) return;
    const ULONG now = beamTick();
    const ULONG elapsed = now - phaseStart;
    frameTicks[phaseId] += elapsed;
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
    frameActive = false;
    const ULONG elapsed = phaseStart - frameStart;
    if (elapsed > gDownwellProfileFrameMax[sceneId]) {
        gDownwellProfileFrameMax[sceneId] = elapsed;
        gDownwellProfileWorstFrame[sceneId] = gDownwellProfileFrames[sceneId] + 1;
        gDownwellProfileWorstSpanCount[sceneId] = terrainSpanCount;
        for (unsigned i = 0; i < terrainSpanCount && i < 16; ++i) {
            volatile auto& target = gDownwellProfileWorstSpans[sceneId][i];
            const auto& source = terrainSpans[i];
            target.viewX = source.viewX; target.firstY = source.firstY;
            target.height = source.height; target.clipX = source.clipX;
            target.width = source.width; target.kind = source.kind; target.cause = source.cause;
        }
        for (unsigned phase = 0; phase < profilePhaseCount; ++phase) {
            gDownwellProfileWorstTicks[sceneId][phase] = frameTicks[phase];
            gDownwellProfileWorstScopeEntries[sceneId][phase] = frameScopeEntries[phase];
        }
    }
#endif
    // The first gameplay frame builds the scene; measure steady-state work
    // separately rather than averaging this one-time setup into frame cost.
    static bool gameplayWarmed = false;
    if (sceneId == 3 && !gameplayWarmed) {
        gameplayWarmed = true;
        gDownwellProfileGameSamples=0;
        gDownwellProfileDeadSamples=0;
        gDownwellProfileMaxEnemySlots=0;
        for (unsigned phase = 0; phase < profilePhaseCount; ++phase) {
            gDownwellProfileTicks[3][phase] = 0;
            gDownwellProfileCalls[3][phase] = 0;
            gDownwellProfileMax[3][phase] = 0;
        }
        gDownwellProfileFrameMax[3] = 0;
#ifdef DOWNWELL_PROBES
        gDownwellProfileWorstFrame[3] = 0;
        gDownwellProfileWorstSpanCount[3] = 0;
        for (unsigned phase = 0; phase < profilePhaseCount; ++phase) {
            gDownwellProfileWorstTicks[3][phase] = 0;
            gDownwellProfileScopeEntries[3][phase] = 0;
            gDownwellProfileWorstScopeEntries[3][phase] = 0;
        }
        for (unsigned counter = 0; counter < profileCounterCount; ++counter)
            gDownwellProfileCounters[counter] = 0;
#endif
        return;
    }
    const ULONG fields = gDownwellProfileVbi - startField;
    const unsigned section = gDownwellProfileSection < 7 ? gDownwellProfileSection : 0;
    ++gDownwellSectionFrames[section];
    gDownwellSectionFields[section] += fields;
    if (fields > gDownwellSectionMaxGap[section]) gDownwellSectionMaxGap[section] = fields;
#ifdef DOWNWELL_PROBES
    for (unsigned phase = 0; phase < profilePhaseCount; ++phase) {
        gDownwellSectionTicks[section][phase] += frameTicks[phase];
        if (frameTicks[phase] > gDownwellSectionMax[section][phase])
            gDownwellSectionMax[section][phase] = frameTicks[phase];
    }
#endif
    ++gDownwellProfileFrames[sceneId];
    if (sceneId == gDownwellProfileTargetScene && fields > 1) {
        const ULONG slot = gDownwellProfileMissCount++;
        if (slot < 32) {
            gDownwellProfileMissFrame[slot] = gDownwellProfileFrames[sceneId];
            gDownwellProfileMissFields[slot] = fields;
            gDownwellProfileMissScroll[slot] = gDownwellProfileScroll;
            gDownwellProfileMissCamera[slot] = gDownwellProfileCameraChange;
        }
    }
    gDownwellProfileFields[sceneId] += fields;
    if (fields > gDownwellProfileMaxGap[sceneId]) {
        gDownwellProfileMaxGap[sceneId] = fields;
        gDownwellProfileMaxGapFrame[sceneId] = gDownwellProfileFrames[sceneId];
    }
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
#ifdef DOWNWELL_PROBES
unsigned downwellProfileEnter(unsigned phase) {
    const unsigned previous = phaseId;
    // Phase segments include resumed parents after nested scopes. Count only
    // constructor entries here when measuring actual operation multiplicity.
    if(frameActive&&phase<profilePhaseCount){
        ++frameScopeEntries[phase];
        ++gDownwellProfileScopeEntries[sceneId][phase];
    }
    downwellProfilePhase(phase);
    return previous;
}
#endif
#endif

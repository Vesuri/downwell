#include "AmigaProfile.h"
#include "AmigaTerrainProfile.h"
#include "core/GameplayContactProfileInput.h"
#include "core/GameplayEntryProfileInput.h"
#include "core/TerrainReadCensus.h"
#include <exec/types.h>

#ifdef DOWNWELL_TERRAIN_READ_CENSUS
extern "C" {
volatile unsigned long gDownwellTerrainReads[2][terrainCensusCount];
volatile unsigned long gDownwellTerrainReadSamples[165][2][terrainCensusCount];
volatile unsigned long gDownwellTerrainReadFrames;
bool gDownwellTerrainReadActive=false;
unsigned gDownwellTerrainConstructionDepth=0;
}
static void terrainReadFrame(unsigned frame){
    if(frame>165){gDownwellTerrainReadActive=false;return;}
    for(unsigned scope=0;scope<2;++scope)for(unsigned item=0;item<terrainCensusCount;++item)
        gDownwellTerrainReadSamples[frame-1][scope][item]=gDownwellTerrainReads[scope][item];
    gDownwellTerrainReadFrames=frame;
    gDownwellTerrainReadActive=frame<165;
}
#endif

#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_FPSCOUNT)
#include <graphics/gfxbase.h>
extern struct GfxBase* GfxBase;
// Revs-style emulated clock: never use host time or stop GDB inside a sample.
static const ULONG ticksPerField = 313UL * 256UL;
static bool gameplayWarmed = false;
extern "C" {
volatile UWORD gDownwellSamplePhase;
volatile UWORD gDownwellSampleActive;
volatile ULONG gDownwellSampleFrame;
volatile ULONG gDownwellSampleFocus =
#ifdef DOWNWELL_PHASE_SAMPLING
    DOWNWELL_SAMPLE_FRAME;
#else
    0;
#endif
volatile ULONG gDownwellSamplePhaseCount = profilePhaseCount;
volatile ULONG gDownwellPhaseSamples[profilePhaseCount];
volatile ULONG gDownwellFocusSamples[profilePhaseCount];
volatile ULONG gDownwellTerrainPreparationCalls, gDownwellTerrainPreparationFields;
volatile ULONG gDownwellTerrainPreparationExpectedCalls =
    // All scenes prepare terrain Create plans. Only the full-service owners
    // used by these fixtures also admit prepared collision birth bounds.
#if defined(DOWNWELL_SCENE_VERIFY) || defined(DOWNWELL_FIELD_VERIFY) || defined(DOWNWELL_ENTRY_WIPE_VERIFY)
    2;
#else
    1;
#endif
#ifdef DOWNWELL_PROFILE_SHOP
volatile ULONG gDownwellProfileTargetScene = 3;
// Completion sets the exact boundary; this hard ceiling rejects a stalled tape.
volatile ULONG gDownwellProfileTargetFrames = 4000;
#elif defined(DOWNWELL_PROFILE_ENTRY)
volatile ULONG gDownwellProfileTargetScene = 3;
volatile ULONG gDownwellProfileTargetFrames = downwell::GameplayEntryProfileInput::gameplayFrames;
#elif defined(DOWNWELL_PROFILE_TRANSITION)
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
#if defined(DOWNWELL_PROFILE_CONTACT) || defined(DOWNWELL_PROFILE_OVERFLOW)
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
volatile ULONG gDownwellProfileOsFieldCount;
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
volatile ULONG gDownwellProfileIdle =
#ifdef DOWNWELL_PROFILE_IDLE
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
volatile ULONG gDownwellProfileGenerationTiming =
#ifdef DOWNWELL_GENERATION_TIMING
    1;
#else
    0;
#endif
volatile ULONG gDownwellProfileGenerationCalls, gDownwellProfileGenerationTicks;
volatile ULONG gDownwellProfileGenerationPhase =
#ifdef DOWNWELL_GENERATION_TIMING
    DOWNWELL_GENERATION_TIMING_PHASE;
#else
    profileGeneration;
#endif
volatile ULONG gDownwellProfileGenerationMax, gDownwellProfileGenerationMaxFrame;
volatile ULONG gDownwellProfileGenerationWorstGapTicks;
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
    (void)gDownwellProfileIdle;
    (void)gDownwellProfileGenerationTiming;
    (void)gDownwellProfileGenerationPhase;
    (void)gDownwellTerrainPreparationCalls;
    (void)gDownwellTerrainPreparationFields;
    (void)gDownwellTerrainPreparationExpectedCalls;
    (void)gDownwellSampleFocus;
    (void)gDownwellSamplePhaseCount;
    __asm__ volatile("" ::: "memory");
}
#ifdef DOWNWELL_TRACE_FRAMES
// Emulator instruction-trace anchors. The debugger starts `monitor profile`
// at the first; the trace decoder splits game frames at entries to the second.
// Neither reads the clock or touches game state.
static const ULONG traceFrames[] = {DOWNWELL_TRACE_FRAMES};
volatile ULONG gDownwellTraceFrameCount = sizeof traceFrames / sizeof traceFrames[0];
__attribute__((noinline, used)) void downwellTraceReady(ULONG frame)
{
    __asm__ volatile("" :: "d"(frame) : "memory");
}
__attribute__((noinline, used)) void downwellTraceFrameMark(ULONG frame)
{
    __asm__ volatile("" :: "d"(frame) : "memory");
}
#endif
}

#ifdef DOWNWELL_DRAW_CACHE_AUDIT
extern "C" { volatile ULONG gDownwellDrawCacheAudit[6][8]; }
namespace downwell {
void auditActorDrawCache(unsigned outcome,unsigned actors,bool captureChanged,bool layersChanged){
    if(!gameplayWarmed||gDownwellProfileFrozen)return;
    const unsigned frame=gDownwellProfileFrames[3]+1;
    auto record=[&](unsigned bucket){
        auto* counts=gDownwellDrawCacheAudit[bucket];
        ++counts[outcome];counts[4]+=actors;
        if(captureChanged)++counts[5];
        if(layersChanged)++counts[6];
        if(outcome==1)counts[7]+=actors;
    };
    record(0);
    if(frame==645)record(1);
    if(frame==701)record(2);
    if(frame==844)record(3);
    if(frame<=300)record(4);
    if(frame>=600&&frame<=660)record(5);
}
}
#endif

unsigned long downwellTerrainPreparationBegin(){
    const ULONG start=GfxBase?*static_cast<volatile ULONG*>(&GfxBase->VBCounter):0;
    __asm__ volatile("" ::: "memory");
    return start;
}
void downwellTerrainPreparationEnd(unsigned long start){
    __asm__ volatile("" ::: "memory");
    if(!GfxBase)return;
    gDownwellTerrainPreparationFields+=*static_cast<volatile ULONG*>(&GfxBase->VBCounter)-start;
    ++gDownwellTerrainPreparationCalls;
}

void downwellProfileVblank()
{
    ++gDownwellProfileVbi;
#ifdef DOWNWELL_PHASE_SAMPLING
    if(gDownwellSampleActive){
        const unsigned phase=gDownwellSamplePhase;
        if(phase<profilePhaseCount){
            ++gDownwellPhaseSamples[phase];
            if(gDownwellSampleFrame==gDownwellSampleFocus)++gDownwellFocusSamples[phase];
        }
    }
#endif
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_GENERATION_TIMING) || defined(DOWNWELL_CENSUS)
    gDownwellProfileEpoch += ticksPerField;
#endif
}

void downwellProfileOsFields(unsigned long fields)
{
    // loadFile borrows the OS VERTB chain. Those fields do not invoke our
    // handler, but still belong to the active frame and its current phase.
    gDownwellProfileOsFieldCount += fields;
    gDownwellProfileVbi += fields;
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_GENERATION_TIMING) || defined(DOWNWELL_CENSUS)
    gDownwellProfileEpoch += fields * ticksPerField;
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

#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_GENERATION_TIMING) || defined(DOWNWELL_CENSUS)
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
#ifdef DOWNWELL_PROBES
unsigned long downwellAmigaProfileTimestamp(){return beamTick();}
#endif
#ifdef DOWNWELL_CENSUS
#include "framework/AmigaPlatform.h"
#ifndef DOWNWELL_CENSUS_UPDATES
#define DOWNWELL_CENSUS_UPDATES 16384
#endif
static_assert((DOWNWELL_CENSUS_UPDATES & (DOWNWELL_CENSUS_UPDATES - 1)) == 0,
              "CENSUS_UPDATES must be a power of two");
// One record per main-loop iteration, mark to mark (as the instruction
// trace splits frames). Ticks are beam positions, 313 x 256 per PAL field,
// so they mean the same on every CPU. work = span minus every display wait.
// The debugger reads the ring once, at the profile freeze; nothing stops
// inside a measured update. Read by tools/amiga-census-report.py.
struct DownwellCensusRecord {
    ULONG work, span, frame;
    UBYTE scene, roomSpeed, flags, stage;
};
static_assert(sizeof(DownwellCensusRecord) == 16, "census record layout");
extern "C" {
// Written by the first update (constant initialisers fold away under LTO).
volatile ULONG gDownwellCensusCapacity, gDownwellCensusTicksPerField;
volatile ULONG gDownwellCensusCount;   // records closed, including overwritten
DownwellCensusRecord gDownwellCensus[DOWNWELL_CENSUS_UPDATES];
}
static bool censusOpen;
static ULONG censusStart, censusWaitStart, censusWaited;
static unsigned censusWaitDepth;
static DownwellCensusRecord censusCurrent;
static int censusCameraX;
#ifdef DOWNWELL_EFFECT_BLIT_CENSUS
// Parallel to gDownwellCensus, same index. words: blit words queued (each
// row of each blit; the surface clear's are its D words). wait: ticks the CPU
// spent draining the effect queue. busy: ticks from the update's first
// queued effect stamp to the end of its first drain (everything queued ahead
// of the stamps included). serial: EFFECT_BLIT_CENSUS=2's blitter ticks per
// DownwellEffectBlit kind. clears: the clear's rectangle blits.
struct DownwellEffectCensusRecord {
    ULONG words[4], clearWords, publishWords, wait, busy, serial[6];
    UWORD stamps[4], drains, clears;
};
static_assert(sizeof(DownwellEffectCensusRecord) == 68, "effect census layout");
extern "C" { DownwellEffectCensusRecord gDownwellEffectCensus[DOWNWELL_CENSUS_UPDATES]; }
static DownwellEffectCensusRecord effectCurrent;
static ULONG effectFirst, effectWaitStart;
static bool effectStamped;
void downwellEffectBlitCensus(DownwellEffectBlit kind, unsigned long words)
{
    if (kind == effectBlitClear) { effectCurrent.clearWords += words; ++effectCurrent.clears; return; }
    if (!effectStamped) { effectStamped = true; effectFirst = beamTick(); }
    ++effectCurrent.stamps[kind];
    effectCurrent.words[kind] += words;
}
void downwellEffectWaitCensus(bool begin)
{
    const ULONG now = beamTick();
    if (begin) { effectWaitStart = now; return; }
    effectCurrent.wait += now - effectWaitStart;
    if (!effectCurrent.drains++ && effectStamped) effectCurrent.busy = now - effectFirst;
}
unsigned long downwellEffectCensusTick() { return beamTick(); }
void downwellEffectPublishCensus(unsigned long words) { effectCurrent.publishWords += words; }
void downwellEffectSerialCensus(DownwellEffectBlit kind, unsigned long ticks)
{
    effectCurrent.serial[kind] += ticks;
}
#endif
#ifdef DOWNWELL_BLIT_CENSUS
#include "AmigaRenderer.h"
// W3.15, parallel to gDownwellCensus, same index: every blit started in the
// update (AmigaHardware), by kind (clear, copy, masked copy, masked copy with
// background, line, fill, solid rectangle, combine, pattern), with its words
// (width x height). A twin repeats the previous blit's kind and size one plane
// further on: the second plane's blit of a non-interleaved bitmap, which one
// interleaved blit would cover. twins[0]: the gameplay ring's plane stride,
// [1]: the display/HUD plane stride, [2]: any other forward stride >= 2 KB.
// queued: blits that waited in the queue for the CPU to start them.
struct DownwellBlitCensusRecord {
    ULONG words[9], twinWords[3];
    UWORD blits[9], twins[3], queued, pad;
};
static_assert(sizeof(DownwellBlitCensusRecord) == 76, "blit census layout");
extern "C" { DownwellBlitCensusRecord gDownwellBlitCensus[DOWNWELL_CENSUS_UPDATES]; }
static DownwellBlitCensusRecord blitCurrent;
static const unsigned char* blitLastDestination;
static unsigned blitLastKind = ~0u;
static UWORD blitLastSize;
void downwellBlitStartCensus(unsigned kind, const void* destination, UWORD size, bool queued)
{
    const unsigned height = (size >> 6) ? (size >> 6) : 1024, width = (size & 63) ? (size & 63) : 64;
    const ULONG words = static_cast<ULONG>(height) * width;
    ++blitCurrent.blits[kind];
    blitCurrent.words[kind] += words;
    if (queued) ++blitCurrent.queued;
    const unsigned char* target = static_cast<const unsigned char*>(destination);
    if (kind == blitLastKind && size == blitLastSize && target > blitLastDestination) {
        const unsigned long stride = static_cast<unsigned long>(target - blitLastDestination);
        const int twin = stride == downwellGameRingPlaneBytes ? 0 : stride == downwellPlaneBytes ? 1 :
            stride >= 2048 ? 2 : -1;
        if (twin >= 0) { ++blitCurrent.twins[twin]; blitCurrent.twinWords[twin] += words; }
    }
    blitLastKind = kind; blitLastSize = size; blitLastDestination = target;
}
#endif
static void censusWait(bool begin)
{
    if (begin) {
        if (censusWaitDepth++ == 0) censusWaitStart = beamTick();
    } else if (censusWaitDepth && --censusWaitDepth == 0) {
        censusWaited += beamTick() - censusWaitStart;
    }
}
void downwellCensusMark(unsigned flags)
{
    censusCurrent.flags |= static_cast<UBYTE>(flags);
}
void downwellCensusUpdate(unsigned scene, int roomSpeed, unsigned stage, bool overlay, int cameraX)
{
    if (gDownwellProfileFrozen) return;
    const ULONG now = beamTick();
    const bool gameplay = scene == 3;
    if (!gameplay) { roomSpeed = 60; stage = 0; overlay = false; cameraX = censusCameraX; }
    if (censusOpen) {
        // The change happened during the closing update: flag it there, and
        // flag the first update of the new scene or level as well.
        if (scene != censusCurrent.scene || stage != censusCurrent.stage)
            censusCurrent.flags |= censusHandoff;
        if (gameplay && censusCurrent.scene == 3 && cameraX != censusCameraX)
            censusCurrent.flags |= censusSideCamera;
        if (overlay) censusCurrent.flags |= censusOverlay;
        censusCurrent.span = now - censusStart;
        censusCurrent.work = censusCurrent.span - censusWaited;
        gDownwellCensus[gDownwellCensusCount & (DOWNWELL_CENSUS_UPDATES - 1)] = censusCurrent;
#ifdef DOWNWELL_EFFECT_BLIT_CENSUS
        gDownwellEffectCensus[gDownwellCensusCount & (DOWNWELL_CENSUS_UPDATES - 1)] = effectCurrent;
        __asm__ volatile("" :: "g"(gDownwellEffectCensus) : "memory");
#endif
#ifdef DOWNWELL_BLIT_CENSUS
        gDownwellBlitCensus[gDownwellCensusCount & (DOWNWELL_CENSUS_UPDATES - 1)] = blitCurrent;
        __asm__ volatile("" :: "g"(gDownwellBlitCensus) : "memory");
#endif
        ++gDownwellCensusCount;
        // Only the debugger reads the ring: keep its stores, and keep the
        // ring and its constants through section GC.
        __asm__ volatile("" :: "g"(gDownwellCensus) : "memory");
    } else {
        gDownwellCensusCapacity = DOWNWELL_CENSUS_UPDATES;
        gDownwellCensusTicksPerField = ticksPerField;
        AmigaPlatform::waitObserver = censusWait;
    }
    const bool handoff = censusOpen && (scene != censusCurrent.scene || stage != censusCurrent.stage);
    censusOpen = true;
    censusStart = now;
    censusWaited = 0;
#ifdef DOWNWELL_EFFECT_BLIT_CENSUS
    effectCurrent = DownwellEffectCensusRecord();
    effectStamped = false;
#endif
#ifdef DOWNWELL_BLIT_CENSUS
    blitCurrent = DownwellBlitCensusRecord();
#endif
    censusCameraX = cameraX;
    censusCurrent.scene = static_cast<UBYTE>(scene);
    censusCurrent.stage = static_cast<UBYTE>(stage);
    censusCurrent.roomSpeed = static_cast<UBYTE>(roomSpeed > 0 && roomSpeed < 255 ? roomSpeed : 0);
    censusCurrent.flags = static_cast<UBYTE>((handoff ? censusHandoff : 0) |
        (roomSpeed > 0 && roomSpeed < 60 ? censusSlowed : 0) | (overlay ? censusOverlay : 0) |
        (gameplay && !gameplayWarmed ? censusWarmup : 0));
    // Gameplay frames use the FPS report's numbering (warm-up excluded).
    censusCurrent.frame = gDownwellProfileFrames[scene < 4 ? scene : 0] + 1;
}
#endif
static ULONG startField;
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_GENERATION_TIMING)
static bool frameActive;
#endif
#ifdef DOWNWELL_GENERATION_TIMING
static ULONG generationFrameTicks;
static bool generationScopeActive;
bool downwellProfileGenerationEnter(unsigned long& start){
    if(!frameActive||sceneId!=3||generationScopeActive)return false;
    generationScopeActive=true;
    start=beamTick();return true;
}
void downwellProfileGenerationLeave(unsigned long start){
    const ULONG elapsed=beamTick()-start;
    generationScopeActive=false;
    generationFrameTicks+=elapsed;
    gDownwellProfileGenerationTicks+=elapsed;
    ++gDownwellProfileGenerationCalls;
}
#endif
#ifdef DOWNWELL_PROBES
static unsigned phaseId;
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
#ifdef DOWNWELL_TRACE_FRAMES
    if(sceneId==3&&gameplayWarmed&&!gDownwellProfileFrozen){
        const ULONG frame=gDownwellProfileFrames[3]+1;
#ifdef DOWNWELL_TERRAIN_READ_CENSUS
        terrainReadFrame(frame);
#endif
        for(ULONG i=0;i<gDownwellTraceFrameCount;++i)
            if(traceFrames[i]==frame)downwellTraceReady(frame);
        downwellTraceFrameMark(frame);
    }
#endif
#ifdef DOWNWELL_PHASE_SAMPLING
    gDownwellSamplePhase=profileOther;
    gDownwellSampleFrame=gDownwellProfileFrames[3]+1;
    gDownwellSampleActive=sceneId==3&&gameplayWarmed&&!gDownwellProfileFrozen;
#endif
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_GENERATION_TIMING)
    frameActive = true;
#endif
#ifdef DOWNWELL_GENERATION_TIMING
    generationFrameTicks=0;
#endif
#ifdef DOWNWELL_PROBES
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
#ifdef DOWNWELL_PHASE_SAMPLING
    gDownwellSampleActive=0;
#endif
#ifdef DOWNWELL_PROBES
    downwellProfilePhase(profileOther);
#endif
#if defined(DOWNWELL_PROBES) || defined(DOWNWELL_GENERATION_TIMING)
    frameActive = false;
#endif
#ifdef DOWNWELL_PROBES
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
#ifdef DOWNWELL_PROFILE_ENTRY
    // Entry profiling explicitly includes cold setup and its first frame.
    if(sceneId==3)gameplayWarmed=true;
#else
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
#ifdef DOWNWELL_GENERATION_TIMING
        gDownwellProfileGenerationCalls=gDownwellProfileGenerationTicks=0;
        gDownwellProfileGenerationMax=gDownwellProfileGenerationMaxFrame=0;
        gDownwellProfileGenerationWorstGapTicks=0;
#endif
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
#endif
#ifdef DOWNWELL_GENERATION_TIMING
    if(sceneId==3&&generationFrameTicks>gDownwellProfileGenerationMax){
        gDownwellProfileGenerationMax=generationFrameTicks;
        gDownwellProfileGenerationMaxFrame=gDownwellProfileFrames[3]+1;
    }
#endif
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
#ifdef DOWNWELL_GENERATION_TIMING
        if(sceneId==3)gDownwellProfileGenerationWorstGapTicks=generationFrameTicks;
#endif
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

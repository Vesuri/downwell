#include "platform/amiga/AmigaStyleScene.h"
#include "platform/amiga/AmigaSceneRuntime.h"
#include <cassert>
#include <cstdio>
#include <vector>

// Lifecycle doubles only. Native scene verification tests the real rendering
// and Copper implementation; these counters test ownership and reset policy.
class Sprite {};
class Palette {};
static unsigned preparations, freshFrames, retainedFrames, builds, patches;
static bool prepareOK=true, renderOK=true, copperOK=true;
CopperList::CopperList(UWORD* words,unsigned capacity)
    : words_(words),capacity_(capacity),count_(0),valid_(words!=nullptr) {}
DownwellStyleCopperList::DownwellStyleCopperList(UWORD* words,unsigned capacity)
    : CopperList(words,capacity) {}
bool DownwellStyleCopperList::build(const UBYTE*,const UBYTE*,const Sprite&,const Palette&,const UBYTE*) {
    ++builds; return copperOK;
}
bool DownwellStyleCopperList::update(const UBYTE*,const UBYTE*,const UBYTE*) {
    ++patches; return copperOK;
}
bool amigaBuildStyleBorder(const UBYTE*,const UBYTE*,UBYTE*) { ++preparations; return prepareOK; }
const UBYTE* amigaStyleMask(const UBYTE* cache,const AmigaSceneRuntime&) { return cache+amigaStyleMaskOffset; }
bool amigaRenderStyleFrame(const UBYTE*,const UBYTE*,const UBYTE*,const AmigaSceneRuntime&,
    UBYTE*,AmigaStyleFrameState& state) {
    if(state.initialized) ++retainedFrames; else ++freshFrames;
    state.initialized=true;
    return renderOK;
}
int main() {
    std::vector<UBYTE> cache(amigaStyleCacheBytes);
    UWORD words[2][400]{};
    UBYTE art[20]{};
    Sprite sprite; Palette palette;
    AmigaSceneRuntime runtime;
    assert(runtime.enterResultsDestination(downwell::ResultsDestination::styleSelect));
    AmigaStyleScene scene(cache.data(),cache.size(),words[0],words[1],400);
    AmigaStyleScene::Frame frame;
    assert(!scene.render(0,runtime,frame));
    assert(!scene.updateCopper(0,frame,sprite,palette));
    assert(!scene.prepare(nullptr,art+20));
    assert(preparations==0);
    assert(scene.prepare(art,art+20));
    assert(!scene.render(2,runtime,frame));
    assert(!scene.updateCopper(2,frame,sprite,palette));
    for(unsigned round=0;round<3;++round) for(unsigned slot=0;slot<2;++slot) {
        assert(scene.render(slot,runtime,frame));
        assert(frame.content==cache.data()+slot*amigaStyleFrameBytes);
        assert(frame.border==cache.data()+2*amigaStyleFrameBytes-runtime.styleTileY()*amigaStyleBytesPerRow);
        assert(frame.mask==cache.data()+amigaStyleMaskOffset);
        assert(scene.updateCopper(slot,frame,sprite,palette));
        assert(scene.copper(slot).data()==words[slot]);
    }
    assert(freshFrames==2 && retainedFrames==4 && builds==2 && patches==4);
    // Re-entry may reuse the same addresses with unrelated scene data.
    assert(scene.prepare(art,art+20));
    for(unsigned slot=0;slot<2;++slot) {
        assert(scene.render(slot,runtime,frame));
        assert(scene.updateCopper(slot,frame,sprite,palette));
    }
    assert(freshFrames==4 && builds==4);
    renderOK=false;
    assert(!scene.render(0,runtime,frame));
    assert(!frame.content && !frame.border && !frame.mask);
    renderOK=true; prepareOK=false;
    assert(!scene.prepare(art,art+20));
    assert(!scene.render(0,runtime,frame));
    prepareOK=true;
    assert(scene.prepare(art,art+20));
    assert(scene.render(0,runtime,frame));
    copperOK=false;
    assert(!scene.updateCopper(0,frame,sprite,palette));
    const unsigned before=builds;
    copperOK=true;
    assert(scene.updateCopper(0,frame,sprite,palette));
    assert(builds==before+1); // failed first build cannot become a patch
    AmigaStyleScene small(cache.data(),cache.size()-1,words[0],words[1],400);
    assert(!small.prepare(art,art+20));
    std::puts("Style scene owns alternating frames/lists, resets on re-entry and propagates failures");
}

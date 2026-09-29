#include "platform/amiga/AmigaStyleScene.h"
#include "platform/amiga/AmigaSceneRuntime.h"
#include "platform/amiga/framework/AmigaHardware.h"
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
DownwellStyleCopperList::DownwellStyleCopperList(UWORD* words,unsigned capacity,unsigned planeBytes)
    : CopperList(words,capacity),framePlaneBytes_(planeBytes) { assert(planeBytes==AmigaSelectionLayer::planeBytes); }
bool DownwellStyleCopperList::build(const UBYTE*,const UBYTE*,const Sprite&,const Palette&,const UBYTE*) {
    ++builds; return copperOK;
}
bool DownwellStyleCopperList::update(const UBYTE*,const UBYTE*,const UBYTE*) {
    ++patches; return copperOK;
}
bool amigaBuildStyleBorderPart(const UBYTE*,const UBYTE*,UBYTE*,AmigaStylePreparation& state) {
    if(!state.part)++preparations;
    if(!prepareOK)return false;
    ++state.part;return true;
}
const UBYTE* amigaStyleMask(const UBYTE* cache,const AmigaSceneRuntime&) { return cache+amigaStyleMaskOffset; }
bool AmigaSelectionLayer::render(UBYTE* target,const UBYTE*,const UBYTE*,const Command*,unsigned,unsigned& painted) {
    if(initialized_) ++retainedFrames; else ++freshFrames;
    initialized_=true; target_=target; painted=0;
    return renderOK;
}
void AmigaHardware::blitterDrain() {}
int main() {
    std::vector<UBYTE> cache(amigaStyleCacheBytes);
    std::vector<UBYTE> canvases(AmigaSelectionLayer::bytes*2);
    UWORD words[2][400]{};
    UBYTE art[20]{};
    Sprite sprite; Palette palette;
    AmigaSceneRuntime runtime;
    assert(runtime.enterResultsDestination(downwell::ResultsDestination::styleSelect));
    AmigaStyleScene scene(cache.data(),cache.size(),words[0],words[1],400,
        canvases.data(),canvases.data()+AmigaSelectionLayer::bytes,AmigaSelectionLayer::bytes);
    AmigaStyleScene::Frame frame;
    assert(!scene.render(0,runtime,frame));
    assert(!scene.updateCopper(0,frame,sprite,palette));
    assert(!scene.prepare(nullptr,art+20));
    assert(preparations==0);
    assert(scene.beginPrepare(art,art+20)&&preparations==0);
    for(unsigned part=0;part<5;++part){
        assert(scene.preparePart());
        assert(!scene.render(0,runtime,frame)); // partial pixels are never publishable
    }
    assert(preparations==1);
    assert(scene.prepare(art,art+20,true)); // finish the pending plan, not restart it
    assert(preparations==1);
    assert(scene.preparePart()&&preparations==1); // completed service is a no-op
    assert(!scene.render(2,runtime,frame));
    assert(!scene.updateCopper(2,frame,sprite,palette));
    for(unsigned round=0;round<3;++round) for(unsigned slot=0;slot<2;++slot) {
        assert(scene.render(slot,runtime,frame));
        assert(frame.content==canvases.data()+slot*AmigaSelectionLayer::bytes+
            (AmigaStyleDrawList::baseView-runtime.styleYOffset())*AmigaSelectionLayer::stride);
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
    const unsigned prepared=preparations;
    assert(scene.prepare(art,art+20,true)&&preparations==prepared);
    for(unsigned slot=0;slot<2;++slot) {
        assert(scene.render(slot,runtime,frame));
        assert(scene.updateCopper(slot,frame,sprite,palette));
    }
    assert(freshFrames==6&&builds==6); // cache reuse cannot retain Copper/canvases
    UBYTE otherArt[20]{};
    assert(scene.prepare(otherArt,otherArt+20,true)&&preparations==prepared+1);
    assert(scene.prepare(art,art+20,true)&&preparations==prepared+2);
    renderOK=false;
    assert(!scene.render(0,runtime,frame));
    assert(!frame.content && !frame.border && !frame.mask);
    renderOK=true; prepareOK=false;
    assert(!scene.prepare(art,art+20));
    assert(!scene.render(0,runtime,frame));
    prepareOK=true;
    const unsigned failedPreparations=preparations;
    assert(scene.prepare(art,art+20,true)&&preparations==failedPreparations+1);
    assert(scene.render(0,runtime,frame));
    copperOK=false;
    assert(!scene.updateCopper(0,frame,sprite,palette));
    const unsigned before=builds;
    copperOK=true;
    assert(scene.updateCopper(0,frame,sprite,palette));
    assert(builds==before+1); // failed first build cannot become a patch
    assert(scene.beginPrepare(art,art+20)&&scene.preparePart());
    prepareOK=false;
    assert(!scene.preparePart()&&!scene.render(0,runtime,frame));
    prepareOK=true;
    assert(!scene.preparePart()); // failure stays terminal until a new preparation
    const auto beforeRecovery=preparations;
    assert(scene.prepare(art,art+20,true)&&preparations==beforeRecovery+1);
    AmigaStyleScene small(cache.data(),cache.size()-1,words[0],words[1],400,
        canvases.data(),canvases.data()+AmigaSelectionLayer::bytes,AmigaSelectionLayer::bytes);
    assert(!small.prepare(art,art+20));
    AmigaStyleScene shortCanvas(cache.data(),cache.size(),words[0],words[1],400,
        canvases.data(),canvases.data()+AmigaSelectionLayer::bytes,AmigaSelectionLayer::bytes-1);
    assert(!shortCanvas.prepare(art,art+20));
    AmigaStyleScene aliased(cache.data(),cache.size(),words[0],words[1],400,
        canvases.data(),canvases.data(),AmigaSelectionLayer::bytes);
    assert(!aliased.prepare(art,art+20));
    std::puts("Style scene owns alternating frames/lists, resets on re-entry and propagates failures");
}

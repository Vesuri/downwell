#include "platform/amiga/AmigaPublisherScene.h"
#include "platform/amiga/AmigaCreditsScene.h"
#include "platform/amiga/AmigaControlsScene.h"
#include "platform/amiga/AmigaSplashCache.h"
#include "platform/amiga/framework/Bitmap.h"
#include <cassert>
#include <cstdio>
#include <vector>
// Lifecycle doubles; bounded native tests exercise actual artwork and lists.
class Sprite {};
class Palette {};
static bool buildOK=true;
static unsigned narrowCalls,wideCalls;
static const UBYTE *lastFrame,*lastDither;
CopperList::CopperList(UWORD* words,unsigned capacity)
    : words_(words),capacity_(capacity),count_(0),valid_(words!=nullptr) {}
DownwellSplashCopperList::DownwellSplashCopperList(UWORD* words,unsigned capacity)
    : CopperList(words,capacity) {}
bool DownwellSplashCopperList::buildNarrow(const UBYTE* frame,const Sprite&,const Palette&,const UBYTE* dither) {
    ++narrowCalls; lastFrame=frame; lastDither=dither; return buildOK;
}
bool DownwellSplashCopperList::buildWide(const Bitmap& frame,const Sprite&,const Palette&) {
    ++wideCalls; lastFrame=static_cast<const UBYTE*>(frame.data); return buildOK;
}
const UBYTE* amigaSplashFrame(const UBYTE* cache,int stage) { return cache+stage*32; }
const UBYTE* amigaSplashDitherTile(const UBYTE* cache,unsigned frame) { return cache+512+frame; }
int main() {
    std::vector<UBYTE> cache(amigaSplashCacheBytes),credits(48*284*2);
    UWORD words[5][400]{};
    Sprite blank; Palette palette;
    AmigaPublisherScene publisher(words[0],400);
    AmigaCreditsScene credit(words[1],400);
    AmigaControlsScene controls(words[2],words[3],words[4],400);
    bool alternating=true;
    assert(!publisher.presentation() && !credit.presentation());
    assert(!controls.presentation(4,0,0,blank,palette,alternating) && !alternating);
    assert(!publisher.prepare(nullptr,cache.size(),blank,palette));
    assert(!controls.prepare(cache.data(),cache.size()-1,blank,palette));
    assert(narrowCalls==0);
    assert(publisher.prepare(cache.data(),cache.size(),blank,palette));
    assert(lastFrame==amigaSplashFrame(cache.data(),2) && !lastDither);
    assert(publisher.presentation()->data()==words[0] && publisher.pixels()==lastFrame);
    Bitmap wide(credits.data(),380,284,2,false,false,384);
    assert(credit.prepare(wide,blank,palette));
    assert(wideCalls==1 && credit.pixels()==credits.data() && credit.presentation()->data()==words[1]);
    assert(controls.prepare(cache.data(),cache.size(),blank,palette));
    assert(lastFrame==amigaSplashFrame(cache.data(),4) && controls.pixels()==lastFrame);
    const unsigned heldCalls=narrowCalls;
    for(unsigned frame=0;frame<240;++frame) {
        assert(controls.presentation(4,11,frame%2,blank,palette,alternating)->data()==words[2]);
        assert(!alternating);
    }
    assert(narrowCalls==heldCalls); // hold cannot rebuild or select a stale fade
    for(int stage=5;stage<=6;++stage) for(unsigned frame=0;frame<12;++frame) {
        const unsigned slot=frame%2;
        const auto* list=controls.presentation(stage,frame,slot,blank,palette,alternating);
        assert(list && alternating && list->data()==words[3+slot]);
        assert(lastFrame==amigaSplashFrame(cache.data(),stage));
        assert(lastDither==amigaSplashDitherTile(cache.data(),frame));
    }
    assert(!controls.presentation(5,0,2,blank,palette,alternating) && !alternating);
    assert(!controls.presentation(3,0,0,blank,palette,alternating));
    buildOK=false;
    assert(!controls.presentation(5,0,0,blank,palette,alternating) && !alternating);
    assert(!publisher.prepare(cache.data(),cache.size(),blank,palette) && !publisher.presentation());
    assert(!credit.prepare(wide,blank,palette) && !credit.presentation());
    assert(!controls.prepare(cache.data(),cache.size(),blank,palette) && !controls.pixels());
    assert(!controls.presentation(4,0,0,blank,palette,alternating));
    buildOK=true;
    assert(publisher.prepare(cache.data(),cache.size(),blank,palette));
    assert(credit.prepare(wide,blank,palette));
    assert(controls.prepare(cache.data(),cache.size(),blank,palette));
    std::puts("Publisher/credits/controls preparation, held publication, both fade slots and failure recovery pass");
}

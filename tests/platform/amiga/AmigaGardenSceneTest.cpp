#include "platform/amiga/AmigaGardenScene.h"
#include "platform/amiga/AmigaSceneRuntime.h"
#include "platform/amiga/AmigaMenu.h"
#include "platform/amiga/framework/Sprite.h"
#include <cassert>
#include <cstdio>

// Ownership doubles; the native startup gate checks real Copper programs.
class Palette {};
static unsigned preparations, titles, sprites, builds, patches;
static bool prepareOK=true, titleOK=true, spriteOK=true, copperOK=true;
static UWORD player[8];
static const UBYTE* lastForeground;
CopperList::CopperList(UWORD* words,unsigned capacity)
    : words_(words),capacity_(capacity),count_(0),valid_(words!=nullptr) {}
DownwellMenuCopperList::DownwellMenuCopperList(UWORD* words,unsigned capacity)
    : CopperList(words,capacity) {}
bool DownwellMenuCopperList::buildScroll(const UBYTE*,const UBYTE*,const Sprite&,
    const Sprite&,const Palette&,const AmigaMenuScrollState&,bool) {
    ++builds; return copperOK;
}
bool DownwellMenuCopperList::updateScroll(const UBYTE* foreground,const UBYTE*,
    const UWORD*,const AmigaMenuScrollState&,bool) {
    ++patches; lastForeground=foreground; return copperOK;
}
Sprite::Sprite(UWORD* data,UWORD,bool,bool) : data_(data) {}
Sprite::~Sprite() {}
UWORD* Sprite::data() const { return data_; }
bool amigaRenderMenuCache(const AmigaMenuDrawList&,const UBYTE*,const UBYTE*,
    UBYTE*,UBYTE*,UBYTE*) { ++preparations; return prepareOK; }
bool amigaRenderMenuTitle(UWORD,const UBYTE*,const UBYTE*,UBYTE*,UBYTE*) {
    ++titles; return titleOK;
}
UWORD* amigaMenuHardwareSprite(const UBYTE*,const UBYTE*,UWORD,UWORD,bool,
    unsigned,int,int) { ++sprites; return spriteOK ? player : nullptr; }
int main() {
    UBYTE foreground[8]{}, background[8]{}, entry[8]{}, art[20]{};
    UWORD words[2][400]{}, blankData[8]{};
    Sprite blank(blankData,1); Palette palette;
    AmigaMenuDrawList list{};
    AmigaSceneRuntime runtime;
    assert(runtime.enterResultsDestination(downwell::ResultsDestination::surface));
    AmigaGardenScene scene(foreground,background,entry,words[0],words[1],400);
    AmigaGardenScene::Frame frame;
    assert(!scene.render(0,runtime,false,blank,palette,frame));
    assert(!scene.prepare(list,nullptr,art+20));
    assert(preparations==0);
    assert(scene.prepare(list,art,art+20));
    assert(!scene.render(2,runtime,false,blank,palette,frame));
    for(unsigned i=0;i<240;++i) {
        assert(scene.render(i%2,runtime,false,blank,palette,frame));
        assert(frame.player==player && lastForeground==foreground);
        assert(scene.copper(i%2).data()==words[i%2]);
    }
    assert(preparations==1 && titles==1 && builds==2 && patches==240 && sprites==240);
    assert(scene.render(0,runtime,true,blank,palette,frame));
    assert(lastForeground==entry && builds==2); // entry is a pointer patch
    scene.enter();
    assert(scene.render(0,runtime,true,blank,palette,frame));
    assert(builds==3 && titles==1); // retired Copper slot, same artwork
    assert(scene.prepare(list,art,art+20));
    titleOK=false;
    assert(!scene.render(0,runtime,false,blank,palette,frame) && !frame.player);
    titleOK=true; spriteOK=false;
    assert(!scene.render(0,runtime,false,blank,palette,frame) && !frame.player);
    spriteOK=true; copperOK=false;
    assert(!scene.render(0,runtime,false,blank,palette,frame) && !frame.player);
    const unsigned before=builds;
    copperOK=true;
    assert(scene.render(0,runtime,false,blank,palette,frame));
    assert(builds==before+1); // a failed first build is never considered ready
    prepareOK=false;
    assert(!scene.prepare(list,art,art+20));
    assert(!scene.render(0,runtime,false,blank,palette,frame));
    prepareOK=true;
    assert(scene.prepare(list,art,art+20));
    assert(scene.render(0,runtime,false,blank,palette,frame));
    std::puts("Garden scene retains artwork, patches alternating lists and resets on re-entry");
}

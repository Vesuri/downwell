#include "platform/amiga/AmigaGameplayScene.h"
#include <cassert>
#include <cstdio>
// Lifecycle doubles. Native gates exercise actual Copper/ring behavior.
class Bitmap {};
class Sprite {};
class Palette {};
static unsigned builds, updates, masks;
static bool buildOK=true, updateOK=true, maskOK=true;
static const Bitmap* bound;
static const UBYTE* lastMask;
CopperList::CopperList(UWORD* words,unsigned capacity)
    : words_(words),capacity_(capacity),count_(0),valid_(words!=nullptr) {}
DownwellGameplayCopperList::DownwellGameplayCopperList(UWORD* words,unsigned capacity)
    : CopperList(words,capacity) {}
bool DownwellGameplayCopperList::build(const Bitmap& game,const Bitmap&,const Sprite&,
    const Sprite*,unsigned,const Sprite&,const Palette&,unsigned) {
    ++builds; bound=&game; return buildOK;
}
bool DownwellGameplayCopperList::update(const Bitmap& game,unsigned) {
    ++updates; bound=&game; return updateOK;
}
bool DownwellGameplayCopperList::updateExitMask(const UBYTE* mask) {
    ++masks; lastMask=mask; return maskOK;
}
int main() {
    UWORD words[2][400]{};
    Bitmap backing, display[2], hud;
    Sprite player, actors[2], overflow;
    Palette palette;
    UBYTE mask[8]{};
    AmigaGameplayScene scene(words[0],words[1],400);
    auto update=[&](unsigned slot) {
        return scene.update(slot,display[slot%2],hud,player,actors,2,overflow,palette,17);
    };
    assert(!scene.mask(0,mask) && masks==0);
    assert(!update(2));
    assert(!scene.build(2,backing,hud,player,actors,2,overflow,palette,0));
    for(unsigned slot=0;slot<2;++slot)
        assert(scene.build(slot,backing,hud,player,actors,2,overflow,palette,0));
    scene.invalidate(); // startup backing/borrowed intro slots are no longer valid
    for(unsigned i=0;i<600;++i) {
        const unsigned slot=i%2;
        assert(update(slot) && bound==&display[slot]);
        assert(scene.mask(slot,i<300 ? mask : nullptr));
        assert(lastMask==(i<300 ? mask : nullptr));
        assert(scene.copper(slot).data()==words[slot]);
    }
    assert(builds==4 && updates==598 && masks==600);
    scene.invalidate(0);
    assert(update(1) && builds==4);
    assert(update(0) && builds==5);
    updateOK=false;
    assert(!update(0));
    assert(!scene.mask(0,mask));
    updateOK=true; buildOK=false;
    assert(!update(0));
    buildOK=true;
    unsigned before=builds;
    assert(update(0) && builds==before+1);
    maskOK=false;
    assert(!scene.mask(0,mask));
    before=builds; maskOK=true;
    assert(update(0) && builds==before+1);
    scene.invalidate();
    before=builds;
    assert(update(0) && update(1) && builds==before+2);
    assert(!scene.mask(2,mask));
    std::puts("Gameplay scene: retired-slot rebuilds, 600 patched frames, exit masks and failure recovery");
}

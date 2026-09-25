#include "platform/amiga/AmigaUpgradeScene.h"
#include <cassert>
#include <cstdio>

class Sprite {};
class Palette {};
static unsigned builds, updates;
static bool buildOK = true, updateOK = true;
static const UBYTE *borderSeen, *layerSeen, *maskSeen;
CopperList::CopperList(UWORD* words, unsigned capacity)
    : words_(words), capacity_(capacity), count_(0), valid_(words != nullptr) {}
bool DownwellSelectionCopperList::build(const UBYTE* border, const UBYTE* layer,
    const UBYTE* mask, const Sprite&, const Sprite*, unsigned, const Sprite&, const Palette&) {
    ++builds; borderSeen=border; layerSeen=layer; maskSeen=mask; return buildOK;
}
bool DownwellSelectionCopperList::update(const UBYTE* border, const UBYTE* layer, const UBYTE* mask) {
    ++updates; borderSeen=border; layerSeen=layer; maskSeen=mask; return updateOK;
}
int main() {
    AmigaUpgradeScene scene;
    UWORD words[2][400]{};
    UBYTE border[608]{}, layer[608]{}, mask[608]{};
    Sprite player, actors[2], overflow;
    Palette palette;
    AmigaUpgradeScene::Frame frame{border,layer,0,0};
    auto draw=[&](unsigned slot, const UBYTE* cover) {
        return scene.updateDisplay(slot,frame,cover,player,actors,2,overflow,palette);
    };
    assert(!draw(0,nullptr) && !draw(2,nullptr) && builds==0 && updates==0);
    scene.bindDisplay(words[0],words[1],400);
    for (unsigned entry=0;entry<2;++entry) {
        scene.invalidateDisplay();
        for (unsigned tick=0;tick<300;++tick) {
            frame.border=border+tick; frame.layer=layer+tick;
            const UBYTE* cover=tick<100 ? mask+tick : nullptr;
            assert(draw(tick%2,cover));
            assert(borderSeen==frame.border && layerSeen==frame.layer && maskSeen==cover);
            assert(scene.copper(tick%2).data()==words[tick%2]);
        }
    }
    assert(builds==4 && updates==596);
    updateOK=false;
    assert(!draw(0,mask));
    updateOK=true; buildOK=false;
    assert(!draw(0,mask));
    buildOK=true;
    const unsigned before=builds;
    assert(draw(0,mask) && builds==before+1);
    scene.bindDisplay(nullptr,words[0],400);
    assert(!draw(0,nullptr) && draw(1,nullptr));
    assert(scene.copper(1).data()==words[0]);
    std::puts("Upgrade display: two entries, 600 frames, mask retirement and failure recovery passed");
}

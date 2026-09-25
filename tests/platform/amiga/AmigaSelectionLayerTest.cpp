#include "platform/amiga/AmigaSelectionLayer.h"
#include "platform/amiga/AmigaClearDrawList.h"
#include <vector>
#include <fstream>
#include <iterator>
#include <cassert>
#include <cstdio>
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int) { return 0; }
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD) { return 0; }
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD) { return 0; }
int main(int argc,char** argv) {
    assert(argc==2);
    std::ifstream f(argv[1],std::ios::binary);
    std::vector<UBYTE> pack{std::istreambuf_iterator<char>(f),{}};
    assert(pack.size()>20);
    static_assert(AmigaSelectionLayer::bytes <= downwellLayerBytes,"reuse existing smallest display allocation");
    std::vector<UBYTE> canvas(AmigaSelectionLayer::bytes,0xa5), game(downwellGameRingBytes), expected(downwellLayerBytes);
    AmigaSelectionLayer layer;
    AmigaSelectionLayer::Command commands[] = {{64500,0,80,96,2},{64500,0,79,95,1},{64001,0,80,278,1}};
    unsigned paint=0;
    assert(layer.render(canvas.data(),pack.data(),pack.data()+pack.size(),commands,3,paint) && paint);
    const auto retained = canvas;
    for(unsigned top=0;top<=324;++top) {
        const auto* view=layer.view(top); assert(view);
        assert(layer.render(canvas.data(),pack.data(),pack.data()+pack.size(),commands,3,paint) && !paint);
        assert(canvas==retained);
        std::fill(expected.begin(),expected.end(),0);
        AmigaRenderer renderer;
        amigaRendererInit(&renderer,game.data(),expected.data());
        renderer.target=renderer.hud; renderer.clipX=110; renderer.clipWidth=160;
        for(const auto& c:commands)
            amigaRendererDrawAssetPen(&renderer,pack.data(),pack.data()+pack.size(),c.id,c.frame,110+c.x,c.y-int(top),0,c.pen);
        for(unsigned p=0;p<2;++p) for(unsigned y=0;y<284;++y) for(unsigned x=0;x<160;++x) {
            const bool a=(view[p*AmigaSelectionLayer::planeBytes+y*22+x/8] & (128u>>(x&7)))!=0;
            const bool b=(expected[p*downwellPlaneBytes+y*48+(110+x)/8] & (128u>>((110+x)&7)))!=0;
            assert(a==b);
        }
    }
    assert(!layer.view(325));
    commands[0].x=87;
    assert(layer.render(canvas.data(),pack.data(),pack.data()+pack.size(),commands,3,paint) && paint);
    std::vector<UBYTE> fresh(canvas.size());
    AmigaSelectionLayer oracle;
    assert(oracle.render(fresh.data(),pack.data(),pack.data()+pack.size(),commands,3,paint) && fresh==canvas);
    const auto before=canvas;
    commands[0].id=65535;
    assert(!layer.render(canvas.data(),pack.data(),pack.data()+pack.size(),commands,3,paint) && canvas==before);
    commands[0].id=64500;
    for(unsigned tick=0;tick<100;++tick) {
        commands[0].x=70+int(tick%16); commands[1].y=90+int(tick%9);
        assert(layer.render(canvas.data(),pack.data(),pack.data()+pack.size(),commands,3,paint));
        oracle.invalidate();
        assert(oracle.render(fresh.data(),pack.data(),pack.data()+pack.size(),commands,3,paint) && fresh==canvas);
    }
    assert(layer.render(canvas.data(),pack.data(),pack.data()+pack.size(),nullptr,0,paint));
    for(auto value:canvas) assert(value==0);
    std::puts("325 selection camera positions match fresh rendering without repainting");
}

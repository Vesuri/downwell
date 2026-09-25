#include "platform/amiga/AmigaSelectionLayer.h"
#include "platform/amiga/AmigaClearDrawList.h"
#include "platform/amiga/AmigaClearBank.h"
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
    // Retain the complete source border loops in world space, including the
    // overlapping old/new-area boundary. Scrolling must only change pointers.
    unsigned borderFrames = 0;
    for (int area = 1; area <= 3; ++area) {
        downwell::LevelClearPresentation presentation;
        layer.invalidate();
        for (unsigned tick = 0; tick < 120; ++tick) {
            const auto snapshot = presentation.advance(area, 3, 3, true);
            AmigaSelectionLayer::Command borders[AmigaSelectionLayer::capacity];
            unsigned count = 0;
            snapshot.borders([&](int a, int image, int x, int y) {
                assert(count < AmigaSelectionLayer::capacity);
                borders[count++] = {downwell::levelBorderSprite(a), unsigned(image), x, y-snapshot.tileY, -1};
            });
            assert(layer.render(canvas.data(),pack.data(),pack.data()+pack.size(),borders,count,paint));
            const unsigned top = 198 + tick % 100;
            const auto* view = layer.view(top - snapshot.tileY);
            assert(view);
            std::fill(expected.begin(),expected.end(),0);
            AmigaRenderer renderer;
            amigaRendererInit(&renderer,game.data(),expected.data());
            renderer.target=renderer.hud; renderer.clipX=110; renderer.clipWidth=160;
            snapshot.borders([&](int a, int image, int x, int y) {
                amigaRendererDrawAsset(&renderer,pack.data(),pack.data()+pack.size(),downwell::levelBorderSprite(a),image,110+x,y-int(top),0);
            });
            for(unsigned p=0;p<2;++p) for(unsigned y=0;y<284;++y) for(unsigned x=0;x<160;++x) {
                const bool actual = view[p*AmigaSelectionLayer::planeBytes+y*22+x/8] & (128u>>(x&7));
                const bool reference = expected[p*downwellPlaneBytes+y*48+(110+x)/8] & (128u>>((110+x)&7));
                assert(actual==reference);
            }
            assert(layer.render(canvas.data(),pack.data(),pack.data()+pack.size(),borders,count,paint) && !paint);
            ++borderFrames;
        }
    }
    std::printf("%u retained border frames match source-order fresh rendering\n",borderFrames);
    // Real offer-bank replacements move directory/payload offsets but never
    // change a stage's border geometry or pixels. Rebinding must retain the
    // canvas, while a later seam change must read the NEW directory correctly.
    std::vector<UBYTE> bank(64000);
    unsigned replacements = 0;
    for (unsigned area = 1; area <= 4; ++area) {
        downwell::LevelClearPresentation::Frame snapshot;
        snapshot.area = area; snapshot.borderRow = 18;
        AmigaSelectionLayer::Command borders[AmigaSelectionLayer::capacity];
        unsigned count = 0;
        snapshot.borders([&](int a, int image, int x, int y) {
            borders[count++] = {downwell::levelBorderSprite(a), unsigned(image), x, y, -1};
        });
        unsigned bytes = AmigaClearBank::build(pack.data(),pack.size(),nullptr,0,bank.data(),bank.size(),area,1);
        assert(bytes);
        layer.invalidate();
        assert(layer.render(canvas.data(),bank.data(),bank.data()+bytes,borders,count,paint));
        const auto preserved = canvas;
        for (unsigned sample = 0; sample < 32; ++sample) {
            unsigned offers[] = {sample%23,(sample+6)%23,(sample+13)%23,100};
            unsigned char owned[23];
            for (auto& item : owned) item = sample%2;
            bytes = AmigaClearBank::build(pack.data(),pack.size(),offers,sample%5,bank.data(),bank.size(),area,1,owned);
            assert(bytes);
            layer.rebindEquivalentArtwork(bank.data(),bank.data()+bytes);
            assert(layer.render(canvas.data(),bank.data(),bank.data()+bytes,borders,count,paint) && !paint);
            oracle.invalidate();
            assert(oracle.render(fresh.data(),bank.data(),bank.data()+bytes,borders,count,paint));
            assert(canvas==preserved && canvas==fresh);
            ++replacements;
        }
        borders[0].y += 3;
        assert(layer.render(canvas.data(),bank.data(),bank.data()+bytes,borders,count,paint) && paint);
        oracle.invalidate();
        assert(oracle.render(fresh.data(),bank.data(),bank.data()+bytes,borders,count,paint) && canvas==fresh);
    }
    std::printf("%u offer-bank replacements retain identical borders without repainting\n",replacements);
    std::puts("325 selection camera positions match fresh rendering without repainting");
}

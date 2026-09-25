#include "platform/amiga/AmigaStyleDrawList.h"
#include "platform/amiga/AmigaRenderer.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int) { return 0; }
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD) { return 0; }
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD) { return 0; }
int main(int argc,char** argv) {
    assert(argc==2);
    std::ifstream file(argv[1],std::ios::binary);
    std::vector<UBYTE> pack{std::istreambuf_iterator<char>(file),{}};
    assert(pack.size()>20);
    AmigaSceneRuntime runtime;
    assert(runtime.enterResultsDestination(downwell::ResultsDestination::styleSelect));
    AmigaSelectionLayer layers[2];
    AmigaStyleDrawList last[2],list;
    bool initialized[2]={false,false};
    std::vector<UBYTE> canvases[2];
    for(auto& c:canvases) c.resize(AmigaSelectionLayer::bytes+32,0xa5);
    std::vector<UBYTE> game(downwellGameRingBytes), expected(downwellLayerBytes);
    unsigned frames=0,retained=0,cursors=0;
    for(unsigned tick=0;tick<400 && runtime.scene()==AmigaScene::styleSelect;++tick) {
        downwell::Input input;
        input.right=tick>=80 && tick<=140 && tick%20==0;
        input.left=tick>=180 && tick<=240 && tick%20==0;
        input.fire=tick==300;
        runtime.step(input);
        if(runtime.scene()!=AmigaScene::styleSelect) break;
        cursors|=1u<<runtime.styleCursor();
        const unsigned slot=tick%2;
        const int top=AmigaStyleDrawList::baseView-runtime.styleYOffset();
        assert(top>=0 && top<=324 && list.build(runtime));
        const auto before=canvases[slot];
        unsigned paint=0;
        assert(layers[slot].render(canvases[slot].data()+16,pack.data(),pack.data()+pack.size(),list.commands,list.count,paint));
        bool unchanged=initialized[slot] && last[slot].count==list.count;
        for(unsigned i=0;unchanged && i<list.count;++i) unchanged=last[slot].commands[i]==list.commands[i];
        if(unchanged) { assert(!paint && canvases[slot]==before); ++retained; }
        last[slot]=list; initialized[slot]=true;
        for(unsigned i=0;i<16;++i) assert(canvases[slot][i]==0xa5 && canvases[slot][canvases[slot].size()-1-i]==0xa5);
        const auto* view=layers[slot].view(top); assert(view);
        std::fill(expected.begin(),expected.end(),0);
        AmigaRenderer renderer;
        amigaRendererInit(&renderer,game.data(),expected.data());
        renderer.target=renderer.hud; renderer.clipX=110; renderer.clipWidth=160;
        const auto draw=[&](unsigned id,unsigned frame,int x,int y,int pen=-1) {
            if(pen<0) amigaRendererDrawAsset(&renderer,pack.data(),pack.data()+pack.size(),id,frame,x+110,y,0);
            else amigaRendererDrawAssetPen(&renderer,pack.data(),pack.data()+pack.size(),id,frame,x+110,y,0,pen);
        };
        // Original full-redraw style layout, using uncropped resource IDs and
        // individual labels rather than the optimized precomposed panel.
        const int center=142+runtime.styleYOffset();
        if(runtime.styleSelected()) draw(27,runtime.styleRunFrame(),80,center-14+runtime.stylePlayerDescent());
        else {
            for(int style=0;style<5;++style) {
                const int x=80+style*48-runtime.styleSmoothX(),distance=x-80;
                if(x<=-16 || x>=176) continue;
                draw(style==0?27:43,style==0 && style==runtime.styleCursor()?runtime.styleRunFrame():0,
                    x,center-14-distance*distance/200);
            }
            if(runtime.styleCursor()<4) draw(44,0,96,center-10);
            if(runtime.styleCursor()>0) draw(65401,0,64,center-10);
            draw(65000,0,80,center-70);
            draw(runtime.styleCursor()==0?65001:65003,0,80,center+10);
            if(runtime.styleCursor()==0) draw(65002,0,80,center+32,1);
        }
        for(unsigned p=0;p<2;++p) for(unsigned y=0;y<284;++y) for(unsigned x=0;x<160;++x) {
            const bool actual=view[p*AmigaSelectionLayer::planeBytes+y*22+x/8] & (128u>>(x&7));
            const bool oracle=expected[p*downwellPlaneBytes+y*48+(110+x)/8] & (128u>>((110+x)&7));
            if(actual!=oracle) { std::fprintf(stderr,"style tick=%u p=%u x=%u y=%u\n",tick,p,x,y); assert(actual==oracle); }
        }
        ++frames;
    }
    assert(cursors==31 && frames>300 && retained>20 && runtime.scene()==AmigaScene::gameplay);
    std::printf("%u style frames match full source-order draws across all five cursors; %u unchanged canvases untouched\n",frames,retained);
}

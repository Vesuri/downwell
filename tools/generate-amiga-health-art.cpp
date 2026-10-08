// Host-only source-order preparation of immutable health-HUD layers.
#include "platform/amiga/AmigaRenderer.h"
#include "core/HudNumbers.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}
static unsigned word(const UBYTE* p){return unsigned(p[0])*256+p[1];}
static void emit(const char* name,const std::vector<UBYTE>& pixels){
    std::printf("static const AmigaHealthPair %s[28][7]={\n",name);
    for(unsigned y=6;y<34;++y){
        std::printf("{");
        for(unsigned w=0;w<7;++w){
            const auto* p=pixels.data()+y*downwellBytesPerPlaneLine+w*2;
            std::printf("{%u,%u},",word(p),word(p+downwellPlaneBytes));
        }
        std::printf("},\n");
    }
    std::printf("};\n");
}
int main(int argc,char** argv){
    assert(argc==2);
    std::ifstream file(argv[1],std::ios::binary);
    std::vector<UBYTE> pack((std::istreambuf_iterator<char>(file)),{});
    assert(!pack.empty());
    std::vector<UBYTE> pixels(downwellLayerBytes);
    AmigaRenderer renderer;
    amigaRendererInit(&renderer,nullptr,pixels.data());
    auto asset=[&](unsigned s,unsigned f,int x,int y){
        int left,top,width,height;
        assert(amigaRendererAssetBounds(pack.data(),pack.data()+pack.size(),s,f,x,y,0,&left,&top,&width,&height));
        assert(left>=8&&top>=6&&left+width<=100&&top+height<=34);
        amigaRendererDrawAsset(&renderer,pack.data(),pack.data()+pack.size(),s,f,x,y,0);
    };
    amigaRendererFillRect(&renderer,8,6,92,28,3);
    asset(415,1,8,6);emit("background",pixels);
    amigaRendererFillRect(&renderer,8,6,92,28,0);
    downwell::drawHeartPieceMeter(8,6,0,4,[](int,int,int,int){},asset);
    asset(415,0,8,6);emit("rim",pixels);
    std::printf("static const AmigaHudGlyph glyphs[11]={\n");
    for(unsigned index=0;index<11;++index){
        AmigaHudGlyph glyph{};
        assert(amigaRendererPrepareHudGlyph(&glyph,pack.data(),pack.data()+pack.size(),65011,index));
        std::printf("{{");
        for(auto row:glyph.low)std::printf("%u,",unsigned(row));
        std::printf("},{");
        for(auto row:glyph.high)std::printf("%u,",unsigned(row));
        std::printf("},%d,%d,%u,%u},\n",glyph.left,glyph.top,glyph.width,glyph.height);
    }
    std::printf("};\n");
}

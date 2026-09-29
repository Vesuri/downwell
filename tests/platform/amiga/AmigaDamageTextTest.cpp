#include "platform/amiga/AmigaDamageText.h"
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
    std::ifstream input(argv[1],std::ios::binary);
    std::vector<UBYTE> pack((std::istreambuf_iterator<char>(input)),{});
    assert(!pack.empty());
    AmigaDamageGlyphs glyphs;
    assert(amigaRendererPrepareDamageGlyphs(&glyphs,pack.data(),pack.data()+pack.size()));
    std::vector<UBYTE> actual(downwellGameRingBytes+16),expected(actual.size());
    unsigned cases=0;
    for(int value:{0,1,2,7,8,9,10,12,99,100,123456789,-17,2147483647,(-2147483647-1)})
    for(int shift=0;shift<16;++shift)
    for(int base:{92,180,265})
    for(int y:{-3,0,40,278})
    for(unsigned origin:{0u,311u,319u}) {
        for(unsigned i=0;i<actual.size();++i) actual[i]=UBYTE(i*37+cases);
        expected=actual;
        AmigaRenderer renderer,oracle;
        amigaRendererInit(&renderer,actual.data()+8,nullptr);
        amigaRendererInit(&oracle,expected.data()+8,nullptr);
        amigaRendererSetGameOrigin(&renderer,origin);
        amigaRendererSetGameOrigin(&oracle,origin);
        amigaRendererSetClipRect(&renderer,110,0,160,284);
        amigaRendererSetClipRect(&oracle,110,0,160,284);
        downwell::DamageNumberText label;label.formatInteger(value);
        AmigaDamageText::draw(renderer,glyphs,label,base+shift,y);
        label.draw(base+shift,y,[&](unsigned glyph,int x,int row,unsigned pen){
            amigaRendererDrawAssetPen(&oracle,pack.data(),pack.data()+pack.size(),glyph,0,x,row,0,pen);
        });
        assert(actual==expected);
        ++cases;
    }
    auto bad=pack;
    bad[0]=0;
    assert(!amigaRendererPrepareDamageGlyphs(&glyphs,bad.data(),bad.data()+bad.size()));
    assert(!amigaRendererPrepareDamageGlyphs(&glyphs,pack.data(),pack.data()+20));
    std::printf("Damage text: %u full-buffer comparisons against five-pass glyph rendering\n",cases);
}

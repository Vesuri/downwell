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
    AmigaDamageTextCache cache;
    assert(cache.bindGlyphs(glyphs));
    for(int value:{0,1,2,3,4,5,6,7,8,9,10,12,99,100,123456789,-17,2147483647,(-2147483647-1)})
    for(int shift=0;shift<16;++shift)
    for(int base:{-40,-1,0,92,180,265,375,400})
    for(int y:{-3,0,40,278})
    for(unsigned origin:{0u,311u,319u})
    for(unsigned target:{0u,1u,2u}) {
        for(unsigned i=0;i<actual.size();++i) actual[i]=UBYTE(i*37+cases);
        expected=actual;
        AmigaRenderer renderer,oracle;
        amigaRendererInit(&renderer,actual.data()+8,nullptr);
        amigaRendererInit(&oracle,expected.data()+8,nullptr);
        amigaRendererSetGameOrigin(&renderer,origin);
        amigaRendererSetGameOrigin(&oracle,origin);
        const int clipX=target?0:110,clipWidth=target?380:160;
        amigaRendererSetClipRect(&renderer,clipX,0,clipWidth,284);
        amigaRendererSetClipRect(&oracle,clipX,0,clipWidth,284);
        if(target==2) {renderer.game=nullptr;oracle.game=nullptr;}
        downwell::DamageNumberText label;label.formatInteger(value);
        cache.draw(renderer,glyphs,value,base+shift,y);
        label.draw(base+shift,y,[&](unsigned glyph,int x,int row,unsigned pen){
            amigaRendererDrawAssetPen(&oracle,pack.data(),pack.data()+pack.size(),glyph,0,x,row,0,pen);
        });
        assert(actual==expected);
        // Replaying an opaque final label is idempotent, on both misses and
        // hits. Invalidating its glyph-bank association must retain pixels.
        if((cases%7)==0)cache.invalidate();
        cache.draw(renderer,glyphs,value,base+shift,y);
        assert(actual==expected);
        assert(cache.bindGlyphs(glyphs));
        ++cases;
    }
    auto changed=glyphs;changed.rows[1][0]^=0x80;
    assert(!cache.bindGlyphs(changed));
    {
        expected=actual;
        AmigaRenderer renderer,oracle;
        amigaRendererInit(&renderer,actual.data()+8,nullptr);
        amigaRendererInit(&oracle,expected.data()+8,nullptr);
        amigaRendererSetClipRect(&renderer,110,0,160,284);
        amigaRendererSetClipRect(&oracle,110,0,160,284);
        downwell::DamageNumberText label;label.formatInteger(1);
        cache.draw(renderer,changed,1,190,40);
        AmigaDamageText::draw(oracle,changed,label,190,40);
        assert(actual==expected);
        assert(cache.bindGlyphs(glyphs));
    }
    auto bad=pack;
    bad[0]=0;
    assert(!amigaRendererPrepareDamageGlyphs(&glyphs,bad.data(),bad.data()+bad.size()));
    assert(!amigaRendererPrepareDamageGlyphs(&glyphs,pack.data(),pack.data()+20));
    std::printf("Damage text: %u full-buffer comparisons against five-pass glyph rendering\n",cases);
}

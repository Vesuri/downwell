#ifndef DOWNWELL_AMIGA_COVERAGE_WORD_VERIFICATION_H
#define DOWNWELL_AMIGA_COVERAGE_WORD_VERIFICATION_H
#include "AmigaRenderer.h"
// Target-side corpus; host tests exhaust clipping and all viewport positions.
// Scratch is ordinary RAM and verification-only, never an extra live bitmap.
inline unsigned verifyAmigaCoverageWords(){
    alignas(2) static UBYTE actual[downwellGameRingBytes],expected[downwellGameRingBytes];
    unsigned cases=0;
    for(unsigned mode=0;mode<5;++mode)for(unsigned phase=0;phase<16;++phase){
        const bool game=mode<3;
        const unsigned origin=mode==1?319:37;
        const int x=mode==0?int(phase)-15:mode==2?373+int(phase):110+int(phase);
        const int y=mode==2||mode==4?283:1;
        const int left=mode==3?117:0,right=mode==3?129:380;
        const UWORD alpha=UWORD(0xef57u^(phase*313u)),low=UWORD(0x5712u+phase*2357u),high=UWORD(0xce71u-phase*723u);
        volatile UBYTE* oracle=expected;
        for(unsigned i=0;i<downwellGameRingBytes;++i)actual[i]=oracle[i]=UBYTE(i*37u+(i>>5));
        AmigaRenderer renderer;
        amigaRendererInit(&renderer,game?actual:nullptr,game?nullptr:actual);
        amigaRendererSetClipRect(&renderer,left,0,right-left,284);
        renderer.target=actual;
        amigaRendererSetGameOrigin(&renderer,origin);
        amigaRendererDrawPlanarWord(&renderer,x,y,alpha,low,high);
        for(unsigned bit=0;bit<16;++bit){
            const int px=x+int(bit);
            if(!(alpha&(0x8000u>>bit))||px<left||px>=right)continue;
            const unsigned row=game?(origin+unsigned(y))%320:unsigned(y);
            for(unsigned plane=0;plane<2;++plane){
                const unsigned at=plane*(game?downwellGameRingPlaneBytes:downwellPlaneBytes)+row*48+unsigned(px)/8;
                const UBYTE mask=UBYTE(128u>>(unsigned(px)&7));
                if((plane?high:low)&(0x8000u>>bit))oracle[at]|=mask;else oracle[at]&=UBYTE(~mask);
            }
        }
        for(unsigned i=0;i<downwellGameRingBytes;++i)if(actual[i]!=oracle[i])return 0;
        ++cases;
    }
    return cases;
}
#endif

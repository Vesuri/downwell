#include "platform/amiga/AmigaPreparedHudText.h"
#include "platform/amiga/AmigaShopMenu.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){return 0;}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){return 0;}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){return 0;}
static std::vector<UBYTE> load(const char* path){
    std::ifstream f(path,std::ios::binary);std::vector<UBYTE> b((std::istreambuf_iterator<char>(f)),{});
    assert(!b.empty());return b;
}
int main(int argc,char** argv){
    AmigaShopMenu overlap;
    overlap.invalidateStreak(-24,-25);
    assert(overlap.hudTop==19&&overlap.hudBottom==44);
    overlap.invalidateStreak(-38,-24);
    assert(overlap.hudTop==6&&overlap.hudBottom==44);
    overlap.reset();overlap.invalidateStreak(-38,-38);
    assert(overlap.hudTop==6&&overlap.hudBottom==30);
    assert(argc==4);auto hud=load(argv[1]),game=load(argv[2]);
    AmigaPreparedHudText text;
    assert(text.bind(hud.data(),hud.data()+hud.size(),game.data(),game.data()+game.size()));
    std::vector<UBYTE> actual(downwellLayerBytes),expected(actual.size());
    AmigaRenderer r,o;amigaRendererInit(&r,nullptr,actual.data());amigaRendererInit(&o,nullptr,expected.data());
    unsigned cases=0;
    auto draw=[&](bool currency,unsigned value,bool reset){
        if(reset){
            for(unsigned i=0;i<actual.size();++i)actual[i]=UBYTE(i*37+cases*11);
            expected=actual;text.invalidate();
        }
        if(currency){
            assert(text.currency(&r,value));AmigaHudText::currencyReference(&o,hud.data(),hud.data()+hud.size(),value);
        }else{
            assert(text.streak(&r,value));AmigaHudText::streakReference(&o,game.data(),game.data()+game.size(),value);
        }
        if(actual!=expected){
            for(unsigned i=0;i<actual.size();++i)if(actual[i]!=expected[i]){
                std::fprintf(stderr,"%s %u reset=%u byte=%u actual=%02x expected=%02x case=%u\n",currency?"currency":"streak",value,reset,i,actual[i],expected[i],cases);break;
            }
            return false;
        }
        ++cases;return true;
    };
    for(bool currency:{true,false}){
        for(unsigned value=0;value<1200;++value)assert(draw(currency,value,true));
        for(unsigned value:{9999u,10000u,12345u,67890u,99999u,100000u,123456u,4294967295u})assert(draw(currency,value,true));
        for(unsigned value=0;value<1200;++value)assert(draw(currency,value,false));
        for(unsigned value=1200;value;--value)assert(draw(currency,value-1,false));
        unsigned rng=43;
        for(unsigned i=0;i<4000;++i){rng=rng*1664525u+1013904223u;assert(draw(currency,i%17?rng:0,false));}
    }
    // Independent owner invalidation, including zero/nonzero icon transitions.
    for(unsigned plane=0;plane<2;++plane)for(unsigned y=44;y<68;++y)
        for(unsigned x=18;x<30;++x){
            const unsigned offset=plane*downwellPlaneBytes+y*downwellBytesPerPlaneLine+x;
            actual[offset]=expected[offset]=UBYTE(offset*19);
        }
    text.invalidateStreak();assert(draw(false,37,false));assert(draw(false,0,false));
    auto moved=hud;assert(text.bind(moved.data(),moved.data()+moved.size(),game.data(),game.data()+game.size()));
    assert(draw(true,12345,false));assert(draw(false,12345,false));
    std::vector<UBYTE> compact(amigaRendererChipPackBytes(game.data(),game.data()+game.size()));
    assert(amigaRendererCopyChipPack(game.data(),game.data()+game.size(),compact.data(),compact.size()));
    assert(text.bind(hud.data(),hud.data()+hud.size(),compact.data(),compact.data()+compact.size()));
    for(unsigned value=0;value<1200;++value){assert(draw(true,value,false));assert(draw(false,value,false));}
    auto bad=hud;bad[0]=0;assert(!text.bind(bad.data(),bad.data()+bad.size(),game.data(),game.data()+game.size()));
    assert(!text.currency(&r,1)&&!text.streak(&r,1));
    assert(text.bind(hud.data(),hud.data()+hud.size(),game.data(),game.data()+game.size()));
    int previous=0;
    for(unsigned frame=0;frame<200;++frame){
        // Includes collection resets, upward motion, fixed rounded positions,
        // expiry, reappearance, and overlapping old/new rectangles.
        const int offset=-24-int((frame%37)<15?(frame%37):14);
        const unsigned value=frame%29?frame*17:0;
        if(offset!=previous){
            assert(text.streak(&r,0,previous));
            AmigaHudText::streakReference(&o,game.data(),game.data()+game.size(),0,previous);
            text.invalidateStreak();
        }
        assert(text.streak(&r,value,offset));
        AmigaHudText::streakReference(&o,game.data(),game.data()+game.size(),value,offset);
        assert(actual==expected);previous=offset;++cases;
    }
    text.release();
    assert(!text.currency(&r,1)&&!text.streak(&r,1));
    assert(text.bind(hud.data(),hud.data()+hud.size(),game.data(),game.data()+game.size()));
    // Production gameplay binds the cavern bank, and its Chip-compacted form.
    game=load(argv[3]);
    for(int pass=0;pass<2;++pass){
        if(pass){
            std::vector<UBYTE> chip(amigaRendererChipPackBytes(game.data(),game.data()+game.size()));
            assert(amigaRendererCopyChipPack(game.data(),game.data()+game.size(),chip.data(),chip.size()));
            game=chip;
        }
        assert(text.bind(hud.data(),hud.data()+hud.size(),game.data(),game.data()+game.size()));
        for(bool currency:{true,false}){
            for(unsigned value=0;value<1200;++value)assert(draw(currency,value,true));
            for(unsigned value=0;value<1200;++value)assert(draw(currency,value,false));
        }
    }
    std::printf("Prepared HUD: %u full-layer comparisons, resets and continuous digit transitions\n",cases);
}

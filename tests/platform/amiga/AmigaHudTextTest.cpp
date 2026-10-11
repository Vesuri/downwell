#include "platform/amiga/AmigaHudText.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int) { return 0; }
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD) { return 0; }
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD) { return 0; }
static std::vector<UBYTE> load(const char* path) {
    std::ifstream input(path,std::ios::binary);
    std::vector<UBYTE> pack((std::istreambuf_iterator<char>(input)),{});
    assert(!pack.empty());
    return pack;
}
int main(int argc,char** argv) {
    assert(argc==3);
    auto hud=load(argv[1]),game=load(argv[2]);
    auto text=std::make_unique<AmigaHudText>();
    std::vector<UBYTE> actual(downwellLayerBytes),expected(actual.size());
    unsigned cases=0;
    auto compare=[&](bool currency,unsigned value){
        for(unsigned i=0;i<actual.size();++i) actual[i]=UBYTE(i*37+cases*11);
        expected=actual;
        AmigaRenderer renderer,oracle;
        amigaRendererInit(&renderer,nullptr,actual.data());
        amigaRendererInit(&oracle,nullptr,expected.data());
        assert(text->bind(hud.data(),hud.data()+hud.size(),game.data(),game.data()+game.size()));
        if(currency){
            assert(text->currency(&renderer,value));
            AmigaHudText::currencyReference(&oracle,hud.data(),hud.data()+hud.size(),value);
        }else{
            assert(text->streak(&renderer,value));
            AmigaHudText::streakReference(&oracle,game.data(),game.data()+game.size(),value);
        }
        if(actual!=expected){std::printf("mismatch %s %u\n",currency?"currency":"streak",value);return false;}
        ++cases;return true;
    };
    for(unsigned value=0;value<1200;++value) assert(compare(true,value));
    for(unsigned value:{9999u,10000u,12345u,67890u,99999u,100000u,123456u,4294967295u}) assert(compare(true,value));
    for(unsigned value=0;value<1200;++value) assert(compare(false,value));
    for(unsigned value:{9999u,10000u,123456u,9876543u,1234567890u,4294967295u}) assert(compare(false,value));
    // A moved pack rebinds; an unusable pack reports failure for the reference fallback.
    auto moved=hud;
    assert(text->bind(moved.data(),moved.data()+moved.size(),game.data(),game.data()+game.size()));
    auto bad=hud;bad[0]=0;
    assert(!text->bind(bad.data(),bad.data()+bad.size(),game.data(),game.data()+game.size()));
    AmigaRenderer renderer;amigaRendererInit(&renderer,nullptr,actual.data());
    assert(!text->currency(&renderer,1)&&!text->streak(&renderer,1));
    assert(text->bind(hud.data(),hud.data()+hud.size(),game.data(),game.data()+game.size()));
    text->release();
    assert(!text->currency(&renderer,1)&&!text->streak(&renderer,1));
    assert(text->bind(hud.data(),hud.data()+hud.size(),game.data(),game.data()+game.size()));
    std::printf("HUD text: %u full-layer comparisons against layered asset drawing\n",cases);
}

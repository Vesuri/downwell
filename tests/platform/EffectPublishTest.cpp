#include "platform/sdl/PlatformSDL.h"
#include "platform/amiga/AmigaRenderer.h"
#include <SDL_image.h>
#include <array>
#include <vector>
#include <filesystem>
#include <cassert>
#include <cstdio>
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int){std::abort();}
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD){std::abort();}
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD){std::abort();}
static downwell::Color color(unsigned p){return p==1?downwell::Color{255,255,255,255}:p==2?downwell::Color{255,0,0,255}:downwell::Color{0,0,0,255};}
int main(){
    downwell::PlatformSDL sdl;assert(sdl.ready());
    unsigned random=84;auto next=[&](){random=random*1664525u+1013904223u;return random;};
    std::string directory;
    for(unsigned i=0;;++i){directory="build/effect-publish-"+std::to_string(i);if(std::filesystem::create_directory(directory))break;}
    for(unsigned origin:{0u,1u,37u,283u,319u}){
        std::vector<UBYTE> game(downwellGameRingBytes,0xaa),hud(downwellLayerBytes,0x55);
        AmigaRenderer native;amigaRendererInit(&native,game.data(),hud.data());
        amigaRendererSetGameOrigin(&native,origin);
        // Independent scalar memory oracle, including undisplayed ring rows.
        auto expected=game;
        std::array<unsigned,380*284> pixels{};
        for(unsigned y=0;y<284;++y)for(unsigned x=0;x<380;++x){
            const unsigned at=((origin+y)%320)*48+x/8,bit=0x80>>(x%8);
            pixels[y*380+x]=((game[at]&bit)?1:0)|((game[at+15360]&bit)?2:0);
        }
        sdl.clear(color(0));
        for(int x=0;x<380;x+=2)sdl.fillRect(x,0,1,284,color(3));
        // Pens zero and three are both black; initialize native to black as well.
        for(unsigned trial=0;trial<600;++trial){
            const int left=trial%3?110:0,width=trial%3?160:380;
            const int top=trial%5?0:13,height=trial%5?284:257;
            amigaRendererSetClipRect(&native,left,top,width,height);
            sdl.setClipRect(left,top,width,height);
            const int x=int(next()%414)-17,y=int(next()%300)-8;
            const auto mask=UWORD(next()>>16);const unsigned pen=trial%4;
            amigaRendererDrawCoverageWord(&native,x,y,mask,pen);
            sdl.drawCoverageWord(x,y,mask,color(pen));
            for(int bit=0;bit<16;++bit){
                const int dx=x+bit;
                if(!(mask&(0x8000>>bit))||dx<left||dx>=left+width||y<top||y>=top+height||dx<0||dx>=380||y<0||y>=284)continue;
                pixels[y*380+dx]=pen;
                const unsigned at=((origin+unsigned(y))%320)*48+unsigned(dx)/8,b=0x80>>(dx%8);
                for(unsigned plane=0;plane<2;++plane){auto& v=expected[at+plane*15360];v=UBYTE((v&~b)|((pen&(1<<plane))?b:0));}
            }
        }
        assert(game==expected && hud==std::vector<UBYTE>(downwellLayerBytes,0x55));
        sdl.clearClipRect();assert(sdl.saveScreenshot(directory));
        std::filesystem::path latest;
        for(const auto& entry:std::filesystem::directory_iterator(directory))if(latest.empty()||entry.path()>latest)latest=entry.path();
        SDL_Surface* png=IMG_Load(latest.c_str());assert(png);
        SDL_Surface* rgba=SDL_ConvertSurfaceFormat(png,SDL_PIXELFORMAT_RGBA32,0);assert(rgba);
        for(unsigned y=0;y<284;++y)for(unsigned x=0;x<380;++x){
            const auto* pixel=static_cast<Uint8*>(rgba->pixels)+y*rgba->pitch+x*4;
            const auto wanted=color(pixels[y*380+x]);
            assert(pixel[0]==wanted.r && pixel[1]==wanted.g && pixel[2]==wanted.b);
        }
        SDL_FreeSurface(rgba);SDL_FreeSurface(png);
    }
    std::puts("Effect publishing: SDL pixels and Amiga masked words agree across 3000 clipped draws and five ring origins");
}

#include "platform/sdl/PlatformSDL.h"
#include "core/SpritePack.h"
#include <SDL_image.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstdio>
int main(){
    std::ifstream file("generated/downwell-sprites.dwsp",std::ios::binary);
    std::vector<std::uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
    downwell::SpritePack pack(bytes.data(),bytes.size());assert(pack.valid());
    downwell::PlatformSDL platform;assert(platform.ready());
    const std::string pages="extracted/utmt-0.9.2.0/TexturePages";
    assert(platform.loadSpriteAssets(pack,pages));
    platform.clear({255,0,0,255});
    std::vector<unsigned char> expected(380*284*3,0);
    for(unsigned i=0;i<380*284;++i)expected[i*3]=255;
    downwell::PackedSprite sprite;assert(pack.sprite(96,sprite));
    for(unsigned frame=0;frame<3;++frame){
        const int anchor=80+frame*80;
        platform.drawSprite(96,frame,anchor,100);
        downwell::PackedTexturePageItem item;assert(pack.spriteFrame(sprite,frame,item));
        SDL_Surface* source=IMG_Load((pages+"/"+std::to_string(item.texturePageId)+".png").c_str());assert(source);
        SDL_Surface* rgba=SDL_ConvertSurfaceFormat(source,SDL_PIXELFORMAT_RGBA32,0);assert(rgba);
        assert(item.targetWidth==item.sourceWidth && item.targetHeight==item.sourceHeight);
        for(int y=0;y<item.sourceHeight;++y)for(int x=0;x<item.sourceWidth;++x){
            const auto* pixel=static_cast<Uint8*>(rgba->pixels)+(item.sourceY+y)*rgba->pitch+(item.sourceX+x)*4;
            if(!pixel[3])continue;
            // Evaluate source shader classification independently of loader.
            const double green=pixel[1]/255.0;
            const int red=pixel[0]>=128,blue=pixel[2]>=128;
            const bool light=green>=0.8,dark=1-red-green-blue>=0.5;
            assert(pixel[3]==255 && light!=dark);
            const int dx=anchor+item.targetX-sprite.originX+x,dy=100+item.targetY-sprite.originY+y;
            const unsigned offset=(dy*380+dx)*3;
            expected[offset]=expected[offset+1]=expected[offset+2]=light?255:0;
        }
        SDL_FreeSurface(rgba);SDL_FreeSurface(source);
    }
    std::string directory;
    for(unsigned i=0;;++i){directory="build/flash-raster-"+std::to_string(i);if(std::filesystem::create_directory(directory))break;}
    assert(platform.saveScreenshot(directory));
    unsigned captures=0;
    for(const auto& entry:std::filesystem::directory_iterator(directory)){
        SDL_Surface* png=IMG_Load(entry.path().c_str());assert(png && png->w==380 && png->h==284);
        SDL_Surface* rgba=SDL_ConvertSurfaceFormat(png,SDL_PIXELFORMAT_RGBA32,0);assert(rgba);
        for(int y=0;y<284;++y)for(int x=0;x<380;++x){
            const auto* pixel=static_cast<Uint8*>(rgba->pixels)+y*rgba->pitch+x*4;
            for(unsigned c=0;c<3;++c)assert(pixel[c]==expected[(y*380+x)*3+c]);
        }
        ++captures;SDL_FreeSurface(rgba);SDL_FreeSurface(png);
    }
    assert(captures==1);
    std::puts("SDL flash raster: all three source-classified frames and opaque-dark occlusion match");
}

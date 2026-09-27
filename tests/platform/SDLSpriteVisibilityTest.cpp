#include "platform/sdl/SpriteVisibility.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
#include <initializer_list>
int main(){
    using downwell::sdlSpriteOutside;
    unsigned rejected=0;
    for(int y=-320;y<600;y+=3)for(int x=-420;x<800;x+=3)
        for(float scale:{0.5F,1.0F,2.0F}){
            const float w=16*scale,h=32*scale;
            const bool outside=sdlSpriteOutside(x,y,w,h,0,380,284);
            // Any overlap with the framebuffer must survive, including edge
            // pixels and fractional-size/position rasterization slack.
            if(x+w>=-1 && y+h>=-1 && x<=381 && y<=285)assert(!outside);
            if(outside)++rejected;
            assert(!sdlSpriteOutside(x,y,w,h,90,380,284));
        }
    assert(rejected>100000);
    assert(!sdlSpriteOutside(-15.5F,20,16,16,0,380,284));
    assert(!sdlSpriteOutside(379.5F,20,16,16,0,380,284));
    assert(!sdlSpriteOutside(20,-31.5F,16,32,0,380,284));
    assert(!sdlSpriteOutside(20,283.5F,16,32,0,380,284));
    assert(!sdlSpriteOutside(std::numeric_limits<float>::quiet_NaN(),20,16,16,0,380,284));
    std::puts("SDL sprite rejection: conservative framebuffer edges and rotated fallback pass");
}

#ifndef DOWNWELL_AMIGA_WALL_BACKGROUND_H
#define DOWNWELL_AMIGA_WALL_BACKGROUND_H
#include "AmigaRenderer.h"
#include "core/WallBackgroundTiles.h"
template<unsigned Capacity>
bool amigaPaintWallBackground(AmigaRenderer* renderer,
    const downwell::WallBackgroundTiles<Capacity>& tiles,int viewX,int viewY,
    int clipX,int clipY,int clipWidth,int clipHeight){
    return tiles.paintRegion(downwell::MotionBinary64::fromInteger(viewX-downwellPlayfieldX),
        downwell::MotionBinary64::fromInteger(viewY),clipX,clipY,clipX+clipWidth,clipY+clipHeight,
        [](downwell::MotionBinary64 n,std::int32_t& out){return n.roundPixel(out);},
        [&](int x,int y,int w,int h){amigaRendererFillRect(renderer,x,y,w,h,0);return true;});
}
#endif

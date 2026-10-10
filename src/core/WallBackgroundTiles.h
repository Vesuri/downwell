#ifndef DOWNWELL_CORE_WALL_BACKGROUND_TILES_H
#define DOWNWELL_CORE_WALL_BACKGROUND_TILES_H
#include "MotionBinary64.h"
#include "RunnerRealComparison.h"
namespace downwell {
// Retained output of wall code 986, independent of the retiring instance.
// This is not general layer_tile_create ownership. All entries use resource
// 739, source (48,48,16,16), depth 100 and default transform/tint. The original
// patch is entirely opaque black (verify-wall-background.py).
template<unsigned Capacity> class WallBackgroundTiles {
    static_assert(Capacity>0,"Wall tiles need bounded retained storage");
public:
    struct Tile {MotionBinary64 x{},y{};};
    static constexpr int depth=100,width=16,height=16;
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    const Tile* at(unsigned i)const{return i<count_?&tiles_[i]:nullptr;}
    bool append(MotionBinary64 x,MotionBinary64 y){
        if(!good_)return false;
        if(count_==Capacity||!x.valid()||!y.valid())return good_=false;
        tiles_[count_++]={x,y};return true;
    }
    void clear(){count_=0;good_=true;}
    // Boolean projection of gml_Script_tile_layer_find for this store's
    // unscaled 16x16 depth-100 tiles. Not a general layer/element selector:
    // callers needing the returned element ID must use ordered layer ownership.
    // Source comparisons use the runner epsilon, not rasterized coordinates.
    bool contains(MotionBinary64 x,MotionBinary64 y,bool& hit)const{
        if(!good_||!x.valid()||!y.valid())return false;
        for(unsigned i=0;i<count_;++i){
            const auto& tile=tiles_[i];int order;
            if(!compareRunnerReal(x,tile.x,order))return false;
            if(order<0)continue;
            if(!compareRunnerReal(y,tile.y,order))return false;
            if(order<0)continue;
            auto right=tile.x,bottom=tile.y;
            if(!right.add(MotionBinary64::fromInteger(width))||
                !compareRunnerReal(x,right,order))return false;
            if(order>=0)continue;
            if(!bottom.add(MotionBinary64::fromInteger(height))||
                !compareRunnerReal(y,bottom,order))return false;
            if(order>=0)continue;
            hit=true;return true;
        }
        hit=false;return true;
    }
    // Paint only an explicitly requested screen-space rectangle. The Amiga
    // caller can request a newly exposed strip or damaged retained-cache area;
    // this API does not require whole-frame repainting. Depth scheduling and
    // the source raster policy belong to the caller, not this tile store.
    template<class Raster,class Paint>
    bool paintRegion(MotionBinary64 viewX,MotionBinary64 viewY,
        int left,int top,int right,int bottom,Raster raster,Paint paint)const{
        if(!good_||!viewX.valid()||!viewY.valid()||left>right||top>bottom)return false;
        if(left==right||top==bottom)return true;
        viewX.high^=0x80000000u;viewY.high^=0x80000000u;
        for(unsigned i=0;i<count_;++i){
            MotionBinary64 dx=tiles_[i].x,dy=tiles_[i].y;std::int32_t x,y;
            if(!dx.add(viewX)||!dy.add(viewY)||!raster(dx,x)||!raster(dy,y))return false;
            // Avoid signed overflow without changing a far-offscreen tile
            // into a visible rectangle. Coordinates are signed 32-bit pixels.
            if(x>=right||y>=bottom)continue;
            const int x2=x>2147483647-width?right:x+width;
            const int y2=y>2147483647-height?bottom:y+height;
            const int l=x>left?x:left,t=y>top?y:top;
            const int r=x2<right?x2:right,b=y2<bottom?y2:bottom;
            if(l<r&&t<b&&!paint(l,t,r-l,b-t))return false;
        }
        return true;
    }
    template<class Digest> void appendState(Digest& d)const{
        d.addBool(good_);d.addU32(count_);
        for(unsigned i=0;i<count_;++i){d.addU32(tiles_[i].x.high);d.addU32(tiles_[i].x.low);
            d.addU32(tiles_[i].y.high);d.addU32(tiles_[i].y.low);}
    }
private:
    Tile tiles_[Capacity]{};unsigned count_=0;bool good_=true;
};
}
#endif

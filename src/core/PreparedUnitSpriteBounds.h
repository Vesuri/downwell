#ifndef DOWNWELL_PREPARED_UNIT_SPRITE_BOUNDS_H
#define DOWNWELL_PREPARED_UNIT_SPRITE_BOUNDS_H
#include "RunnerSpriteBounds.h"
#include "SpriteCollisionPack.h"
namespace downwell {
// Compatibility bounds for +1/+1 scale, zero angle and integer positions.
// Only valid for the runner's integer-preserving roundPixel policy; generic
// callbacks and other transforms must retain RunnerSpriteBounds's reference.
struct PreparedUnitSpriteBounds {
    std::int32_t left=0,right=0,top=0,bottom=0;
    bool available=false;
    template<class Resource> void prepare(const Resource& r){
        available=small(r.marginLeft)&&small(r.marginRight)&&small(r.marginTop)&&
            small(r.marginBottom)&&small(r.originX)&&small(r.originY);
        if(!available)return;
        axis(r.marginLeft,r.marginRight,r.originX,left,right);
        axis(r.marginTop,r.marginBottom,r.originY,top,bottom);
    }
    bool at(RunnerImageValue x,RunnerImageValue y,RunnerSpatialSplit::Box& out)const{
        std::int32_t px,py;
        if(!available||!position(x,px)||!position(y,py))return false;
        // All intermediates stay inside +/-2^22 and are exact binary32
        // integers. Translation commutes with sorting and far-edge decrement.
        RunnerSpatialSplit::Box next;
        if(!next.left.assignInteger(px+left)||!next.right.assignInteger(px+right)||
           !next.top.assignInteger(py+top)||!next.bottom.assignInteger(py+bottom))return false;
        out=next;return true;
    }
private:
    static bool small(std::int32_t v){return v>=-32768&&v<=32767;}
    static void axis(int low,int high,int origin,std::int32_t& near,std::int32_t& far){
        const auto a=low-origin,b=high-origin+1;
        near=a<b?a:b;far=(a>b?a:b)-1;
    }
    static bool position(RunnerImageValue value,std::int32_t& out){
        const auto magnitude=value.bits&0x7fffffffu;
        if(!magnitude){out=0;return true;}
        const unsigned exponent=magnitude>>23;
        if(exponent<127||exponent>147)return false;
        const unsigned shift=150-exponent,mantissa=(magnitude&0x7fffffu)|0x800000u;
        if(mantissa&((1u<<shift)-1))return false;
        const auto n=std::int32_t(mantissa>>shift);
        out=value.bits>>31?-n:n;return true;
    }
};
template<unsigned Capacity> class PreparedSpriteGeometry {
    PreparedUnitSpriteBounds entries_[Capacity?Capacity:1];
    unsigned count_=0;
public:
    bool prepare(const SpriteCollisionPack& sprites){
        count_=0;
        if(!sprites.valid()||sprites.spriteCount()>Capacity)return false;
        for(unsigned id=0;id<sprites.spriteCount();++id){
            PackedSpriteCollision r;if(!sprites.sprite(id,r))return false;
            entries_[id].prepare(r);
        }
        count_=sprites.spriteCount();return true;
    }
    bool at(int id,RunnerImageValue x,RunnerImageValue y,RunnerSpatialSplit::Box& out)const{
        return id>=0&&unsigned(id)<count_&&entries_[id].at(x,y,out);
    }
};
}
#endif

#ifndef DOWNWELL_RUNNER_SPRITE_POINT_H
#define DOWNWELL_RUNNER_SPRITE_POINT_H
#include "RunnerSpatialSplit.h"
#include "RunnerMaskPair.h"
#include "RunnerMaskPoint.h"
#include "SpriteCollisionPack.h"
namespace downwell {
// Ordinary geometric tail of 0x465520. The owner performs lazy refresh before
// calling, even for destroyed candidates. Outer queries own family/active gates.
struct RunnerSpritePoint {
    template<class State,class Sprite,class Trig,class Round>
    static bool test(const State& state,const Sprite& sprite,RunnerImageValue x,RunnerImageValue y,
        bool compatibility,bool precise,const SpriteCollisionPack& sprites,
        const SpriteMaskPack& masks,Trig inverseTrig,Round round,bool& hit){
        using Math=RunnerSpatialSplit;using Real=RunnerImageValue;
        if(!Math::valid(x)||!Math::valid(y))return false;
        const auto& b=state.bounds;
        const Real extension=compatibility?Real{0x3f800000}:Real{0xb727c5ac};
        Real right,bottom;
        if(!Math::valid(b.left)||!Math::valid(b.top)||!Math::valid(b.right)||!Math::valid(b.bottom)||
           !Math::sub(b.right,Math::negate(extension),right)||
           !Math::sub(b.bottom,Math::negate(extension),bottom))return false;
        if(!Math::less(x,right)||Math::less(x,b.left)||!Math::less(y,bottom)||Math::less(y,b.top)||
           (state.flags&0x100001u)){hit=false;return true;}
        if(!sprites.valid())return false;
        PackedSpriteCollision resource;
        if(!sprites.collisionSprite(sprite.spriteId,sprite.maskId,resource)||!resource.frameCount){hit=false;return true;}
        if(!precise||!resource.collisionMode){hit=true;return true;}
        RunnerMaskPoint::Shape shape;std::int32_t px,py;
        // Original fistp order: query Y/X, then instance Y/X.
        if(!round(y,py)||!round(x,px)||!round(state.y,shape.y)||!round(state.x,shape.x))return false;
        shape.originX=resource.originX;shape.originY=resource.originY;
        shape.scaleX=sprite.scaleX;shape.scaleY=sprite.scaleY;shape.angle=sprite.angle;
        if(!resource.collisionMaskCount){hit=true;return true;}
        unsigned frame;SpriteMaskPack::Mask mask;
        if(!masks.valid()||!RunnerMaskPair::storedFrame(sprite.imageIndex,resource.collisionMaskCount,frame)||
           !masks.mask(unsigned(sprite.maskId<0?sprite.spriteId:sprite.maskId),frame,mask))return false;
        bool result;if(!RunnerMaskPoint::contains(shape,mask,px,py,inverseTrig,result))return false;
        hit=result;return true;
    }
};
}
#endif

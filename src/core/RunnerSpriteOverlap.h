#ifndef DOWNWELL_RUNNER_SPRITE_OVERLAP_H
#define DOWNWELL_RUNNER_SPRITE_OVERLAP_H
#include "RunnerSpatialSplit.h"
#include "SpriteCollisionPack.h"
#include "RunnerMaskPair.h"
namespace downwell {
// Geometric tail of 0x464d30. Caller preserves exclusion checks and OTHER-first
// lazy refresh before supplying current cached bounds and sprite fields.
struct RunnerSpriteOverlap {
    template<class State,class Sprite,class Trig,class Round>
    static bool test(const State& first,const State& second,const Sprite& a,const Sprite& b,
        const SpriteCollisionPack& sprites,const SpriteMaskPack& masks,
        const RunnerMaskDispatch& dispatch,Trig inverseTrig,Round round,bool& hit){
        using Math=RunnerSpatialSplit;using Real=RunnerImageValue;
        // Compatibility mode adds binary32 1 to each inclusive far edge and
        // uses a strict comparison. Do not replace this with integer overlap.
        const Real near[]={second.bounds.left,first.bounds.left,second.bounds.top,first.bounds.top};
        const Real far[]={first.bounds.right,second.bounds.right,first.bounds.bottom,second.bounds.bottom};
        for(unsigned i=0;i<4;++i){
            Real upper;
            if(!Math::valid(near[i])||!Math::valid(far[i])||
                !Math::sub(far[i],Real{0xbf800000},upper))return false;
            if(!Math::less(near[i],upper)){hit=false;return true;}
        }
        if(!sprites.valid())return false;
        PackedSpriteCollision resources[2];
        if(!sprites.collisionSprite(a.spriteId,a.maskId,resources[0])||!resources[0].frameCount||
           !sprites.collisionSprite(b.spriteId,b.maskId,resources[1])||!resources[1].frameCount){hit=false;return true;}
        if(!resources[0].collisionMode&&!resources[1].collisionMode){hit=true;return true;}
        if(!masks.valid())return false;
        const Sprite* visuals[]={&a,&b};const State* states[]={&first,&second};
        RunnerMaskPair::Shape shapes[2];
        for(unsigned i=0;i<2;++i){
            const auto& v=*visuals[i];const auto& s=*states[i];const auto& r=resources[i];
            unsigned frame;
            if(!RunnerMaskPair::storedFrame(v.imageIndex,r.collisionMaskCount,frame)||
                !masks.mask(unsigned(v.maskId<0?v.spriteId:v.maskId),frame,shapes[i].mask))return false;
            shapes[i].x=s.x;shapes[i].y=s.y;shapes[i].bounds=s.bounds;
            shapes[i].transform={v.scaleX,v.scaleY,v.angle};
            shapes[i].originX=r.originX;shapes[i].originY=r.originY;shapes[i].frameCount=r.frameCount;
        }
        RunnerMaskPair::Result result;
        if(!RunnerMaskPair::test(shapes[0],shapes[1],dispatch,inverseTrig,round,result))return false;
        hit=result.hit;return true;
    }
};
}
#endif

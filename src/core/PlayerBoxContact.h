#ifndef DOWNWELL_CORE_PLAYER_BOX_CONTACT_H
#define DOWNWELL_CORE_PLAYER_BOX_CONTACT_H
#include "RunnerSpriteBounds.h"
#include "SpriteCollisionPack.h"
#include "DivideBinary64.h"
namespace downwell {
// scrPlayerInit mask 31 and objBox_n's explicit mask 585 are rectangular.
// These descriptors are checked against the collision pack by the tests.
// The current motion adapter supplies fifths of Q8, relative to a room origin;
// retain those fractions until the absolute query is narrowed by the runner.
struct PlayerBoxContact {
    using Bounds=RunnerSpriteBounds::Box;
    static constexpr PackedSpriteCollision player{12,12,8,8,15,19,0,1,4};
    static constexpr PackedSpriteCollision box{8,8,0,0,15,15,0,1,1};
    static bool test(long x,long y,int originX,int originY,int boxX,int boxY,bool& hit){
        Bounds bounds;
        return prepare(x,y,originX,originY,bounds)&&testPrepared(bounds,boxX,boxY,hit);
    }
    // Reuse the query bounds while visiting boxes; do not repeat software
    // division and float-bound construction for every terrain instance.
    static bool prepare(long x,long y,int originX,int originY,Bounds& result){
        RunnerImageValue px,py;
        if(!coordinate(x,originX,px)||!coordinate(y,originY,py))return false;
        auto round=[](RunnerImageValue n,std::int32_t& pixel){return n.promoted().roundPixel(pixel);};
        const RunnerImageValue one{0x3f800000};
        return RunnerSpriteBounds::fromSprite(player,px,py,one,one,false,one,{},true,round,result);
    }
    static bool testPrepared(const Bounds& a,int boxX,int boxY,bool& hit){
        Bounds b;
        // Integer tile centres in this range and all +/-8 edge intermediates
        // are exactly representable in binary32. Avoid general mask arithmetic
        // for ordinary terrain, retaining it outside the proven range.
        if(boxX>=-8388592&&boxX<=8388592&&boxY>=-8388592&&boxY<=8388592){
            if(!b.left.assign(MotionBinary64::fromInteger(boxX-8))||
               !b.right.assign(MotionBinary64::fromInteger(boxX+7))||
               !b.top.assign(MotionBinary64::fromInteger(boxY-8))||
               !b.bottom.assign(MotionBinary64::fromInteger(boxY+7)))return false;
        }else if(!boxBounds(boxX,boxY,b))return false;
        using Math=RunnerSpatialSplit;
        const RunnerImageValue near[]={b.left,a.left,b.top,a.top};
        const RunnerImageValue far[]={a.right,b.right,a.bottom,b.bottom};
        bool exact=true;
        for(auto edge:far)exact=exact&&((edge.bits&0x7fffffffu)<0x4b000000u);
        if(!exact){
            // Above the exact-integer range, inclusive comparison is not
            // interchangeable with the runner's binary32 far+1 operation.
            for(unsigned i=0;i<4;++i){
                RunnerImageValue upper;
                if(!Math::sub(far[i],RunnerImageValue{0xbf800000},upper))return false;
                if(!Math::less(near[i],upper)){hit=false;return true;}
            }
            hit=true;return true;
        }
        hit=!Math::less(a.right,b.left)&&!Math::less(b.right,a.left)&&
            !Math::less(a.bottom,b.top)&&!Math::less(b.bottom,a.top);
        return true;
    }
private:
    static bool boxBounds(int boxX,int boxY,Bounds& b){
        RunnerImageValue bx,by;
        if(!bx.assign(MotionBinary64::fromInteger(boxX))||
           !by.assign(MotionBinary64::fromInteger(boxY)))return false;
        auto round=[](RunnerImageValue n,std::int32_t& pixel){return n.promoted().roundPixel(pixel);};
        const RunnerImageValue one{0x3f800000};
        return RunnerSpriteBounds::fromSprite(box,bx,by,one,one,false,one,{},true,round,b);
    }
    static bool coordinate(long value,int origin,RunnerImageValue& result){
        if(value<(-2147483647L-1)||value>2147483647L)return false;
        MotionBinary64 n;
        return divideBinary64(MotionBinary64::fromInteger(static_cast<std::int32_t>(value)),
            MotionBinary64::fromInteger(1280),n)&&n.add(MotionBinary64::fromInteger(origin))&&result.assign(n);
    }
};
}
#endif

#ifndef DOWNWELL_CORE_PLAYER_BOX_CONTACT_H
#define DOWNWELL_CORE_PLAYER_BOX_CONTACT_H
#include "RunnerSpriteBounds.h"
#include "SpriteCollisionPack.h"
#include "DivideBinary64.h"
#include "GameMakerRounding.h"
namespace downwell {
// scrPlayerInit mask 31 and objBox_n's explicit mask 585 are rectangular.
// These descriptors are checked against the collision pack by the tests.
// The current motion adapter supplies fifths of Q8, relative to a room origin;
// retain those fractions until the absolute query is narrowed by the runner.
struct PlayerBoxContact {
    using Bounds=RunnerSpriteBounds::Box;
    static constexpr PackedSpriteCollision player{12,12,8,8,15,19,0,1,4};
    static constexpr PackedSpriteCollision box{8,8,0,0,15,15,0,1,1};
    // Adapter for the existing four-corner/indexed terrain backend. Return a
    // relative integer centre only when it reproduces the original 8x12 mask
    // bounds. The backend need not be replaced to stop truncating VM queries.
    static bool terrainCentre(long x,long y,int originX,int originY,int& outX,int& outY){
        if(x<(-2147483647L-1)||x>2147483647L||y<(-2147483647L-1)||y>2147483647L||
           originX< -1000000||originX>1000000||originY< -1000000||originY>1000000)return false;
        // Live upward probes have Q8 X and integer Y. In this range binary32
        // preserves every Q8 bit, including the +/-4 margin operations, so no
        // software float division/bounds construction is needed per probe.
        if(x%5==0 && y%1280==0 && originX>=-16384 && originX<=16384){
            const long absolute=x/5+originX*256L;
            if(absolute>=-8387584L&&absolute<=8387584L){
                outX=static_cast<int>(roundGameMakerQ8(absolute)-originX);
                outY=static_cast<int>(y/1280);return true;
            }
        }
        Bounds b;
        std::int32_t left,right,top,bottom;
        if(!prepare(x,y,originX,originY,b)||!b.left.roundPixel(left)||
           !b.right.roundPixel(right)||!b.top.roundPixel(top)||
           !b.bottom.roundPixel(bottom)||right-left!=7||bottom-top!=11)return false;
        outX=left+4-originX;outY=top+4-originY;return true;
    }
    static bool test(long x,long y,int originX,int originY,int boxX,int boxY,bool& hit){
        Bounds bounds;
        return prepare(x,y,originX,originY,bounds)&&testPrepared(bounds,boxX,boxY,hit);
    }
    // Reuse the query bounds while visiting boxes; do not repeat software
    // division and float-bound construction for every terrain instance.
    static bool prepare(long x,long y,int originX,int originY,Bounds& result){
        RunnerImageValue px,py;
        if(!coordinate(x,originX,px)||!coordinate(y,originY,py))return false;
        auto round=[](RunnerImageValue n,std::int32_t& pixel){return n.roundPixel(pixel);};
        const RunnerImageValue one{0x3f800000};
        return RunnerSpriteBounds::fromSprite(player,px,py,one,one,false,one,{},true,round,result);
    }
    static bool testPrepared(const Bounds& a,int boxX,int boxY,bool& hit){
        Bounds b;
        // Integer tile centres in this range and all +/-8 edge intermediates
        // are exactly representable in binary32. Avoid general mask arithmetic
        // for ordinary terrain, retaining it outside the proven range.
        if(boxX>=-8388592&&boxX<=8388592&&boxY>=-8388592&&boxY<=8388592){
            if(!b.left.assignInteger(boxX-8)||!b.right.assignInteger(boxX+7)||
               !b.top.assignInteger(boxY-8)||!b.bottom.assignInteger(boxY+7))return false;
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
    // Public for the full-range test (verify-player-box-coordinate).
    // float(double(value)/1280 + origin) is the nearest-even binary32 of
    // N/1280, N = value + 1280*origin. A multiple of 5 makes every binary64
    // step exact. Otherwise N/1280 repeats 0011 in binary, so neither
    // binary64 rounding can reach a binary32 midpoint and only the final
    // rounding is visible: with |origin| <= 16384 the binary64 errors (about
    // 2^-52*|origin|) stay below a non-dyadic quotient's distance from any
    // binary32 midpoint (at least 2^(e-24)/5, e >= -11), even when value/1280
    // nearly cancels the origin. Larger origins keep the binary64 form.
    static bool coordinate(long value,int origin,RunnerImageValue& result){
        if(value<(-2147483647L-1)||value>2147483647L)return false;
        if(origin< -16384||origin>16384){
#ifdef DOWNWELL_WORLD_ONE_ONLY
            return false; // world one's origins are 240 and 176
#else
            return coordinateBinary64(value,origin,result);
#endif
        }
        const long long n=static_cast<long long>(value)+static_cast<long long>(origin*1280);
        if(!n){result.bits=0;return true;}
        const std::uint32_t magnitude=static_cast<std::uint32_t>(n<0?-n:n);
        const std::uint32_t whole=magnitude/5,rest=magnitude%5; // N/1280 = (whole+rest/5)/256
        // x = floor(N/1280 * 2^(8+s)) with at least 25 significant bits.
        unsigned s=0;
        std::uint32_t x=whole;bool sticky=rest!=0;
        if(whole<(1u<<24)){
            s=whole?31-(32-leadingZeros32(whole)):29;
            if(s>29)s=29; // rest<<s < 2^32
            const std::uint32_t scaled=rest<<s;
            x=(whole<<s)+scaled/5;sticky=scaled%5!=0;
        }
        unsigned top=31-leadingZeros32(x);
        std::uint32_t mantissa;
        if(top<=23)mantissa=x<<(23-top);
        else{
            const unsigned shift=top-23;
            mantissa=x>>shift;
            const std::uint32_t tail=x&((1u<<shift)-1),half=1u<<(shift-1);
            if(tail>half||(tail==half&&(sticky||(mantissa&1u))))++mantissa;
            if(mantissa==0x1000000u){mantissa>>=1;++top;}
        }
        result.bits=(n<0?0x80000000u:0u)|((127u+top-8u-s)<<23)|(mantissa&0x7fffffu);
        return true;
    }
    // Portable binary64 reference for tests.
    static bool coordinateBinary64(long value,int origin,RunnerImageValue& result){
        if(value<(-2147483647L-1)||value>2147483647L)return false;
        MotionBinary64 n;
        return divideBinary64(MotionBinary64::fromInteger(static_cast<std::int32_t>(value)),
            MotionBinary64::fromInteger(1280),n)&&n.add(MotionBinary64::fromInteger(origin))&&result.assign(n);
    }
private:
    static bool boxBounds(int boxX,int boxY,Bounds& b){
        RunnerImageValue bx,by;
        if(!bx.assign(MotionBinary64::fromInteger(boxX))||
           !by.assign(MotionBinary64::fromInteger(boxY)))return false;
        auto round=[](RunnerImageValue n,std::int32_t& pixel){return n.roundPixel(pixel);};
        const RunnerImageValue one{0x3f800000};
        return RunnerSpriteBounds::fromSprite(box,bx,by,one,one,false,one,{},true,round,b);
    }
};
}
#endif

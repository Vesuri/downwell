#ifndef DOWNWELL_CORE_CURRENCY_COLLISION_BOUNDS_H
#define DOWNWELL_CORE_CURRENCY_COLLISION_BOUNDS_H
#include "EnemyContact.h"
#include "RunnerSpriteBounds.h"
#include "FixedPoint.h"
namespace downwell {
// Unit, unrotated currency masks. Narrow position to binary32 before edge
// arithmetic; rounding the centre first is not the bounds algorithm.
struct CurrencyCollisionBounds {
    static bool calculate(int sprite,MotionBinary64 x,MotionBinary64 y,ContactBounds& out){
        if(sprite!=133&&sprite!=134)return false;
        return calculateMask(sprite,x,y,out);
    }
    // Any verified unrotated, unit-scale rectangle mask (spawner and
    // steering-family place_meeting queries use 153, 174 and 702).
    static bool calculateMask(int sprite,MotionBinary64 x,MotionBinary64 y,ContactBounds& out){
        const auto local=contactSpriteBounds(sprite);
        RunnerImageValue px,py;
        if(local.left<local.right&&local.top<local.bottom&&px.assign(x)&&py.assign(y)){
            ContactBounds next;
            if(pixelAxis(local.left,local.right-1,px,next.left,next.right)&&
               pixelAxis(local.top,local.bottom-1,py,next.top,next.bottom)){out=next;return true;}
        }
        return calculateMaskReference(sprite,x,y,out);
    }
    // The same unit rectangle at a Q16 position (W3 fixed point): each edge
    // is the position plus its margin rounded to a pixel (halves to even),
    // without the binary32 narrowing, whose pixel W3 accepts moving.
    // |x|, |y| < 2^30; false elsewhere.
    static bool calculateMaskQ16(int sprite,std::int32_t x,std::int32_t y,ContactBounds& out){
        const auto local=contactSpriteBounds(sprite);
        constexpr std::int32_t limit=std::int32_t(1)<<30;
        if(local.left>=local.right||local.top>=local.bottom||local.left<-(1<<7)||local.right>(1<<7)||
           local.top<-(1<<7)||local.bottom>(1<<7)||x<=-limit||x>=limit||y<=-limit||y>=limit)return false;
        ContactBounds next;
        // The margins are whole pixels, so each edge rounds as the position
        // does plus its margin; only an exact half (to even) depends on the
        // edge's own parity.
        if((x&0xffff)!=0x8000){
            const int whole=int((x+0x8000)>>16);
            next.left=whole+local.left;next.right=whole+local.right;
        }else{
            next.left=int(Fixed16::roundQ16(x+local.left*65536));
            next.right=int(Fixed16::roundQ16(x+local.right*65536));
        }
        if((y&0xffff)!=0x8000){
            const int whole=int((y+0x8000)>>16);
            next.top=whole+local.top;next.bottom=whole+local.bottom;
        }else{
            next.top=int(Fixed16::roundQ16(y+local.top*65536));
            next.bottom=int(Fixed16::roundQ16(y+local.bottom*65536));
        }
        out=next;return true;
    }
    // unitIntegerAxis's rounded edges for a non-negative position p =
    // s * 2^-k (k 2..23) whose sums with the margins stay in [0, 2^24) at
    // that scale: every binary32 add is then exact, and each edge is the
    // sum rounded to a pixel (halves to even). False (nothing written)
    // elsewhere: calculateMaskReference decides those.
    static bool pixelAxis(std::int32_t low,std::int32_t high,RunnerImageValue p,int& near,int& far){
        const unsigned field=p.bits>>23; // sign bit set: refused below
        if(field<127||field>148||low<-(1<<7)||high>=(1<<7))return false;
        const unsigned k=150-field;
        // 32-bit only (68020: no 64-bit shifts): |low << k| < 2^30, and the
        // width is refused before its shift could pass 2^24.
        const std::uint32_t width=std::uint32_t(high-low+1);
        if(width>(0xffffffu>>k))return false;
        const std::int32_t first=std::int32_t((p.bits&0x7fffffu)|0x800000u)+low*(std::int32_t(1)<<k);
        const std::int32_t second=first+std::int32_t(width<<k);
        if(first<0||second>=(1<<24))return false;
        const auto round=[k](std::int32_t sum){
            std::int32_t whole=sum>>k;
            const std::int32_t rest=sum&((std::int32_t(1)<<k)-1),half=std::int32_t(1)<<(k-1);
            if(rest>half||(rest==half&&(whole&1)))++whole;
            return int(whole);
        };
        near=round(first);far=round(second);
        return true;
    }
    // The binary32 bounds as the runner computes them; calculateMask's oracle.
    static bool calculateMaskReference(int sprite,MotionBinary64 x,MotionBinary64 y,ContactBounds& out){
        RunnerImageValue px,py;RunnerSpriteBounds::Box margins,edges;
        const auto local=contactSpriteBounds(sprite);
        if(local.left>=local.right||local.top>=local.bottom)return false;
        if(!px.assign(x)||!py.assign(y))return false;
        {   // Integer margins at unit scale: the same edges without the
            // binary32 margin conversions, where sumInteger admits them.
            RunnerSpriteBounds::Box fast;ContactBounds next;
            if(RunnerSpriteBounds::unitIntegerAxis(local.left,local.right-1,px,fast.left,fast.right)&&
               RunnerSpriteBounds::unitIntegerAxis(local.top,local.bottom-1,py,fast.top,fast.bottom)){
                std::int32_t l,r,t,b;
                if(!fast.left.roundPixel(l)||!fast.right.roundPixel(r)||
                   !fast.top.roundPixel(t)||!fast.bottom.roundPixel(b))return false;
                next.left=int(l);next.right=int(r);next.top=int(t);next.bottom=int(b);
                out=next;return true;
            }
        }
        if(!margins.left.assignInteger(local.left)||
            !margins.right.assignInteger(local.right-1)||!margins.top.assignInteger(local.top)||
            !margins.bottom.assignInteger(local.bottom-1))return false;
        const RunnerImageValue unit{0x3f800000u};
        auto round=[](RunnerImageValue value,std::int32_t& n){return value.roundPixel(n);};
        if(!RunnerSpriteBounds::unrotated(margins,0,0,px,py,unit,unit,false,round,edges))return false;
        ContactBounds next;
        auto pixel=[](RunnerImageValue value,int& out){
            std::int32_t n;if(!value.roundPixel(n))return false;out=int(n);return true;
        };
        if(!pixel(edges.left,next.left)||!pixel(edges.right,next.right)||
            !pixel(edges.top,next.top)||!pixel(edges.bottom,next.bottom))return false;
        out=next;return true;
    }
};
}
#endif

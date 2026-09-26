#ifndef DOWNWELL_CORE_PLAYER_SOLID_COLLISION_BINARY64_H
#define DOWNWELL_CORE_PLAYER_SOLID_COLLISION_BINARY64_H
#include "MotionBinary64.h"
#include "SolidCollision.h"
namespace downwell {
// scrCheckCollisionWith: checked binary64 version for the player migration.
// Query(x,y,hit) owns runner mask sampling and reports unsupported inputs.
// Failed queries stop immediately; preceding coordinate writes are retained.
// Velocities are not integrated here: scrWallCol responses precede that step.
// Rounded positions and velocity magnitudes must fit signed 32-bit pixels.
// Query callbacks must not mutate these coordinates or velocities.
struct PlayerSolidCollisionBinary64 {
    using N=MotionBinary64;
    static N magnitude(N n){n.high&=0x7fffffffu;return n;}
    static int sign(N n){return n.zero()?0:(n.high>>31)?-1:1;}
    static bool atLeast(N a,N b){
        a=magnitude(a);b=magnitude(b);
        return a.high>b.high || (a.high==b.high && a.low>=b.low);
    }
    static bool rounded(N n,N& out){
        std::int32_t pixel;
        if(!n.roundPixel(pixel))return false;
        out=N::fromInteger(pixel);return true;
    }
    static bool probe(N position,N velocity,N& out){
        N extent;
        if(!rounded(position,out) || !rounded(magnitude(velocity),extent))return false;
        if(!atLeast(extent,velocity) && !extent.add(N::fromInteger(1)))return false;
        if(sign(velocity)<0)extent.high^=0x80000000u;
        return out.add(extent);
    }
    static bool offset(N base,N move,N delta,N& out){
        out=base;return out.add(move) && out.add(delta);
    }
    template<class Query>
    static bool resolve(N& x,N& y,N vx,N vy,Query query,SolidCollisionResult& result){
        return resolveWithDiagonalQuery(x,y,vx,vy,query,query,result);
    }
    // scrPlayerPlatformCollision differs only in its diagonal walk: query
    // checkTarget first, then parentWall (57) on a miss, for each axis probe.
    // Do not apply that union to the initial predictive or axis-only queries.
    template<class Platform,class Wall>
    static bool resolvePlatform(N& x,N& y,N vx,N vy,Platform platform,Wall wall,
                                SolidCollisionResult& result){
        return resolveWithDiagonalQuery(x,y,vx,vy,platform,
            [&](N px,N py,bool& hit){
                if(!platform(px,py,hit))return false;
                return hit || wall(px,py,hit);
            },result);
    }
private:
    template<class Query,class Diagonal>
    static bool resolveWithDiagonalQuery(N& x,N& y,N vx,N vy,Query query,Diagonal diagonal,
                                         SolidCollisionResult& result){
        if(!x.valid()||!y.valid()||!vx.valid()||!vy.valid())return false;
        N px,py;
        if(!probe(x,vx,px)||!probe(y,vy,py))return false;
        result={0,0};
        const int sx=sign(vx),sy=sign(vy);
        const N dx=N::fromInteger(sx),dy=N::fromInteger(sy);
        bool hit;
        if(!query(px,y,hit))return false;
        if(hit){
            if(!rounded(x,x))return false;
            N move{};
            for(;;){
                N candidate;
                if(!offset(x,move,dx,candidate)||!query(candidate,y,hit))return false;
                if(hit)break;
                if(!move.add(dx))return false;
                if(atLeast(move,vx)){move={};break;}
            }
            if(!x.add(move))return false;
            result.x=sx;
        }
        if(!query(x,py,hit))return false;
        if(hit){
            if(!rounded(y,y))return false;
            N move{};
            for(;;){
                N candidate;
                if(!offset(y,move,dy,candidate)||!query(x,candidate,hit))return false;
                if(hit)break;
                if(!move.add(dy))return false;
                if(atLeast(move,vy)){move={};break;}
            }
            if(!y.add(move))return false;
            result.y=sy;
        }
        if(result.x || result.y)return true;
        if(!query(px,py,hit))return false;
        if(!hit)return true;
        if(!rounded(y,y)||!rounded(x,x))return false;
        N mx{},my{};
        for(;;){
            N qx,qy;
            if(!offset(x,mx,dx,qx)||!offset(y,my,{},qy)||!diagonal(qx,qy,hit))return false;
            if(hit)break;
            if(!mx.add(dx))return false;
            if(!offset(x,mx,{},qx)||!offset(y,my,dy,qy)||!diagonal(qx,qy,hit))return false;
            if(hit)break;
            if(!my.add(dy))return false;
            if(atLeast(my,vy)){mx={};my={};break;}
        }
        if(!y.add(my)||!x.add(mx))return false;
        result.y=sy;result.x=sx;return true;
    }
};
}
#endif

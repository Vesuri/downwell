#ifndef DOWNWELL_CORE_CHECK_COLLISION_UNITS_H
#define DOWNWELL_CORE_CHECK_COLLISION_UNITS_H
#include <cstdint>
namespace downwell {
// scrCheckCollisionWith on fixed-point positions and velocities (W3 fixed
// point): CheckCollisionWithScript's walks with whole-pixel steps. UnitX and
// UnitY are each axis's units per pixel (65536 for Q16, 1280 for the fifths
// of Q8 that keep a 0.15 gravity exact). Query(x, y, hit) takes positions in
// those units and must not mutate the caller. Positions must stay inside
// +-2^29 units and velocities inside +-8 pixels.
template<std::int32_t UnitX,std::int32_t UnitY> struct CheckCollisionUnits {
    static_assert(UnitX>1&&UnitY>1&&!(UnitX&1)&&!(UnitY&1),"even units per pixel keep exact halves");
    int xcollision=0,ycollision=0;
    static std::int32_t magnitude(std::int32_t v){return v<0?-v:v;}
    static int sign(std::int32_t v){return v>0?1:v<0?-1:0;}
    // round(): the nearest pixel, halves to even.
    template<std::int32_t Unit> static std::int32_t roundPixel(std::int32_t v){
        std::int32_t whole=v/Unit,fraction=v%Unit;
        if(fraction<0){fraction+=Unit;--whole;}
        if(fraction>Unit/2||(fraction==Unit/2&&(whole&1)))++whole;
        return whole;
    }
    template<std::int32_t Unit> static std::int32_t rounded(std::int32_t v){return roundPixel<Unit>(v)*Unit;}
    // round(position) plus the velocity rounded away from zero to a pixel.
    template<std::int32_t Unit> static std::int32_t probe(std::int32_t position,std::int32_t velocity){
        const std::int32_t speed=magnitude(velocity);
        std::int32_t extent=rounded<Unit>(speed);
        if(extent<speed)extent+=Unit;
        return rounded<Unit>(position)+(velocity<0?-extent:extent);
    }
    template<class Query> bool run(std::int32_t& xx,std::int32_t& yy,std::int32_t xsp,std::int32_t ysp,Query query){
        constexpr std::int32_t range=std::int32_t(1)<<29;
        if(magnitude(xx)>=range||magnitude(yy)>=range||magnitude(xsp)>8*UnitX||magnitude(ysp)>8*UnitY)return false;
        const std::int32_t wx=probe<UnitX>(xx,xsp),wy=probe<UnitY>(yy,ysp);
        xcollision=0;ycollision=0;
        const int sx=sign(xsp),sy=sign(ysp);
        const std::int32_t dx=sx*UnitX,dy=sy*UnitY;
        bool hit;
        if(!query(wx,yy,hit))return false;
        if(hit){
            xx=rounded<UnitX>(xx);
            std::int32_t move=0;
            for(;;){
                if(!query(xx+move+dx,yy,hit))return false;
                if(hit)break;
                move+=dx;
                if(magnitude(move)>=magnitude(xsp)){move=0;break;}
            }
            xx+=move;xcollision=sx;
        }
        if(!query(xx,wy,hit))return false;
        if(hit){
            yy=rounded<UnitY>(yy);
            std::int32_t move=0;
            for(;;){
                if(!query(xx,yy+move+dy,hit))return false;
                if(hit)break;
                move+=dy;
                if(magnitude(move)>=magnitude(ysp)){move=0;break;}
            }
            yy+=move;ycollision=sy;
        }
        if(xcollision||ycollision)return true;
        if(!query(wx,wy,hit))return false;
        if(!hit)return true;
        yy=rounded<UnitY>(yy);xx=rounded<UnitX>(xx);
        std::int32_t mx=0,my=0;
        for(;;){
            if(!query(xx+mx+dx,yy+my,hit))return false;
            if(hit)break;
            mx+=dx;
            if(!query(xx+mx,yy+my+dy,hit))return false;
            if(hit)break;
            my+=dy;
            if(magnitude(my)>=magnitude(ysp)){mx=0;my=0;break;}
        }
        yy+=my;xx+=mx;ycollision=sy;xcollision=sx;return true;
    }
};
}
#endif

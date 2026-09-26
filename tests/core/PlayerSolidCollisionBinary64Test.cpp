#include "core/PlayerSolidCollisionBinary64.h"
// Reviewed UTMT 0.9.2.0 scrCheckCollisionWith assembly SHA-256:
// 436c35e576d8e3b7d31b2db6b8346521b5d53115bc0d25a06fc798a7ca94a959
// data.win: c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8
#ifdef DOWNWELL_FREESTANDING
extern "C" bool playerSolidCollisionBinary64Compile(downwell::MotionBinary64& x,
    downwell::MotionBinary64& y,downwell::MotionBinary64 vx,downwell::MotionBinary64 vy,
    bool(*query)(downwell::MotionBinary64,downwell::MotionBinary64,bool&),downwell::SolidCollisionResult& result){
    return downwell::PlayerSolidCollisionBinary64::resolve(x,y,vx,vy,query,result);
}
#else
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace downwell;
static MotionBinary64 number(double value){std::uint64_t b;std::memcpy(&b,&value,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static double value(MotionBinary64 n){const std::uint64_t b=(std::uint64_t(n.high)<<32)|n.low;double d;std::memcpy(&d,&b,8);return d;}
struct ScalarMath {
    using Number=double;
    static double unit(){return 1;}
    static double round(double v){return std::nearbyint(v);}
    static double ceilAbs(double v){return std::ceil(std::abs(v));}
};
struct Query {double x,y;};
int main(){
    unsigned cases=0;
    for(int origin=-12;origin<=12;++origin)for(int vx=-9;vx<=9;++vx)for(int vy=-9;vy<=9;++vy)
        for(unsigned shape=0;shape<5;++shape){
            double x=origin/7.0,y=-origin/11.0;
            auto nx=number(x),ny=number(y);
            const auto collision=[&](double a,double b){
                switch(shape){case 0:return a>=2;case 1:return b<=-2;case 2:return a>=1 && b>=1;
                    case 3:return a<=-1 || b>=3;default:return false;}
            };
            std::vector<Query> queries;
            const auto expected=resolveSolidCollision<ScalarMath>(x,y,vx/3.0,vy/3.0,
                [&](double a,double b){queries.push_back({a,b});return collision(a,b);});
            unsigned index=0;SolidCollisionResult actual{};
            assert(PlayerSolidCollisionBinary64::resolve(nx,ny,number(vx/3.0),number(vy/3.0),
                [&](auto a,auto b,bool& hit){assert(index<queries.size());
                    assert(value(a)==queries[index].x && value(b)==queries[index].y);++index;
                    hit=collision(value(a),value(b));return true;},actual));
            assert(index==queries.size() && value(nx)==x && value(ny)==y);
            assert(actual.x==expected.x && actual.y==expected.y);++cases;
        }
    // Fail each query of a diagonal walk. Nothing after the failure may run.
    for(unsigned fail=0;fail<6;++fail){
        auto x=number(0.25),y=number(0.25);SolidCollisionResult result{};unsigned calls=0;
        const bool ok=PlayerSolidCollisionBinary64::resolve(x,y,number(3),number(3),
            [&](auto a,auto b,bool& hit){if(calls++==fail)return false;
                hit=value(a)>=2 && value(b)>=2;return true;},result);
        assert(!ok && calls==fail+1);
        assert(value(x)==(fail<3?0.25:0.0) && value(y)==(fail<3?0.25:0.0));
    }
    for(int half=-9;half<=9;++half){
        auto x=number(half/2.0),y=number(-half/2.0);SolidCollisionResult result{};
        unsigned calls=0;
        assert(PlayerSolidCollisionBinary64::resolve(x,y,number(0),number(0),
            [&](auto,auto,bool& hit){++calls;hit=true;return true;},result));
        assert(value(x)==std::nearbyint(half/2.0) && value(y)==std::nearbyint(-half/2.0));
        assert(calls==6 && result.x==0 && result.y==0);
    }
    auto invalid=MotionBinary64{0x7ff00000u,0},y=number(1);SolidCollisionResult unchanged{7,8};
    assert(!PlayerSolidCollisionBinary64::resolve(invalid,y,number(1),number(1),
        [](auto,auto,bool&){assert(false);return true;},unchanged));
    assert(unchanged.x==7 && unchanged.y==8 && value(y)==1);
    std::printf("Binary64 solid collision: %u coordinate/query-order comparisons and failure cutoffs passed\n",cases);
}
#endif

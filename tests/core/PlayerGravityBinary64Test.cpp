#include "core/PlayerGravityBinary64.h"
// Reviewed UTMT 0.9.2.0 scrGravity_n assembly SHA-256:
// c8de972902fdc0118f1b4bfd1c7b88550f6a9ea7f4939c426e09998d41684b1b
// data.win: c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8
#ifdef DOWNWELL_FREESTANDING
extern "C" bool playerGravityBinary64Compile(downwell::PlayerGravityBinary64& state,
    downwell::MotionBinary64& velocity,int area,bool water,int style,bool ballooning,
    downwell::MotionBinary64 gravity,downwell::MotionBinary64 maximum){
    return state.apply(velocity,area,water,style,ballooning,gravity,maximum);
}
#else
#include <cassert>
#include <cstring>
#include <cstdint>
#include <cstdio>
using namespace downwell;
static MotionBinary64 n(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static bool same(MotionBinary64 a,MotionBinary64 b){return a.high==b.high && a.low==b.low;}
int main(){
    unsigned checks=0;
    for(int area=1;area<=4;++area)for(int style=0;style<5;++style)
        for(unsigned flags=0;flags<4;++flags)for(int initial=-32;initial<=32;++initial){
            PlayerGravityBinary64 state;double expected=initial/4.0;auto actual=n(expected);
            const bool water=flags&1,balloon=flags&2;
            double maximum,gravity;
            if(area==4){maximum=6;gravity=0.2;}
            else if(water){maximum=6;gravity=0.2/1.4;}
            else if(style==3){maximum=7;gravity=0.2/1.5;}
            else{maximum=7;gravity=0.2;}
            if(balloon)maximum-=1;
            for(unsigned tick=0;tick<180;++tick){
                expected+=gravity;if(expected>=maximum)expected=maximum;
                assert(state.apply(actual,area,water,style,balloon,n(0.2),n(7)));
                assert(same(actual,n(expected)) && same(state.finalGravity,n(gravity)) && same(state.finalMaximum,n(maximum)));
                ++checks;
            }
        }
    PlayerGravityBinary64 failed;auto velocity=n(1);
    assert(!failed.apply(velocity,1,true,0,false,{0x7ff00000u,0},n(7)));
    assert(same(velocity,n(1)) && same(failed.finalMaximum,n(6)));
    assert(!failed.apply(velocity,1,false,0,false,n(0.2),{0x7ff00000u,0}));
    assert(same(velocity,n(1.2))); // Addition completed before failed cap comparison.
    std::printf("Player gravity: %u bitwise steps across area/water/style/balloon combinations passed\n",checks);
}
#endif

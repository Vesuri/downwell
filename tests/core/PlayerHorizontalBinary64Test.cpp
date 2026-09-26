#include "core/PlayerHorizontalBinary64.h"
// Reviewed UTMT 0.9.2.0 assembly (SHA-256):
// scrPlMovement: 3ff0e28b9dcef221ed9dd6a1ceb801bef746d7538016547fbe49a60af0177644
// scrPlayerInit: f17aa6e41f1a28c05adc914c0da94699745d8407783ca0ec0418d2b5dc14b7b8
// data.win: c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8
#ifdef DOWNWELL_FREESTANDING
extern "C" bool playerHorizontalBinary64Compile(downwell::MotionBinary64& speed,
    int& facing,bool ground,bool left,bool right,bool locked){
    return downwell::PlayerHorizontalBinary64::apply(speed,facing,ground,left,right,locked);
}
#else
#include <cassert>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <initializer_list>
using namespace downwell;
static MotionBinary64 bits(double value){
    std::uint64_t raw;std::memcpy(&raw,&value,8);
    return {std::uint32_t(raw>>32),std::uint32_t(raw)};
}
static void reference(double& speed,int& facing,bool ground,bool left,bool right,bool locked){
    if(locked){speed=0;return;}
    if(left){speed-=0.2;if(speed>0)speed-=ground?2:1;
        if(speed<-(ground?2:2.5))speed=-(ground?2:2.5);facing=-1;}
    if(right){speed+=0.2;if(speed<0)speed+=ground?2:1;
        if(speed>(ground?2:2.5))speed=ground?2:2.5;facing=1;}
    if(!left && !right){speed-=(speed>0?1:speed<0?-1:0)*(ground?0.4:0.1);
        if((speed<0?-speed:speed)<0.3)speed=0;}
    if(left && right)speed=0;
}
int main(){
    unsigned checks=0;
    for(int initial=-1536;initial<=1536;++initial)for(unsigned mode=0;mode<16;++mode){
        double expected=initial/256.0;auto actual=bits(expected);int ef=-1,af=-1;
        for(unsigned tick=0;tick<8;++tick){
            const bool ground=mode&1,left=mode&2,right=mode&4,locked=mode&8;
            reference(expected,ef,ground,left,right,locked);
            assert(PlayerHorizontalBinary64::apply(actual,af,ground,left,right,locked));
            const auto e=bits(expected);assert(actual.high==e.high && actual.low==e.low && af==ef);
            ++checks;
        }
    }
    auto invalid=MotionBinary64{0x7ff00000u,0};int facing=-1;
    assert(!PlayerHorizontalBinary64::apply(invalid,facing,false,true,false));
    assert(invalid.high==0x7ff00000u && facing==-1);
    // Non-Q8 results from direction/trigonometry must survive the next input
    // update. Include both neighbours of the source branch thresholds.
    for(double boundary : {0.0,0.2,0.3,0.4,1.0,2.0,2.5,3.0})
        for(int sign : {-1,1})for(int neighbour : {-1,0,1})for(unsigned mode=0;mode<16;++mode){
            double expected=sign*boundary;
            if(neighbour)expected=std::nextafter(expected,neighbour<0?-10.0:10.0);
            // The portable number contract deliberately excludes subnormals.
            if(expected!=0 && std::abs(expected)<0x1p-1022)continue;
            auto actual=bits(expected);int ef=1,af=1;
            reference(expected,ef,mode&1,mode&2,mode&4,mode&8);
            assert(PlayerHorizontalBinary64::apply(actual,af,mode&1,mode&2,mode&4,mode&8));
            const auto e=bits(expected);assert(actual.high==e.high && actual.low==e.low && af==ef);
            ++checks;
        }
    std::printf("Exact horizontal velocity: %u source-order comparisons passed\n",checks);
}
#endif

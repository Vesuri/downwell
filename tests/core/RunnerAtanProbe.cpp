// Read-only numeric probe of the recovered normal finite atan2 path.
// Compile for x86_64; on Apple Silicon execution requires Rosetta.
#if !defined(__x86_64__)
#error This probe requires x87 instructions
#endif
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "core/PortableAtan.h"
#include "core/RunnerImageValue.h"
static double recovered(double y,double x){
    const double ax=std::abs(x),ay=std::abs(y);
    const bool negativeX=std::signbit(x),negativeY=std::signbit(y);
    unsigned short saved;
    const unsigned short control=0x133f;
    double result;
    __asm__ volatile("fnstcw %0":"=m"(saved));
    __asm__ volatile("fldcw %0"::"m"(control));
    if(negativeX){
        __asm__ volatile("fldl %1\n\tfldl %2\n\tfpatan\n\tfldpi\n\tfsubp %%st,%%st(1)\n\tfstpl %0"
            :"=m"(result):"m"(ay),"m"(ax):"st","st(1)");
        // AT&T fsubp encodes reverse subtraction here: pi - angle.
    }else __asm__ volatile("fldl %1\n\tfldl %2\n\tfpatan\n\tfstpl %0"
        :"=m"(result):"m"(ay),"m"(ax):"st","st(1)");
    if(negativeY)result=-result;
    __asm__ volatile("fldcw %0"::"m"(saved));
    return result;
}
static unsigned bits(float f){unsigned n;std::memcpy(&n,&f,4);return n;}
int main(){
    unsigned state=0x12345678,checked=0,mismatch=0,portableChecked=0;
    auto sample=[&](){state^=state<<13;state^=state>>17;state^=state<<5;return float(int(state&0xffffff)-0x800000)/1024.f;};
    for(unsigned i=0;i<1000000;++i){
        const float x=sample(),y=sample();if(x==0 || y==0)continue;
        const float actual=float(recovered(y,x)),reference=float(std::atan2(double(y),double(x)));
        if(i<10000){
            auto number=[](double d){std::uint64_t n;std::memcpy(&n,&d,8);return downwell::MotionBinary64{unsigned(n>>32),unsigned(n)};};
            downwell::MotionBinary64 candidate;downwell::RunnerImageValue narrowed;
            if(!downwell::portableAtan(number(y),number(x),candidate) || !narrowed.assign(candidate) || narrowed.bits!=bits(actual)){
                std::puts("Portable candidate differs from x87");return 2;
            }
            ++portableChecked;
        }
        if(bits(actual)!=bits(reference)){
            if(mismatch<8)std::printf("difference x=%a y=%a x87=%a libm=%a\n",double(x),double(y),double(actual),double(reference));
            ++mismatch;
        }
        ++checked;
    }
    std::printf("x87 finite atan2 probe: %u inputs, %u binary32 differences against host libm\n",checked,mismatch);
    std::printf("Portable candidate: %u binary32 comparisons against recovered x87 sequence passed\n",portableChecked);
    return mismatch?1:0;
}

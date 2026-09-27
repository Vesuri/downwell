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
#include "core/RunnerPointDirection.h"
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
    // Contacts use world coordinates narrowed separately by the wrapper,
    // including fractional player positions. Exercise that complete path,
    // rather than comparing only unconstrained atan inputs near the origin.
    unsigned contacts=0;
    auto number=[](double d){std::uint64_t n;std::memcpy(&n,&d,8);return downwell::MotionBinary64{unsigned(n>>32),unsigned(n)};};
    auto x87=[&](downwell::MotionBinary64 y,downwell::MotionBinary64 x,downwell::MotionBinary64& out){
        auto value=[](downwell::MotionBinary64 v){std::uint64_t n=(std::uint64_t(v.high)<<32)|v.low;double d;std::memcpy(&d,&n,8);return d;};
        out=number(recovered(value(y),value(x)));return true;
    };
    for(int dx=-32;dx<=32;++dx)for(int dy=-32;dy<=32;++dy)
    for(unsigned fraction=0;fraction<4;++fraction){
        downwell::MotionBinary64 candidate,original;
        const auto ax=number(240+dx),ay=number(176+dy);
        const auto bx=number(240+fraction/4.0),by=number(176+fraction/4.0);
        if(!downwell::RunnerPointDirection::evaluate(ax,ay,bx,by,candidate,downwell::portableAtan)||
           !downwell::RunnerPointDirection::evaluate(ax,ay,bx,by,original,x87)||
           candidate.high!=original.high||candidate.low!=original.low){
            std::printf("Contact direction differs: dx=%d dy=%d fraction=%u\n",dx,dy,fraction);return 3;
        }
        ++contacts;
    }
    std::printf("Contact wrapper: %u integer/fractional world-coordinate comparisons against recovered x87 sequence passed\n",contacts);
    return mismatch?1:0;
}

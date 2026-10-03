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
#ifdef DOWNWELL_PROBE_TABLE_ATAN
#include "core/TableAtan.h"
#include "core/PlayerDamageNumber.h"
#include "core/SmokeDirection.h"
#include "core/SmokeTrig.h"
static unsigned tableAccepted=0,tableFallback=0;
static bool candidateAtan(downwell::MotionBinary64 y,downwell::MotionBinary64 x,downwell::MotionBinary64& out){
    if(downwell::tableAtanCandidate(y,x,out)){++tableAccepted;return true;}
    ++tableFallback;return downwell::portableAtan(y,x,out);
}
#else
static constexpr auto candidateAtan=downwell::portableAtan;
#endif
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
static double value(downwell::MotionBinary64 v){
    std::uint64_t n=(std::uint64_t(v.high)<<32)|v.low;
    double d;std::memcpy(&d,&n,8);return d;
}
static double maxDirectionError=0,maxRadiansError=0;
static bool directionMatches(downwell::MotionBinary64 candidate,downwell::MotionBinary64 original){
#ifdef DOWNWELL_PROBE_TABLE_ATAN
    double error=std::abs(value(candidate)-value(original));
    if(error>180)error=360-error;
    maxDirectionError=std::fmax(maxDirectionError,error);
    return error<1.0/256;
#else
    return candidate.high==original.high&&candidate.low==original.low;
#endif
}
#ifdef DOWNWELL_PROBE_TABLE_ATAN
static double maxPixelError=0;
static bool movementMatches(downwell::MotionBinary64 x,downwell::MotionBinary64 y,
    downwell::MotionBinary64 candidate,downwell::MotionBinary64 original){
    downwell::PlayerDamageNumber a,b;
    const auto amount=downwell::MotionBinary64::fromInteger(1);
    if(!a.create(x,y)||!b.create(x,y)||!a.assignDamage(amount,candidate)||!b.assignDamage(amount,original))return false;
    auto lengthdir=[](auto speed,auto direction,auto& result){
        downwell::RunnerImageValue length,angle,radian;downwell::MotionBinary64 cosine;
        return length.assign(speed)&&angle.assign(direction)&&downwell::SmokeDirection::radians(angle,radian)&&
            downwell::smokeCosine(radian.promoted(),cosine)&&downwell::SmokeDirection::component(length,cosine,false,result);
    };
    for(unsigned tick=0;tick<60;++tick){
        a.advanceAlarm(0);b.advanceAlarm(0);
        if(!a.step(lengthdir,[](bool,const auto&){return true;})||
           !b.step(lengthdir,[](bool,const auto&){return true;}))return false;
        const double error=std::abs(value(a.x.promoted())-value(b.x.promoted()));
        maxPixelError=std::fmax(maxPixelError,error);
        if(error>=1.0/64||a.y.bits!=b.y.bits||a.alarm0!=b.alarm0||a.destroyRequested!=b.destroyRequested)return false;
    }
    return true;
}
#endif
int main(){
    unsigned state=0x12345678,checked=0,mismatch=0,portableChecked=0;
    auto sample=[&](){state^=state<<13;state^=state>>17;state^=state<<5;return float(int(state&0xffffff)-0x800000)/1024.f;};
    for(unsigned i=0;i<1000000;++i){
        const float x=sample(),y=sample();if(x==0 || y==0)continue;
        const float actual=float(recovered(y,x)),reference=float(std::atan2(double(y),double(x)));
        if(i<10000
#ifdef DOWNWELL_PROBE_TABLE_ATAN
            || true
#endif
        ){
            auto number=[](double d){std::uint64_t n;std::memcpy(&n,&d,8);return downwell::MotionBinary64{unsigned(n>>32),unsigned(n)};};
            downwell::MotionBinary64 candidate;downwell::RunnerImageValue narrowed;
            if(!candidateAtan(number(y),number(x),candidate) || !narrowed.assign(candidate))return 2;
#ifdef DOWNWELL_PROBE_TABLE_ATAN
            const double error=std::abs(value(narrowed.promoted())-actual);
            maxRadiansError=std::fmax(maxRadiansError,error);
            if(error>=3.141592653589793/(180*256)){
#else
            if(narrowed.bits!=bits(actual)){
#endif
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
    std::printf("Candidate: %u comparisons against recovered x87 sequence passed\n",portableChecked);
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
        if(!downwell::RunnerPointDirection::evaluate(ax,ay,bx,by,candidate,candidateAtan)||
           !downwell::RunnerPointDirection::evaluate(ax,ay,bx,by,original,x87)||
           !directionMatches(candidate,original)){
            std::printf("Contact direction differs: dx=%d dy=%d fraction=%u\n",dx,dy,fraction);return 3;
        }
        ++contacts;
    }
    std::printf("Contact wrapper: %u integer/fractional world-coordinate comparisons against recovered x87 sequence passed\n",contacts);
    // Actual contact/laser tape observations, including independent Q8
    // fractions. The earlier quarter-pixel diagonal grid is not this domain.
    const int observed[][4]={{12381,364851,38,1418},{0,279846,-6,1107},
        {2982,348160,0,1360},{-243,940339,3,3689},{3798,1086617,18,4259},
        {12534,438272,45,1704},{12212,438272,46,1715}};
    auto contact=[&](int px,int py,int cx,int cy){
        downwell::MotionBinary64 candidate,original;
        const auto ax=number(240.0+cx),ay=number(176.0+cy);
        const auto bx=number(240.0+px/256.0),by=number(176.0+py/256.0);
        return downwell::RunnerPointDirection::evaluate(ax,ay,bx,by,candidate,candidateAtan)&&
            downwell::RunnerPointDirection::evaluate(ax,ay,bx,by,original,x87)&&
            directionMatches(candidate,original)
#ifdef DOWNWELL_PROBE_TABLE_ATAN
            &&movementMatches(bx,by,candidate,original)
#endif
            ;
    };
    for(const auto& c:observed){
        if(!contact(c[0],c[1],c[2],c[3])){
            std::puts("Observed contact direction differs from x87");return 4;
        }
#ifdef DOWNWELL_PROBE_TABLE_ATAN
        downwell::MotionBinary64 result;
        std::printf("Observed table admission q8=%d,%d: %s\n",c[0],c[1],
            downwell::tableAtanCandidate(number(c[1]/256.0-c[3]),number(c[0]/256.0-c[2]),result)?
                "table":"reference");
#endif
    }
    for(unsigned i=0;i<4096;++i){
        state=state*1664525u+1013904223u;const int dx=int(state&16383u)-8192;
        state=state*1664525u+1013904223u;const int dy=int(state&16383u)-8192;
        const int cx=int(i%161)-80,cy=int(i*17%8192);
        if(!contact(cx*256+dx,cy*256+dy,cx,cy)){
            std::printf("Q8 contact direction differs: dx=%d dy=%d\n",dx,dy);return 5;
        }
    }
    std::puts("Contact domain: 7 observed and 4096 independent Q8-offset comparisons passed");
#ifdef DOWNWELL_PROBE_TABLE_ATAN
    std::printf("Table admissions: %u accepted, %u reference fallbacks\n",tableAccepted,tableFallback);
    std::printf("Maximum error: radians=%g, complete direction=%g degrees (limit 1/256 degree)\n",
        maxRadiansError,maxDirectionError);
    std::printf("Damage-number 60-tick movement: maximum horizontal error=%g pixels; vertical motion and lifetime exact\n",maxPixelError);
#endif
    return mismatch?1:0;
}

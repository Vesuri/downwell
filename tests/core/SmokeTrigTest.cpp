#include "core/SmokeTrig.h"
#include "core/SmokeDirection.h"
#include "core/PreparedActorBoundsTrig.h"
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
static void checked(bool ok,int line){if(!ok){std::fprintf(stderr,"Smoke trig check failed at line %d\n",line);std::abort();}}
#define check(ok) checked((ok),__LINE__)
using downwell::MotionBinary64;
static MotionBinary64 number(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static double real(MotionBinary64 x){const std::uint64_t b=(std::uint64_t(x.high)<<32)|x.low;double d;std::memcpy(&d,&b,8);return d;}
struct Scalar {
    double value=0;
    static Scalar constant(std::uint32_t h,std::uint32_t l){return {real({h,l})};}
    bool add(Scalar b){value+=b.value;return true;}
    bool subtract(Scalar b){value-=b.value;return true;}
    bool multiply(Scalar b){value*=b.value;return true;}
    bool integer(int& out)const{out=int(std::nearbyint(value));return true;}
};
int main(){
    for(int degrees=0;degrees<=450;degrees+=90){
        downwell::RunnerImageValue angle,radians,c,s,expectedC,expectedS;
        MotionBinary64 dc,ds;
        check(angle.assignInteger(degrees)&&downwell::SmokeDirection::radians(angle,radians)&&
            downwell::smokeTrig(radians.promoted(),dc,ds)&&expectedC.assign(dc)&&expectedS.assign(ds));
        check(downwell::PreparedActorBoundsTrig::at(angle,c,s)&&
            c.bits==expectedC.bits&&s.bits==expectedS.bits);
        for(auto unsupported:{angle.bits-1,angle.bits+1,angle.bits|0x80000000u}){
            c.bits=123;s.bits=456;
            check(!downwell::PreparedActorBoundsTrig::at({unsupported},c,s)&&c.bits==123&&s.bits==456);
        }
    }
    downwell::GameMakerRandom random(42);
    unsigned count=0;
    // Source endpoints, the smallest nonzero WELL-derived smoke angle and
    // neighboring float radians at table-reduction boundaries.
    auto compare=[&](double radian){
        MotionBinary64 c,s,only;
        check(downwell::smokeTrig(number(radian),c,s));
        check(downwell::smokeCosine(number(radian),only)&&only.high==c.high&&only.low==c.low);
        if(radian!=0){
            Scalar oc,os;
            check(downwell::smoke_trig_detail::smokeTrigProgram(Scalar{radian},true,oc));
            check(downwell::smoke_trig_detail::smokeTrigProgram(Scalar{radian},false,os));
            check(real(c)==oc.value&&real(s)==os.value);
        }
        check(float(real(c))==float(std::cos(double(radian))) &&
              float(real(s))==float(std::sin(double(radian))));
    };
    const float minimumAngle=float(359.0/4294967296.0);
    compare((minimumAngle*float(3.14159265358979323846))/180.f);
    compare(-13.f);compare(13.f);
    for(unsigned i=0;i<132;++i){
        const float boundary=float((i+0.5)*3.14159265358979323846/32);
        compare(boundary);compare(std::nextafter(boundary,0.f));compare(std::nextafter(boundary,14.f));
        compare(-boundary);compare(std::nextafter(-boundary,0.f));compare(std::nextafter(-boundary,-14.f));
    }
    // Exact source-dispatch boundaries, including lower words that the
    // original pextrw does not inspect. The oracle executes the recovered
    // instruction sequence in host binary64, not a substituted libm path.
    for(auto bits:{MotionBinary64{0x30300000u,0},MotionBinary64{0x30300000u,1},
                   MotionBinary64{0x40f5ffffu,0xffffffffu}}){
        compare(real(bits));bits.high|=0x80000000u;compare(real(bits));
    }
    for(unsigned i=0;i<20000;++i){
        const double radian=random.random(180224)-90112;
        compare(radian);++count;
    }
    // Reduction boundaries near the top of the admitted interval exercise
    // the much larger integer index before the source's table-index mask.
    for(unsigned i=917800;i<917873;++i){
        const double boundary=(i+0.5)*3.14159265358979323846/32;
        for(double value:{boundary,std::nextafter(boundary,0.),std::nextafter(boundary,90112.)}){
            compare(value);compare(-value);
        }
    }
    for(auto invalid:{number(-90112),number(90112),MotionBinary64{0x302fffffu,0xffffffffu},
                     MotionBinary64{0x7ff00000u,0},MotionBinary64{0,1}}){
        auto c=number(2),s=number(3);
        check(!downwell::smokeTrig(invalid,c,s) && real(c)==2 && real(s)==3);
        check(!downwell::smokeCosine(invalid,c)&&real(c)==2);
    }
    for(auto zero:{MotionBinary64{},MotionBinary64{0x80000000u,0}}){
        MotionBinary64 c,s,only;
        check(downwell::smokeTrig(zero,c,s)&&s.high==zero.high&&s.low==zero.low);
        check(downwell::smokeCosine(zero,only)&&only.high==c.high&&only.low==c.low);
    }
    for(unsigned i=0;i<40000;++i){
        const float angle=i<1441?float(int(i)-720):float(random.random(1440)-720);
        const float multiplied=angle*float(3.14159265358979323846),radian=multiplied/180.f;
        MotionBinary64 c,s;
        if(!downwell::smokeTrig(number(radian),c,s)){std::fprintf(stderr,"unsupported %u %g\n",i,radian);std::abort();}
        MotionBinary64 only;
        check(downwell::smokeCosine(number(radian),only)&&only.high==c.high&&only.low==c.low);
        if(radian!=0){
            Scalar oc,os;
            check(downwell::smoke_trig_detail::smokeTrigProgram(Scalar{radian},true,oc));
            check(downwell::smoke_trig_detail::smokeTrigProgram(Scalar{radian},false,os));
            check(real(c)==oc.value && real(s)==os.value);
        }
        check(float(real(c))==float(std::cos(double(radian))) && float(real(s))==float(std::sin(double(radian))));
        MotionBinary64 x,y;
        downwell::RunnerImageValue input,converted;
        check(input.assign(number(angle))&&downwell::SmokeDirection::radians<0x44340000u>(input,converted)&&real(converted.promoted())==double(radian));
        if(std::abs(angle)<=585)check(downwell::SmokeDirection::vector(number(3.125),number(angle),x,y,downwell::smokeTrig));
        ++count;
    }
    std::printf("Smoke trig: %u source-program/scalar pairs and float libm comparisons passed\n",count);
}

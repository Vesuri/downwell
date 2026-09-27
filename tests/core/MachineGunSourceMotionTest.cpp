#include "core/MachineGunSourceMotion.h"
#ifdef DOWNWELL_NATIVE_MOTION_PROBE
// Compile the actual arithmetic for 68000 and inspect undefined helpers.
struct Sample { unsigned value; unsigned nextU32(){return value;} };
extern "C" bool machineGunSourceMotionProbe(downwell::MachineGunSourceMotion& motion,unsigned word){
    Sample sample{word};unsigned queries=0,iterations=0;
    return motion.reset(downwell::MotionBinary64::fromInteger(239),
        downwell::MotionBinary64::fromInteger(8191)) && motion.initialize(sample) &&
        motion.approachSolid([&](auto,auto,bool& hit){hit=++queries==3;return true;},
            [](auto,auto){return true;},iterations) && motion.commitPosition();
}
#else
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
using downwell::MotionBinary64;
static void check(bool ok){if(!ok)std::abort();}
static MotionBinary64 number(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static double real(MotionBinary64 n){const std::uint64_t b=(std::uint64_t(n.high)<<32)|n.low;double d;std::memcpy(&d,&b,8);return d;}
static bool same(MotionBinary64 n,double d){const auto expected=number(d);return n.high==expected.high&&n.low==expected.low;}
struct Scalar {
    double value=0;
    static Scalar constant(std::uint32_t h,std::uint32_t l){return {real({h,l})};}
    bool add(Scalar b){value+=b.value;return true;}
    bool subtract(Scalar b){value-=b.value;return true;}
    bool multiply(Scalar b){value*=b.value;return true;}
    bool integer(int& out)const{out=int(std::nearbyint(value));return true;}
};
static double snap(float value){double integral;const float fraction=float(std::modf(double(value),&integral));return std::abs(fraction)<0.0001f?integral:double(value);}
int main(){
    struct Sample {std::uint32_t word;unsigned draws=0;std::uint32_t nextU32(){++draws;return word;}};
    unsigned trajectories=0,steps=0,differentEndpoints=0,localRoundingDifferences=0;
    downwell::GameMakerRandom random(42);
    for(unsigned i=0;i<4096;++i)for(bool laser:{false,true}){
        Sample sample{i==0?0u:i==1?0xffffffffu:i==2?0x80000000u:random.nextU32()};
        const double direction=270.+(laser?0.:double(sample.word)/4294967296.*6.-3.);
        const float scaled=float(direction)*float(3.14159265358979323846);
        const float radians=scaled/180.f;
        Scalar c,s;
        check(downwell::smoke_trig_detail::smokeTrigProgram(Scalar{radians},true,c));
        check(downwell::smoke_trig_detail::smokeTrigProgram(Scalar{radians},false,s));
        const double vx=snap(float(c.value)*8.f),vy=snap(-(float(s.value)*8.f));
        downwell::MachineGunSourceMotion motion;
        const double birthX=int(i%479)-239+0.123456789;
        const double birthY=int(i%31)*511-176+0.987654321;
        float x=float(birthX),y=float(birthY);
        check(motion.reset(number(birthX),number(birthY),laser));
        MotionBinary64 untouchedX=number(1),untouchedY=number(2);
        check(!motion.endpoint(untouchedX,untouchedY)&&same(untouchedX,1)&&same(untouchedY,2));
        check(!motion.commitPosition());
        check(motion.initialize(sample)&&motion.initialize(sample)&&sample.draws==1);
        check(same(motion.direction,direction)&&same(motion.imageAngle.promoted(),float(direction)));
        check(same(motion.xsp,vx)&&same(motion.ysp,vy));
        for(unsigned tick=0;tick<32;++tick){
            const double ex=double(x)+vx,ey=double(y)+vy;
            MotionBinary64 px,py;
            check(motion.endpoint(px,py)&&same(px,ex)&&same(py,ey));
            if(ex!=double(float(ex))||ey!=double(float(ey)))++differentEndpoints;
            const float local=float(double(x)-240.);
            if(float(double(float(double(local)+vx))+240.)!=float(ex))++localRoundingDifferences;
            x=float(ex);y=float(ey);
            check(motion.commitPosition()&&same(motion.x.promoted(),double(x))&&same(motion.y.promoted(),double(y)));
            ++steps;
        }
        check(motion.reset(number(1),number(2))&&!motion.initialized&&!motion.zeroSpread&&motion.xsp.zero());
        ++trajectories;
    }
    check(differentEndpoints&&localRoundingDifferences);
    downwell::MachineGunSourceMotion motion;
    downwell::GameMakerRandom source(2),reference(2);
    check(motion.initialize(source));reference.nextU32();
    for(unsigned i=0;i<16;++i)check(source.stateWord(i)==reference.stateWord(i));
    check(source.stateIndex()==reference.stateIndex()&&source.drawCount()==reference.drawCount());
    const auto previousX=motion.x.bits,previousY=motion.y.bits;
    check(!motion.reset({0x7ff00000u,0},number(2))&&motion.x.bits==previousX&&motion.y.bits==previousY);
    check(!motion.reset(number(1),{0,1})&&motion.initialized);
    MotionBinary64 rejected=number(123);
    check(!downwell::SmokeDirection::component({0x41000001u},number(1),false,rejected)&&same(rejected,123));
    unsigned approachWrites=0;
    for(unsigned stop=0;stop<=101;++stop){
        Sample sample{0x12345678u};
        downwell::MachineGunSourceMotion shot;
        check(shot.reset(number(239.12345),number(8191.98765)));
        unsigned phase=0;
        check(shot.initialize(sample,[&](downwell::RunnerImageValue angle){
            check(phase++==0&&!shot.initialized&&angle.bits==shot.imageAngle.bits);return true;
        },[&](const downwell::MachineGunSourceMotion& newborn){
            check(phase++==1&&!newborn.initialized&&same(newborn.direction,real(shot.direction)));return true;
        }));
        check(phase==2&&sample.draws==1);
        // A second Step must not re-run angle publication or muzzle Create.
        check(shot.initialize(sample,[](auto){std::abort();return false;},
            [](const auto&){std::abort();return false;}));
        float px=float(real(shot.x.promoted())),py=float(real(shot.y.promoted()));
        const double dx=real(shot.xsp)/10.,dy=real(shot.ysp)/10.;
        unsigned queries=0,writes=0,iterations=999;
        check(shot.approachSolid([&](MotionBinary64 x,MotionBinary64 y,bool& hit){
            check(same(x,px)&&same(y,py)&&writes==queries*2);
            hit=queries++==stop;return true;
        },[&](downwell::RunnerImageValue x,downwell::RunnerImageValue y){
            if(writes%2==0)py=float(double(py)+dy);else px=float(double(px)+dx);
            check(same(x.promoted(),px)&&same(y.promoted(),py));++writes;return true;
        },iterations));
        check(iterations==stop&&writes==stop*2&&queries==(stop==101?101:stop+1));
        approachWrites+=writes;
        // The caller may now request destruction. The event nevertheless
        // reaches its final X-before-Y writes, from the approached position.
        unsigned finalWrites=0;
        check(shot.commitPosition([&](downwell::RunnerImageValue x,downwell::RunnerImageValue y){
            if(finalWrites++==0)px=float(double(px)+real(shot.xsp));
            else py=float(double(py)+real(shot.ysp));
            check(same(x.promoted(),px)&&same(y.promoted(),py));return true;
        }));
        check(finalWrites==2&&same(shot.x.promoted(),px)&&same(shot.y.promoted(),py));
    }
    // A failed Y publication during the final Step must retain successful X;
    // approach failure on X must retain successful Y. Owners latch terminal
    // failure and never retry initialization or these partially completed events.
    for(bool approach:{false,true}){
        Sample sample{0x12345678u};downwell::MachineGunSourceMotion shot;
        check(shot.reset(number(239),number(1000))&&shot.initialize(sample));
        const auto oldX=shot.x.bits,oldY=shot.y.bits;
        unsigned calls=0,iterations=999;
        auto failSecond=[&](auto,auto){return ++calls!=2;};
        const bool ok=approach?shot.approachSolid([](auto,auto,bool& hit){hit=false;return true;},failSecond,iterations)
            :shot.commitPosition(failSecond);
        check(!ok&&calls==2);
        if(approach)check(shot.x.bits==oldX&&shot.y.bits!=oldY&&iterations==0);
        else check(shot.x.bits!=oldX&&shot.y.bits==oldY);
    }
    for(unsigned failAt=0;failAt<2;++failAt){
        Sample sample{0};downwell::MachineGunSourceMotion shot;unsigned calls=0;
        check(!shot.initialize(sample,[&](auto){return calls++!=failAt;},
            [&](const auto&){return calls++!=failAt;}));
        check(!shot.initialized&&sample.draws==1&&calls==failAt+1);
    }
    std::printf("Source gun motion: %u trajectories, %u steps, %u unrounded endpoints, %u local-origin rounding differences\n",
        trajectories,steps,differentEndpoints,localRoundingDifferences);
    std::printf("Source solid approach: %u ordered writes, all 0..101 iteration limits and partial failures passed\n",approachWrites);
}
#endif

#include "core/DroneBulletVerification.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
static MotionBinary64 number(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static double real(MotionBinary64 n){const std::uint64_t b=(std::uint64_t(n.high)<<32)|n.low;double d;std::memcpy(&d,&b,8);return d;}
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
    assert(DroneBulletVerification::run());
    unsigned steps=0;
    for(int degrees:{0,45,90,180,270,359})for(unsigned alarm:{0u,1u,16u}){
        DroneBulletSourceStep shot;GameMakerRandom random(9),oracle(9);bool good=true;
        assert(shot.reset(number(240.25),number(8191.75)));
        shot.motion.direction=number(degrees);
        double speed=7;float x=240.25f,y=8191.75f;bool animated=false;
        struct Ops {
            bool publishAngle(RunnerImageValue){return true;}
            bool publishImageSpeed(RunnerImageValue){return true;}
            bool publishPosition(RunnerImageValue,RunnerImageValue){return true;}
            bool solid(DroneBulletSourceStep&){return true;}
        } ops;
        for(unsigned tick=0;tick<40;++tick){
            if(tick==alarm)shot.alarmZero();
            if(!tick)(void)oracle.nextU32(); // equal-bound random_range
            if(tick>=alarm)speed*=.8;
            if(speed<3)animated=true;
            const float radians=float(float(degrees)*float(3.14159265358979323846))/180.f;
            Scalar c,s;
            assert(smoke_trig_detail::smokeTrigProgram(Scalar{radians},true,c)&&
                   smoke_trig_detail::smokeTrigProgram(Scalar{radians},false,s));
            const double vx=snap(float(c.value)*float(speed)),vy=snap(-(float(s.value)*float(speed)));
            x=float(double(x)+vx);y=float(double(y)+vy);
            assert(shot.step(random,ops,good)&&good&&real(shot.speed)==speed&&shot.animation==animated&&
                real(shot.motion.x.promoted())==double(x)&&real(shot.motion.y.promoted())==double(y));
            assert(random.drawCount()==oracle.drawCount()&&random.stateIndex()==oracle.stateIndex());
            for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
            ++steps;
        }
    }
    std::printf("Drone bullet: %u source scalar steps, one-time RNG, delayed decay and retained post-destroy movement passed\n",steps);
}

#include "core/CurrencyHomingSource.h"
#include "core/CurrencyHomingMath.h"
#include "core/PortableAtan.h"
#include <cassert>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
static MotionBinary64 real(double d){std::uint64_t bits;std::memcpy(&bits,&d,8);return {unsigned(bits>>32),unsigned(bits)};}
static bool equal(MotionBinary64 a,double d){auto b=real(d);return a.high==b.high&&a.low==b.low;}
static int compare(double a,double b){const double delta=a-b;return delta>0.00001?1:delta<-.00001?-1:0;}
int main(){
    unsigned cases=0;
    const double dirs[]={-60,0,10,180,350,400},targets[]={0,180,359},rates[]={5,31.8,32},speeds[]={1,8.875,9};
    for(double dir:dirs)for(double target:targets)for(double rate:rates)for(double speed:speeds)for(unsigned failure=0;failure<4;++failure){
        double difference=dir-target;if(compare(difference,180)>0)difference-=360;else if(compare(difference,-180)<0)difference+=360;
        double heading=dir;if(compare(difference,rate)>0)heading-=rate;else if(compare(difference,-rate)<0)heading+=rate;else heading=target;
        double rotation=rate+.6;if(compare(rotation,32)>0)rotation=32;
        double velocity=speed+.125;if(compare(velocity,9)>0)velocity=9;
        CurrencyCreate state;state.direction=real(dir);state.rotationSpeed=real(rate);state.gemSpeed=real(speed);
        state.xx=real(12.25);state.yy=real(-3.5);state.stepRate=7;
        struct Math {
            double target,heading,velocity;unsigned failure,calls=0;
            bool pointDirection(MotionBinary64 x,MotionBinary64 y,MotionBinary64 px,MotionBinary64 py,MotionBinary64& out){
                assert(equal(x,12.25)&&equal(y,-3.5)&&equal(px,240)&&equal(py,176));
                if(++calls==failure)return false;out=real(target);return true;
            }
            bool lengthdirX(MotionBinary64 speed,MotionBinary64 dir,MotionBinary64& out){
                assert(equal(speed,velocity)&&equal(dir,heading));if(++calls==failure)return false;out=real(2);return true;
            }
            bool lengthdirY(MotionBinary64 speed,MotionBinary64 dir,MotionBinary64& out){
                assert(equal(speed,velocity)&&equal(dir,heading));if(++calls==failure)return false;out=real(-3);return true;
            }
        } math{target,heading,velocity,failure};bool good=true;
        assert(CurrencyHomingSource::run(state,real(240),real(176),math,good)==!failure&&good==!failure);
        assert(state.stepRate==1&&math.calls==(failure?failure:3));
        if(!failure)assert(equal(state.playerDirection,target)&&equal(state.directionDifference,difference)&&equal(state.direction,heading)&&
            equal(state.rotationSpeed,rotation)&&equal(state.gemSpeed,velocity)&&equal(state.xx,14.25)&&equal(state.yy,-6.5));
        else assert(equal(state.xx,12.25)&&equal(state.yy,-3.5)&&!CurrencyHomingSource::run(state,real(240),real(176),math,good)&&math.calls==failure);
        ++cases;
    }
    // Verify the speed-nine component arithmetic independently of recovered
    // trig, including the near-integer fractional snap in the runner wrapper.
    for(float speed:{8.125f,8.5f,8.875f,9.f})for(float trig:{-1.f,-.99999f,-.5f,0.f,.00001f,.5f,.99999f,1.f})for(bool negate:{false,true}){
        const float product=speed*trig;const float value=negate?-product:product;double integer;
        const float fraction=float(std::modf(double(value),&integer));const double expected=std::abs(fraction)<.0001f?integer:double(value);
        RunnerImageValue s;s.assign(real(speed));MotionBinary64 actual;
        assert(SmokeDirection::component<0x41100000u>(s,real(trig),negate,actual)&&equal(actual,expected));
    }
    // portableAtan is a candidate, not original-runner proof. A prolonged
    // orbit can leave the recovered trig domain: reject, never normalize the
    // stored direction to hide that missing domain.
    CurrencyHomingMath math{portableAtan};CurrencyCreate state;state.xx=real(240);state.yy=real(512);state.attracted=true;bool good=true;
    unsigned ticks=0;for(;ticks<80&&good;++ticks)CurrencyHomingSource::run(state,real(250),real(500),math,good);
    RunnerImageValue angle;assert(!good&&ticks>12&&angle.assign(state.direction)&&(angle.bits&0x7fffffffu)>0x44124000u);
    state={};state.xx=real(240);state.yy=real(512);state.gemSpeed=real(9);state.rotationSpeed=real(32);good=true;
    assert(CurrencyHomingSource::run(state,real(300),real(512),math,good)&&equal(state.gemSpeed,9)&&equal(state.rotationSpeed,32)&&equal(state.xx,249));
    std::printf("Currency homing: %u turn/cap/failure cases, speed-nine wrapper and explicit trig-domain rejection passed\n",cases);
}

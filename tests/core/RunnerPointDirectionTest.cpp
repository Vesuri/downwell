#include "core/RunnerPointDirection.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
using namespace downwell;
static MotionBinary64 number(double d){unsigned long long n;std::memcpy(&n,&d,8);return {unsigned(n>>32),unsigned(n)};}
static double value(MotionBinary64 n){unsigned long long bits=(static_cast<unsigned long long>(n.high)<<32)|n.low;double d;std::memcpy(&d,&bits,8);return d;}
static void run(double ax,double ay,double bx,double by){
    const float dx=float(bx)-float(ax),dy=float(by)-float(ay);
    bool called=false;MotionBinary64 result;
    assert(RunnerPointDirection::evaluate(number(ax),number(ay),number(bx),number(by),result,
        [&](auto y,auto x,auto& out){called=true;assert(value(x)==double(dx) && value(y)==double(dy));out=number(std::atan2(value(y),value(x)));return true;}));
    float expected=0;
    if(dx==0){expected=dy>0?270:dy<0?90:0;assert(!called);}
    else{
        expected=(float(std::atan2(double(dy),double(dx)))*180.f)/float(3.14159265358979323846);
        expected=expected<=0?-expected:360.f-expected;assert(called);
    }
    const auto oracle=number(expected);assert(result.high==oracle.high && result.low==oracle.low);
}
int main(){
    GameMakerRandom random(123);
    for(unsigned i=0;i<100000;++i)run(random.random(2000)-1000,random.random(2000)-1000,random.random(2000)-1000,random.random(2000)-1000);
    for(double x:{-1.,-0.0,0.,1.})for(double y:{-1.,-0.0,0.,1.})run(0,0,x,y);
    run(16777216,0,16777217,1); // Narrowing first collapses dx to zero.
    MotionBinary64 output=number(7);
    assert(!RunnerPointDirection::evaluate({}, {},number(1),number(1),output,[](auto,auto,auto&){return false;}));
    assert(value(output)==7);
    std::puts("Point-direction wrapper: 100000 float-rounded scalar cases, axes, coincident points and callback failure passed");
}

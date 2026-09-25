#include "core/SmokeMotion.h"
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
using downwell::MotionBinary64;
static void check(bool ok){if(!ok)std::abort();}
static MotionBinary64 number(double x){std::uint64_t b;std::memcpy(&b,&x,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static bool same(MotionBinary64 x,double y){auto n=number(y);return x.high==n.high && x.low==n.low;}
static bool outside(double sx,double sy,double x,double y){
    const float dx=float(x-sx),dy=float(y-sy),xx=dx*dx,yy=dy*dy,total=xx+yy;
    return double(float(std::sqrt(double(total))))-16>1e-5;
}
int main(){
    unsigned steps=0,escapes=0;
    for(unsigned seed=0;seed<1024;++seed){
        downwell::GameMakerRandom rng(seed),oracle(seed);
        downwell::SmokeMotion smoke;
        const double ox=double(float(192.123456)),oy=double(float(-96.123456));
        check(smoke.create(number(192.123456),number(-96.123456),rng));
        double speed=oracle.random(2)+2;
        oracle.random(0.13-0.05);
        const double angle=oracle.random(359),radians=angle*(3.14159265358979323846/180);
        double vx=std::cos(radians)*speed,vy=-std::sin(radians)*speed,xx=ox,yy=oy;
        bool nocol=true;
        unsigned directions=0,distances=0;
        auto direction=[&](MotionBinary64 s,MotionBinary64 d,MotionBinary64& x,MotionBinary64& y){
            check(same(s,speed) && same(d,angle) && rng.drawCount()==3);
            ++directions;x=number(vx);y=number(vy);return true;
        };
        // Count calls while exercising the shared integer-only distance gate.
        auto escaped=[&](MotionBinary64 sx,MotionBinary64 sy,MotionBinary64 x,MotionBinary64 y,bool& outside){
            ++distances;check(same(sx,ox) && same(sy,oy));
            return downwell::smokeEscapedDistance(sx,sy,x,y,outside);
        };
        const float rate=float(speed*(oracle.random(0.13-0.05)+0.05));
        check(smoke.collideWall() && same(smoke.speed,speed)); // no-collision gate
        for(unsigned frame=0;frame<80;++frame){
            if(frame==2 && seed%2==0){smoke.alarm0();nocol=false;}
            const unsigned priorDistances=distances;
            check(smoke.step(rng,direction,escaped));
            xx+=vx;yy+=vy;vx*=0.9;vy*=0.9;
            const double px=double(float(std::nearbyint(xx))),py=double(float(std::nearbyint(yy)));
            check(same(smoke.xx,xx) && same(smoke.yy,yy) && same(smoke.xsp,vx) && same(smoke.ysp,vy));
            check(same(smoke.x.promoted(),px) && same(smoke.y.promoted(),py));
            check(same(smoke.imageSpeed.promoted(),double(rate)));
            check(distances==priorDistances+unsigned(nocol));
            if(nocol && outside(ox,oy,px,py)){nocol=false;++escapes;}
            check(smoke.noCollision==nocol && smoke.allSet && directions==1);
            if(frame==30){
                check(smoke.collideWall());if(!nocol)speed*=0.2;
                check(same(smoke.speed,speed) && same(smoke.xsp,vx) && same(smoke.ysp,vy));
                smoke.collideWater();check(same(smoke.speed,speed));
            }
            check(rng.drawCount()==4 && oracle.drawCount()==4);
            for(unsigned i=0;i<16;++i)check(rng.stateWord(i)==oracle.stateWord(i));
            ++steps;
        }
    }
    check(escapes>0);
    std::printf("Smoke motion: %u scalar-matched Steps, %u distance escapes, one-time initialization and wall/alarm events passed\n",steps,escapes);
}

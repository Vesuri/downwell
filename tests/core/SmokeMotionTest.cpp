#include "core/SmokeMotion.h"
#include "core/SmokeDirection.h"
#include "core/SmokeTrig.h"
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
static void publishedSteps(){
    for(unsigned seed=0;seed<1024;++seed){
        downwell::GameMakerRandom random(seed),referenceRandom(seed);
        downwell::SmokeMotion live,reference;
        check(live.create(number(192.125),number(-96.125),random));
        check(reference.create(number(192.125),number(-96.125),referenceRandom));
        auto direction=[](auto speed,auto angle,auto& x,auto& y){
            return downwell::SmokeDirection::vector(speed,angle,x,y,downwell::smokeTrig);
        };
        for(unsigned frame=0;frame<40;++frame){
            const auto before=live;
            unsigned calls=0;
            struct Publish {
                downwell::SmokeMotion& live;
                const downwell::SmokeMotion& before;
                unsigned& calls;
                bool imageSpeed(downwell::RunnerImageValue value){
                    check(!live.allSet&&calls++==0&&value.bits==live.imageSpeed.bits);return true;
                }
                bool position(downwell::RunnerImageValue x,downwell::RunnerImageValue y){
                    const unsigned axis=calls++-unsigned(!before.allSet);
                    check(axis<2&&live.allSet&&x.bits==live.x.bits&&y.bits==live.y.bits);
                    if(axis==0)check(y.bits==before.y.bits);
                    return true;
                }
            } publish{live,before,calls};
            bool good=true;
            check(live.stepPublished(random,direction,publish,downwell::smokeEscapedDistance,good));
            check(reference.step(referenceRandom,direction));
            check(calls==2+unsigned(!before.allSet));
            check(live.xx.high==reference.xx.high&&live.xx.low==reference.xx.low&&
                  live.yy.high==reference.yy.high&&live.yy.low==reference.yy.low&&
                  live.xsp.high==reference.xsp.high&&live.xsp.low==reference.xsp.low&&
                  live.ysp.high==reference.ysp.high&&live.ysp.low==reference.ysp.low&&
                  live.x.bits==reference.x.bits&&live.y.bits==reference.y.bits&&
                  live.imageSpeed.bits==reference.imageSpeed.bits&&live.noCollision==reference.noCollision);
            check(random.drawCount()==referenceRandom.drawCount());
            for(unsigned i=0;i<16;++i)check(random.stateWord(i)==referenceRandom.stateWord(i));
        }
        for(unsigned fail=0;fail<4;++fail){
            auto state=reference;
            state.allSet=false;state.noCollision=true;
            const auto before=state;
            unsigned calls=0;
            struct FailingPublish {
                unsigned& calls;unsigned fail;
                bool imageSpeed(downwell::RunnerImageValue){return calls++!=fail;}
                bool position(downwell::RunnerImageValue,downwell::RunnerImageValue){return calls++!=fail;}
            } publish{calls,fail};
            bool good=true;
            auto escaped=[&](auto,auto,auto,auto,bool&){return calls++!=fail;};
            check(!state.stepPublished(random,direction,publish,escaped,good)&&!good&&calls==fail+1);
            if(fail==0)check(!state.allSet&&state.x.bits==before.x.bits&&state.y.bits==before.y.bits);
            if(fail==1)check(state.allSet&&state.y.bits==before.y.bits);
            const auto draws=random.drawCount();
            check(!state.stepPublished(random,direction,publish,escaped,good)&&calls==fail+1&&random.drawCount()==draws);
        }
    }
    std::puts("Smoke publication: 40960 scalar/RNG matches and 4096 terminal callback failures passed");
}
int main(){
    publishedSteps();
    unsigned steps=0,escapes=0;
    for(unsigned seed=0;seed<1024;++seed){
        downwell::GameMakerRandom rng(seed),oracle(seed);
        downwell::SmokeMotion smoke;
        const double ox=double(float(192.123456)),oy=double(float(-96.123456));
        check(smoke.create(number(192.123456),number(-96.123456),rng));
        double speed=oracle.random(2)+2;
        oracle.random(0.13-0.05);
        const double angle=oracle.random(359);
        const float scaled=float(angle)*float(3.14159265358979323846),radians=scaled/180.f;
        auto snap=[](float value){double integral;const float fraction=float(std::modf(double(value),&integral));return std::abs(fraction)<0.0001f?double(float(integral)):double(value);};
        double vx=snap(float(std::cos(double(radians)))*float(speed));
        double vy=snap(-(float(std::sin(double(radians)))*float(speed))),xx=ox,yy=oy;
        bool nocol=true;
        unsigned directions=0,distances=0;
        auto direction=[&](MotionBinary64 s,MotionBinary64 d,MotionBinary64& x,MotionBinary64& y){
            check(same(s,speed) && same(d,angle) && rng.drawCount()==3);
            ++directions;return downwell::SmokeDirection::vector(s,d,x,y,downwell::smokeTrig);
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

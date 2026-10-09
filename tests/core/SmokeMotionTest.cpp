#include "core/SmokeMotion.h"
#include "core/SmokeInstances.h"
#include "core/SmokeDirection.h"
#include "core/SmokeTrig.h"
#include "core/GameMakerRandom.h"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
using downwell::MotionBinary64;
static void checked(bool ok,int line){if(!ok){std::fprintf(stderr,"Smoke check failed at line %d\n",line);std::abort();}}
#define check(expression) checked(bool(expression),__LINE__)
static MotionBinary64 number(double x){std::uint64_t b;std::memcpy(&b,&x,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static bool same(MotionBinary64 x,double y){auto n=number(y);return x.high==n.high && x.low==n.low;}
static bool outside(double sx,double sy,double x,double y){
    const float dx=float(x-sx),dy=float(y-sy),xx=dx*dx,yy=dy*dy,total=xx+yy;
    return double(float(std::sqrt(double(total))))-16>1e-5;
}
struct NativeSmokeAudit {
    using Record=downwell::SmokeInstances<1>::Record;
    int reject=-1;unsigned moves=0,marks=0,retires=0;
    bool birth(downwell::InstanceHandle key,const Record& r){
        check(key.generation==r.lifetimeId&&r.sourceId>=105187&&r.state.imageSpeed.bits==0x3f800000u);
        return reject!=0;
    }
    bool birthAt(downwell::InstanceHandle key,const Record& r,downwell::RunnerImageValue x,downwell::RunnerImageValue y){
        check(x.bits==r.x.bits&&y.bits==r.y.bits);return birth(key,r);
    }
    bool transform(downwell::InstanceHandle,const Record&,downwell::RunnerTransformField,
        downwell::RunnerImageValue,downwell::RunnerImageValue,std::uint32_t){return true;}
    bool mask(downwell::InstanceHandle,const Record&,int){return true;}
    bool retire(downwell::InstanceHandle,const Record&){++retires;return reject!=4;}
    bool activation(downwell::InstanceHandle,const Record&,bool){return true;}
    bool membership(downwell::InstanceHandle,const Record&,bool){return true;}
    bool move(downwell::InstanceHandle,const Record& r,downwell::RunnerImageValue x,downwell::RunnerImageValue y){
        ++moves;check(x.bits==r.state.x.bits&&y.bits==r.state.y.bits);return reject!=1;
    }
    bool sprite(downwell::InstanceHandle,const Record&,int){return true;}
    bool imageIndex(downwell::InstanceHandle,const Record&,downwell::RunnerImageValue){return reject!=2;}
    bool markDestroyed(downwell::InstanceHandle,const Record& r){++marks;check(r.state.destroyRequested&&!r.destroyRequested);return reject!=3;}
};
static void nativeSmoke(){
    using Owner=downwell::SmokeInstances<1>;
    unsigned frames=0;
    for(unsigned seed=0;seed<64;++seed){
        NativeSmokeAudit audit;Owner owner(Owner::Binding::bindPositionedActor(audit));
        downwell::GameMakerRandom random(seed),oracleRandom(seed);downwell::InstanceHandle key;
        downwell::SmokeEffect oracle;
        check(owner.create(105187,number(240.25),number(192.5),random,key)&&
            oracle.create(number(240.25),number(192.5),oracleRandom));
        check(key.generation!=105187&&owner.sourceHandle(105187).generation==key.generation);
        check(owner.activate(key,false));const auto draws=random.drawCount();
        check(owner.step(key,random)&&owner.advanceAnimation(key)&&random.drawCount()==draws&&audit.moves==0);
        check(owner.activate(key,true));
        while(!oracle.destroyRequested){
            check(owner.advanceAnimation(key)&&oracle.advanceImage());
            if(!oracle.destroyRequested){
                check(owner.step(key,random)&&oracle.step(oracleRandom,[](auto s,auto d,auto& x,auto& y){
                    return downwell::SmokeDirection::vector(s,d,x,y,downwell::smokeTrig);
                }));
            }
            const auto* r=owner.find(key);
            check(r&&r->x.bits==oracle.x.bits&&r->y.bits==oracle.y.bits&&
                r->state.imageIndex.bits==oracle.imageIndex.bits&&r->destroyRequested==oracle.destroyRequested);
            check(random.drawCount()==oracleRandom.drawCount());
            for(unsigned i=0;i<16;++i)check(random.stateWord(i)==oracleRandom.stateWord(i));
            check(++frames<10000);
        }
        check(owner.size()==1&&audit.marks==1&&audit.retires==0&&owner.retire(key)&&!owner.find(key));
        const auto old=key;
        check(owner.create(105188,{}, {},random,key)&&key.generation>old.generation&&!owner.find(old)&&owner.clear());
    }
    for(int reject=0;reject<5;++reject){
        NativeSmokeAudit audit;Owner owner(Owner::Binding::bindPositionedActor(audit));
        downwell::GameMakerRandom random(2);downwell::InstanceHandle key;bool worldGood=true;owner.observeFailure(worldGood);
        if(reject==0)audit.reject=0;
        const bool born=owner.create(105187,{}, {},random,key);
        if(reject==0)check(!born&&random.drawCount()==0&&owner.size()==1);
        else{
            check(born);audit.reject=reject;
            if(reject==1)check(!owner.step(key,random)&&audit.moves==1);
            else if(reject==2)check(!owner.advanceAnimation(key));
            else{
                unsigned limit=0;
                while(owner.valid()&&!owner.find(key)->destroyRequested){owner.advanceAnimation(key);check(++limit<200);}
                if(reject==4)check(!owner.retire(key));
            }
        }
        check(!owner.valid()&&!worldGood);const auto draws=random.drawCount();
        check(!owner.step(key,random)&&random.drawCount()==draws);
        audit.reject=-1;check(owner.clear()&&owner.size()==0);
    }
    std::printf("Native smoke: %u scalar/RNG-matched frames, deferred retirement and five callback failures passed\n",frames);
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
    nativeSmoke();
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

#include "core/CompactRoomEffects.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
// The compact world's Q16 no-loop and moving effect records against
// ScriptSpawnEffect and MovingFx, their VM forms, over every admitted sprite,
// a sweep of image speeds, angles and move speeds and a time stop: published
// pixels stay within one; frames and lifetimes are exact for rates of 1/16
// and above and within one below.
static void checkAt(bool ok,int line){if(!ok){std::fprintf(stderr,"Compact room effect check failed at line %d\n",line);std::abort();}}
#define check(ok) checkAt((ok),__LINE__)
using N=downwell::MotionBinary64;
using Fx=downwell::CompactRoomEffects<1,1>;
namespace {
int pixel(downwell::RunnerImageValue v){std::int32_t n;checkAt(v.promoted().roundPixel(n),__LINE__);return int(n);}
N real(double v){std::uint64_t b;std::memcpy(&b,&v,8);N n;n.high=std::uint32_t(b>>32);n.low=std::uint32_t(b);return n;}
struct Ops {
    bool moving=true;
    bool timeStopBound(bool& m){m=moving;return true;}
    bool imageSpeed(downwell::RunnerImageValue v,downwell::RunnerImageValue& a){a=v;return true;}
    bool position(bool,downwell::RunnerImageValue v,downwell::RunnerImageValue& a){a=v;return true;}
};
const int sprites[]={84,118,571,572,111,112,96,101,94,62094,95,601,602,603,605,704};
}
int main(){
    unsigned effects=0,ticks=0,pixelDifferences=0,frameDifferences=0,lifeDifferences=0;
    for(int sprite:sprites){
        unsigned count;check(Fx::frames(sprite,count));
        for(unsigned s=1;s<=40;++s){
            const N speed=real(s/40.0);
            for(unsigned a=0;a<8;++a){
                const N angle=real(a*45.0+(s%3)*7.25),x=real(100.5+s*0.37),y=real(2000.25+a*1.3);
                const N move=real((a%4)*0.75+s*0.011);
                for(unsigned family=0;family<2;++family){
                    Fx fx;downwell::ScriptSpawnEffect noLoop;downwell::MovingFx moving;
                    const auto accept=[](const auto&){return true;};
                    if(family==0){
                        check(fx.spawnNoLoop(x,y,sprite,speed,angle,{}));
                        check(noLoop.create(x,y)&&noLoop.setSprite(sprite,count,accept)&&
                              noLoop.setValue(noLoop.imageSpeed,speed,accept)&&noLoop.setValue(noLoop.angle,angle,accept));
                    }else{
                        check(fx.spawnMoving(x,y,sprite,speed,angle,move));
                        downwell::RunnerImageValue px,py;N component;
                        check(px.assign(x)&&py.assign(y)&&moving.create(px,py)&&moving.setSprite(sprite,count,accept)&&
                              moving.imageSpeed.assign(speed)&&moving.angle.assign(angle));
                        check(downwell::WhiteJellyStep::movementComponent(false,move,angle,component));moving.xsp=component;
                        check(downwell::WhiteJellyStep::movementComponent(true,move,angle,component));moving.ysp=component;
                    }
                    ++effects;
                    downwell::NonLoopSpriteAnimation& reference=family?static_cast<downwell::NonLoopSpriteAnimation&>(moving):noLoop;
                    const auto& rx=family?moving.x:noLoop.x;const auto& ry=family?moving.y:noLoop.y;
                    const auto record=[&]()->const Fx::Effect&{return family?fx.moving(0).effect:fx.noLoop(0).effect;};
                    std::int32_t degrees=0;
                    {downwell::RunnerImageValue t;check(t.assign(angle)&&t.promoted().roundPixel(degrees));}
                    degrees%=360;if(degrees<0)degrees+=360;
                    check(record().turn==(unsigned(degrees+45)/90&3u));
                    unsigned referenceLife=0,fixedLife=0;
                    Ops ops;
                    const auto live=[&]{return (family?fx.movingCount():fx.noLoopCount())!=0;};
                    for(unsigned tick=0;tick<1000&&(!reference.destroyRequested||live());++tick){
                        if(!reference.destroyRequested){check(reference.advanceImage());if(reference.destroyRequested)referenceLife=tick+1;}
                        if(live()){check(fx.advanceAnimation(fx.beginPhase()));if(!live())fixedLife=tick+1;}
                        if(reference.destroyRequested||!live())continue;
                        const bool go=!(tick>=3&&tick<9); // TimeStopBound false for six steps
                        if(family){
                            ops.moving=go;check(moving.step(ops));
                            check(fx.stepMoving(fx.beginPhase(),[&](const Fx::Effect&,bool& m){m=go;return true;}));
                        }
                        ++ticks;
                        const auto& e=record();
                        check(std::abs(pixel(rx)-e.x)<=1&&std::abs(pixel(ry)-e.y)<=1);
                        pixelDifferences+=(pixel(rx)!=e.x)+(pixel(ry)!=e.y);
                        const int rf=int(reference.drawFrame()),ff=int(e.drawFrame());
                        check(std::abs(rf-ff)<=1&&(s<3||rf==ff));frameDifferences+=rf!=ff;
                    }
                    if(!(referenceLife&&fixedLife&&(referenceLife>fixedLife?referenceLife-fixedLife:fixedLife-referenceLife)<=1))
                        std::fprintf(stderr,"sprite %d s %u a %u family %u life %u/%u\n",sprite,s,a,family,referenceLife,fixedLife);
                    // Rates of 1/16 and above are exact (Binary32Timeline).
                    if(s>=3)check(referenceLife==fixedLife);
                    check(referenceLife&&fixedLife&&(referenceLife>fixedLife?referenceLife-fixedLife:fixedLife-referenceLife)<=1);
                    lifeDifferences+=referenceLife!=fixedLife;
                }
            }
        }
    }
    std::printf("Compact room effects Q16: %u effects, %u ticks, %u of %u published pixels and %u frames differ by one, "
                "%u lifetimes by a tick\n",effects,ticks,pixelDifferences,2*ticks,frameDifferences,lifeDifferences);
    check(pixelDifferences*100<2*ticks&&frameDifferences*100<ticks&&lifeDifferences*100<effects);
}

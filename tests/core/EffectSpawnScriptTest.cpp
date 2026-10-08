#include "core/MachineGunSourceMotion.h"
#include "core/NonLoopSpriteAnimation.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
using namespace downwell;
struct Sample {unsigned draws=0;unsigned nextU32(){++draws;return 0x12345678u;}};
static bool same(MotionBinary64 a,MotionBinary64 b){return a.high==b.high&&a.low==b.low;}
struct Effects {
    using Handle=unsigned;
    MachineGunSourceMotion& shot;
    unsigned stage=0,fail=0;bool high=false;
    NonLoopSpriteAnimation animation;
    bool next(unsigned expected){assert(!shot.initialized&&++stage==expected);return stage!=fail;}
    bool create(int object,MotionBinary64 x,MotionBinary64 y,Handle& h){
        assert(object==219&&same(x,shot.x.promoted())&&same(y,shot.y.promoted()));h=123;
        assert(animation.spriteId==-1&&animation.frames==0&&animation.imageSpeed.bits==0x3f800000u);
        assert(!animation.destroyRequested);
        return next(1);
    }
    bool myFx(Handle h){assert(h==123);return next(2);}
    bool sprite(Handle h,int sprite){
        assert(h==123&&sprite==(high?602:603));
        return next(3)&&animation.setSprite(sprite,1,[](const auto&){return true;});
    }
    bool imageSpeed(Handle h,MotionBinary64 v){
        assert(h==123&&same(v,{0x3fe00000u,0}));return next(4)&&animation.imageSpeed.assign(v);
    }
    bool imageAngle(Handle h,MotionBinary64 v){assert(h==123&&same(v,shot.direction));return next(5);}
    bool depth(Handle h,MotionBinary64 v){assert(h==123&&same(v,{}));return next(6);}
};
int main(){
    for(unsigned fail=0;fail<=4;++fail){
        struct Nol {
            using Handle=unsigned;
            unsigned calls=0,fail=0,caller=0;
            bool next(){return ++calls!=fail;}
            bool create(int id,MotionBinary64 x,MotionBinary64 y,Handle& h){
                assert(id==219&&same(x,MotionBinary64::fromInteger(241))&&same(y,MotionBinary64::fromInteger(987)));
                h=7;return next();
            }
            bool myFx(Handle h){assert(h==7);if(!next())return false;caller=h;return true;}
            bool sprite(Handle h,int id){assert(h==caller&&id==85);return next();}
            bool imageSpeed(Handle h,MotionBinary64 value){assert(h==caller&&same(value,{0x3fd33333u,0x33333333u}));return next();}
            // No angle/depth methods: scrFxNol must not require those writes.
        } ops{0,fail,0};
        assert(EffectSpawnScript::runNol(ops,MotionBinary64::fromInteger(241),MotionBinary64::fromInteger(987),
            85,{0x3fd33333u,0x33333333u})==!fail);
        assert(ops.calls==(fail?fail:4)&&ops.caller==(!fail||fail>2?7u:0u));
    }
    for(bool high:{false,true})for(unsigned fail=0;fail<=7;++fail){
        MachineGunSourceMotion shot;Sample sample;
        assert(shot.reset({0x406e1000u,0},{0x40bfff40u,0})); // fractional absolute room coordinates
        Effects effects{shot,0,fail,high,{}};unsigned angles=0;
        auto publish=[&](RunnerImageValue angle){
            assert(++angles==1&&effects.stage==0&&!shot.initialized&&angle.bits==shot.imageAngle.bits);
            return fail!=7;
        };
        const bool ok=shot.initializeWithEffects(sample,publish,high,effects);
        assert(ok==(fail==0)&&shot.initialized==ok&&sample.draws==1&&angles==1);
        assert(effects.stage==(fail==7?0:fail?fail:6));
        if(ok){
            assert(shot.initializeWithEffects(sample,publish,high,effects));
            assert(sample.draws==1&&angles==1&&effects.stage==6);
            assert(effects.animation.imageIndex.bits==0&&effects.animation.drawFrame()==0);
            assert(effects.animation.advanceImage()&&!effects.animation.destroyRequested&&
                effects.animation.imageIndex.bits==0x3f000000u&&effects.animation.drawFrame()==0);
            assert(effects.animation.advanceImage()&&effects.animation.destroyRequested&&effects.animation.imageIndex.bits==0);
            assert(effects.animation.advanceImage()&&effects.animation.imageIndex.bits==0);
        }
    }
    std::puts("Source muzzle script: ordered birth/caller/setter writes, both profiles, seven failure boundaries and no repeated birth passed");
}

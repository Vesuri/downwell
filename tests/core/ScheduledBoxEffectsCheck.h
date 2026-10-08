#ifndef DOWNWELL_TEST_SCHEDULED_BOX_EFFECTS_CHECK_H
#define DOWNWELL_TEST_SCHEDULED_BOX_EFFECTS_CHECK_H
#include "core/InstanceHandle.h"
#include "core/MotionBinary64.h"
#include "core/GameMakerRandom.h"
#include "core/DebrisEffects.h"
#include "core/SmokeEffects.h"
#include "core/FlashBallEffects.h"
#include <cassert>
#include <functional>
// A scheduled failure-prefix check, not a complete destruction backend.
// Smoke, flash and impact use real world owners. Debris deliberately fails
// after the engine destruction mark, proving the Step cannot retire early or
// continue to rewards/alarmStop when its child adapter fails.
struct ScheduledBoxCounts {unsigned sound=0,smoke=0,flash=0,impact=0,mark=0,debris=0,stop=0;};
template<class World,class Mark> struct ScheduledBoxEffects {
    using Handle=downwell::InstanceHandle;
    using Real=downwell::MotionBinary64;
    World& world;Handle box;ScheduledBoxCounts& counts;Mark mark;
    bool verifyDebris=false,sawSound=false;
    downwell::GameMakerRandom before{0};
    std::function<bool(Handle,bool&)> timeQuery{};
    bool timeAllowed(bool& allowed){return alarmStop(0)&&timeQuery&&timeQuery(box,allowed);}
    bool soundOverlap(int,int,int,int,const char*){++counts.sound;sawSound=true;before=world.randomState();return true;}
    bool smoke(Real x,Real y,int,int){++counts.smoke;return world.emitBoxSmoke(x,y);}
    bool flash(Real x,Real y,int,int,int){++counts.flash;return world.emitBoxFlash(x,y);}
    bool effect(Real x,Real y,int sprite,Real speed,Real angle,int depth){
        ++counts.impact;
        return world.spawnBoxScriptEffect(box,x,y,sprite,speed,angle,Real::fromInteger(depth),
            [](int id,unsigned& count){assert(id==603);count=1;return true;});
    }
    bool chainReaction(){return false;}bool lowSpec(){return false;}int area(){return 1;}
    bool destroySelf(){++counts.mark;return mark(box);}
    bool create(int id,Real,Real,Handle&){assert(id==370&&counts.mark==1);++counts.debris;return false;}
    bool alarmStop(int n){
        assert(n==0);++counts.stop;
        if(verifyDebris&&sawSound){
            downwell::SmokeEffects<1> smoke;downwell::FlashBallEffects<1> flash;downwell::DebrisEffects<2> debris;
            const auto x=Real::fromInteger(240),y=Real::fromInteger(512);
            assert(smoke.boxBurst(x,Real::fromInteger(516),before)&&flash.boxBurst(x,y,before));
            assert(debris.create(x,y,1,before)&&debris.create(x,y,1,before));
            assert(before.drawCount()==world.randomState().drawCount()&&before.stateIndex()==world.randomState().stateIndex());
            for(unsigned i=0;i<16;++i)assert(before.stateWord(i)==world.randomState().stateWord(i));
            for(unsigned h=1;h<=2;++h){
                const auto* a=world.debrisEffects().retained(h);const auto* b=debris.retained(h);
                assert(a&&b&&a->x.bits==b->x.bits&&a->y.bits==b->y.bits&&
                    a->xsp.high==b->xsp.high&&a->xsp.low==b->xsp.low&&a->ysp.high==b->ysp.high&&a->ysp.low==b->ysp.low&&
                    a->imageIndex.bits==b->imageIndex.bits&&a->imageSpeed.bits==b->imageSpeed.bits);
            }
        }
        return true;
    }
    bool myBullet(Handle){return false;}bool bulletDirection(Handle,int){return false;}
    bool bulletDamage(Handle,int){return false;}bool sound(int,int,int,int){return false;}
    bool place(int,Real,Real,Handle&,bool&){return false;}bool target(Handle){return false;}
    bool targetAlarm(Handle,int,int){return false;}bool targetChecked(Handle,bool){return false;}
    bool currency(int){assert(false);return false;}
};
template<class World,class Mark>
ScheduledBoxEffects<World,Mark> scheduledBoxEffects(World& world,downwell::InstanceHandle box,
    ScheduledBoxCounts& counts,Mark mark){return {world,box,counts,mark};}
#endif

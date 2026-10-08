#ifndef DOWNWELL_TEST_SCHEDULED_BOX_EFFECTS_CHECK_H
#define DOWNWELL_TEST_SCHEDULED_BOX_EFFECTS_CHECK_H
#include "core/InstanceHandle.h"
#include "core/MotionBinary64.h"
#include <cassert>
// A scheduled failure-prefix check, not a complete destruction backend.
// Smoke, flash and impact use real world owners. Debris deliberately fails
// after the engine destruction mark, proving the Step cannot retire early or
// continue to rewards/alarmStop when its child adapter fails.
struct ScheduledBoxCounts {unsigned sound=0,smoke=0,flash=0,impact=0,mark=0,debris=0,stop=0;};
template<class World,class Mark> struct ScheduledBoxEffects {
    using Handle=downwell::InstanceHandle;
    using Real=downwell::MotionBinary64;
    World& world;Handle box;ScheduledBoxCounts& counts;Mark mark;
    bool soundOverlap(int,int,int,int,const char*){++counts.sound;return true;}
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
    bool alarmStop(int n){assert(n==0);++counts.stop;return true;}
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

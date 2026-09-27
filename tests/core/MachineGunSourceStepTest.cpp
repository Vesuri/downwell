#include "core/MachineGunSourceStep.h"
#ifdef DOWNWELL_NATIVE_QUERY_PROBE
struct Random {unsigned nextU32();};
struct Effects {
    using Handle=unsigned;
    bool create(int,downwell::MotionBinary64,downwell::MotionBinary64,Handle&);
    bool myFx(Handle);bool sprite(Handle,int);bool imageSpeed(Handle,downwell::MotionBinary64);
    bool imageAngle(Handle,downwell::MotionBinary64);bool depth(Handle,downwell::MotionBinary64);
};
struct Ops {
    bool publishAngle(downwell::RunnerImageValue);bool publishPosition(downwell::RunnerImageValue,downwell::RunnerImageValue);
    bool line(int,bool&);bool selectWall();bool approachSolid();bool wallParent(int&);
    bool wallHealth(downwell::MotionBinary64&);bool wallHealthWrite(downwell::MotionBinary64);bool wallHit();
    bool destroySelf();bool effect(int,downwell::MotionBinary64,downwell::MotionBinary64);
    bool soundWall();bool explosion(int);bool placeShootable(int,bool&);bool selectShootable(int);
    bool thingHealth(downwell::MotionBinary64&);bool thingHealthWrite(downwell::MotionBinary64);
    bool thingHit();bool thingHitEffect(bool&);
};
bool nativeSourceStep(downwell::MachineGunSourceMotion& m,Random& r,Effects& e,Ops& o,bool& ok){
    return downwell::MachineGunSourceStep::run(m,r,false,e,o,ok);
}
#else
#include <cassert>
#include <cstdio>
#include <vector>
using namespace downwell;
enum Event {Angle,Create,Caller,Sprite,Speed,EffectAngle,Depth,WallLine,EnemyLine,
    SelectWall,Point,ApproachY,ApproachX,Parent,Health,HealthWrite,Hit,Destroy,
    WallEffect,WallSound,Place,SelectThing,ThingHealth,ThingWrite,ThingHit,HitEffect,ThingEffect,MoveX,MoveY};
enum Route {Empty,Box,Stone,Enemy,Thing};
struct Fixture {
    MachineGunSourceMotion motion;
    Route route;bool high;unsigned fail,draws=0,queries=0,writes=0,finalWrites=0;
    bool approaching=false,destroyed=false;MotionBinary64 hp=MotionBinary64::fromInteger(1);
    std::vector<Event> events;
    Fixture(Route r,bool h,unsigned f):route(r),high(h),fail(f){
        assert(motion.reset(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512)));
    }
    bool event(Event e){events.push_back(e);return events.size()!=fail;}
    unsigned nextU32(){++draws;return 0x12345678u;}
    bool publishAngle(RunnerImageValue a){assert(!motion.initialized&&a.bits==motion.imageAngle.bits);return event(Angle);}
    using Handle=unsigned;
    bool create(int id,MotionBinary64 x,MotionBinary64 y,Handle& child){
        assert(!motion.initialized&&id==219&&x.high==motion.x.promoted().high&&y.high==motion.y.promoted().high);
        child=1;return event(Create);
    }
    bool myFx(Handle h){assert(h==1);return event(Caller);}
    bool sprite(Handle h,int id){assert(h==1&&id==(high?602:603));return event(Sprite);}
    bool imageSpeed(Handle,MotionBinary64 speed){assert(speed.high==0x3fe00000u&&!speed.low);return event(Speed);}
    bool imageAngle(Handle,MotionBinary64 angle){
        assert(angle.high==motion.direction.high&&angle.low==motion.direction.low);return event(EffectAngle);
    }
    bool depth(Handle,MotionBinary64 d){assert(d.zero()&&!motion.initialized);return event(Depth);}
    bool line(int object,bool& found){
        assert(motion.initialized);RunnerLineArguments args;assert(motion.lineArguments(args));
        assert(args.x1.bits==motion.x.bits&&args.y1.bits==motion.y.bits);
        assert(object==59||object==83);found=object==59?(route==Box||route==Stone||route==Enemy):route==Enemy;
        return event(object==59?WallLine:EnemyLine);
    }
    bool selectWall(){return event(SelectWall);}
    bool approachSolid(){
        approaching=true;unsigned count;
        const bool ok=motion.approachSolid([&](auto,auto,bool& hit){hit=queries++==2;return event(Point);},
            [&](auto x,auto y){return publishPosition(x,y);},count);
        if(ok){assert(count==2);approaching=false;}return ok;
    }
    bool wallParent(int& p){p=route==Box?84:59;return event(Parent);}
    bool wallHealth(MotionBinary64& value){value=hp;return event(Health);}
    bool wallHealthWrite(MotionBinary64 value){if(!event(HealthWrite))return false;hp=value;return true;}
    bool wallHit(){return event(Hit);}
    bool destroySelf(){if(!event(Destroy))return false;destroyed=true;return true;}
    bool effect(int sprite,MotionBinary64 speed,MotionBinary64 angle){
        if(route==Stone){assert(sprite==101&&speed.high==0x3fe00000u&&!speed.low&&angle.zero());return event(WallEffect);}
        assert(sprite==605&&speed.high==0x3fd99999u&&speed.low==0x9999999au&&
            angle.high==motion.direction.high&&angle.low==motion.direction.low);return event(ThingEffect);
    }
    bool soundWall(){return event(WallSound);}
    bool explosion(int){assert(false);return false;}
    bool placeShootable(int object,bool& hit){assert(object==90);hit=route==Thing;return event(Place);}
    bool selectShootable(int object){assert(object==90);return event(SelectThing);}
    bool thingHealth(MotionBinary64& value){value=hp;return event(ThingHealth);}
    bool thingHealthWrite(MotionBinary64 value){if(!event(ThingWrite))return false;hp=value;return true;}
    bool thingHit(){return event(ThingHit);}
    bool thingHitEffect(bool& value){value=true;return event(HitEffect);}
    bool publishPosition(RunnerImageValue x,RunnerImageValue y){
        if(approaching){
            if(writes%2==0)assert(x.bits==motion.x.bits);else assert(y.bits==motion.y.bits);
            return event(writes++%2==0?ApproachY:ApproachX);
        }
        if(route==Box||route==Stone||route==Thing)assert(destroyed);
        if(finalWrites%2==0)assert(y.bits==motion.y.bits);else assert(x.bits==motion.x.bits);
        return event(finalWrites++%2==0?MoveX:MoveY);
    }
};
int main(){
    unsigned failures=0;
    for(bool high:{false,true})for(Route route:{Empty,Box,Stone,Enemy,Thing}){
        std::vector<Event> expected{Angle,Create,Caller,Sprite,Speed,EffectAngle,Depth,WallLine};
        if(route==Box||route==Stone||route==Enemy)expected.push_back(EnemyLine);
        if(route==Box||route==Stone){
            for(auto e:{SelectWall,Point,ApproachY,ApproachX,Point,ApproachY,ApproachX,Point,Parent})expected.push_back(e);
            if(route==Box)for(auto e:{Health,HealthWrite,Hit,Destroy})expected.push_back(e);
            else for(auto e:{WallEffect,WallSound,Destroy})expected.push_back(e);
        }else{
            expected.push_back(Place);
            if(route==Thing)for(auto e:{SelectThing,ThingHealth,ThingWrite,ThingHit,HitEffect,ThingEffect,Destroy})expected.push_back(e);
        }
        expected.push_back(MoveX);expected.push_back(MoveY);
        Fixture f(route,high,0);bool ok=true;
        assert(MachineGunSourceStep::run(f.motion,f,high,f,f,ok)&&ok&&f.events==expected&&f.draws==1);
        if(route==Box||route==Thing){int hp;assert(f.hp.roundPixel(hp)&&hp==(high?-14:-8));}
        for(unsigned fail=1;fail<=expected.size();++fail){
            Fixture bad(route,high,fail);bool success=true;
            assert(!MachineGunSourceStep::run(bad.motion,bad,high,bad,bad,success)&&!success&&bad.draws==1&&
                bad.events==std::vector<Event>(expected.begin(),expected.begin()+fail));
            assert(bad.motion.initialized==(fail>7));
            const auto x=bad.motion.x.bits,y=bad.motion.y.bits;
            assert(!MachineGunSourceStep::run(bad.motion,bad,high,bad,bad,success)&&bad.draws==1&&
                bad.events.size()==fail&&bad.motion.x.bits==x&&bad.motion.y.bits==y);++failures;
        }
    }
    Fixture alive(Empty,false,0);bool ok=true;
    assert(MachineGunSourceStep::run(alive.motion,alive,false,alive,alive,ok));
    alive.events.clear();
    assert(MachineGunSourceStep::run(alive.motion,alive,false,alive,alive,ok)&&alive.draws==1&&
        alive.events==std::vector<Event>({WallLine,Place,MoveX,MoveY}));
    std::printf("Source bullet Step: ten routes, %u failure prefixes and second-Step initialization guard passed\n",failures);
}
#endif

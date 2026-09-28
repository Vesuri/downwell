#ifndef DOWNWELL_CORE_DRONE_BULLET_SOLID_VERIFICATION_H
#define DOWNWELL_CORE_DRONE_BULLET_SOLID_VERIFICATION_H
#include "DroneBulletVerification.h"
namespace downwell {
// Ordered event composition tape. Resource-backed selection remains the live
// world's responsibility; this exercises the real approach and solid scripts.
struct DroneBulletSolidVerification {
    enum Event {Angle,WallLine,EnemyLine,Select,Approach,Point,Move,Parent,Health,
        Write,Hit,Destroy,Effect,Sound,Place,SelectThing,HitEffect};
    struct Ops {
        DroneBulletSourceStep& shot;
        unsigned route,fail=0,count=0,points=0;
        Event events[40]{};
        MotionBinary64 hp=MotionBinary64::fromInteger(1);
        bool destroyed=false,valid=true;
        bool call(Event e){if(count>=40)return false;events[count++]=e;return count!=fail;}
        bool publishAngle(RunnerImageValue v){valid=valid&&v.bits==0x42b40000u;return valid&&call(Angle);}
        bool publishImageSpeed(RunnerImageValue){return false;}
        bool publishPosition(RunnerImageValue x,RunnerImageValue y){
            // Each setter publishes one component while retaining the other's
            // previous value, never a simultaneous translated position pair.
            valid=valid&&(x.bits==shot.motion.x.bits||y.bits==shot.motion.y.bits);
            return valid&&call(Move);
        }
        bool line(int object,bool& found){
            valid=valid&&(object==59||object==83);
            found=object==59?route!=0:route==3;
            return valid&&call(object==59?WallLine:EnemyLine);
        }
        bool selectWall(){return call(Select);}
        bool approachSolid(){
            if(!call(Approach))return false;
            unsigned iterations=0;
            return shot.motion.approachSolid([&](auto,auto,bool& hit){hit=++points==2;return call(Point);},
                [&](auto x,auto y){return publishPosition(x,y);},iterations);
        }
        bool wallParent(int& parent){parent=route==2?59:84;return call(Parent);}
        bool wallHealth(MotionBinary64& health){health=hp;return call(Health);}
        bool wallHealthWrite(MotionBinary64 health){if(!call(Write))return false;hp=health;return true;}
        bool wallHit(){return call(Hit);}
        bool destroySelf(){if(!call(Destroy))return false;destroyed=true;return true;}
        bool effect(int sprite,MotionBinary64 rate,MotionBinary64 angle){
            valid=valid&&(route==2?(sprite==101&&rate.high==0x3fe00000u&&!rate.low&&angle.zero()):
                (sprite==605&&rate.high==0x3fd99999u&&rate.low==0x9999999au&&angle.high==0x40568000u&&!angle.low));
            return valid&&call(Effect);
        }
        bool soundWall(){return call(Sound);}
        bool explosion(int){return false;}
        bool placeShootable(int object,bool& hit){hit=route==3;return object==90&&call(Place);}
        bool selectShootable(int object){return object==90&&call(SelectThing);}
        bool thingHealth(MotionBinary64& health){return wallHealth(health);}
        bool thingHealthWrite(MotionBinary64 health){return wallHealthWrite(health);}
        bool thingHit(){return wallHit();}
        bool thingHitEffect(bool& yes){yes=true;return call(HitEffect);}
    };
    static bool run(bool failures=false){
        for(unsigned route=0;route<5;++route)for(unsigned powered=0;powered<2;++powered){
            DroneBulletSourceStep shot;DroneBulletVerification::Random random;bool good=true;
            if(!shot.reset(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512)))return false;
            shot.motion.direction=MotionBinary64::fromInteger(90);
            shot.damage=MotionBinary64::fromInteger(powered?20:4);
            Ops ops{shot,route};if(route==4)ops.hp={};
            if(!shot.stepWithSolid(random,ops,good)||!good||!ops.valid||random.draws!=1)return false;
            Event expected[40];unsigned n=0;
            auto add=[&](Event e){expected[n++]=e;};
            add(Angle);add(Angle);add(WallLine);
            if(route)add(EnemyLine);
            if(route&&route!=3){
                add(Select);add(Approach);add(Point);add(Move);add(Move);add(Point);add(Parent);
                if(route==2){add(Effect);add(Sound);add(Destroy);}
                else{add(Health);if(route!=4){add(Write);add(Hit);add(Destroy);}}
            }else{
                add(Place);
                if(route==3){add(SelectThing);add(Health);add(Write);add(Hit);add(HitEffect);add(Effect);add(Destroy);}
            }
            add(Move);add(Move);
            if(ops.count!=n||ops.destroyed!=(route>0&&route<4))return false;
            for(unsigned i=0;i<n;++i)if(ops.events[i]!=expected[i])return false;
            if(route==1||route==3){
                const auto hp=MotionBinary64::fromInteger(powered?-19:-3);
                if(ops.hp.high!=hp.high||ops.hp.low!=hp.low)return false;
            }
            if(!failures)continue;
            for(unsigned fail=1;fail<=n;++fail){
                DroneBulletSourceStep failed;DroneBulletVerification::Random rng;bool okay=true;
                if(!failed.reset(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(512)))return false;
                failed.motion.direction=MotionBinary64::fromInteger(90);
                failed.damage=MotionBinary64::fromInteger(powered?20:4);
                Ops rejected{failed,route};rejected.fail=fail;if(route==4)rejected.hp={};
                if(failed.stepWithSolid(rng,rejected,okay)||okay||rejected.count!=fail)return false;
                for(unsigned i=0;i<fail;++i)if(rejected.events[i]!=expected[i])return false;
                if(failed.stepWithSolid(rng,rejected,okay)||rejected.count!=fail||rng.draws!=1)return false;
            }
        }
        return true;
    }
};
}
#endif

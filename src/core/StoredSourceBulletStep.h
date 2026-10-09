#ifndef DOWNWELL_CORE_STORED_SOURCE_BULLET_STEP_H
#define DOWNWELL_CORE_STORED_SOURCE_BULLET_STEP_H
#include "StoredWorldCollision.h"
namespace downwell {
// Stored counterpart of NativeSourceBulletStep. Shootables is the owning
// registration (or mixed registration dispatcher), never copied actor fields.
struct StoredSourceBulletStep {
    template<class Shootables,class Frames,class Audio>
    static bool run(StartupRuntime& world,StoredWorldCollision& owner,InstanceHandle shot,
        MachineGunSourceMotion& motion,Shootables& shootables,Frames frames,
        SoundPlayEnvironment& environment,Audio& audio){
        struct Ops {
            StartupRuntime& world;StoredWorldCollision& owner;InstanceHandle shot;
            MachineGunSourceMotion& motion;Shootables& shootables;
            SoundPlayEnvironment& environment;Audio& audio;InstanceHandle selected{};
            bool publishAngle(RunnerImageValue angle){return world.publishBulletAngle(shot,angle);}
            bool publishPosition(RunnerImageValue x,RunnerImageValue y){return world.publishBulletCollisionPosition(shot,x,y);}
            bool query(int object,InstancePlaceResult& result){MotionBinary64 x,y;
                return motion.endpoint(x,y)&&owner.nonPreciseLine(shot,object,
                    motion.x.promoted(),motion.y.promoted(),x,y,false,result);}
            bool line(int object,bool& hit){InstancePlaceResult result;
                if(!query(object,result))return false;hit=result.id!=-4;return true;}
            bool selectWall(){InstancePlaceResult result;
                if(!query(59,result)||result.id==-4)return false;selected=result.handle;return true;}
            bool approachSolid(){unsigned iterations;
                return motion.approachSolid([&](MotionBinary64 x,MotionBinary64 y,bool& hit){
                    InstancePlaceResult result;
                    if(!owner.collisionPoint(shot,59,x,y,false,false,result))return false;
                    hit=result.id!=-4;return true;
                },[&](RunnerImageValue x,RunnerImageValue y){return publishPosition(x,y);},iterations);
            }
            bool wallParent(int& parent){return owner.projectileWallParent(selected,parent);}
            bool wallHealth(MotionBinary64& hp){return owner.projectileBoxHealth(selected,hp);}
            bool wallHealthWrite(MotionBinary64 hp){return owner.projectileBoxHealthWrite(world,selected,hp);}
            bool wallHit(){return owner.projectileBoxHit(world,selected);}
            bool destroySelf(){return world.markBulletDestroyed(shot);}
            bool soundWall(){return world.playBulletWallSound(shot,environment,
                [&](const char*& material){return owner.projectileWallMaterial(selected,material);},audio);}
            bool explosion(int){return false;}
            bool placeShootable(int object,bool& hit){const auto* caller=owner.resolve(shot);
                return object==90&&caller&&owner.placeMeeting(shot,*caller,object,
                    motion.x.promoted(),motion.y.promoted(),hit);}
            bool selectShootable(int object){const auto* caller=owner.resolve(shot);InstancePlaceResult result;
                if(object!=90||!caller||!owner.instancePlace(shot,*caller,object,motion.x.promoted(),motion.y.promoted(),result)||result.id==-4)return false;
                selected=result.handle;return true;}
            bool thingHealth(MotionBinary64& hp){bool effect;return owner.resolve(selected)&&shootables.readShootable(selected,hp,effect);}
            bool thingHealthWrite(MotionBinary64 hp){return owner.resolve(selected)&&shootables.writeShootableHealth(selected,hp);}
            bool thingHit(){return owner.resolve(selected)&&shootables.writeShootableHit(selected);}
            bool thingHitEffect(bool& effect){MotionBinary64 hp;return owner.resolve(selected)&&shootables.readShootable(selected,hp,effect);}
        } ops{world,owner,shot,motion,shootables,environment,audio};
        static_assert(!machineGunProfile(false).explode&&!machineGunProfile(true).explode,
            "explosive profiles require their synchronous explosion binding");
        if(!owner.boundTo(world))return world.stepSourceBullet({},motion,ops,frames);
        return world.stepSourceBullet(shot,motion,ops,frames);
    }
};
}
#endif

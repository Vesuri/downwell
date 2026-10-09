#ifndef DOWNWELL_CORE_NATIVE_SOURCE_BULLET_STEP_H
#define DOWNWELL_CORE_NATIVE_SOURCE_BULLET_STEP_H
#include "GameplayCollisionOwner.h"
namespace downwell {
// Concrete resource-owner binding for the usual gun. Selected identities are
// retained across synchronous child Create, but every field access re-resolves
// its generation. Audio is supplied by the enclosing platform/session.
struct NativeSourceBulletStep {
    template<class Frames,class Audio>
    static bool run(GameplayWorld& world,GameplayCollisionOwner& owner,
        InstanceHandle shot,MachineGunSourceMotion& motion,const SpriteMaskPack& masks,
        Frames frames,SoundPlayEnvironment& environment,Audio& audio){
        struct Ops {
            GameplayWorld& world;GameplayCollisionOwner& owner;InstanceHandle shot;
            MachineGunSourceMotion& motion;const SpriteMaskPack& masks;
            SoundPlayEnvironment& environment;Audio& audio;
            InstanceHandle self{},selected{};
            bool publishAngle(RunnerImageValue angle){return world.publishBulletAngle(shot,angle);}
            bool publishPosition(RunnerImageValue x,RunnerImageValue y){return world.publishBulletCollisionPosition(shot,x,y);}
            bool query(int object,GameplayCollisionOwner::Owner::Result& result){
                MotionBinary64 x,y;
                return motion.endpoint(x,y)&&owner.projectileNonPreciseLine(self,object,
                    motion.x.promoted(),motion.y.promoted(),x,y,false,result);
            }
            bool line(int object,bool& hit){GameplayCollisionOwner::Owner::Result result;
                if(!query(object,result))return false;hit=result.found;return true;}
            bool selectWall(){GameplayCollisionOwner::Owner::Result result;
                if(!query(59,result)||!result.found)return false;selected=result.handle;return true;}
            bool approachSolid(){unsigned iterations;
                return motion.approachSolid([&](MotionBinary64 x,MotionBinary64 y,bool& hit){
                    GameplayCollisionOwner::Owner::Result result;
                    if(!owner.projectileSolidPoint(self,x,y,false,masks,result))return false;
                    hit=result.found;return true;
                },[&](RunnerImageValue x,RunnerImageValue y){return publishPosition(x,y);},iterations);
            }
            bool wallParent(int& parent){return owner.projectileWallParent(selected,parent);}
            bool wallHealth(MotionBinary64& hp){return owner.projectileBoxHealth(selected,hp);}
            bool wallHealthWrite(MotionBinary64 hp){return owner.projectileBoxHealthWrite(selected,hp);}
            bool wallHit(){return owner.projectileBoxHit(selected);}
            bool destroySelf(){return world.markBulletDestroyed(shot);}
            bool soundWall(){return world.playBulletWallSound(shot,environment,
                [&](const char*& material){return owner.projectileWallMaterial(selected,material);},audio);}
            bool explosion(int){return false;} // Both admitted usual-gun profiles have explode=false.
            bool placeShootable(int object,bool& hit){return object==90&&owner.projectileShootableMeeting(
                self,motion.x.promoted(),motion.y.promoted(),masks,hit);}
            bool selectShootable(int object){GameplayCollisionOwner::Owner::Result result;
                if(object!=90||!owner.projectileShootableSelection(self,motion.x.promoted(),motion.y.promoted(),masks,result)||!result.found)return false;
                selected=result.handle;return true;}
            bool thingHealth(MotionBinary64& hp){bool effect;return owner.projectileShootableFields(selected,hp,effect);}
            bool thingHealthWrite(MotionBinary64 hp){return owner.projectileShootableHealthWrite(selected,hp);}
            bool thingHit(){return owner.projectileShootableHit(selected);}
            bool thingHitEffect(bool& effect){MotionBinary64 hp;return owner.projectileShootableFields(selected,hp,effect);}
        } ops{world,owner,shot,motion,masks,environment,audio};
        static_assert(!machineGunProfile(false).explode&&!machineGunProfile(true).explode,
            "explosive profiles require their synchronous explosion binding");
        // Invalid keys still go through the world's failure latch.
        if(!owner.boundTo(world)||!masks.valid()||
            !GameplayCollisionView::encode(GameplayCollisionView::Kind::bullet,shot,ops.self))
            return world.stepSourceBullet({},motion,ops,frames);
        return world.stepSourceBullet(shot,motion,ops,frames);
    }
};
}
#endif

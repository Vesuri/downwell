#ifndef DOWNWELL_CORE_REGISTERED_SKELETON_THROW_H
#define DOWNWELL_CORE_REGISTERED_SKELETON_THROW_H
#include "BoneMotion.h"
#include "InstanceLifecycle.h"
namespace downwell {
// enmSkeleton Step blocks 21–26. Retain caller variables before callbacks;
// a failed port operation is terminal, not permission to replay child Create.
struct RegisteredSkeletonThrow {
    InstanceHandle myBone{};
    unsigned myBoneSourceId=0;
    MotionBinary64 yarc{},xarc{};
    template<class Create,class Edit,class Random,class Sound>
    bool run(int x,int y,bool separated,int facing,bool& shot,bool& good,
             Create create,Edit edit,Random& random,Sound sound){
        if(!good)return false;
        if(y<(-2147483647-1)+10||facing< -1||facing>1)return good=false;
        if(!create(x,y-10,myBone)||!good)return good=false;
        if(!sound(134)||!good)return good=false;
        const MotionBinary64 vertical[]={{0xc0080000u,0},{0xc008ccccu,0xcccccccdu},{0xc0099999u,0x9999999au}};
        yarc=vertical[random.chooseIndexSmall(3)];
        const int horizontal[]={10,8,12};
        const int vx=separated?facing*horizontal[random.chooseIndexSmall(3)]:10;
        if(!BoneMotion::horizontal(vx,xarc))return good=false;
        const bool applied=edit(myBone,[&](auto& bone,auto&,bool&){
            bone.motion.ysp=yarc;bone.motion.xsp=xarc;
            if(vx>0)bone.sampledAnimationRate.high^=0x80000000u;
            return true;
        });
        if(!applied||!good)return good=false;
        shot=true;return true;
    }
};
}
#endif

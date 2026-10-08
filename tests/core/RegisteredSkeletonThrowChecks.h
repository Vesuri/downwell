#ifndef DOWNWELL_TEST_REGISTERED_SKELETON_THROW_CHECKS_H
#define DOWNWELL_TEST_REGISTERED_SKELETON_THROW_CHECKS_H
#include "core/BoneProjectile.h"
#include "core/RegisteredSkeletonThrow.h"
#include <cstdlib>
// Exercise the real world allocator/RNG and registered child, not a fake Store.
template<class World,class Throw,class Find,class Clear>
void checkRegisteredSkeletonThrows(World& world,Throw fire,Find find,Clear clear){
    const auto check=[](bool ok){if(!ok)std::abort();};
    const auto same=[](downwell::MotionBinary64 a,downwell::MotionBinary64 b){return a.high==b.high&&a.low==b.low;};
    for(bool separated:{false,true})for(int facing:{-1,0,1}){
        auto random=world.randomState();downwell::BoneProjectile expected;
        check(expected.create(240,190,random));
        const auto createRandom=random;
        const int vy=-30-int(random.chooseIndexSmall(3));
        const int horizontal[]={10,8,12};
        const int vx=separated?facing*horizontal[random.chooseIndexSmall(3)]:10;
        check(expected.motion.launch(240,190,vx,vy));
        if(vx>0)expected.sampledAnimationRate.high^=0x80000000u;
        downwell::RegisteredSkeletonThrow caller;bool shot=false;unsigned sounds=0;
        check(fire(caller,separated,facing,shot,[&](int id){
            ++sounds;const auto* child=find(caller.myBone);
            check(id==134&&child&&caller.myBoneSourceId==child->sourceId&&
                  child->state.imageSpeed.bits==0x3f800000u&&!shot&&
                  world.randomDrawCount()==createRandom.drawCount());
            return true;
        }));
        const auto* child=find(caller.myBone);
        check(child&&shot&&sounds==1&&child->state.motion.x==240&&child->state.motion.y==190&&
              same(child->state.motion.xsp,expected.motion.xsp)&&same(child->state.motion.ysp,expected.motion.ysp)&&
              same(child->state.sampledAnimationRate,expected.sampledAnimationRate)&&
              child->state.imageSpeed.bits==0x3f800000u&&same(caller.xarc,expected.motion.xsp)&&same(caller.yarc,expected.motion.ysp)&&
              world.randomDrawCount()==random.drawCount());
        for(unsigned i=0;i<16;++i)check(world.randomState().stateWord(i)==random.stateWord(i));
        check(clear());
    }
    downwell::RegisteredSkeletonThrow caller;bool shot=false;unsigned sounds=0;
    const auto before=world.randomDrawCount();
    const auto reject=[&](int){++sounds;check(find(caller.myBone)!=nullptr);return false;};
    check(!fire(caller,true,1,shot,reject)&&!world.effectsSucceeded()&&!shot&&sounds==1&&
          world.randomDrawCount()==before+3&&find(caller.myBone));
    check(!fire(caller,true,1,shot,reject)&&sounds==1&&world.randomDrawCount()==before+3);
    check(clear());
}
#endif

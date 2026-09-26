#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_VERIFICATION_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_VERIFICATION_H
#include "RunnerSpatialInstances.h"
namespace downwell {
// CPU-independent mutation tape. Expected flags/queue membership are checked
// separately from the digest; equal host/target failures must never pass.
// Does not claim live scheduler integration or original-runner execution.
struct RunnerSpatialVerification {
    unsigned checks=0,failures=0;
    std::uint32_t digest=2166136261u;
    void check(bool value){++checks;if(!value)++failures;}
    void word(std::uint32_t value){digest=(digest^value)*16777619u;}
};
inline RunnerSpatialVerification verifyRunnerSpatialMutations(){
    RunnerSpatialVerification result;
    using Owner=RunnerSpatialInstances<2,8>;
    const InstanceHandle h{0,1};
    const std::uint32_t values[]={0u,0x80000000u,0x3f800000u,0x40000000u,0xbf800000u};
    for(unsigned axis=0;axis<3;++axis)for(auto bits:values)
    for(unsigned tinted=0;tinted<2;++tinted)for(unsigned faded=0;faded<2;++faded){
        Owner owner;Owner::State clean;clean.flags=0;
        result.check(owner.attach(h,1,clean));
        Owner::Sprite sprite;
        auto& target=axis==0?sprite.scaleX:axis==1?sprite.scaleY:sprite.angle;
        target={0x40000000u};
        const auto field=axis==0?Owner::TransformField::scaleX:
            axis==1?Owner::TransformField::scaleY:Owner::TransformField::angle;
        const std::uint32_t alpha=faded?0x3f000000u:0x3f800000u;
        result.check(owner.changeTransform(h,sprite,field,{bits},{alpha},tinted?0xff0000u:0xffffffu,0x38));
        const bool changed=bits!=0x40000000u;
        const bool fast=sprite.scaleX.bits==0x3f800000u&&sprite.scaleY.bits==0x3f800000u&&
            !(sprite.angle.bits&0x7fffffffu)&&!tinted&&!faded;
        const auto* state=owner.resolve(h);
        result.check(state&&target.bits==bits&&state->flags==(changed?(8u|(fast?0x4000u:0u)):0u));
        result.check(owner.pendingSpatial()==unsigned(changed)&&owner.pendingCollision()==unsigned(changed));
        result.word(target.bits);result.word(state?state->flags:0xffffffffu);
        result.word(owner.pendingSpatial());result.word(owner.pendingCollision());
    }
    for(unsigned objectFlags=0;objectFlags<256;++objectFlags)for(unsigned prepared=0;prepared<2;++prepared){
        Owner owner;Owner::State state;state.flags=0x200070u;
        result.check(owner.attach(h,1,state));
        const Owner::Box box{};
        result.check(owner.refresh(h,0x38,false,&box,false,true));
        result.check(owner.move(h,{0x3f800000u},{},0x38));
        unsigned expected=owner.resolve(h)->flags;
        const bool remove=!(objectFlags&1u)&&!prepared&&!(objectFlags&0x28u);
        if(objectFlags&1u)expected|=0x28u;else expected&=~0x20u;
        if(remove)expected&=~0x8000u;
        expected=(expected&~0x50u)|((objectFlags&2u)?0x10u:0u)|((objectFlags&4u)?0x40u:0u);
        unsigned callbacks=0;
        result.check(owner.applyBindingProperties(h,objectFlags,prepared,[&](InstanceHandle){
            ++callbacks;
            result.check(owner.resolve(h)->flags==expected);
            result.check(owner.pendingSpatial()==(remove?0u:1u));
            return true;
        }));
        expected=(expected&~0x200000u)|((objectFlags&0x80u)?0x200000u:0u)|8u;
        result.check(callbacks==1&&owner.resolve(h)->flags==expected);
        result.check(owner.pendingCollision()==(remove?0u:1u));
        result.word(owner.resolve(h)->flags);result.word(owner.pendingSpatial());result.word(owner.pendingCollision());
        result.check(owner.retire(h)&&!owner.resolve(h)&&!owner.pendingSpatial()&&!owner.pendingCollision());
        result.check(owner.attach({0,2},1,{})&&!owner.resolve(h));
    }
    return result;
}
}
#endif

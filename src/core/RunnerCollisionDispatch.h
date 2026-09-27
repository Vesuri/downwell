#ifndef DOWNWELL_CORE_RUNNER_COLLISION_DISPATCH_H
#define DOWNWELL_CORE_RUNNER_COLLISION_DISPATCH_H
#include "InstanceHandle.h"
namespace downwell {
// 0x5030d0. Ops owns source-faithful overlap, field access, queued membership,
// position/path rollback, event context and movement arithmetic. All methods
// are checked. No lifetime may be physically retired until the pass ends.
// dispatch must apply the runner's room-transition guards and look up the
// current object IDs separately for each direction, even after destruction.
class RunnerCollisionDispatch {
public:
    bool succeeded()const{return good_;}
    template<class Ops> bool pair(InstanceHandle self,InstanceHandle other,Ops& ops){
        if(!good_)return false;
        bool hit=false;
        if(!check(ops.overlap(self,other,hit)))return false;
        if(!hit)return true;
        std::uint32_t flags;
        if(!check(ops.flags(self,flags)))return false;
        if(flags&2u)return true;
        if(!check(ops.flags(other,flags)))return false;
        if(flags&2u)return true;
        if(!check(ops.queueCollision(other)))return false;
        bool solid=false;
        if(!solidPair(self,other,ops,solid))return false;
        if(solid&&!rollback(self,other,ops))return false;
        if(!check(ops.dispatch(self,other))||!check(ops.dispatch(other,self)))return false;
        // Do not cache the pre-event solid flags or add a new exclusion test.
        if(!solidPair(self,other,ops,solid))return false;
        if(!solid)return true;
        if(!check(ops.prepareMotion(self))||!check(ops.prepareMotion(other))||
           !check(ops.advanceMotion(self))||!check(ops.advanceMotion(other)))return false;
        if(!check(ops.overlap(self,other,hit)))return false;
        return !hit||rollback(self,other,ops);
    }
private:
    bool good_=true;
    bool check(bool ok){if(!ok)good_=false;return ok;}
    template<class Ops> bool solidPair(InstanceHandle a,InstanceHandle b,Ops& ops,bool& solid){
        std::uint32_t flags;
        if(!check(ops.flags(a,flags)))return false;
        if(flags&0x20u){solid=true;return true;}
        if(!check(ops.flags(b,flags)))return false;
        solid=(flags&0x20u)!=0;return true;
    }
    template<class Ops> bool rollback(InstanceHandle a,InstanceHandle b,Ops& ops){
        return check(ops.rollback(a))&&check(ops.rollback(b));
    }
};
}
#endif

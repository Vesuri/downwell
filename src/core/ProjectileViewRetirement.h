#ifndef DOWNWELL_PROJECTILE_VIEW_RETIREMENT_H
#define DOWNWELL_PROJECTILE_VIEW_RETIREMENT_H
#include "RunnerRealComparison.h"
namespace downwell {
// Source-ordered scrOutofview followed by scrBulletOov. Read published values
// afresh after synchronous destruction: marking self does not abort End Step.
struct ProjectileViewRetirement {
    template<class Ops> static bool run(Ops& ops,bool& good){
        if(!good)return false;
        const auto check=[&](bool ok){return good=good&&ok;};
        MotionBinary64 y,view,height;int order;
        if(!check(ops.readY(y))||!check(ops.viewY(view))||
            !check(view.add(MotionBinary64::fromInteger(-160)))||
            !check(compareRunnerReal(y,view,order)))return false;
        bool outside=order<0;
        if(!outside){
            if(!check(ops.readY(y))||!check(ops.viewY(view))||
                !check(view.add(MotionBinary64::fromInteger(3000)))||
                !check(compareRunnerReal(y,view,order)))return false;
            outside=order>0;
        }
        if(outside){
            bool main=false;
            if(!check(ops.mainRoom(main)))return false;
            if(main&&!check(ops.destroySelf()))return false;
        }
        // Both bullet checks execute in every room and are independent ifs.
        if(!check(ops.readY(y))||!check(ops.viewY(view))||!check(ops.viewHeight(height))||
            !check(view.add(height))||!check(view.add(MotionBinary64::fromInteger(32)))||
            !check(compareRunnerReal(y,view,order)))return false;
        if(order>0&&!check(ops.destroySelf()))return false;
        if(!check(ops.readY(y))||!check(ops.viewY(view))||
            !check(view.add(MotionBinary64::fromInteger(-100)))||
            !check(compareRunnerReal(y,view,order)))return false;
        if(order<0&&!check(ops.destroySelf()))return false;
        return good;
    }
};
}
#endif

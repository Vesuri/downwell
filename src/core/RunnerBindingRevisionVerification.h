#ifndef DOWNWELL_RUNNER_BINDING_REVISION_VERIFICATION_H
#define DOWNWELL_RUNNER_BINDING_REVISION_VERIFICATION_H
#include "RunnerSpatialInstances.h"
namespace downwell {
struct RunnerBindingRevisionVerification {
    static bool run(){
        RunnerSpatialInstances<1,2> owner;
        auto revision=owner.bindingRevision();
        const InstanceHandle first{0,1},second{0,2};
        if(!owner.bindingsUnchanged(revision)||owner.bindingsUnchanged(0xffffffffu)||
            !owner.attach(first,0,{})||owner.bindingsUnchanged(revision))return false;
        revision=owner.bindingRevision();
        if(!owner.setExclusions(first,2)||!owner.bindingsUnchanged(revision)||
            !owner.rebindObjectMembership(first,1,false)||owner.bindingsUnchanged(revision))return false;
        revision=owner.bindingRevision();
        if(!owner.retire(first)||owner.bindingsUnchanged(revision))return false;
        revision=owner.bindingRevision();
        if(!owner.attach(second,0,{})||owner.bindingsUnchanged(revision))return false;
        revision=owner.bindingRevision();owner.clear();
        return !owner.bindingsUnchanged(revision)&&!owner.resolve(first)&&!owner.resolve(second);
    }
};
}
#endif

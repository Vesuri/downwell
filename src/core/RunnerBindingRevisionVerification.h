#ifndef DOWNWELL_RUNNER_BINDING_REVISION_VERIFICATION_H
#define DOWNWELL_RUNNER_BINDING_REVISION_VERIFICATION_H
#include "RunnerSpatialInstances.h"
namespace downwell {
struct RunnerBindingRevisionVerification {
    static bool run(){
        {
            RunnerSpatialInstances<1,2> capture;
            const InstanceHandle h{0,1};
            auto revision=capture.captureRevision();
            if(!capture.captureUnchanged(revision)||capture.captureUnchanged(0xffffffffu)||
               !capture.attach(h,0,{})||capture.captureUnchanged(revision))return false;
            revision=capture.captureRevision();
            if(!capture.move(h,{0x3f800000u},{},0)||!capture.captureUnchanged(revision)||
               !capture.setActive(h,true,0)||!capture.captureUnchanged(revision))return false;
            if(!capture.setActive(h,false,0)||capture.captureUnchanged(revision))return false;
            revision=capture.captureRevision();
            if(!capture.setActive(h,false,0)||!capture.captureUnchanged(revision)||
               !capture.setActive(h,true,0)||capture.captureUnchanged(revision))return false;
            for(unsigned flag=1;flag<=0x100000u;flag=flag==1?2:flag==2?0x100000u:0x200000u){
                revision=capture.captureRevision();
                if(!capture.setExclusions(h,flag)||capture.captureUnchanged(revision))return false;
                revision=capture.captureRevision();
                if(!capture.setExclusions(h,flag)||!capture.captureUnchanged(revision))return false;
            }
            for(unsigned registered=0;registered<2;++registered){
                revision=capture.captureRevision();
                if(!capture.registerObject(h,registered)||capture.captureUnchanged(revision))return false;
                revision=capture.captureRevision();
                if(!capture.registerObject(h,registered)||!capture.captureUnchanged(revision))return false;
            }
            revision=capture.captureRevision();
            if(!capture.rebindObjectMembership(h,0,true)||capture.captureUnchanged(revision))return false;
            revision=capture.captureRevision();
            if(!capture.retire(h)||capture.captureUnchanged(revision))return false;
            revision=capture.captureRevision();capture.clear();
            if(capture.captureUnchanged(revision))return false;
        }
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

#ifndef DOWNWELL_CORE_RUNNER_COLLISION_EVENT_CONTEXT_H
#define DOWNWELL_CORE_RUNNER_COLLISION_EVENT_CONTEXT_H
#include "InstanceHandle.h"
#include "ObjectPack.h"
namespace downwell {
// Collision specialization of 0x4449e0. For event type 4, no room or any
// pending room change suppresses execution, even for persistent instances.
// Lookup returns the actual other object ID separately from the inherited
// binding. Lifetime retirement is deferred throughout paired dispatch.
class RunnerCollisionEventContext {
public:
    struct Frame {
        InstanceHandle self{},other{};
        int type=4,subtype=-1,owner=-1,code=-1;
    };
    struct RoomState {bool present=false,pendingChange=false;};
    bool succeeded()const{return good_;}
    unsigned depth()const{return depth_;}
    const Frame* current()const{return depth_?&current_:nullptr;}
    template<class Lookup,class Execute>
    bool dispatch(InstanceHandle self,InstanceHandle other,RoomState room,Lookup lookup,Execute execute){
        if(!good_)return false;
        if(!room.present||room.pendingChange)return true;
        bool found=false;int subtype=-1;PackedObjectEvent event{};
        if(!check(lookup(self,other,subtype,found,event)))return false;
        if(!found)return true;
        // Source reports a recursion error above 32. The port fails loudly
        // rather than silently omitting the handler and continuing gameplay.
        if(depth_==32||subtype<0||event.eventType!=4||event.ownerObjectId<0||event.codeId<0)return check(false);
        const Frame previous=current_;
        current_={self,other,4,subtype,event.ownerObjectId,event.codeId};++depth_;
        const bool ok=execute(current_);
        --depth_;current_=previous; // Also restore when execution failed.
        return check(ok)&&good_;
    }
private:
    bool check(bool ok){if(!ok)good_=false;return ok;}
    Frame current_{};unsigned depth_=0;bool good_=true;
};
}
#endif

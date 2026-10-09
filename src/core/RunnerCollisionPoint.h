#ifndef DOWNWELL_RUNNER_COLLISION_POINT_H
#define DOWNWELL_RUNNER_COLLISION_POINT_H
#include "InstanceHandle.h"
#include "RunnerImageValue.h"
namespace downwell {
// Object-resource collision_point (0x4fd79c): registration order, no family
// preparation or global bounds flush. Candidate owns its lazy refresh.
struct RunnerCollisionPoint {
    struct Result {bool found=false;InstanceHandle handle{};};
    template<class Members,class Flags,class Candidate>
    static bool run(InstanceHandle self,bool notMe,RunnerImageValue x,RunnerImageValue y,
        Members members,Flags flags,Candidate candidate,Result& result){
        Result next;bool good=true;
        const bool enumerated=members([&](InstanceHandle h){
            std::uint32_t value;
            if(!flags(h,value)){good=false;return false;}
            if((value&0x100003u)||(notMe&&h.slot==self.slot&&h.generation==self.generation))return true;
            bool hit=false;if(!candidate(h,x,y,hit)){good=false;return false;}
            if(hit){next={true,h};return false;}return true;
        });
        if(!enumerated||!good)return false;
        result=next;return true;
    }
};
}
#endif

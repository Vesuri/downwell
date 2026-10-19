#ifndef DOWNWELL_CORE_PREPARED_ACTOR_BOUNDS_TRIG_H
#define DOWNWELL_CORE_PREPARED_ACTOR_BOUNDS_TRIG_H
#include "RunnerImageValue.h"
namespace downwell {
// enmCrawler Step publishes movedir or movedir+180, without normalizing
// 360/450. These are SmokeDirection::radians -> smokeTrig -> binary32, not
// ideal quadrant values. SmokeTrigTest checks every bit against that oracle.
// Other angles retain the complete runtime program in the bounds caller.
struct PreparedActorBoundsTrig {
    static bool at(RunnerImageValue angle,RunnerImageValue& cosine,RunnerImageValue& sine){
        unsigned c,s;
        switch(angle.bits){
            case 0x00000000u:c=0x3f800000u;s=0x00000000u;break;
            case 0x42b40000u:c=0xb33bbd2eu;s=0x3f800000u;break;
            case 0x43340000u:c=0xbf800000u;s=0xb3bbbd2eu;break;
            case 0x43870000u:c=0x324cde2eu;s=0xbf800000u;break;
            case 0x43b40000u:c=0x3f800000u;s=0x343bbd2eu;break;
            case 0x43e10000u:c=0x34155386u;s=0x3f800000u;break;
            default:return false;
        }
        cosine.bits=c;sine.bits=s;return true;
    }
};
}
#endif

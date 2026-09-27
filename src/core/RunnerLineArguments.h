#ifndef DOWNWELL_CORE_RUNNER_LINE_ARGUMENTS_H
#define DOWNWELL_CORE_RUNNER_LINE_ARGUMENTS_H
#include "RunnerImageValue.h"
namespace downwell {
// collision_line VM wrapper 0x528ba0 stores each of its first four arguments
// as binary32 before dispatching 0x4fd370. Do not pass the VM's unrounded
// x+xsp/y+ysp directly to the spatial traversal or candidate geometry.
struct RunnerLineArguments {
    RunnerImageValue x1{},y1{},x2{},y2{};
    bool assign(MotionBinary64 ax,MotionBinary64 ay,MotionBinary64 bx,MotionBinary64 by){
        RunnerLineArguments next;
        if(!next.x1.assign(ax)||!next.y1.assign(ay)||!next.x2.assign(bx)||!next.y2.assign(by))return false;
        *this=next;return true;
    }
};
}
#endif

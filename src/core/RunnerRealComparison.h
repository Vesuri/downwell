#ifndef DOWNWELL_CORE_RUNNER_REAL_COMPARISON_H
#define DOWNWELL_CORE_RUNNER_REAL_COMPARISON_H
#include "MotionBinary64.h"
namespace downwell {
// Finite normal/zero subset of the original runner's real comparison path.
// Returns -1/0/+1 for delta below -epsilon, inside the band, or above epsilon.
// Unsupported arithmetic is explicit, not silently interpreted as equality.
inline bool compareRunnerReal(MotionBinary64 left,MotionBinary64 right,int& order) {
    right.high^=0x80000000u;
    if(!left.add(right)) return false;
    const auto magnitude=left.high&0x7fffffffu;
    const bool outside=magnitude>0x3ee4f8b5u ||
        (magnitude==0x3ee4f8b5u && left.low>0x88e368f1u);
    order=outside?(left.high&0x80000000u?-1:1):0;
    return true;
}
}
#endif

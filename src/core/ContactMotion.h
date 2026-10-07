#ifndef DOWNWELL_CONTACT_MOTION_H
#define DOWNWELL_CONTACT_MOTION_H
#include "ContactMotionCodec.h"
namespace downwell {
namespace contact_motion_detail {
#include "../../generated/contact-motion.inc"
}
// Only the signed-Q16 atan domain certified by the host generator is admitted.
// This is a lossless table decoder: intermediate polynomial rounding errors
// are corrected by exact entries, not tolerated as a new trig implementation.
inline bool contactMotion(MotionBinary64 direction,MotionBinary64& speed){
    RunnerImageValue angle,radians;
    return angle.assign(direction)&&SmokeDirection::radians(angle,radians)&&
        ContactMotionCodec::decode(radians,contact_motion_detail::coefficients,
            contact_motion_detail::corrections,
            sizeof(contact_motion_detail::corrections)/sizeof(contact_motion_detail::corrections[0]),speed);
}
}
#endif

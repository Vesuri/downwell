#ifndef DOWNWELL_CORE_EMIT_SMOKE_SCRIPT_H
#define DOWNWELL_CORE_EMIT_SMOKE_SCRIPT_H
#include "MotionBinary64.h"
namespace downwell {
// emitSmoke: synchronous Create first, caller ID next, then two custom-field
// writes. Create's random direction/speed are deliberately NOT skipped.
struct EmitSmokeScript {
    std::uint32_t mySmoke=0;
    template<class Create,class Direction,class Speed>
    bool execute(MotionBinary64 x,MotionBinary64 y,MotionBinary64 direction,MotionBinary64 speed,
        Create create,Direction setDirection,Speed setSpeed){
        std::uint32_t child=0;
        if(!create(x,y,child))return false;
        mySmoke=child;
        return setDirection(mySmoke,direction)&&setSpeed(mySmoke,speed);
    }
};
}
#endif

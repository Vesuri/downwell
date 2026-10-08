#ifndef DOWNWELL_CORE_PACKED_WALL_STATE_H
#define DOWNWELL_CORE_PACKED_WALL_STATE_H
#include "WallEndStep.h"
#include <cstdint>
namespace downwell {
// Exact domains written by wall Create, rmMain creation code and code 986.
// This is event state, not geometry, ownership or a generic runner record.
struct PackedWallState {
    std::uint32_t bits=4; // zero state, including pillar=0
    bool store(const WallEndStepState& s){
        if(s.checked<0||s.checked>2||s.pillar<-1||s.pillar>1||
            s.neighbors<0||s.neighbors>15||s.imageIndex<0||s.imageIndex>15||
            (s.checkThing!=0&&s.checkThing!=60)||(s.checkDistance!=0&&s.checkDistance!=16)||s.lights>15)return false;
        bits=unsigned(s.checked)|(unsigned(s.pillar+1)<<2)|(unsigned(s.neighbors)<<4)|
            (unsigned(s.imageIndex)<<8)|(unsigned(s.checkThing!=0)<<12)|
            (unsigned(s.checkDistance!=0)<<13)|(unsigned(s.surrounded)<<14)|
            (unsigned(s.deactivate)<<15)|(s.lights<<16);
        return true;
    }
    WallEndStepState load()const{
        WallEndStepState s;
        s.checked=bits&3;s.pillar=int((bits>>2)&3)-1;s.neighbors=(bits>>4)&15;
        s.imageIndex=(bits>>8)&15;s.checkThing=(bits&(1u<<12))?60:0;
        s.checkDistance=(bits&(1u<<13))?16:0;s.surrounded=(bits&(1u<<14))!=0;
        s.deactivate=(bits&(1u<<15))!=0;s.lights=(bits>>16)&15;return s;
    }
};
}
#endif

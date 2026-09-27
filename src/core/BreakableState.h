#ifndef DOWNWELL_CORE_BREAKABLE_STATE_H
#define DOWNWELL_CORE_BREAKABLE_STATE_H
#include "MotionBinary64.h"
namespace downwell {
// objBox_n's retained Create fields, separate from collision lifetime flags.
// The cavern builder supplies its already-consumed sound/gem choices; never
// repeat those random draws when adopting the command on either backend.
struct BreakableState {
    MotionBinary64 wallHp{0x3ff00000u,0};
    unsigned short breakSound=171;
    unsigned char haveGem=0;
    bool hit=false,drilled=false,autoTile=false,checked=false;
};
}
#endif

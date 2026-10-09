#ifndef DOWNWELL_CORE_SHOOTABLE_THING_H
#define DOWNWELL_CORE_SHOOTABLE_THING_H
#include "MotionBinary64.h"
namespace downwell {
// parentShootableThing (90) Create. These are custom VM fields, not engine
// activation/health. Descendant Create and later cross-instance writes may
// override them; damage must retain fractional and negative objHp values.
struct ShootableThing {
    MotionBinary64 health{0x3ff00000u,0};
    bool hit=false,hitEffect=false;
};
}
#endif

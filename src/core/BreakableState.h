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
    std::int32_t scriptIndex=0; // self.i, shared by alarmStop and autotiling.
    // scrBulCheckSolid [11..13]: positive health only; field writes precede
    // bullet destruction. Box destruction belongs to its later ordinary Step.
    bool bulletHit(MotionBinary64 damage,bool& applied){
        if(!wallHp.valid()||!damage.valid())return false;
        applied=false;
        if(wallHp.zero()||(wallHp.high&0x80000000u))return true;
        damage.high^=0x80000000u;
        auto remaining=wallHp;if(!remaining.add(damage))return false;
        wallHp=remaining;hit=true;applied=true;return true;
    }
    bool exhausted()const{return wallHp.zero()||(wallHp.high&0x80000000u);}
    // gml_Object_objBox_n_Alarm_0: no effects or retirement here. The ordinary
    // Step observes zero health and owns the destruction sequence.
    void alarmZero(){wallHp={};}
    bool stopAlarmZero(bool timeAllowed,std::int32_t& alarm){
        if(timeAllowed)return true;
        scriptIndex=0;
        if(alarm>0){
            // Reject an unrepresentable native countdown without signed UB;
            // the live source uses short positive delays, never this boundary.
            if(alarm==0x7fffffff)return false;
            ++alarm;
        }
        scriptIndex=1;return true;
    }
};
}
#endif

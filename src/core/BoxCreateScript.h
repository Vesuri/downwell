#ifndef DOWNWELL_CORE_BOX_CREATE_SCRIPT_H
#define DOWNWELL_CORE_BOX_CREATE_SCRIPT_H
#include "BreakableState.h"
namespace downwell {
// Object 92 Create (1085). Property callbacks commit in source order; RNG is
// not precomputed across a setter that could fail. Caller supplies the current
// wallTile and room, including destination room during synchronous room entry.
template<class Random,class Sprite,class Speed,class Index>
bool boxCreateScript(BreakableState& state,int area,int wallTile,int room,bool glitch,
                     Random& random,Sprite sprite,Speed speed,Index index){
    state.materialBreakable=true;
    if(!sprite(wallTile))return false;
    state.autoTile=false;state.checked=false;state.breakSound=171;
    if(area>=1&&area<=4){
        const unsigned base=area==2?175:area==4?179:171;
        state.breakSound=static_cast<unsigned short>(base+random.chooseIndexSmall(4));
    }
    if(glitch){
        constexpr int tiles[]={51,52,53,55,57};
        if(!sprite(tiles[random.chooseIndexSmall(5)]))return false;
    }
    if(!speed(0)||!index(16+random.chooseIndexSmall(2)))return false;
    state.wallHp=MotionBinary64::fromInteger(1);state.drilled=false;
    state.haveGem=random.irandomSmall(50)<1;
    if(area==3)state.haveGem=0;
    // groundRoom's caller-local grgrgr write, after the haveGem draw/override.
    state.groundRoomResult=false;
    if(room>=20&&room<=24)state.groundRoomResult=true;
    if(state.groundRoomResult)state.haveGem=0;
    if(state.haveGem==1){
        if(!index(18+random.chooseIndexSmall(2)))return false;
        state.haveGem=2;
    }
    return state.haveGem!=2||index(22);
}
}
#endif

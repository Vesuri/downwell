#ifndef DOWNWELL_CORE_WALL_END_STEP_H
#define DOWNWELL_CORE_WALL_END_STEP_H
#include "GameMakerRandom.h"
namespace downwell {
// objWall_n Create/End Step fields. Query coordinates below are offsets from
// the published instance position; adapters retain its source numeric format.
struct WallEndStepState {
    int checked=0,pillar=0,neighbors=0,imageIndex=0,checkThing=0,checkDistance=0;
    bool surrounded=false,deactivate=false;
    unsigned lights=0; // upper-left, upper-right, lower-left, lower-right
};
inline int wallPillarFrame(int frame,int pillar){
    if(pillar==1){
        switch(frame){
            case 0:case 8:case 9:case 10:case 11:case 12:case 13:case 14:return 15;
            case 1:case 3:case 4:case 5:return 7;
        }
    }else if(pillar==-1){
        switch(frame){
            case 0:case 2:case 3:case 6:case 7:case 10:case 11:case 14:return 15;
            case 1:case 4:case 5:case 9:return 13;
        }
    }
    return frame;
}
// Complete control flow of code 986. Ops must implement source place_meeting
// (shape query) and position_meeting (point query), not interchangeable grid
// membership. Destroy queues engine destruction: the record remains available
// for the rest of this event, including a following deactivation request.
// All callbacks are checked; completed effects and random draws are retained.
template<class Ops>
bool wallEndStep(WallEndStepState& s,int area,GameMakerRandom& random,Ops& ops){
    if(area==5&&!ops.outOfView())return false;
    if(s.checked==1){
        if(s.surrounded){
            if(!ops.tile(739,48,48,16,16,-8,-8,100)||!ops.destroy())return false;
        }else s.checked=2;
        if(s.deactivate&&!ops.deactivate())return false;
    }
    if(s.checked)return true;
    s.checkThing=60;s.neighbors=0;
    const int dx[]={0,16,0,-16},dy[]={-16,0,16,0};
    for(unsigned i=0;i<4;++i){
        bool hit=false;if(!ops.place(dx[i],dy[i],60,hit))return false;
        if(hit)s.neighbors+=1<<i;
    }
    s.neighbors=wallPillarFrame(s.neighbors,s.pillar);
    if(s.neighbors==15&&!s.surrounded){
        s.lights=15;s.checkDistance=16;
        const int px[]={-16,16,-16,16},py[]={-16,-16,16,16};
        for(unsigned i=0;i<4;++i){
            bool hit=false;if(!ops.point(px[i],py[i],60,hit))return false;
            if(hit)s.lights&=~(1u<<i);
        }
        if(!s.lights){s.surrounded=true;s.checked=1;}
    }
    auto image=[&](int frame){if(!ops.image(frame))return false;s.imageIndex=frame;return true;};
    if(!image(s.neighbors))return false;
    if(area==4){
        switch(s.imageIndex){
            case 2:case 6:case 0:case 10:case 14:case 8:case 12:
                if(random.chooseIndexSmall(3)==2&&!ops.create(379,0,-16))return false;
                break;
        }
    }
    if(area==0){
        switch(s.imageIndex){case 2:case 0:case 10:case 14:case 8:if(!image(14))return false;break;}
        if(s.imageIndex!=14&&s.imageIndex!=6&&s.imageIndex!=12&&!image(15))return false;
    }
    s.checked=1;return true;
}
}
#endif

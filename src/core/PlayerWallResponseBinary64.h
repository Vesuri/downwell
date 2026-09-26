#ifndef DOWNWELL_CORE_PLAYER_WALL_RESPONSE_BINARY64_H
#define DOWNWELL_CORE_PLAYER_WALL_RESPONSE_BINARY64_H
#include "PlayerSolidCollisionBinary64.h"
#include "PlayerHorizontalBinary64.h"
namespace downwell {
// scrWallCol state and synchronous script ordering. Instance selection for
// boxAbove remains the world's responsibility, not an arbitrary first box.
// Boolean fields represent the supported source 0/1 domain. The world must
// latch a false return; completed state changes and script effects remain.
struct PlayerWallResponseBinary64 {
    using N=MotionBinary64;
    N x{},y{},vx{},vy{},hardLandSpeed{0x3ff33333u,0x33333333u};
    bool grounded=false,hardLand=false,hardLandJump=false,airStatus=false,wallColliding=false;
    SolidCollisionResult collision{};
    // Events: meeting(target,x,y,hit), breakBox(x,y,found), timeStopped(),
    // comboDone(), recharge(), gravity(). Boolean operations report failure.
    // Synchronous script callbacks may change state; later operations reread it.
    template<class Events> bool resolve(bool left,bool right,Events& events){
        if(!hardLand && !hardLandJump)hardLand=true;
        else if(hardLand){
            if(!vy.valid()||!hardLandSpeed.valid())return false;
            if(PlayerHorizontalBinary64::less(vy,hardLandSpeed))hardLand=false;
        }
        if(grounded)vy={};
        auto wall=[&](N px,N py,bool& hit){return events.meeting(57,px,py,hit);};
        if(!PlayerSolidCollisionBinary64::resolvePlatform(x,y,vx,vy,
            [&](N px,N py,bool& hit){return events.meeting(87,px,py,hit);},wall,collision))return false;
        bool hit;
        if(collision.y==1){
            if(!events.meeting(87,x,y,hit))return false;
            if(!hit){
                if(!queryOffset(events,87,1,hit))return false;
                if(hit){grounded=true;vy={};}
            }
        }
        if(!PlayerSolidCollisionBinary64::resolve(x,y,vx,vy,wall,collision))return false;
        if(collision.x){
            vx={};
            if((collision.x==1 && right)||(collision.x==-1 && left))wallColliding=true;
        }else wallColliding=false;
        if(collision.y==1){
            if(!queryOffset(events,57,1,hit))return false;
            if(hit){grounded=true;vy={};}
        }else if(collision.y==-1){
            vy={};N py=y;
            if(!py.add(N::fromInteger(-16))||!events.breakBox(x,py,hit))return false;
            if(hit)vy=N::fromInteger(1);
        }
        if(grounded){
            if(airStatus){
                if(!events.timeStopped() && collision.x==0 && !events.comboDone())return false;
                if(!events.recharge())return false;
            }
            if(!events.meeting(57,x,y,hit))return false;
            if(hit){
                if(!queryOffset(events,57,-1,hit))return false;
                if(!hit && !y.add(N::fromInteger(-1)))return false;
            }
        }
        if(!queryOffset(events,56,1,hit))return false;
        if(!hit && grounded){grounded=false;if(!vy.add(events.gravity()))return false;}
        return true;
    }
private:
    template<class Events> bool queryOffset(Events& events,int target,int offset,bool& hit){
        N py=y;return py.add(N::fromInteger(offset)) && events.meeting(target,x,py,hit);
    }
};
}
#endif

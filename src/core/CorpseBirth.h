#ifndef DOWNWELL_CORE_CORPSE_BIRTH_H
#define DOWNWELL_CORE_CORPSE_BIRTH_H
#include "RandomBinary64.h"
#include "RunnerImageValue.h"
#include "AlarmBank.h"
#include "RunnerMoveSnap.h"
namespace downwell {
// Object 263 and inherited object 90 Create. Wall queries select parent 57;
// tile queries use layer 100. Queries report success separately from a hit.
// publish runs after each builtin coordinate write, including move_snap.
// Failure is terminal: RNG, earlier writes and membership are not rolled back.
struct CorpseBirth {
    RunnerImageValue x{},y{},imageSpeed{};
    MotionBinary64 xx{},yy{},xsp{},ysp{};
    MotionBinary64 gravity{0x3fb99999u,0x9999999au},hardGravity{0x3fc99999u,0x9999999au};
    AlarmBank alarms;
    int health=1,hitEffect=0,explosionFx=102,flash=-1,bounceSound=-1;
    int attempts=0,radius=0,maxRadius=0,rx=0,ry=0;
    bool grounded=false,active=false,hit=false,disappearing=false,noTouch=true,bouncing=true;
    static bool snap(RunnerImageValue coordinate,RunnerImageValue& out){
        return RunnerMoveSnap::coordinate16(coordinate,out);
    }
    template<class Random,class Queries>
    bool create(MotionBinary64 px,MotionBinary64 py,bool restInPieces,Random& random,Queries& queries){
        *this=CorpseBirth{};
        if(!x.assign(px)||!y.assign(py))return false;
        // Constructor publication precedes Create; the owner supplies it.
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(6),xsp)||
           !xsp.add(MotionBinary64::fromInteger(-3))||
           !randomBinary64(random.nextU32(),MotionBinary64::fromInteger(4),ysp)||
           !ysp.add(MotionBinary64::fromInteger(-2)))return false;
        if(restInPieces)hitEffect=1;else health=0;
        alarms.set(3,8);
        const MotionBinary64 rates[]={{0xbfe00000u,0},{0xbfd33333u,0x33333333u},{},
                                      {0x3fd33333u,0x33333333u},{0x3fe00000u,0}};
        if(!imageSpeed.assign(rates[random.chooseIndexSmall(5)]))return false;
        xx=x.promoted();yy=y.promoted();alarms.set(0,180);alarms.set(1,120);
        bool blocked;
        if(!queries.wall(x.promoted(),y.promoted(),blocked))return false;
        if(!blocked&&!queries.tile(x.promoted(),y.promoted(),blocked))return false;
        if(blocked){
            if(!snap(x,x)||!queries.publish(x,y)||!snap(y,y)||!queries.publish(x,y))return false;
            radius=4;maxRadius=32;
            for(;;){
                rx=int(random.irandomSmall(unsigned(radius*2)))-radius;
                ry=int(random.irandomSmall(unsigned(radius*2)))-radius;
                auto tx=x.promoted(),ty=y.promoted();
                if(!tx.add(MotionBinary64::fromInteger(rx))||!ty.add(MotionBinary64::fromInteger(ry))||
                   !queries.wall(tx,ty,blocked))return false;
                // Source deliberately checks the snapped origin for tiles,
                // NOT the random candidate tested against walls.
                if(!blocked&&!queries.tile(x.promoted(),y.promoted(),blocked))return false;
                if(!blocked)break;
                if(radius<=32)radius+=4; // Reaches 36, despite rrmax=32.
                if(++attempts>32)break; // Applies even the last blocked candidate.
            }
            auto tx=x.promoted(),ty=y.promoted();
            if(!tx.add(MotionBinary64::fromInteger(rx))||!x.assign(tx)||!queries.publish(x,y)||
               !ty.add(MotionBinary64::fromInteger(ry))||!y.assign(ty)||!queries.publish(x,y))return false;
        }
        xx=x.promoted();yy=y.promoted();return true;
    }
};
}
#endif

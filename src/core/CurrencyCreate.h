#ifndef DOWNWELL_CORE_CURRENCY_CREATE_H
#define DOWNWELL_CORE_CURRENCY_CREATE_H
#include "RandomBinary64.h"
#include "RunnerMoveSnap.h"
#include "AlarmBank.h"
#include "CurrencyStepSound.h"
namespace downwell {
// Object 248 Create, before scrCurrencySpawn applies amount/size/attraction.
// Constructor membership precedes this call. Queries use parentWall (57) and
// tile layer 100; publish follows each separate builtin x/y write.
struct CurrencyCreate : CurrencyStepSound {
    RunnerImageValue x{},y{},imageSpeed{};
    MotionBinary64 xx{},yy{},xsp{},ysp{},imgSp{};
    MotionBinary64 gravity{0x3fa1eb85u,0x1eb851ecu},hardGravity{0x3fc99999u,0x9999999au};
    MotionBinary64 maximumGravity=MotionBinary64::fromInteger(4);
    AlarmBank alarms;
    int unobtime=10,obtime=180,distime=120,stuckKamo=0,bounceGain=1,stepRate=1;
    int size=0,getAmount=2,flash=-1,attractionRadius=24,attractionSpeed=4;
    int rotationSpeed=5,gemSpeed=1,direction=0;
    int attempts=0,radius=0,maxRadius=0,rx=0,ry=0;
    bool active=true,suckable=false,obtainable=false,disappearing=false,attracted=false;
    template<class Random,class Queries>
    bool create(MotionBinary64 px,MotionBinary64 py,int area,bool attractor,Random& random,Queries& queries){
        *this=CurrencyCreate{};
        if(!x.assign(px)||!y.assign(py))return false;
        if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(4),xsp)||
           !xsp.add(MotionBinary64::fromInteger(-2))||
           !randomBinary64(random.nextU32(),MotionBinary64::fromInteger(4),ysp)||
           !ysp.add(MotionBinary64::fromInteger(-2)))return false;
        if(area==5){
            if(!randomBinary64(random.nextU32(),MotionBinary64::fromInteger(3),ysp))return false;
            ysp.high^=0x80000000u;
        }
        if(area==3||area==4){gravity.high-=2u<<20;maximumGravity={0x3fe00000u,0};}
        alarms.set(0,unobtime);
        const MotionBinary64 rates[]={{0xbfe00000u,0},{0xbfd33333u,0x33333333u},
                                      {0x3fd33333u,0x33333333u},{0x3fe00000u,0}};
        imgSp=rates[random.chooseIndexSmall(4)];if(!imageSpeed.assign(imgSp))return false;
        if(attractor){attractionRadius=48;attractionSpeed=6;alarms.set(3,30);}
        else alarms.set(3,15);
        // VM evaluates choose's second argument first. Keep negative angles;
        // Create does not normalize gemdir into 0..360.
        const int second=170+int(random.irandomSmall(70));
        const int first=int(random.irandomSmall(70))-60;
        direction=random.chooseIndexSmall(2)?second:first;
        bool blocked=false;
        if(!queries.wall(x.promoted(),y.promoted(),blocked))return false;
        if(!blocked&&!queries.tile(x.promoted(),y.promoted(),blocked))return false;
        if(blocked){
            if(!RunnerMoveSnap::coordinate16(x,x)||!queries.publish(x,y)||
               !RunnerMoveSnap::coordinate16(y,y)||!queries.publish(x,y))return false;
            radius=4;maxRadius=32;
            for(;;){
                rx=int(random.irandomSmall(unsigned(radius*2)))-radius;
                ry=int(random.irandomSmall(unsigned(radius)))-radius;
                auto tx=x.promoted(),ty=y.promoted();
                if(!tx.add(MotionBinary64::fromInteger(rx))||!ty.add(MotionBinary64::fromInteger(ry))||
                   !queries.wall(tx,ty,blocked))return false;
                // Original tile lookup stays at the snapped origin.
                if(!blocked&&!queries.tile(x.promoted(),y.promoted(),blocked))return false;
                if(!blocked)break;
                if(radius<=64)radius+=4; // rrmax=32 is not used by this loop.
                if(++attempts>32)break;
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

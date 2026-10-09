#ifndef DOWNWELL_CORE_CORPSE_MOTION_H
#define DOWNWELL_CORE_CORPSE_MOTION_H
#include "CorpseLifecycle.h"
#include "RunnerRealComparison.h"
#include "MultiplyBinary64.h"
#include "TimeStopBound.h"
namespace downwell {
struct CorpseMotion : CorpseLifecycle {
    int collisionDistance=0,loopIndex=0,checkRange=0;
    template<class Random,class Queries>
    bool create(MotionBinary64 px,MotionBinary64 py,bool rip,Random& random,Queries& queries){
        collisionDistance=loopIndex=checkRange=0;
        return CorpseLifecycle::create(px,py,rip,random,queries);
    }
    template<class Events> bool step(bool stopped,Events& events){
        if(!valid)return false;
        if(destroyRequested)return true;
        if(!run(stopped,events)){valid=false;return false;}
        return true;
    }
    // Live adapter: queries use this registered instance's transformed mask.
    // Create's wall family is 57; ordinary movement deliberately selects 56.
    template<class Publish,class Environment> bool stepInWorld(bool playerStopped,int roomWidth,
        Publish& publish,Environment& environment,bool& good){
        if(!good||!valid)return false;
        bool allowed=false;
        if(!TimeStopBound::evaluate(playerStopped,x.promoted(),y.promoted(),roomWidth,
            [&](auto px,auto py,bool& hit){return publish.meeting(82,px,py,hit);},allowed))return good=false;
        struct Events {
            Publish& output;Environment& environment;bool& good;
            bool meeting(MotionBinary64 x,MotionBinary64 y,bool& hit){return output.meeting(56,x,y,hit);}
            bool publish(RunnerImageValue x,RunnerImageValue y){return output.position(x,y);}
            int area(){return environment.area();}
            int restInPieces(){return environment.restInPieces();}
            bool create(int object,MotionBinary64 x,MotionBinary64 y){return environment.create(object,x,y);}
            void destroy(){if(!output.destroy())good=false;}
        } events{publish,environment,good};
        return (step(!allowed,events)&&good)||(good=false);
    }
private:
    static bool scale(MotionBinary64& value,MotionBinary64 factor){return multiplyBinary64(value,factor,value);}
    template<class Events> bool run(bool stopped,Events& events){
        if(stopped){
            for(loopIndex=0;loopIndex<=2;++loopIndex){
                const auto value=alarms.get(loopIndex);
                if(value==2147483647)return false;
                if(value>0)alarms.set(loopIndex,value+1);
            }
        }else{
            int order;
            if(!compareRunnerReal(ysp,MotionBinary64::fromInteger(-1),order)||
               !ysp.add(order<0?hardGravity:gravity))return false;
            auto probe=xx;bool hitWall;
            if(!probe.add(xsp)||!events.meeting(probe,yy,hitWall))return false;
            if(hitWall){xsp.high^=0x80000000u;if(!scale(xsp,{0x3fc99999u,0x9999999au}))return false;}
            probe=yy;
            if(!probe.add(ysp)||!events.meeting(xx,probe,hitWall))return false;
            if(hitWall){
                if(!compareRunnerReal(ysp,{},order))return false;
                if(order<=0)ysp={};
                if(bouncing){
                    if(!compareRunnerReal(ysp,{0x3fc99999u,0x9999999au},order))return false;
                    if(order>0){
                        if(!scale(ysp,{0xbfe00000u,0})||!scale(xsp,{0x3fd99999u,0x9999999au})||
                           !compareRunnerReal(ysp,{0xbfe00000u,0},order))return false;
                        if(order>0)bouncing=false;
                    }
                }
                if(!bouncing){
                    if(!compareRunnerReal(ysp,{},order))return false;
                    if(order>0){
                        probe=yy;
                        if(!probe.add(MotionBinary64::fromInteger(1))||!events.meeting(xx,probe,hitWall))return false;
                        if(hitWall){
                            ysp={};grounded=true;
                            if(!scale(xsp,{0x3fe00000u,0}))return false;
                            auto absolute=xsp;absolute.high&=0x7fffffffu;
                            if(!compareRunnerReal(absolute,{0x3fd33333u,0x33333333u},order))return false;
                            if(order<0)xsp={};
                        }else{
                            collisionDistance=0;loopIndex=1;
                            for(;;){
                                probe=yy;
                                if(!probe.add(MotionBinary64::fromInteger(loopIndex))||!events.meeting(xx,probe,hitWall))return false;
                                if(hitWall)break;
                                ++collisionDistance;if(++loopIndex>16)break;
                            }
                            ysp=MotionBinary64::fromInteger(collisionDistance);
                        }
                    }
                }
            }
            auto absolute=xsp;absolute.high&=0x7fffffffu;
            if(!compareRunnerReal(absolute,{0x3fa99999u,0x9999999au},order))return false;
            if(order<0)xsp=xsp.zero()?MotionBinary64{}:MotionBinary64{0x3fa99999u|(xsp.high&0x80000000u),0x9999999au};
            auto dx=xsp,dy=ysp;
            if(events.area()==3&&(!scale(dx,{0x3fd00000u,0})||!scale(dy,{0x3fd00000u,0})))return false;
            if(!xx.add(dx)||!yy.add(dy))return false;
            std::int32_t coordinate;
            if(!yy.roundPixel(coordinate)||!y.assign(MotionBinary64::fromInteger(coordinate))||!events.publish(x,y)||
               !xx.roundPixel(coordinate)||!x.assign(MotionBinary64::fromInteger(coordinate))||!events.publish(x,y))return false;
            checkRange=4;
            if(events.restInPieces()){
                if(!active)health=1;
                if(health<=0&&active){
                    if(!events.create(272,x.promoted(),yy))return false;
                    if(events.restInPieces()==2)for(unsigned n=0;n<4;++n)
                        if(!events.create(272,x.promoted(),yy))return false;
                    for(unsigned n=0;n<3;++n)if(!events.create(274,x.promoted(),yy))return false;
                    destroyRequested=true;events.destroy();
                }
            }
        }
        // Unlike debris, frozen corpses still update disappearance flashing.
        flash=disappearing?-flash:-1;
        return true;
    }
};
}
#endif

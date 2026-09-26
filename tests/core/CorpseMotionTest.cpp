#include "core/CorpseMotion.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <vector>
using namespace downwell;
static MotionBinary64 real(double d){std::uint64_t b;std::memcpy(&b,&d,8);return {std::uint32_t(b>>32),std::uint32_t(b)};}
static void equal(MotionBinary64 a,double d){const auto b=real(d);assert(a.high==b.high&&a.low==b.low);}
struct Query {double x,y;bool hit;};
struct Events {
    std::vector<Query> queries;
    unsigned next=0,published=0,born=0,destroyed=0,failQuery=0,failBirth=0,failPublish=0;
    int region=1,upgrade=0;
    bool meeting(MotionBinary64 x,MotionBinary64 y,bool& hit){
        assert(next<queries.size());const auto q=queries[next++];equal(x,q.x);equal(y,q.y);hit=q.hit;
        return next!=failQuery;
    }
    int area(){return region;}
    int restInPieces(){return upgrade;}
    bool publish(RunnerImageValue,RunnerImageValue){return ++published!=failPublish;}
    bool create(int object,MotionBinary64,MotionBinary64){assert(object==(born<unsigned(upgrade==2?5:1)?272:274));return ++born!=failBirth;}
    void destroy(){++destroyed;}
};
int main(){
    unsigned cases=0;
    for(double vx:{-2.,-.3,-.04998,0.,.04998,.3,2.})
    for(double vy:{-1.00002,-1.,-.1,.10002,.3,1.,4.})
    for(bool bounce:{false,true})for(unsigned tape=0;tape<32;++tape)for(int area:{1,3}){
        CorpseMotion corpse;corpse.xx=real(12.25);corpse.yy=real(24.5);
        corpse.xsp=real(vx);corpse.ysp=real(vy);corpse.bouncing=bounce;
        Events events;events.region=area;
        double sx=vx,sy=vy+(vy+1 < -1e-5?.2:.1);bool bouncing=bounce,grounded=false;
        unsigned calls=0;int distance=0,index=0;
        auto query=[&](double x,double y){
            const bool hit=calls<3?bool(tape&(1u<<calls)):(tape&8)&&calls>=((tape&16)?18u:5u);
            ++calls;events.queries.push_back({x,y,hit});return hit;
        };
        if(query(12.25+sx,24.5)){sx*=-1;sx*=.2;}
        if(query(12.25,24.5+sy)){
            if(sy<=1e-5)sy=0;
            if(bouncing&&sy-.2>1e-5){sy*=-.5;sx*=.4;if(sy+.5>1e-5)bouncing=false;}
            if(!bouncing&&sy>1e-5){
                if(query(12.25,25.5)){sy=0;grounded=true;sx*=.5;if(std::abs(sx)-.3 < -1e-5)sx=0;}
                else{
                    index=1;
                    while(!query(12.25,24.5+index)){++distance;if(++index>16)break;}
                    sy=distance;
                }
            }
        }
        if(std::abs(sx)-.05 < -1e-5)sx=sx>0?.05:sx<0?-.05:0.;
        const double xx=12.25+sx/(area==3?4:1),yy=24.5+sy/(area==3?4:1);
        assert(corpse.step(false,events));
        equal(corpse.xsp,sx);equal(corpse.ysp,sy);equal(corpse.xx,xx);equal(corpse.yy,yy);
        equal(corpse.x.promoted(),double(float(int(std::nearbyint(xx)))));
        equal(corpse.y.promoted(),double(float(int(std::nearbyint(yy)))));
        assert(corpse.bouncing==bouncing&&corpse.grounded==grounded);
        assert(corpse.collisionDistance==distance&&corpse.loopIndex==index&&corpse.checkRange==4);
        assert(events.next==events.queries.size()&&events.published==2);++cases;
    }
    for(bool fading:{false,true}){
        CorpseMotion corpse;corpse.disappearing=fading;corpse.alarms.set(0,3);corpse.alarms.set(1,0);corpse.alarms.set(2,-1);corpse.alarms.set(3,7);
        Events events;assert(corpse.step(true,events));
        assert(corpse.alarms.get(0)==4&&corpse.alarms.get(1)==0&&corpse.alarms.get(2)==-1&&corpse.alarms.get(3)==7);
        assert(corpse.flash==(fading?1:-1)&&events.next==0&&events.published==0&&corpse.loopIndex==3);
    }
    for(int upgrade:{0,1,2})for(bool active:{false,true})for(int health:{0,1}){
        CorpseMotion corpse;corpse.active=active;corpse.health=health;corpse.disappearing=true;
        Events events;events.upgrade=upgrade;events.queries={{0,0,false},{0,.1,false}};
        assert(corpse.step(false,events));const bool explode=upgrade&&active&&!health;
        assert(events.born==(explode?(upgrade==2?8u:4u):0u)&&events.destroyed==explode&&corpse.destroyRequested==explode);
        assert(corpse.flash==1&&corpse.health==(upgrade&&!active?1:health));
    }
    for(unsigned failure=1;failure<=8;++failure){
        CorpseMotion corpse;corpse.active=true;corpse.health=0;
        Events events;events.upgrade=2;events.failBirth=failure;events.queries={{0,0,false},{0,.1,false}};
        assert(!corpse.step(false,events)&&!corpse.valid&&!corpse.destroyRequested);
        assert(events.born==failure&&!events.destroyed&&corpse.flash==-1);
        assert(!corpse.step(false,events)&&events.born==failure);
    }
    for(unsigned failure=1;failure<=4;++failure){
        CorpseMotion corpse;Events events;events.queries={{0,0,false},{0,.1,false}};
        if(failure<=2)events.failQuery=failure;else events.failPublish=failure-2;
        assert(!corpse.step(false,events)&&!corpse.valid&&!corpse.destroyRequested);
        const auto queries=events.next,publications=events.published;
        assert(!corpse.step(false,events)&&queries==events.next&&publications==events.published);
    }
    std::printf("Corpse motion: %u scalar query tapes, freeze/upgrade gates and eight partial explosion failures passed\n",cases);
}

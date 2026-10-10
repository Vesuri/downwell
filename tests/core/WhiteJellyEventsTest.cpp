#include "core/WhiteJellyStep.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <vector>
#include <cstdio>
using N=downwell::MotionBinary64;
struct Ops {
    std::vector<int> calls;unsigned reject=0;int otherx=0,othery=0,top=0,roomId=11;
    N published{},emitted{};
    bool call(int id){calls.push_back(id);return calls.size()!=reject;}
    bool otherX(N& out){out=N::fromInteger(otherx);return call(1);}
    bool otherY(N& out){out=N::fromInteger(othery);return call(2);}
    bool pointDirection(N x,N y,N tx,N ty,N& out){
        assert(x.high==N::fromInteger(otherx).high&&y.high==N::fromInteger(othery).high);
        assert(tx.zero()&&ty.zero());out=N::fromInteger(45);return call(3);
    }
    bool direction(N value,N& out){published=out=value;return call(4);}
    bool movingFx(N,N,int sprite,N rate,N angle,N speed){
        assert(sprite==704&&rate.high==0x3fb99999u&&rate.low==0x9999999au&&speed.high==0x3fe00000u);
        emitted=angle;return call(5);
    }
    bool emitTimer(N& value){value=N::fromInteger(17);return call(6);}
    bool effectAlarmReset(N value){assert(value.high==N::fromInteger(17).high);return call(7);}
    bool viewY(N& value){value=N::fromInteger(top);return call(8);}
    bool room(int& value){value=roomId;return call(9);}
    bool destroy(){return call(10);}
};
int main(){
    struct Player {
        N y{};unsigned stomps=0,damageCalls=0;
        bool globalY(N& out){out=y;return true;}
        bool stomp(){++stomps;return true;}
        bool damage(int a,int b,int c){assert(a==1&&b==3&&c==2);++damageCalls;return true;}
    };
    for(int offset:{1,2,3}){
        downwell::WhiteJellyStep s;s.initialized=true;s.yy=N::fromInteger(100+offset);
        assert(s.y.assignInteger(-500)); // published Y must not drive typStompable
        Player p{N::fromInteger(100)};assert(s.playerContact(p));
        assert(p.stomps==unsigned(offset>2)&&p.damageCalls==unsigned(offset<=2));
    }
    struct Stomp {
        downwell::WhiteJellyStep& s;unsigned fail=0,calls=0,recharges=0,smokes=0;int style=0;
        bool next(){return ++calls!=fail;}
        bool playerDead(){return false;}int playStyle(){return style;}
        bool blood(int a,int b){assert(a==3&&b==0);return next();}
        void setSpinJumping(){assert(++calls==2);}
        bool recharge(){assert(!s.hit&&!s.stomped);++recharges;return next();}
        bool setGrounded(bool v){assert(!v);return next();}
        bool setHorizontal(N){assert(false);return false;}
        bool setVertical(N v){assert(v.high==(style==2?0xbff80000u:0xc0040000u));return next();}
        bool shake(int a,int b){assert(a==2&&b==10&&s.health==25);return next();}
        bool stompSound(int id,int p,bool loop,bool cut,const char*){
            assert(id==122&&p==50&&!loop&&cut&&s.health==-75);return next();
        }
        bool explodingKnees(){return true;}
        bool createBlast(int id,N x,N y){assert(id==276&&x.zero()&&y.zero()&&s.hit&&s.stomped);return next();}
        unsigned choose(unsigned n){assert(n==3);return next()?1:3;}
        bool explosionSound(int id,int p,bool loop,bool cut){assert(id==92&&p==60&&!loop&&cut);return next();}
        N playerY(){return N::fromInteger(20+int(smokes));}
        N playerX(){return N::fromInteger(10+int(smokes));}
        bool emitSmoke(N x,N y,int direction,int speed){
            const int angles[]={0,180,15,165};assert(x.high==N::fromInteger(10+int(smokes)).high&&
                y.high==N::fromInteger(24+int(smokes)).high&&direction==angles[smokes]&&speed==(smokes<2?4:3));
            ++smokes;return next();
        }
    };
    for(int style:{0,2})for(unsigned failure=0;failure<=15;++failure){
        if(failure==2)continue; // The source assignment has no fallible service.
        downwell::WhiteJellyStep s;s.health=25;s.alive=true;bool good=true;Stomp ops{s,failure};ops.style=style;
        assert(s.stompPlayerChecked(ops,good)==!failure&&good==!failure&&ops.calls==(failure?failure:15));
        assert(s.health==((failure&&failure<=6)?25:-75)&&s.alive);
        assert(s.hit==(!failure||failure>=9)&&s.stomped==s.hit);
        if(!failure)assert(ops.recharges==2&&ops.smokes==4);
    }
    for(bool coincidentSum:{false,true})for(unsigned failure=0;failure<=(coincidentSum?3u:6u);++failure){
        downwell::WhiteJellyStep s;s.initialized=true;
        downwell::GameMakerRandom rng(15),oracle(15);Ops ops;ops.reject=failure;
        ops.otherx=5;ops.othery=coincidentSum?-5:2;
        assert(s.enemyContact(rng,ops)==!failure);
        const std::vector<int> expected=coincidentSum?std::vector<int>{1,2,4}:std::vector<int>{1,2,2,1,3,4};
        assert(ops.calls==std::vector<int>(expected.begin(),expected.begin()+(failure?failure:expected.size())));
        if(coincidentSum&&(!failure||failure==3)){
            const auto expectedAngle=N::fromInteger(int(oracle.irandomSmall(359)));
            assert(ops.published.high==expectedAngle.high&&ops.published.low==expectedAngle.low);
        }
        assert(rng.drawCount()==oracle.drawCount());
        for(unsigned i=0;i<16;++i)assert(rng.stateWord(i)==oracle.stateWord(i));
    }
    for(bool stopped:{false,true})for(unsigned failure=0;failure<=(stopped?2u:3u);++failure){
        downwell::WhiteJellyStep s;s.initialized=true;
        downwell::GameMakerRandom rng(33),oracle(33);Ops ops;ops.reject=failure;
        assert(s.effectAlarm(stopped,rng,ops)==!failure);
        std::vector<int> expected=stopped?std::vector<int>{6,7}:std::vector<int>{5,6,7};
        assert(ops.calls==std::vector<int>(expected.begin(),expected.begin()+(failure?failure:expected.size())));
        if(!stopped){
            N angle;assert(downwell::randomBinary64(oracle.nextU32(),N::fromInteger(40),angle));
            assert(angle.add(N::fromInteger(-20))&&angle.add(N::fromInteger(90)));
            assert(angle.high==ops.emitted.high&&angle.low==ops.emitted.low);
        }
        assert(rng.drawCount()==oracle.drawCount());
    }
    for(int y:{-161,-160,3000,3001})for(int room:{11,14}){
        downwell::WhiteJellyStep s;s.initialized=true;assert(s.y.assignInteger(y));Ops ops;ops.roomId=room;
        assert(s.endStep(ops));
        const std::vector<int> expected=y==-161?(room==11?std::vector<int>{8,9,10}:std::vector<int>{8,9}):
            y==3001?(room==11?std::vector<int>{8,8,9,10}:std::vector<int>{8,8,9}):std::vector<int>{8,8};
        assert(ops.calls==expected);
    }
    std::puts("White jelly events: sum-based contacts, effect alarm RNG/reset and End Step bounds passed");
}

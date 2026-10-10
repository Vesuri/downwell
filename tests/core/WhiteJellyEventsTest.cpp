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

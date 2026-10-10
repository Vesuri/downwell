#include "core/RedBallBirth.h"
#include "core/RunnerDirectionValue.h"
#include "core/GameMakerRandom.h"
using S=downwell::RedBallBirth;
using N=downwell::MotionBinary64;
using F=downwell::RunnerImageValue;
#ifdef DOWNWELL_FREESTANDING
struct Ops {
    bool position(bool,F,F&);bool playerY(N&);bool playerX(N&);
    bool pointDirection(N,N,N,N,N&);bool direction(N,N&);
    bool alarm(int,int);bool imageSpeed(F,F&);
};
bool nativeRedBallBirth(S& state,F x,F y,downwell::GameMakerRandom& random,Ops& ops){
    return state.create(x,y,random,ops);
}
#else
#include <cassert>
#include <cstdio>
#include <vector>
static bool same(N a,N b){return a.high==b.high&&a.low==b.low;}
struct Ops {
    S& state;downwell::GameMakerRandom& random;
    unsigned reject=0;std::vector<unsigned> calls;
    int alarms[3]={-1,-1,-1};
    bool accept(unsigned call){calls.push_back(call);return calls.size()!=reject;}
    bool position(bool vertical,F wanted,F& accepted){
        assert(state.alive&&state.health==1&&state.money==1&&!state.scriptActive&&!random.drawCount());
        assert(wanted.bits==(vertical?0x43000000u:0x43800000u));
        accepted=wanted;return accept(vertical?2:1);
    }
    bool playerY(N& value){
        assert(state.health==30&&state.money==20&&random.drawCount()==1&&!state.scriptActive);
        assert(same(state.moveSpeed,{0x3fb99999u,0x9999999au})&&same(state.maxSpeed,{0x3ff33333u,0x33333333u}));
        assert(state.cone==90&&same(state.rotateSpeed,N::fromInteger(2)));
        value=N::fromInteger(80);return accept(3);
    }
    bool playerX(N& value){value=N::fromInteger(240);return accept(4);}
    bool pointDirection(N x,N y,N px,N py,N& out){
        assert(same(x,N::fromInteger(256))&&same(y,N::fromInteger(128))&&
            same(px,N::fromInteger(240))&&same(py,N::fromInteger(80)));
        out=N::fromInteger(450);return accept(5);
    }
    bool direction(N wanted,N& accepted){
        F value;if(!accept(6))return false;
        assert(downwell::RunnerDirectionValue::normalize(wanted,value));accepted=value.promoted();return true;
    }
    bool alarm(int number,int value){
        assert(state.scriptActive&&same(state.emitTimer,N::fromInteger(10))&&state.imageRate.zero());
        assert(same(state.direction,N::fromInteger(90))&&same(state.playerDirection,state.direction));
        assert((number==0&&value==10&&alarms[2]==-1)||(number==2&&value==30&&alarms[0]==10));
        if(!accept(number==0?7:8))return false;alarms[number]=value;return true;
    }
    bool imageSpeed(F value,F& accepted){
        assert(alarms[0]==10&&alarms[1]==-1&&alarms[2]==30&&value.bits==0x3e4ccccdu);
        accepted=value;return accept(9);
    }
};
int main(){
    for(unsigned failure=0;failure<=9;++failure){
        S state;downwell::GameMakerRandom random(23),oracle(23);Ops ops{state,random,failure,{}};
        const bool ok=state.create({0x43788000u},{0x43070000u},random,ops);
        assert(ok==!failure&&state.initialized==!failure&&ops.calls.size()==(failure?failure:9));
        for(unsigned i=0;i<ops.calls.size();++i)assert(ops.calls[i]==i+1);
        if(!failure||failure>=3){
            N expected;assert(downwell::randomBinary64(oracle.nextU32(),{0x3fe00000u,0},expected)&&expected.add(N::fromInteger(1)));
            assert(same(state.velocity,expected));
        }
        assert(random.drawCount()==oracle.drawCount()&&random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
        assert(ops.alarms[0]==(!failure||failure>=8?10:-1));
        assert(ops.alarms[2]==(!failure||failure>=9?30:-1));
        if(!failure){
            assert(!state.bulletThrough&&!state.hit&&!state.grounded&&state.imageSpeed.bits==0x3e4ccccdu);
            assert(same(state.acceleration,{0x3fa99999u,0x9999999au})&&same(state.deceleration,{0x3fd00000u,0}));
            assert(!state.create({}, {},random,ops)&&ops.calls.size()==9&&random.drawCount()==1);
        }
    }
    std::puts("Red-ball Create: inherited publication, RNG, heading readback, ordered alarms and failure prefixes passed");
}
#endif

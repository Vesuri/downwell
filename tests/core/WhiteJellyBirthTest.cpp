#include "core/WhiteJellyBirth.h"
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstdio>
#include <vector>
using downwell::WhiteJellyBirth;
using N=downwell::MotionBinary64;
using F=downwell::RunnerImageValue;
struct Ops {
    WhiteJellyBirth& state;downwell::GameMakerRandom& random;
    unsigned reject=0;std::vector<int> calls;
    bool accept(int code){calls.push_back(code);return calls.size()!=reject;}
    bool position(bool vertical,F wanted,F& accepted){
        assert(state.alive&&state.health==1&&state.money==1&&!state.scriptActive&&!random.drawCount());
        assert(wanted.bits==(vertical?0x43000000u:0x43800000u)); // 128,256
        if(vertical)assert(state.x.bits==0x43800000u&&state.y.bits==0x43070000u);
        accepted=wanted;return accept(vertical?2:1);
    }
    bool playerY(N& value){
        assert(state.health==25&&state.money==6&&random.drawCount()==1&&!state.scriptActive);
        value=N::fromInteger(80);return accept(3);
    }
    bool playerX(N& value){value=N::fromInteger(240);return accept(4);}
    bool pointDirection(N x,N y,N px,N py,N& out){
        assert(x.high==N::fromInteger(256).high&&y.high==N::fromInteger(128).high&&
            px.high==N::fromInteger(240).high&&py.high==N::fromInteger(80).high);
        out={0x405ec000u,1};return accept(5); // exercise exact readback without assuming setter narrowing
    }
    bool direction(N angle,N& out){out=angle;return accept(6);}
    bool imageSpeed(F value,F& out){
        assert(state.scriptActive&&state.playerDirection.high==state.direction.high&&
            state.playerDirection.low==state.direction.low&&value.bits==0x3e4ccccdu);
        out=value;return accept(7);
    }
};
int main(){
    for(unsigned failure=0;failure<=7;++failure){
        WhiteJellyBirth state;downwell::GameMakerRandom random(23),oracle(23);
        Ops ops{state,random,failure,{}};
        const bool ok=state.create(F{0x43788000u},F{0x43070000u},random,ops); // 248.5,135
        assert(ok==!failure&&state.initialized==!failure&&ops.calls.size()==(failure?failure:7));
        for(unsigned i=0;i<ops.calls.size();++i)assert(ops.calls[i]==int(i+1));
        if(!failure||failure>=3){
            N expected;assert(downwell::randomBinary64(oracle.nextU32(),{0x3fe00000u,0},expected)&&expected.add(N::fromInteger(1)));
            assert(state.velocity.high==expected.high&&state.velocity.low==expected.low);
        }
        assert(random.drawCount()==oracle.drawCount()&&random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
        if(!failure){
            assert(state.direction.high==0x405ec000u&&state.direction.low==1&&state.imageSpeed.bits==0x3e4ccccdu);
            assert(state.imageRate.low==0x9999999au&&state.imageRate.high==0x3fc99999u);
            assert(!state.hit&&!state.stomped&&!state.grounded&&!state.bulletThrough);
            assert(!state.create({}, {},random,ops)&&random.drawCount()==1&&ops.calls.size()==7);
        }
    }
    F out;
    assert(WhiteJellyBirth::snap({0x41000000u},out)&&out.bits==0); // 8 -> 0
    assert(WhiteJellyBirth::snap({0x41c00000u},out)&&out.bits==0x42000000u); // 24 ->32
    assert(WhiteJellyBirth::snap({0xc1c00000u},out)&&out.bits==0xc2000000u);
    std::puts("White jelly Create: inherited setter order, full RNG state, float publication and failure prefixes passed");
}

#include "core/RedBallStep.h"
#include "core/RunnerAlarmValue.h"
#include "core/RunnerDirectionValue.h"
#include "core/GameMakerRandom.h"
using S=downwell::RedBallStep;using N=downwell::MotionBinary64;using F=downwell::RunnerImageValue;
#ifdef DOWNWELL_FREESTANDING
bool nativeAlarmValue(N value,std::int32_t& out){return downwell::runnerAlarmValue(value,out);}
struct Ops {
    bool direction(N,N&);bool damage(int,int,int);bool movingFx(N,N,int,N,N,N);
    bool effectAlarmReset(N);bool viewY(N&);bool room(int&);bool destroy();
};
bool nativeRedBallEvents(S& s,bool stopped,downwell::GameMakerRandom& random,Ops& ops){
    return s.enemyContact(random,ops)&&s.playerContact(ops)&&s.hitStunAlarm()&&
        s.effectAlarm(stopped,random,ops)&&s.endStep(ops);
}
#else
#include <cassert>
#include <vector>
#include <cstdio>
#include <cstring>
static bool same(N a,N b){return a.high==b.high&&a.low==b.low;}
int main(){
    const auto checkAlarm=[](double value){
        std::uint64_t bits;std::memcpy(&bits,&value,8);
        N input{unsigned(bits>>32),unsigned(bits)};std::int32_t ticks=0;
        assert(downwell::runnerAlarmValue(input,ticks)&&ticks==static_cast<std::int32_t>(value));
    };
    for(int i=-10000;i<=10000;++i)checkAlarm(double(i)+0.75);
    for(double value:{-2147483648.75,-2147483648.0,2147483647.75,0.0,-0.0})checkAlarm(value);
    std::int32_t ticks=0;
    assert(downwell::runnerAlarmValue({0,1},ticks)&&ticks==0);
    assert(!downwell::runnerAlarmValue({0x41e00000u,0},ticks));
    assert(!downwell::runnerAlarmValue({0x7ff00000u,0},ticks));
    assert(!downwell::runnerAlarmValue({0xc1e00000u,0x00200000u},ticks));
    for(bool reject:{false,true}){
        S s;s.initialized=true;s.direction=N::fromInteger(359);
        downwell::GameMakerRandom random(91),oracle(91);
        N wanted,offset;
        assert(downwell::randomBinary64(oracle.nextU32(),N::fromInteger(40),offset)&&offset.add(N::fromInteger(-20)));
        wanted=N::fromInteger(359);assert(wanted.add(offset));
        struct Random {
            S& s;downwell::GameMakerRandom& random;
            unsigned nextU32(){s.direction=N::fromInteger(90);return random.nextU32();}
        } proxy{s,random};
        struct Ops {
            N expected;bool reject;unsigned calls=0;
            bool direction(N wanted,N& accepted){
                ++calls;assert(same(wanted,expected));F f;
                if(reject)return false;assert(downwell::RunnerDirectionValue::normalize(wanted,f));accepted=f.promoted();return true;
            }
        } ops{wanted,reject};
        assert(s.enemyContact(proxy,ops)==!reject&&ops.calls==1&&random.drawCount()==1);
        if(reject)assert(same(s.direction,N::fromInteger(90))); // Capture heading before RNG.
        else {F f;assert(downwell::RunnerDirectionValue::normalize(wanted,f)&&same(s.direction,f.promoted()));}
        assert(random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
        struct Player {
            bool reject;unsigned calls=0;
            bool damage(int amount,int direction,int time){assert(amount==1&&direction==3&&time==2);++calls;return !reject;}
        } player{reject};
        assert(s.playerContact(player)==!reject&&player.calls==1);
        s.hitStun=true;assert(s.hitStunAlarm()&&!s.hitStun);
    }
    for(bool stopped:{false,true})for(unsigned reject=0;reject<=(stopped?1u:2u);++reject){
        S s;s.initialized=true;s.emitTimer=N::fromInteger(10);s.xx=N::fromInteger(2);s.yy=N::fromInteger(3);
        downwell::GameMakerRandom random(91),oracle(91);N angle;
        if(!stopped){assert(downwell::randomBinary64(oracle.nextU32(),N::fromInteger(40),angle)&&
            angle.add(N::fromInteger(-20))&&angle.add(N::fromInteger(90)));oracle.nextU32();}
        struct Ops {
            S& s;downwell::GameMakerRandom& random;N angle;unsigned reject,calls=0;
            bool movingFx(N x,N y,int sprite,N rate,N actualAngle,N speed){
                assert(calls==0&&same(x,s.xx)&&same(y,s.yy)&&sprite==704&&
                    same(rate,{0x3fb99999u,0x9999999au})&&same(actualAngle,angle)&&same(speed,{0x3fe00000u,0}));
                random.nextU32();s.emitTimer=N::fromInteger(17);return ++calls!=reject;
            }
            bool effectAlarmReset(N timer){assert(same(timer,s.emitTimer));return ++calls!=reject;}
        } ops{s,random,angle,reject};
        assert(s.effectAlarm(stopped,random,ops)==!reject&&ops.calls==(reject?reject:(stopped?1u:2u)));
        assert(random.drawCount()==oracle.drawCount()&&random.stateIndex()==oracle.stateIndex());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
    }
    for(int room:{11,12})for(int relative:{-201,-160,-140,-124,0,3000,3001}){
        std::vector<int> expected{1};if(relative>=-160)expected.push_back(1);
        auto destroy=[&]{expected.push_back(2);if(room==11)expected.push_back(3);};
        if(relative< -160||relative>3000)destroy();
        expected.push_back(1);if(relative< -124)destroy();
        for(unsigned reject=0;reject<=expected.size();++reject){
            S s;s.initialized=true;assert(s.y.assignInteger(relative+100));
            struct Ops {
                int roomId;unsigned reject;std::vector<int> calls;
                bool accept(int code){calls.push_back(code);return calls.size()!=reject;}
                bool viewY(N& value){value=N::fromInteger(100);return accept(1);}
                bool room(int& value){value=roomId;return accept(2);}
                bool destroy(){return accept(3);}
            } ops{room,reject,{}};
            assert(s.endStep(ops)==!reject&&ops.calls.size()==(reject?reject:expected.size()));
            for(unsigned i=0;i<ops.calls.size();++i)assert(ops.calls[i]==expected[i]);
        }
    }
    std::puts("Red-ball events: contact RNG, direct damage, stopped alarm reset and post-destroy End Step ordering passed");
}
#endif

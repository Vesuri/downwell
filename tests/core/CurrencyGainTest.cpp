#include "core/CurrencyGainAccountOperations.h"
#include "core/GameMakerRandom.h"
#include "core/RunnerRealComparison.h"
#include <initializer_list>
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace downwell;
struct Trace {unsigned calls=0,fail=0;bool destroyed=false;int ids[32]{};bool call(int id){ids[calls++]=id;return calls!=fail;}};
struct Controls {
    using Handle=unsigned;
    CurrencyAccount& account;CurrencyCreate& state;Trace& trace;unsigned flags;
    bool achievement(int id){assert(account.gameGem==0&&account.streak==0);return trace.call(id);}
    bool timerStart(std::uint32_t& timer){if(!trace.call(1))return false;timer=300;return true;}
    bool lessResist(){return flags&1;}
    bool resetControllers(){assert(account.gameGem==unsigned(state.gemAmount)&&account.streak==unsigned(state.gemAmount)&&account.timer==(flags&1?480u:300u));return trace.call(2);}
    bool gemPop(){return flags&2;}
    bool gemPowered(){return flags&4;}
    bool playerPosition(MotionBinary64& x,MotionBinary64& y){x=MotionBinary64::fromInteger(240);y=MotionBinary64::fromInteger(512);return trace.call(3);}
    bool effect(MotionBinary64 x,MotionBinary64 y,int sprite,MotionBinary64 speed,int angle,int depth){
        assert(x.high==0x406e0000u&&y.high==0x407f8000u&&sprite==120&&speed.high==0x3fd99999u&&speed.low==0x9999999au&&!angle&&!depth);return trace.call(4);}
    bool create(int id,MotionBinary64,MotionBinary64,Handle& h){assert(id==266);if(!trace.call(5))return false;h=800;return true;}
    bool myBullet(Handle h){assert(h==800);return trace.call(6);}
    bool bulletDirection(MotionBinary64 direction){int order;assert(compareRunnerReal(direction,MotionBinary64::fromInteger(88),order)&&order>=0);return trace.call(7);}
    bool bulletSprite(int sprite){assert(sprite==(state.gemAmount>=10?473:472));return trace.call(8);}
    bool bulletDamage(int damage){assert(damage==(state.gemAmount>=10?30:20));return trace.call(9);}
    bool bulletSpeed(int speed){assert(speed==6);return trace.call(10);}
    bool sound(int id,int priority,bool loop,bool cut){assert(id==341&&priority==70&&!loop&&cut);return trace.call(11);}
    bool soundOverlap(int id,int priority,bool loop,bool cut,const char* name){assert(id==167&&priority==60&&!loop&&cut&&std::strcmp(name,"UI")==0);return trace.call(12);}
    bool fxNoLoop(int sprite,MotionBinary64 speed){assert(sprite==105&&speed.high==0x3fe99999u&&speed.low==0x9999999au);return trace.call(13);}
    bool recharge(){return trace.call(14);}
    bool destroy(){if(!trace.call(16))return false;trace.destroyed=true;return true;}
};
int main(){
    unsigned cases=0,prefixes=0;
    for(unsigned seed=0;seed<8;++seed)for(unsigned flags=0;flags<8;++flags)for(unsigned balance:{0u,2999u,4999u})for(int amount:{2,10}){
        CurrencyCreate state;state.obtainable=true;CurrencyAccount account;account.currency=balance;Trace trace;GameMakerRandom random(seed);
        CurrencyGainAccountOperations<Controls> ops{account,{account,state,trace,flags}};bool good=true;
        assert(CurrencyGainScript::run(state,amount,random,ops,good)&&good&&trace.destroyed);
        assert(account.currency==balance+unsigned(amount)&&account.gameGem==unsigned(amount)&&account.streak==unsigned(amount)&&
            state.gemAmount==amount&&random.drawCount()==((flags&2)?1u:0u));
        int expected[32];unsigned n=0;
        if(balance+amount>=3000)expected[n++]=15;if(balance+amount>=5000)expected[n++]=22;
        expected[n++]=1;expected[n++]=2;
        if(flags&2){for(int id:{3,4,3,5,6,7,8,9,10,11})expected[n++]=id;}else expected[n++]=12;
        expected[n++]=13;if(flags&4)expected[n++]=14;expected[n++]=16;
        assert(trace.calls==n);for(unsigned i=0;i<n;++i)assert(trace.ids[i]==expected[i]);
        for(unsigned fail=1;fail<=n;++fail){
            CurrencyCreate s;s.obtainable=true;CurrencyAccount a;a.currency=balance;Trace t;t.fail=fail;GameMakerRandom rng(seed);
            CurrencyGainAccountOperations<Controls> rejected{a,{a,s,t,flags}};bool ok=true;
            assert(!CurrencyGainScript::run(s,amount,rng,rejected,ok)&&!ok&&!t.destroyed&&t.calls==fail&&a.currency==balance+unsigned(amount));
            for(unsigned i=0;i<fail;++i)assert(t.ids[i]==expected[i]);const auto draws=rng.drawCount();
            assert(!CurrencyGainScript::run(s,amount,rng,rejected,ok)&&t.calls==fail&&rng.drawCount()==draws);++prefixes;
        }
        state.obtainable=false;trace={};const auto draws=random.drawCount();const auto money=account.currency;
        assert(CurrencyGainScript::run(state,amount,random,ops,good)&&!trace.calls&&random.drawCount()==draws&&account.currency==money);++cases;
    }
    std::printf("Currency collection: %u source-order/upgrade cases and %u retained failure prefixes passed\n",cases,prefixes);
}

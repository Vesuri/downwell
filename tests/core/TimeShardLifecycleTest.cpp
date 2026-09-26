#include "core/TimeShardLifecycle.h"
// UTMT 0.9.2.0 gml_Object_fxTimeShard_Collision_objPlayer_n.asm SHA-256:
// 90ff28fd6ee603745ef5d9c75b0062ec0a0c62a9bb5852285ef2c052bdc00ce7.
// data.win: c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8.
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace downwell;
static MotionBinary64 number(double d){unsigned long long n;std::memcpy(&n,&d,8);return {unsigned(n>>32),unsigned(n)};}
static void equal(MotionBinary64 a,double d){const auto b=number(d);assert(a.high==b.high && a.low==b.low);}
int main(){
    for(unsigned seed=0;seed<512;++seed)for(double x:{-3.0,-0.0,0.0,.25})for(double y:{-2.0,0.0,.5}){
        GameMakerRandom rng(seed),oracle(seed);TimeShardLifecycle shard;
        assert(shard.playerContact(number(x),number(y),rng));assert(rng.drawCount()==0);
        for(unsigned tick=0;tick<29;++tick){shard.advanceAlarm(2);assert(shard.noTouch);}
        shard.advanceAlarm(2);assert(!shard.noTouch);
        assert(shard.playerContact(number(x),number(y),rng));
        equal(shard.xsp,(x>0?1:x<0?-1:0)*oracle.random(2));
        equal(shard.ysp,(y>0?1:y<0?-1:0)*oracle.random(2));
        assert(shard.noTouch && shard.alarm2==5 && rng.drawCount()==2);
        for(unsigned i=0;i<16;++i)assert(rng.stateWord(i)==oracle.stateWord(i));
        for(unsigned tick=0;tick<4;++tick){
            shard.advanceAlarm(2);assert(shard.playerContact(number(x),number(y),rng));assert(rng.drawCount()==2);
        }
        shard.advanceAlarm(2);assert(!shard.noTouch);
        assert(shard.playerContact(number(x),number(y),rng));assert(rng.drawCount()==4);
    }
    TimeShardLifecycle shard;shard.alarm0=2;shard.alarm1=1;
    shard.advanceAlarm(0);shard.advanceAlarm(1);assert(shard.disappearing && !shard.destroyRequested);
    shard.advanceAlarm(0);assert(shard.destroyRequested);
    const int cooldown=shard.alarm2;shard.advanceAlarm(2);assert(shard.alarm2==cooldown);
    GameMakerRandom rng(42);assert(shard.create({}, {},false,rng));
    assert(!shard.destroyRequested && !shard.disappearing && shard.noTouch);
    shard.alarm1=0;shard.advanceAlarm(1);assert(!shard.disappearing && shard.alarm1==-1);
    shard.advanceAlarm(1);assert(shard.alarm1==-1);
    std::puts("Time-shard lifecycle: 6144 contact/RNG cases, initial and repeat cooldowns, alarm retirement and reuse passed");
}

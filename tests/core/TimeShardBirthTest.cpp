#include "core/TimeShardBirth.h"
// UTMT 0.9.2.0 gml_Object_fxTimeShard_Create_0.asm SHA-256:
// 1027a202521a5862c416313cf8c613be1b3878275202e03e54b3089bbb9d9f98.
// data.win: c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8.
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cstring>
#include <cstdio>
using namespace downwell;
static double value(MotionBinary64 n){std::uint64_t b=(std::uint64_t(n.high)<<32)|n.low;double d;std::memcpy(&d,&b,8);return d;}
static unsigned bits(float f){unsigned n;std::memcpy(&n,&f,4);return n;}
int main(){
    for(unsigned seed=0;seed<4096;++seed){
        GameMakerRandom random(seed),oracle(seed);TimeShardBirth shard;
        assert(shard.create(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(176),seed&1,random));
        const int vx=int(oracle.irandom(4))-2;
        const int vy[]={0,-1,-1,-1,-2,-2,-3};const int y=vy[oracle.nextU32()%7];
        const double speeds[]={-.5,-.3,0,.3,.5};const double speed=speeds[oracle.nextU32()%5];
        const float image=float(oracle.random(7));const int life=int(90+oracle.random(70));
        assert(value(shard.xsp)==vx && value(shard.ysp)==y && value(shard.savedSpeed)==speed);
        assert(shard.imageSpeed.bits==bits(float(speed)) && shard.imageIndex.bits==bits(image));
        assert(shard.alarm0==life && shard.alarm1==40 && shard.alarm2==30);
        assert(shard.shardLimit==(seed&1?3:20) && !shard.disappearing && shard.dflash==-1 && shard.noTouch);
        assert(value(shard.xx)==240 && value(shard.yy)==176 && value(shard.gravity)==.08 && value(shard.hardGravity)==.1);
        assert(random.drawCount()==oracle.drawCount() && random.drawCount()==6);
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
    }
    std::puts("Time-shard Create: 4096 seeds, six ordered RNG draws, binary64 locals/binary32 fields and alarms passed");
}

#include "core/BloodEffect.h"
// Source: UTMT 0.9.2.0 Assembly exports from data.win SHA-256
// c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8.
// gml_GlobalScript_scrBloodfx.asm:
// 84f30940e9d06d69b3cd886a53803106c9418e42a9b7df747e4d75b301cd897f
// gml_GlobalScript_emitMovingFx.asm:
// 5971093b936584478f261fa56f1201f3a174b1bb750d9a395008d9e4e6d3b108
// gml_Object_parentMovingFx_Create_0.asm:
// ee1e21a6d8c86f140da3bed0f2eb66e0828f4d2843c9021ac70dac8823c7f64d
// gml_Object_parentMovingFx_Step_0.asm:
// e2d91724e976a16d0025b17b15b7ab34c824fbb022f5e9c13431b1597e2680a5
// Movement oracle below isolates integration from the direction calculation;
// it does not independently validate trig or the Windows runner FP environment.
#include "core/GameMakerRandom.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
using namespace downwell;
static double value(MotionBinary64 n){std::uint64_t bits=(std::uint64_t(n.high)<<32)|n.low;double d;std::memcpy(&d,&bits,8);return d;}
static unsigned bits(float f){unsigned n;std::memcpy(&n,&f,4);return n;}
int main(){
    unsigned checked=0;
    for(unsigned seed=0;seed<256;++seed){
        GameMakerRandom random(seed),reference(seed);BloodEffect effects[2];unsigned count=0;
        assert(bloodBurst(MotionBinary64::fromInteger(240),MotionBinary64::fromInteger(176),random,
            [&](auto x,auto y,auto angle){
                const double raw=double(reference.nextU32())/4294967296.0*20;
                assert(value(angle)==(count?180-raw:raw));
                return effects[count++].create(x,y,angle);
            }));
        assert(count==2 && random.drawCount()==reference.drawCount());
        for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==reference.stateWord(i));
        for(auto& effect:effects){
            double x=240,y=176;float phase=0,rate=.5f;unsigned saved=0;
            for(unsigned tick=0;tick<40 && !effect.destroyRequested;++tick){
                assert(effect.advanceImage());phase+=rate;
                if(phase>=9){assert(effect.destroyRequested);break;}
                const bool allowed=tick%7>=3;
                if(!saved)saved=bits(rate);
                rate=allowed?.5f:0;
                if(allowed){x+=value(effect.xsp);y+=value(effect.ysp);}
                assert(effect.step(allowed));
                assert(value(effect.xx)==x && value(effect.yy)==y && effect.x.bits==bits(float(std::nearbyint(x))) &&
                    effect.y.bits==bits(float(std::nearbyint(y))) && effect.imageSpeed.bits==bits(rate) &&
                    effect.savedSpeed.bits==saved && effect.drawFrame()==unsigned(phase));
                ++checked;
            }
            assert(effect.destroyRequested);
        }
        GameMakerRandom fail(seed),oracle(seed);oracle.nextU32();
        assert(!bloodBurst({}, {},fail,[](auto,auto,auto){return false;}));
        assert(fail.drawCount()==oracle.drawCount());
        for(unsigned i=0;i<16;++i)assert(fail.stateWord(i)==oracle.stateWord(i));
        // Prove the second angle is evaluated after synchronous child creation,
        // including any future creation hook's random consumption.
        GameMakerRandom interleaved(seed),ordered(seed);unsigned births=0;
        assert(bloodBurst({}, {},interleaved,[&](auto,auto,auto angle){
            const double raw=double(ordered.nextU32())/4294967296.0*20;
            assert(value(angle)==(births?180-raw:raw));
            assert(interleaved.nextU32()==ordered.nextU32());
            ++births;return true;
        }));
        assert(births==2 && interleaved.drawCount()==ordered.drawCount());
        for(unsigned i=0;i<16;++i)assert(interleaved.stateWord(i)==ordered.stateWord(i));
    }
    std::printf("Blood effects: %u movement/time-stop frames, 512 births, retirement and RNG failure ordering passed\n",checked);
}

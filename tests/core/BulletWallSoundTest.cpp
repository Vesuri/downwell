#include "core/BulletWallSound.h"
#include "core/GameMakerRandom.h"
#include "BulletWallSoundChecks.h"
#include <cassert>
#include <cstdio>
using namespace downwell;
int main(){
    const char* materials[]={"breakable","thin","foliage","rock","metal","wood",""};
    unsigned cases=0;
    for(unsigned seed=0;seed<512;++seed)for(unsigned material=0;material<7;++material)
    for(bool water:{false,true})for(unsigned fail=0;fail<=8;++fail){
        GameMakerRandom random(seed),oracle(seed);SoundPlayState caller;caller.sfxIndex=123;
        SoundPlayEnvironment environment;environment.gameVolume=AudioLevel::raw(16384);environment.inWater=water;
        const unsigned index=material<5?material:0;
        // The general-width integer path checks the optimized small-range
        // path, including equal bounds. The table is pinned by the tool.
        const int choice=1+int(oracle.irandom(index<3?0:1));
        const int sound=index<3?29:index==3?33+choice:31+choice;
        int channel=41;bool good=true;unsigned reads=0;
        BulletWallAudio audio{caller,41,sound,water,fail?fail-1:0};
        auto read=[&](const char*& out){++reads;assert(caller.sfxIndex==0&&random.drawCount()==0);
            out=materials[material];return fail!=1;};
        assert(BulletWallSound::play(caller,environment,channel,random,read,audio,good)==!fail&&good==!fail&&reads==1);
        assert(audio.valid&&audio.calls==(fail==1?0:fail?fail-1:7));
        if(fail==1){assert(caller.sfxIndex==0&&random.drawCount()==0&&channel==41);}
        else {
            assert(random.drawCount()==2&&random.stateIndex()==oracle.stateIndex());
            for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==oracle.stateWord(i));
            assert(caller.sfxIndex==sound&&channel==(!fail||fail>=4?sound:41));
        }
        if(fail){const auto draws=random.drawCount();const auto calls=audio.calls;
            assert(!BulletWallSound::play(caller,environment,channel,random,read,audio,good)&&reads==1&&
                random.drawCount()==draws&&audio.calls==calls);}
        ++cases;
    }
    std::printf("Bullet wall sound: %u material/RNG/audio/failure cases passed\n",cases);
}

#ifndef DOWNWELL_CORE_BULLET_WALL_SOUND_H
#define DOWNWELL_CORE_BULLET_WALL_SOUND_H
#include "SoundPlayScript.h"
namespace downwell {
// soundBullet + the immutable initSfxBhit table. The caller retains script
// fields; the target material must be read from its selected live lifetime.
struct BulletWallSound {
    static unsigned materialIndex(const char* material){
        const char* names[]={"breakable","thin","foliage","rock","metal"};
        for(unsigned i=0;i<5;++i){const char* a=material;const char* b=names[i];
            while(*a&&*a==*b){++a;++b;}if(*a==*b)return i;}
        return 0; // Authored switch leaves sfxIndex at zero for other strings.
    }
    template<class Random,class Material,class Audio>
    static bool play(SoundPlayState& caller,SoundPlayEnvironment& environment,int& channel,
        Random& random,Material readMaterial,Audio& audio,bool& good){
        if(!good)return false;
        caller.sfxIndex=0;const char* material=nullptr;
        if(!readMaterial(material)||!material||!good)return good=false;
        const unsigned index=materialIndex(material);caller.sfxIndex=int(index);
        const int sounds[5][2]={{29,29},{29,29},{29,29},{34,35},{32,33}};
        // irandom_range(1,count) consumes two words even when count is one.
        const unsigned chosen=random.irandomSmall(index<3?0:1);
        int result=-1;bool returned=false;
        return SoundPlayScript::overlap(caller,environment,audio,[]{return false;},channel,
            sounds[index][chosen],60,false,true,"gunwall",result,returned,good);
    }
};
}
#endif

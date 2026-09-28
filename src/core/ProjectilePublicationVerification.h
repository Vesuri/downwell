#ifndef DOWNWELL_PROJECTILE_PUBLICATION_VERIFICATION_H
#define DOWNWELL_PROJECTILE_PUBLICATION_VERIFICATION_H
#include "ProjectilePublication.h"
namespace downwell {
// Shared host/68000 conversion check, independent of trajectory scheduling.
struct ProjectilePublicationVerification {
    static bool run(){
        auto check=[](int pixel,unsigned fraction,int origin){
            RunnerImageValue actual{0xdeadbeefu},reference=actual;
            const bool a=ProjectilePublication::coordinate({pixel,fraction},origin,actual);
            const bool b=ProjectilePublication::coordinateBinary64({pixel,fraction},origin,reference);
            return a==b&&actual.bits==reference.bits;
        };
        const int pixels[]={-2147483647-1,-16777217,-4097,-1,0,1,4095,16777217,2147483647};
        const unsigned fractions[]={0,1,524287,524288,524289,1048575};
        for(auto p:pixels)for(auto f:fractions)if(!check(p,f,0)||!check(p,f,240))return false;
        for(unsigned exponent=4;exponent<24;++exponent){
            const int p=int(1u<<exponent);const unsigned half=1u<<(exponent-4);
            for(unsigned f=half-1;f<=half+1;++f)
                if(!check(p,f,0)||!check(-p-1,f,0))return false;
        }
        return check(0,0,-1)&&check(0,1048576,0)&&check(2147483647,0,1);
    }
};
}
#endif

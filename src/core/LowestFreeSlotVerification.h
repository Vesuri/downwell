#ifndef DOWNWELL_LOWEST_FREE_SLOT_VERIFICATION_H
#define DOWNWELL_LOWEST_FREE_SLOT_VERIFICATION_H
#include "LowestFreeSlotMap.h"
namespace downwell {
struct LowestFreeSlotVerification {
    static_assert(sizeof(LowestFreeSlotMap<3323>)==436,"Gameplay free map storage");
    template<unsigned N> static bool size(){
        LowestFreeSlotMap<N> map;
        for(unsigned i=0;i<N;++i){
            if(map.first()!=i||map.first(i)!=i)return false;
            map.occupy(i);
        }
        if(map.first()!=N||map.first(N)!=N||map.first(0xffffffffu)!=N)return false;
        for(unsigned i=0;i<N;i+=31)map.release(i);
        for(unsigned start=0;start<N;++start){
            const unsigned next=start+(31-start%31)%31;
            if(map.first(start)!=(next<N?next:N))return false;
        }
        // Summary words must become empty again as their final slot is taken.
        for(unsigned i=0;i<N;i+=31)map.occupy(i);
        if(map.first()!=N)return false;
        if(N){map.release(N-1);if(map.first()!=N-1)return false;}
        map.clear();
        return map.first()==(N?0:N)&&map.first(N)==N;
    }
    static bool run(){return size<0>()&&size<1>()&&size<31>()&&size<32>()&&
        size<33>()&&size<1023>()&&size<1024>()&&size<1025>()&&size<3323>();}
};
}
#endif

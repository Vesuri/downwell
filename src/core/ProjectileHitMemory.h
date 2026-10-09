#ifndef DOWNWELL_CORE_PROJECTILE_HIT_MEMORY_H
#define DOWNWELL_CORE_PROJECTILE_HIT_MEMORY_H
#include <cstdint>
namespace downwell {
// hitIgnoreCheck overwrites its argument with other.id. Keep the loop's final
// i and scan the whole prefix even after a match. Capacity exhaustion is a
// terminal owner error, never permission to damage a target a second time.
template<unsigned Capacity> struct ProjectileHitMemory {
    static_assert(Capacity>0,"hit memory needs its initial -1 entry");
    std::int32_t ids[Capacity]={-1};
    std::uint32_t checkTarget=0;
    unsigned count=0,i=0;bool hitIgnore=false;
    bool check(std::uint32_t otherId,bool& good){
        if(!good)return false;
        checkTarget=otherId;hitIgnore=false;
        if(!otherId||otherId>0x7fffffffu||count>Capacity)return good=false;
        if(count)for(i=0;i<count;++i)if(ids[i]==std::int32_t(checkTarget))hitIgnore=true;
        if(!hitIgnore){
            if(count==Capacity)return good=false;
            ids[count++]=std::int32_t(checkTarget);
        }
        return true;
    }
};
}
#endif

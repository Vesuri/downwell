#ifndef DOWNWELL_CORE_OBJECT_ALARM_METADATA_H
#define DOWNWELL_CORE_OBJECT_ALARM_METADATA_H
#include "ObjectPack.h"
namespace downwell {
// Cache only immutable inherited event existence, never instance countdowns
// or lifetime state. Lazy population avoids scanning unused object families.
template<unsigned Capacity> class ObjectAlarmMetadata {
public:
    bool bind(const ObjectPack& pack){
        pack_=nullptr;
        if(!pack.valid()||pack.objectCount()>Capacity)return false;
        for(unsigned i=0;i<pack.objectCount();++i)masks_[i]=0xffffu;
        pack_=&pack;return true;
    }
    bool mask(int id,unsigned& result){
        if(!pack_||id<0||unsigned(id)>=pack_->objectCount())return false;
        auto& cached=masks_[id];
        if(cached==0xffffu){
            unsigned bits=0;
            for(unsigned alarm=0;alarm<12;++alarm){
                PackedObjectEvent event;
                if(pack_->findEvent(id,2,int(alarm),event))bits|=1u<<alarm;
            }
            cached=std::uint16_t(bits);
        }
        result=cached;return true;
    }
private:
    const ObjectPack* pack_=nullptr;
    std::uint16_t masks_[Capacity?Capacity:1]{};
};
}
#endif

#ifndef DOWNWELL_CORE_OBJECT_ALARM_METADATA_H
#define DOWNWELL_CORE_OBJECT_ALARM_METADATA_H
#include "ObjectPack.h"
namespace downwell {
// Cache only immutable inherited event metadata, never instance countdowns
// or lifetime state. Lazy population avoids scanning unused object families.
template<unsigned Capacity> class ObjectAlarmMetadata {
    static_assert(Capacity<=0xffffffffu/12u,"Alarm cache keys must not wrap");
public:
    bool bind(const ObjectPack& pack){
        pack_=nullptr;
        if(!pack.valid()||pack.objectCount()>Capacity)return false;
        for(unsigned i=0;i<pack.objectCount();++i)masks_[i]=0xffffu;
        for(auto& entry:events_)entry.key=0xffffffffu;
        pack_=&pack;return true;
    }
    bool mask(int id,unsigned& result){
        if(!pack_||id<0||unsigned(id)>=pack_->objectCount())return false;
        auto& cached=masks_[id];
        if(cached==0xffffu){
            unsigned bits=0;
            for(unsigned alarm=0;alarm<12;++alarm){
                PackedObjectEvent event;
                if(pack_->findEvent(id,2,int(alarm),event)){
                    bits|=1u<<alarm;remember(id,alarm,event);
                }
            }
            cached=std::uint16_t(bits);
        }
        result=cached;return true;
    }
    bool findEvent(int id,unsigned alarm,PackedObjectEvent& out){
        if(!pack_||id<0||unsigned(id)>=pack_->objectCount()||alarm>=12)return false;
        auto& entry=eventEntry(id,alarm);
        const unsigned key=unsigned(id)*12+alarm;
        if(entry.key==key){out=entry.event;return true;}
        if(!pack_->findEvent(id,2,int(alarm),out))return false;
        entry.event=out;entry.key=key;return true;
    }
private:
    struct EventEntry {unsigned key=0xffffffffu;PackedObjectEvent event{};};
    EventEntry events_[32]{};
    EventEntry& eventEntry(int id,unsigned alarm){return events_[(unsigned(id)^(unsigned(id)>>3)^(alarm<<1))&31u];}
    void remember(int id,unsigned alarm,const PackedObjectEvent& event){
        auto& entry=eventEntry(id,alarm);entry.event=event;entry.key=unsigned(id)*12+alarm;
    }
    const ObjectPack* pack_=nullptr;
    std::uint16_t masks_[Capacity?Capacity:1]{};
};
}
#endif

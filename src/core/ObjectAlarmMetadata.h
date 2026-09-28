#ifndef DOWNWELL_CORE_OBJECT_ALARM_METADATA_H
#define DOWNWELL_CORE_OBJECT_ALARM_METADATA_H
#include "ObjectPack.h"
namespace downwell {
// Prepare immutable inherited handlers before gameplay, never countdowns or
// lifetime state. Sparse rows contain only present alarms, in alarm order.
template<unsigned Capacity,unsigned EventCapacity=512> class ObjectAlarmMetadata {
    static_assert(EventCapacity<=65535u,"Alarm row offsets must fit");
public:
    bool bind(const ObjectPack& pack){
        count_=0;
        if(!pack.valid()||pack.objectCount()>Capacity)return false;
        unsigned used=0;
        for(unsigned id=0;id<pack.objectCount();++id){
            masks_[id]=0;offsets_[id]=std::uint16_t(used);
            for(unsigned alarm=0;alarm<12;++alarm){
                PackedObjectEvent event;
                if(!pack.findEvent(int(id),2,int(alarm),event))continue;
                if(used==EventCapacity)return false;
                events_[used++]={event.ownerObjectId,event.codeId};
                masks_[id]|=std::uint16_t(1u<<alarm);
            }
        }
        count_=pack.objectCount();return true;
    }
    bool mask(int id,unsigned& result)const{
        if(id<0||unsigned(id)>=count_)return false;
        result=masks_[id];return true;
    }
    bool findEvent(int id,unsigned alarm,PackedObjectEvent& out)const{
        if(id<0||unsigned(id)>=count_||alarm>=12)return false;
        const unsigned bit=1u<<alarm;
        if(!(masks_[id]&bit))return false;
        // Rank within a sparse row; bounded integer work, no resource walk.
        unsigned rank=masks_[id]&(bit-1u);
        rank=rank-((rank>>1)&0x5555u);
        rank=(rank&0x3333u)+((rank>>2)&0x3333u);
        rank=(rank+(rank>>4))&0x0f0fu;
        rank=(rank+(rank>>8))&0x1fu;
        const auto& event=events_[offsets_[id]+rank];
        out={event.owner,2,int(alarm),event.code};return true;
    }
private:
    struct EventEntry {std::int32_t owner,code;};
    EventEntry events_[EventCapacity?EventCapacity:1];
    unsigned count_=0;
    std::uint16_t masks_[Capacity?Capacity:1]{};
    std::uint16_t offsets_[Capacity?Capacity:1]{};
};
}
#endif

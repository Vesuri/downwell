#ifndef DOWNWELL_CORE_OBJECT_DRAW_METADATA_H
#define DOWNWELL_CORE_OBJECT_DRAW_METADATA_H
#include "ObjectPack.h"
namespace downwell {
// Immutable resource facts only. Instance visibility, sprite selection and
// lifetime remain live reads. Includes inherited ordinary Draw (8,0), not GUI
// or other Draw subevents. Rebind when replacing the object resource pack.
template<unsigned Capacity> class ObjectDrawMetadata {
public:
    static constexpr unsigned exists=1,drawEvent=2;
    bool bind(const ObjectPack& pack){
        count_=0;
        if(!pack.valid()||pack.objectCount()>Capacity)return false;
        for(unsigned id=0;id<pack.objectCount();++id){
            PackedObject object;PackedObjectEvent event;
            if(!pack.findObject(id,object))return false;
            entries_[id]=exists|(pack.findEvent(id,8,0,event)?drawEvent:0);
        }
        count_=pack.objectCount();return true;
    }
    unsigned get(int id)const{return id>=0&&unsigned(id)<count_?entries_[id]:0;}
private:
    std::uint8_t entries_[Capacity?Capacity:1]{};
    unsigned count_=0;
};
}
#endif

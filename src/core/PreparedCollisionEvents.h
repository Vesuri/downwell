#ifndef DOWNWELL_CORE_PREPARED_COLLISION_EVENTS_H
#define DOWNWELL_CORE_PREPARED_COLLISION_EVENTS_H
#include "ObjectPack.h"
namespace downwell {
// Scene-immutable decoded collision bindings, not a dense object-pair matrix.
// Preserve serialized duplicate order and resolve target ancestry BEFORE owner
// ancestry, exactly as ObjectPack::collisionEvent. The pack remains the oracle.
template<unsigned Objects,unsigned Events> class PreparedCollisionEvents {
    static_assert(Objects<=32767&&Events<=65535,"Compact metadata limits");
    struct Object {std::int16_t parent;std::uint16_t first,count;};
    struct Event {std::int32_t target,code;};
    Object objects_[Objects];Event events_[Events];
    unsigned count_=0;bool ready_=false;
public:
    bool ready()const{return ready_;}
    bool prepare(const ObjectPack& pack){
        ready_=false;count_=0;
        if(!pack.valid()||pack.objectCount()>Objects)return false;
        unsigned used=0;
        for(unsigned i=0;i<pack.objectCount();++i){
            PackedObject object;if(!pack.object(i,object))return false;
            auto& out=objects_[i];out={static_cast<std::int16_t>(object.parentId),static_cast<std::uint16_t>(used),0};
            for(unsigned j=0;j<object.bindingCount;++j){
                PackedObjectEvent event;if(!pack.binding(object.firstBinding+j,event))return false;
                if(event.eventType!=4)continue;
                if(used==Events)return false;
                events_[used++]={event.eventSubtype,event.codeId};++out.count;
            }
        }
        count_=pack.objectCount();ready_=true;return true;
    }
    bool collisionEvent(int self,int other,bool& found,PackedObjectEvent& output)const{
        if(!ready_||self<0||other<0||unsigned(self)>=count_||unsigned(other)>=count_)return false;
        for(int target=other;target>=0;target=objects_[target].parent){
            for(int owner=self;owner>=0;owner=objects_[owner].parent){
                const auto& object=objects_[owner];
                for(unsigned i=object.first,end=i+object.count;i<end;++i){
                    const auto& event=events_[i];
                    if(event.target==target){output={owner,4,target,event.code};found=true;return true;}
                }
            }
        }
        found=false;return true;
    }
};
}
#endif

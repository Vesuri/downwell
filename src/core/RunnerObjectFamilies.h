#ifndef DOWNWELL_CORE_RUNNER_OBJECT_FAMILIES_H
#define DOWNWELL_CORE_RUNNER_OBJECT_FAMILIES_H
#include "ObjectPack.h"
namespace downwell {
// Validated immutable topology for 0x47b700's parent/child links and 0x4798f0's
// spatial family marking. Runtime flags are supplied by the owner: DWOB flags
// are serialized properties and are NOT the runner's +0x50 flag word.
template<unsigned Capacity> class RunnerObjectFamilies {
    struct Entry {unsigned parent=Capacity,child=Capacity,next=Capacity;std::uint32_t flags=0;};
public:
    static constexpr unsigned End=Capacity;
    void clear(){count_=0;good_=false;}
    bool valid()const{return good_;}
    unsigned size()const{return count_;}
    // Loaded property bits from 0x47934b..0x4793b8 plus collision endpoints.
    // This is not a complete event-cache flag word.
    bool bindInstanceFlags(const ObjectPack& pack){
        if(!bindSpatialFlags(pack))return false;
        for(unsigned i=0;i<count_;++i){
            PackedObject object;
            if(!pack.object(i,object)){clear();return false;}
            entries_[i].flags|=(object.solid()?1u:0u)|(object.visible()?2u:0u)|
                (object.persistent()?4u:0u)|(object.managed()?0x80u:0u);
        }
        return true;
    }
    // Spatial-relevant subset of runner +0x50: constructor enables 0x10;
    // expanded collision-event endpoints get 0x08. Serialized visibility,
    // solidity, managed-object and event-cache bits are not represented here.
    bool bindSpatialFlags(const ObjectPack& pack){
        if(!bind(pack,[](const PackedObject&){return 0x10u;}))return false;
        for(unsigned owner=0;owner<count_;++owner){
            PackedObject object;
            if(!pack.object(owner,object)){clear();return false;}
            for(unsigned i=0;i<object.bindingCount;++i){
                PackedObjectEvent event;
                if(!pack.binding(object.firstBinding+i,event)){clear();return false;}
                if(event.eventType!=4)continue;
                // Inheriting owners retain a collision entry; target expansion
                // retains entries for descendants even when code is overridden.
                markFamily(owner,8u,8u);
                if(event.eventSubtype>=0 && unsigned(event.eventSubtype)<count_)
                    markFamily(unsigned(event.eventSubtype),8u,8u);
            }
        }
        return true;
    }
    template<class Flags> bool bind(const ObjectPack& pack,Flags flags){
        clear();
        if(!pack.valid() || pack.objectCount()>Capacity)return false;
        count_=pack.objectCount();
        for(unsigned i=0;i<count_;++i){
            PackedObject object;
            if(!pack.object(i,object)){clear();return false;}
            entries_[i]=Entry{};
            entries_[i].parent=object.parentId<0?End:unsigned(object.parentId);
            entries_[i].flags=flags(object);
        }
        // Reverse construction gives resource-ID child order. Marking only
        // changes flags, so sibling order cannot affect the resulting state.
        for(unsigned i=count_;i>0;--i){
            auto& child=entries_[i-1];
            if(child.parent==End)continue;
            child.next=entries_[child.parent].child;
            entries_[child.parent].child=i-1;
        }
        good_=true;return true;
    }
    bool isA(int child,int ancestor)const{
        if(!good_ || child<0 || ancestor<0 || unsigned(child)>=count_ || unsigned(ancestor)>=count_)return false;
        for(unsigned i=unsigned(child);i!=End;i=entries_[i].parent)
            if(i==unsigned(ancestor))return true;
        return false;
    }
    const std::uint32_t* flags(unsigned object)const{
        return good_ && object<count_?&entries_[object].flags:nullptr;
    }
    bool markSpatialFamily(unsigned object){
        if(!good_ || object>=count_)return false;
        markFamily(object,0x20u,0x28u);return true;
    }
    // 0x4879ca..0x4879f3: birth inherits preparation from an ancestor only
    // onto its own object. Unlike a query, this does not mark a whole subtree.
    bool prepareBirth(unsigned object){
        if(!good_ || object>=count_)return false;
        if(entries_[object].flags&0x28u)return true;
        for(unsigned parent=entries_[object].parent;parent!=End;parent=entries_[parent].parent){
            if(entries_[parent].flags&0x28u){entries_[object].flags|=0x20u;break;}
        }
        return true;
    }
private:
    void markFamily(unsigned object,std::uint32_t flag,std::uint32_t barrier){
        unsigned current=object;
        for(;;){
            auto& e=entries_[current];
            if(!(e.flags&barrier)){
                e.flags|=flag;
                if(e.child!=End){current=e.child;continue;}
            }
            // A prepared/collision-pair node is a subtree barrier, not just
            // an instruction to skip writing that node's flags.
            while(current!=object && entries_[current].next==End)
                current=entries_[current].parent;
            if(current==object)return;
            current=entries_[current].next;
        }
    }
    Entry entries_[Capacity?Capacity:1]{};
    unsigned count_=0;bool good_=false;
};
}
#endif

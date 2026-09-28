#ifndef DOWNWELL_PREPARED_OBJECT_DEFAULTS_H
#define DOWNWELL_PREPARED_OBJECT_DEFAULTS_H
#include "ObjectPack.h"
#include "SpriteCollisionPack.h"
#include "RunnerImageValue.h"
namespace downwell {
// Immutable resource-derived defaults, prepared before gameplay. Runtime
// family preparation bits, instance state and memberships are NOT cached.
struct PreparedObjectDefault {
    std::int32_t spriteId,maskId;
    RunnerImageValue depth,creationDepth;
    std::uint32_t propertyFlags,frames,initialFlags;
};
template<unsigned Capacity> class PreparedObjectDefaults {
    PreparedObjectDefault entries_[Capacity?Capacity:1];
    unsigned count_=0;
    bool good_=false;
public:
    bool prepare(const ObjectPack& objects,const SpriteCollisionPack& sprites){
        good_=false;count_=0;
        if(!objects.valid()||!sprites.valid()||objects.objectCount()>Capacity)return false;
        for(unsigned id=0;id<objects.objectCount();++id){
            PackedObject source;if(!objects.findObject(id,source))return false;
            auto& out=entries_[id];
            out.spriteId=source.spriteId;out.maskId=source.textureMaskId;
            if(!out.depth.assignInteger(source.depth)||!out.creationDepth.assignInteger(source.creationDepth))return false;
            out.propertyFlags=(source.solid()?1u:0u)|(source.visible()?2u:0u)|
                (source.persistent()?4u:0u)|(source.managed()?0x80u:0u);
            out.initialFlags=8u|(source.solid()?0x20u:0u)|(source.visible()?0x10u:0u)|
                (source.persistent()?0x40u:0u)|(source.managed()?0x200000u:0u);
            out.frames=0;
            if(source.spriteId>=0)sprites.frameCount(unsigned(source.spriteId),out.frames);
        }
        count_=objects.objectCount();good_=true;return true;
    }
    const PreparedObjectDefault* get(int id)const{
        return good_&&id>=0&&unsigned(id)<count_?&entries_[id]:nullptr;
    }
};
}
#endif

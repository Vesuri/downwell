#ifndef DOWNWELL_CORE_STORED_BUILDER_PUBLICATION_H
#define DOWNWELL_CORE_STORED_BUILDER_PUBLICATION_H
#include "BuilderCommonDispatch.h"
#include "InstanceStore.h"
#include "InstanceLifecycle.h"
#include "RunnerRealComparison.h"
#include <cstring>
namespace downwell {
// Post-Create half of the builder owner. Creation remains with the registered
// family owner; all operations resolve its returned generation-safe handle.
template<unsigned Capacity> class StoredBuilderPublication {
    InstanceStore<Capacity>& records_;
    InstanceLifecycleBinding lifecycle_;
    bool& good_;
    bool fail(){good_=false;return false;}
public:
    StoredBuilderPublication(InstanceStore<Capacity>& records,InstanceLifecycleBinding lifecycle,bool& good)
        :records_(records),lifecycle_(lifecycle),good_(good){}
    bool write(InstanceHandle key,BuilderChildField field,int value){
        auto* r=records_.resolve(key);
        if(!good_||!r||r->objectId!=60||r->destroyRequested)return fail();
        switch(field){
        case BuilderChildField::deactivate:
            if(value!=1)return fail();
            r->wall.deactivate=true;return true;
        case BuilderChildField::maskIndex:
            if((value!=594&&value!=595)||!lifecycle_.mask(key,*r,value))return fail();
            r->maskId=value;return true;
        case BuilderChildField::indexVariable:
            if(value!=15)return fail();
            r->wall.imageIndex=value;return true;
        case BuilderChildField::imageIndex: {
            RunnerImageValue index;
            if(value!=15||!index.assignInteger(value)||!lifecycle_.imageIndex(key,*r,index))return fail();
            std::memcpy(&r->imageIndex,&index.bits,4);return true;
        }
        case BuilderChildField::checked:
            if(value!=1)return fail();
            r->wall.checked=value;return true;
        case BuilderChildField::isPillar:
            if(value!=1&&value!=-1)return fail();
            r->wall.pillar=value;return true;
        }
        return fail();
    }
    bool deactivate(InstanceHandle key){
        const auto* r=records_.resolve(key);
        if(!good_||!r||r->objectId!=92||r->destroyRequested)return fail();
        return records_.requestActivation(key,false,[&](auto h,const auto& child,bool active){
            return lifecycle_.activation(h,child,active);
        })||fail();
    }
    bool rightOfMiddle(InstanceHandle key,int roomWidth,bool& right){
        const auto* r=records_.resolve(key);
        if(!good_||!r||r->objectId!=60||r->destroyRequested||roomWidth<0)return fail();
        RunnerImageValue x;std::memcpy(&x.bits,&r->preciseX,4);
        if((x.bits&0x7f800000u)==0 && (x.bits&0x7fffffu))return fail();
        // Generated children initialize both views. Older authored-room paths
        // may leave preciseX unset; never interpret that default as source X.
        std::int32_t rounded=0;
        if(!x.roundPixel(rounded)||rounded!=r->x)return fail();
        auto middle=MotionBinary64::fromInteger(roomWidth);
        if(roomWidth)middle.high-=1u<<20; // exact positive binary64 / 2
        int order=0;
        if(!compareRunnerReal(x.promoted(),middle,order))return fail();
        right=order>0;return true;
    }
};
}
#endif

#ifndef DOWNWELL_CORE_RUNNER_DEPTH_CHANGES_H
#define DOWNWELL_CORE_RUNNER_DEPTH_CHANGES_H
#include "RunnerSpatialSplit.h"
#include "FailureSignal.h"
namespace downwell {
// 0x431ce0 / 0x50d220 / 0x50d740. Borrowed instance slots must be removed
// from this queue before retirement/reuse. Service follows activation upkeep.
template<unsigned Capacity> class RunnerDepthChanges {
public:
    unsigned size()const{return count_;}
    bool succeeded()const{return !failed_;}
    void observeFailure(bool& owner){failed_.observe(owner);}
    bool request(unsigned instance,RunnerImageValue& published,MotionBinary64 value){
        if(failed_||servicing_||!RunnerSpatialSplit::valid(published))return false;
        RunnerImageValue next;if(!next.assign(value))return false;
        if(RunnerSpatialSplit::equal(next,published))return true;
        published=next; // Setter publication precedes queue insertion.
        for(unsigned i=0;i<count_;++i)if(pending_[i]==instance)return true;
        if(count_==Capacity){failed_=true;return false;}
        pending_[count_++]=instance;return true;
    }
    bool retire(unsigned instance){
        if(failed_||servicing_)return false;
        for(unsigned i=0;i<count_;++i)if(pending_[i]==instance){
            for(unsigned j=i+1;j<count_;++j)pending_[j-1]=pending_[j];
            --count_;break;
        }
        return true;
    }
    // The selected service does not invoke game events. Callbacks must not
    // request depths or retire instances recursively. Failure is terminal;
    // successful earlier migrations are never replayed or rolled back.
    template<class Apply> bool service(Apply apply){
        if(failed_||servicing_)return false;
        servicing_=true;
        for(unsigned i=0;i<count_;++i)if(!apply(pending_[i])){
            servicing_=false;failed_=true;return false;
        }
        count_=0;servicing_=false;return true;
    }
private:
    unsigned pending_[Capacity?Capacity:1]{},count_=0;
    FailureSignal<false> failed_=false;bool servicing_=false;
};

// Selected 0x414660 service, composed with RunnerLayers. All arithmetic is
// integer-only on target; finite normal/zero depths in signed-int range.
struct RunnerDepthService {
    struct Instance {
        RunnerImageValue depth;
        std::int32_t layerId=-1;
        bool attached=false,append=false;
    };
    static bool integerDepth(RunnerImageValue value,std::int32_t& result){
        if(!RunnerSpatialSplit::valid(value))return false;
        const unsigned magnitude=value.bits&0x7fffffffu;
        if(magnitude>0x4f000000u || (magnitude==0x4f000000u&&!(value.bits>>31)))return false;
        const int exponent=int(magnitude>>23)-127;
        if(exponent<0){result=0;return true;}
        unsigned integer=(magnitude&0x7fffffu)|0x800000u;
        if(exponent>=23)integer<<=unsigned(exponent-23);else integer>>=unsigned(23-exponent);
        if(integer==0x80000000u)result=(-2147483647-1);
        else result=value.bits>>31?-std::int32_t(integer):std::int32_t(integer);
        return true;
    }
    // self(element) must check type 2 and exact instance identity. createLayer
    // allocates a source ID and inserts a dynamic layer at the requested depth.
    // merged(element,id) publishes layer IDs for non-null type-2 instances;
    // inactive(element) filters type/null before testing instance flag bit 2.
    // Caller propagates any false result as terminal for the owning world.
    template<class Layers,class Self,class Create,class Merged,class Inactive>
    static bool apply(Layers& layers,Instance& instance,unsigned element,
        Self self,Create createLayer,Merged merged,Inactive inactive){
        if(!layers.succeeded())return false;
        if(!instance.attached)return true;
        const unsigned oldSlot=layers.find(instance.layerId);
        const auto* old=layers.get(oldSlot);if(!old)return true;
        std::int32_t depth;if(!integerDepth(instance.depth,depth))return false;
        if(depth==old->depth)return true;
        RunnerImageValue difference;
        if(!RunnerSpatialSplit::sub(instance.depth,old->encodedDepth,difference))return false;
        if((difference.bits&0x7fffffffu)<0x41a00000u&&old->dynamic&&layers.elementCount(oldSlot)==1){
            bool matches=false;
            if(!layers.visitElements(oldSlot,[&](unsigned e){matches=self(e);return true;}))return false;
            return !matches||layers.move(oldSlot,depth,true,merged,inactive);
        }
        if(layers.elementOwner(element)!=oldSlot||!layers.detach(element))return false;
        instance.attached=false;instance.layerId=-1;
        unsigned target=layers.findDynamic(depth);
        if(target==Layers::End&&!createLayer(depth,target))return false;
        const auto* layer=layers.get(target);
        if(!layer||!layer->dynamic||layer->depth!=depth)return false;
        instance.attached=true;instance.layerId=layer->id;instance.depth=layer->encodedDepth;
        return layers.attach(target,element,true,instance.append,[](unsigned){return false;});
    }
};
}
#endif

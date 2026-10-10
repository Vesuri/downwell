#ifndef DOWNWELL_RUNNER_LOGICAL_LAYER_ORDER_H
#define DOWNWELL_RUNNER_LOGICAL_LAYER_ORDER_H
#include "RunnerLayers.h"
#include "RunnerRegistrationOrder.h"
#include "InstanceHandle.h"
namespace downwell {
struct RunnerLayerOrderKey {
    RunnerRegistrationOrder serial;
    bool tail=false;
    bool present()const{return serial.present();}
    bool before(const RunnerLayerOrderKey& b)const{
        if(tail!=b.tail)return !tail;
        return tail?serial.before(b.serial):b.serial.before(serial);
    }
};
struct RunnerRetainedLayerState {
    RunnerLayerOrderKey order;
    int publishedId=-1;
    unsigned layer=0xffffffffu;
    bool inactive=false;
    bool bound()const{return layer!=0xffffffffu;}
    RunnerRetainedLayerState shifted(unsigned offset)const{
        auto result=*this;
        if(order.present()){
            result.order.serial.low+=offset;
            if(result.order.serial.low<order.serial.low)++result.order.serial.high;
        }
        return result;
    }
    bool same(const RunnerRetainedLayerState& b)const{
        return order.serial.low==b.order.serial.low&&order.serial.high==b.order.serial.high&&
            order.tail==b.order.tail&&publishedId==b.publishedId&&layer==b.layer&&inactive==b.inactive;
    }
};
class RunnerRetainedLayerRead {
    RunnerRetainedLayerState value_;
    bool present_=false;
public:
    RunnerRetainedLayerRead(decltype(nullptr)=nullptr){}
    RunnerRetainedLayerRead(const RunnerRetainedLayerState& value):value_(value),present_(true){}
    explicit operator bool()const{return present_;}
    const RunnerRetainedLayerState* operator->()const{return &value_;}
    const RunnerRetainedLayerState& operator*()const{return value_;}
};
// Terrain supplies layerState(slot/handle) and visitLayerStates(handle,state).
// Its records own retained state; this adapter owns only physical order keys,
// per-layer cutoffs and scratch used at logical merge/normalization boundaries.
template<unsigned L,unsigned E,class Terrain> class RunnerLogicalLayerOrder:public RunnerLayers<L,E,true> {
    using Base=RunnerLayers<L,E,true>;
    static constexpr unsigned Virtual=0x80000000u;
    static_assert(E<Virtual&&Terrain::capacity<Virtual,"Logical layer tokens");
    Terrain* terrain_=nullptr;
    RunnerLayerOrderKey physical_[E?E:1]{};
    RunnerRegistrationOrder clock_;
    struct Cutoff {RunnerLayerOrderKey first;bool dirty=true,found=false;};
    mutable Cutoff cutoffs_[L?L:1]{};
    unsigned work_[E?E:1]{},count_=0;
    std::uint8_t inactivePhysical_[(E+7)/8?(E+7)/8:1]{};
    FailureSignal<> good_=true;
    RunnerLayerOrderKey stamp(bool tail){clock_.advance();return {clock_,tail};}
    bool fail(){good_=false;return false;}
    auto state(unsigned token)const{return terrain_->layerState(token&~Virtual);}
    RunnerLayerOrderKey key(unsigned token)const{return token&Virtual?state(token)->order:physical_[token];}
    void setKey(unsigned token,RunnerLayerOrderKey value){
        if(token&Virtual){auto s=*state(token);s.order=value;terrain_->setLayerState(token&~Virtual,s);}
        else physical_[token]=value;
    }
    bool collect(unsigned layer){
        const unsigned first=count_;
        auto append=[&](unsigned token){if(count_==E||!key(token).present())return false;work_[count_++]=token;return true;};
        if(!Base::visitElements(layer,append))return false;
        if(terrain_&&!terrain_->visitLayerStates([&](InstanceHandle h,const RunnerRetainedLayerState& s){
            return !s.bound()||s.layer!=layer||append(Virtual|h.slot);
        }))return false;
        // Rare observation boundary, not per-cell birth maintenance.
        auto sift=[&](unsigned root,unsigned end){
            const unsigned token=work_[first+root];const auto k=key(token);
            while(root<end/2){unsigned child=root*2+1;
                if(child+1<end&&key(work_[first+child]).before(key(work_[first+child+1])))++child;
                if(!k.before(key(work_[first+child])))break;
                work_[first+root]=work_[first+child];root=child;
            }work_[first+root]=token;
        };
        const unsigned size=count_-first;
        for(unsigned i=size/2;i;--i)sift(i-1,size);
        for(unsigned end=size;end>1;){--end;const auto v=work_[first+end];
            work_[first+end]=work_[first];work_[first]=v;sift(0,end);}
        return count_-first==Base::elementCount(layer);
    }
    template<class Inactive> bool normalize(unsigned layer,Inactive inactive){
        // work_ is the complete concatenated logical order. Publish fresh
        // ascending keys, then perform the source backwards-to-tail pass.
        for(unsigned i=0;i<count_;++i)setKey(work_[i],stamp(true));
        for(unsigned i=count_;i;){const unsigned token=work_[--i];
            const bool dormant=token&Virtual?state(token)->inactive:inactive(token);
            if(!(token&Virtual)){
                auto& word=inactivePhysical_[token>>3];const unsigned bit=1u<<(token&7);
                if(dormant)word|=bit;else word&=~bit;
            }
            if(dormant)setKey(token,stamp(true));
        }
        cutoffs_[layer].dirty=true;
        // The source predicate is observed once per physical element, above.
        return Base::normalizePhysicalInstances(layer,[&](unsigned e){return (inactivePhysical_[e>>3]&(1u<<(e&7)))!=0;});
    }
public:
    bool bindTerrain(Terrain& terrain){if(terrain_)return false;terrain_=&terrain;return true;}
    bool succeeded()const{return good_&&Base::succeeded();}
    void observeFailure(bool& owner){good_.observe(owner);Base::observeFailure(owner);}
    bool discardUninitialized(){
        if(terrain_&&!terrain_->visitLayerStates([](InstanceHandle,const RunnerRetainedLayerState& s){return !s.bound();}))return fail();
        if(!succeeded()||!Base::discardUninitialized())return false;
        for(auto& k:physical_)k={};for(auto& c:cutoffs_)c=Cutoff{};return true;
    }
    RunnerLayerOrderKey physicalOrder(unsigned e)const{return e<E?physical_[e]:RunnerLayerOrderKey{};}
    template<class Active> bool attach(unsigned layer,unsigned e,bool instance,bool append,Active active){
        if(!succeeded())return false;
        // Live non-instance insertion requires a logical active-prefix owner.
        // Resource backgrounds are loaded before retained births.
        if(!instance&&Base::retainedCount(layer))return fail();
        if(!Base::attach(layer,e,instance,append,active))return false;
        if(instance)physical_[e]=stamp(append);
        else if(!Base::visitElements(layer,[&](unsigned n){physical_[n]=stamp(true);return true;}))return fail();
        return true;
    }
    bool detach(unsigned e){if(!succeeded()||!Base::detach(e))return false;physical_[e]={};return true;}
    bool detachForRebind(unsigned e){if(!succeeded()||!Base::detachForRebind(e))return false;physical_[e]={};return true;}
    bool reconcileInstanceActivation(unsigned e,bool inactive){
        if(!succeeded()||!Base::reconcileInstanceActivation(e,inactive))return false;
        physical_[e]=stamp(inactive);return true;
    }
    bool retainInstance(InstanceHandle h,unsigned layer,bool inactive){
        auto s=terrain_?terrain_->layerState(h):RunnerRetainedLayerRead{};
        if(!succeeded()||!s||s->bound()||!Base::retain(layer,1))return false;
        terrain_->setLayerState(h.slot,{stamp(false),Base::get(layer)->id,layer,inactive});
        cutoffs_[layer].dirty=true;return true;
    }
    bool retainAllocated(unsigned slot,unsigned layer,bool inactive,bool inherited=false){
        if(!Base::retain(layer,1))return false;
        if(inherited){clock_.advance();terrain_->publishInheritedLayer(slot);}
        else terrain_->setLayerState(slot,{stamp(false),Base::get(layer)->id,layer,inactive});
        cutoffs_[layer].dirty=true;return true;
    }
    bool retainInheritedCount(unsigned layer,unsigned count){
        if(!Base::retain(layer,count))return false;
        clock_=clock_.shifted(count);cutoffs_[layer].dirty=true;return true;
    }
    bool retainConstructedSpan(unsigned layer,unsigned count,RunnerRetainedLayerState& first){
        if(!count||!succeeded()||!Base::get(layer)||!Base::retain(layer,count))return false;
        first={stamp(false),Base::get(layer)->id,layer,false};
        clock_=clock_.shifted(count-1);cutoffs_[layer].dirty=true;return true;
    }
    bool releaseInstance(InstanceHandle h){
        auto s=terrain_?terrain_->layerState(h):RunnerRetainedLayerRead{};
        if(!succeeded()||!s||!s->bound()||!Base::releaseRetained(s->layer,1))return false;
        cutoffs_[s->layer].dirty=true;terrain_->setLayerState(h.slot,{});return true;
    }
    bool publishInactive(InstanceHandle h,bool inactive){
        auto s=terrain_?terrain_->layerState(h):RunnerRetainedLayerRead{};
        if(!succeeded()||!s||!s->bound())return false;
        if(s->inactive!=inactive){auto changed=*s;changed.inactive=inactive;
            terrain_->setLayerState(h.slot,changed);cutoffs_[s->layer].dirty=true;}return true;
    }
    bool reconcileRetained(InstanceHandle h){
        auto s=terrain_?terrain_->layerState(h):RunnerRetainedLayerRead{};
        if(!succeeded()||!s||!s->bound())return false;
        auto changed=*s;changed.order=stamp(s->inactive);terrain_->setLayerState(h.slot,changed);
        cutoffs_[s->layer].dirty=true;return true;
    }
    // after is the last dispatched physical key saved BEFORE its callback.
    // A newly born head cutoff must not be visited retroactively in this pass.
    bool cutsOff(unsigned layer,const RunnerLayerOrderKey* after,RunnerLayerOrderKey before)const{
        if(layer>=L||!terrain_)return false;
        auto& c=cutoffs_[layer];
        if(c.dirty){
            c.found=false;
            terrain_->visitLayerCutoffs(layer,nullptr,[&](RunnerLayerOrderKey order){
                if(!c.found||order.before(c.first)){c.first=order;c.found=true;}
                return true;
            });c.dirty=false;
        }
        if(!c.found)return false;
        if(!after||after->before(c.first))return c.first.before(before);
        bool found=false;
        terrain_->visitLayerCutoffs(layer,after,[&](RunnerLayerOrderKey order){
            if(order.before(before)){found=true;return false;}
            return true;
        });return found;
    }
    template<class Publish,class Inactive> bool move(unsigned source,int depth,bool merge,Publish publish,Inactive inactive){
        if(!succeeded())return false;
        unsigned destination=L;
        return Base::moveLogical(source,depth,merge,[&](unsigned from,int id){
            destination=Base::find(id);count_=0;
            if(!collect(destination))return false;
            const unsigned sourceFirst=count_;
            if(!collect(from))return false;
            for(unsigned i=sourceFirst;i<count_;++i){const unsigned token=work_[i];
                if(token&Virtual){auto s=*state(token);s.publishedId=id;terrain_->setLayerState(token&~Virtual,s);}
                else if(!publish(token,id))return false;
            }return true;
        },[&](unsigned to){
            if(to!=destination)return false;
            for(unsigned i=0;i<count_;++i)if(work_[i]&Virtual){auto s=*state(work_[i]);s.layer=to;
                terrain_->setLayerState(work_[i]&~Virtual,s);}
            cutoffs_[source].dirty=true;
            return normalize(to,inactive);
        });
    }
    template<class Inactive> bool normalizeRoomInstances(Inactive inactive){
        if(!succeeded())return false;
        return Base::normalizeRoomLogical([&](unsigned layer){count_=0;return collect(layer)&&normalize(layer,inactive);});
    }
    // Verification/observing callers only; mutation during this walk is forbidden.
    template<class Visit> bool visitLogical(unsigned layer,Visit visit){
        count_=0;if(!collect(layer))return false;
        for(unsigned i=0;i<count_;++i)if(!visit(bool(work_[i]&Virtual),work_[i]&~Virtual))return false;
        return true;
    }
};
}
#endif

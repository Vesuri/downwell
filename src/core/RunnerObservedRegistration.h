#ifndef DOWNWELL_RUNNER_OBSERVED_REGISTRATION_H
#define DOWNWELL_RUNNER_OBSERVED_REGISTRATION_H
#include "RunnerObjectMembership.h"
#include "RunnerRegistrationOrder.h"
namespace downwell {
// Terrain membership lives in the authoritative typed store. Only actors own
// list nodes here. Mixed enumeration observes an order-key projection on demand;
// births do not maintain a second terrain list, locator, object or identity.
template<unsigned Capacity,unsigned First,unsigned Last,class Terrain>
class RunnerObservedRegistration {
    static_assert(First<Last&&Last<=Capacity&&Capacity<65535,"Observed registration domain");
    static constexpr unsigned ActorCapacity=Capacity-(Last-First);
    using Slot=std::uint16_t;
    RunnerObjectMembership<ActorCapacity> actors_;
    RunnerRegistrationOrder actorOrder_[ActorCapacity?ActorCapacity:1]{};
    RunnerRegistrationOrder clock_;
    Terrain* terrain_=nullptr;
    unsigned revision_=0;
    mutable Slot order_[Capacity]{};
    mutable unsigned orderCount_=0,terrainRevision_=0;
    mutable bool ordered_=false;
    static bool external(unsigned s){return s>=First&&s<Last;}
    static unsigned actorSlot(unsigned s){return s<First?s:s-(Last-First);}
    static unsigned sharedSlot(unsigned s){return s<First?s:s+(Last-First);}
    unsigned terrainRevision()const{return terrain_?terrain_->registrationRevision():0;}
    RunnerRegistrationOrder key(unsigned s)const{
        return external(s)?terrain_->registrationOrder(s-First):actorOrder_[actorSlot(s)];
    }
    bool read(unsigned s,std::uint32_t& id,int& object)const{
        if(external(s)){
            if(!terrain_||!terrain_->registered(s-First))return false;
            id=terrain_->registrationHandle(s-First).generation;object=terrain_->registrationObject(s-First);return true;
        }
        return actors_.read(actorSlot(s),id,object);
    }
    bool observe()const{
        if(ordered_&&terrainRevision_==terrainRevision())return true;
        orderCount_=0;
        const auto collect=[&](unsigned s,auto,auto){
            if(orderCount_==Capacity)return false;order_[orderCount_++]=Slot(s);return true;
        };
        if(!actors_.visitRegistrations([&](unsigned s,auto id,auto object){return collect(sharedSlot(s),id,object);}))return false;
        if(terrain_&&!terrain_->visitRegistrations([&](unsigned s,auto id,auto object){return collect(First+s,id,object);}))return false;
        // In-place heapsort: no target stack-sized scratch and no birth-time
        // ordering work. Registration keys are unique, including rebindings.
        const auto sift=[&](unsigned root,unsigned end){
            const Slot value=order_[root];const auto valueKey=key(value);
            while(root<end/2){
                unsigned child=root*2+1;
                if(child+1<end&&key(order_[child]).before(key(order_[child+1])))++child;
                if(!valueKey.before(key(order_[child])))break;
                order_[root]=order_[child];root=child;
            }
            order_[root]=value;
        };
        for(unsigned i=orderCount_/2;i;--i)sift(i-1,orderCount_);
        for(unsigned end=orderCount_;end>1;){
            --end;const Slot last=order_[end];order_[end]=order_[0];order_[0]=last;sift(0,end);
        }
        ordered_=true;terrainRevision_=terrainRevision();return true;
    }
public:
    template<unsigned A,unsigned B> struct RegistrationProjection {
        static_assert(A<=B&&B<=Capacity,"Observed projection range");
    };
    bool bindTerrain(Terrain& terrain){if(terrain_)return false;terrain_=&terrain;ordered_=false;return true;}
    bool contains(unsigned s)const{
        return s<Capacity&&(external(s)?terrain_&&terrain_->registered(s-First):actors_.contains(actorSlot(s)));
    }
    unsigned size()const{return actors_.size()+(terrain_?terrain_->registeredCount():0);}
    void repeatFreshTailBinding(){revision_+=2;}
    bool bind(unsigned s,int object,std::uint32_t id){
        if(s>=Capacity||object<0)return false;
        const bool wasRegistered=contains(s);
        auto next=clock_;next.advance();
        const bool ok=external(s)?terrain_&&terrain_->registerInstance({s-First,id},object,next):
            actors_.bind(actorSlot(s),object,id);
        if(!ok)return false;
        clock_=next;if(!external(s))actorOrder_[actorSlot(s)]=next;
        revision_+=wasRegistered?2u:1u;ordered_=false;return true;
    }
    bool unbind(unsigned s){
        if(!contains(s))return false;
        if(external(s))terrain_->unregisterInstance(s-First);else actors_.unbind(actorSlot(s));
        ++revision_;ordered_=false;return true;
    }
    void clear(){
        revision_+=size();actors_.clear();if(terrain_)terrain_->clearRegistrations();ordered_=false;
    }
    template<class Visit> bool visitRegistrationRange(unsigned first,unsigned last,Visit visit)const{
        if(first>last||last>Capacity)return false;
        if(first==last)return true;
        if(first>=Last||last<=First)return actors_.visitRegistrations([&](unsigned s,auto id,auto object){
            s=sharedSlot(s);return s<first||s>=last||visit(s,id,object);
        });
        if(!observe())return false;
        for(unsigned i=0;i<orderCount_;++i){const unsigned s=order_[i];
            if(s<first||s>=last)continue;
            std::uint32_t id;int object;if(!read(s,id,object)||!visit(s,id,object))return false;
        }
        return true;
    }
    template<class Visit> bool visitRegistrations(Visit visit)const{return visitRegistrationRange(0,Capacity,visit);}
    template<unsigned A,unsigned B,class Visit>
    bool visitRegistrationRange(RegistrationProjection<A,B>&,unsigned first,unsigned last,Visit visit)const{
        if(first>last||last>Capacity)return false;
        if(first==last)return true;
        if(A<B&&first<B&&last>A)return false;
        const unsigned revision=revision_,tr=terrainRevision();
        return visitRegistrationRange(first,last,[&](unsigned s,auto id,auto object){
            return visit(s,id,object)&&revision==revision_&&tr==terrainRevision();
        });
    }
    template<class Visit> bool visitRetainedRegistrationRange(unsigned first,unsigned last,Visit visit)const{
        if(first>last||last>Capacity)return false;
        if(first==last)return true;
        if(first<Last&&last>First)return false;
        const unsigned revision=revision_,tr=terrainRevision();
        return visitRegistrationRange(first,last,[&](unsigned s,auto id,auto object){
            return visit(s,id,object)&&revision==revision_&&tr==terrainRevision();
        });
    }
    template<class IsA,class Visit> void visit(int object,IsA isA,Visit visit)const{
        if(object<0)return;
        // Enemy/actor selectors need no terrain projection at all.
        if(!terrain_||!terrain_->countRegistered(object,isA)){
            actors_.visit(object,isA,[&](unsigned s,auto id){return visit(sharedSlot(s),id);});return;
        }
        visitRegistrations([&](unsigned s,auto id,int kind){
            return !(kind==object||isA(kind,object))||visit(s,id);
        });
    }
    template<class IsA> unsigned count(int object,IsA isA)const{
        if(object<0)return 0;
        return actors_.count(object,isA)+(terrain_?terrain_->countRegistered(object,isA):0);
    }
};
}
#endif

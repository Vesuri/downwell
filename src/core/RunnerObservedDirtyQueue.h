#ifndef DOWNWELL_RUNNER_OBSERVED_DIRTY_QUEUE_H
#define DOWNWELL_RUNNER_OBSERVED_DIRTY_QUEUE_H
#include "RunnerSpatialDirtyQueue.h"
#include "RunnerRegistrationOrder.h"
namespace downwell {
// Actors keep the ordinary intrusive list. Terrain owns only queue visibility
// and order bases in its identity runs. A derived successor table is built at
// a traversal/removal observation, never as part of an ordinary terrain mark.
template<unsigned Capacity,unsigned First,unsigned Last,class Terrain,unsigned Which>
class RunnerObservedDirtyQueue {
    static_assert(First<Last&&Last<=Capacity&&Which<2,"Terrain dirty queue domain");
    static constexpr unsigned TerrainEnd=Last-First,ActorEnd=Capacity-TerrainEnd;
    using Index=typename SlotIndex<Capacity>::Type;
    using TerrainIndex=typename SlotIndex<TerrainEnd>::Type;
    using Order=RunnerRegistrationOrder;
    using Rules=RunnerSpatialDirtyQueue<Capacity>;
    RunnerSpatialDirtyQueue<ActorEnd> actors_;
    Order actorOrders_[ActorEnd]{};
    Terrain* terrain_=nullptr;
    Order clock_;
    unsigned head_=Capacity,terrainCount_=0;
    mutable unsigned terrainHead_=TerrainEnd;
    mutable TerrainIndex terrainNext_[TerrainEnd]{};
    mutable bool projected_=true;
    static bool isTerrain(unsigned slot){return slot>=First&&slot<Last;}
    static unsigned actorSlot(unsigned slot){return slot<First?slot:slot-TerrainEnd;}
    static unsigned sharedActor(unsigned slot){return slot==ActorEnd?Capacity:slot<First?slot:slot+TerrainEnd;}
    Order order(unsigned slot)const{
        return isTerrain(slot)?terrain_->template dirtyOrder<Which>(slot-First):actorOrders_[actorSlot(slot)];
    }
    unsigned newer(unsigned a,unsigned b)const{
        if(a==Capacity)return b;
        if(b==Capacity)return a;
        return order(a).before(order(b))?b:a;
    }
    unsigned sharedTerrain(unsigned slot)const{return slot==TerrainEnd?Capacity:slot+First;}
    void projectTerrain()const{
        if(projected_)return;
        unsigned head=TerrainEnd;Order previous;bool ordered=true;
        terrain_->template visitDirty<Which>([&](unsigned slot,Order key){
            if(head!=TerrainEnd&&!previous.before(key))ordered=false;
            terrainNext_[slot]=TerrainIndex(head);head=slot;previous=key;return true;
        });
        // Birth-order runs normally already produce the complete queue order.
        // Re-marks can disturb it. Sort links only, without a second slot array
        // or per-cell authoritative predecessor/successor ownership.
        if(!ordered)for(unsigned width=1;;width*=2){
            unsigned p=head,first=TerrainEnd,tail=TerrainEnd,merges=0;
            while(p!=TerrainEnd){
                ++merges;unsigned q=p,left=0,right=width;
                for(unsigned i=0;i<width&&q!=TerrainEnd;++i){++left;q=terrainNext_[q];}
                while(left||(right&&q!=TerrainEnd)){
                    unsigned selected;
                    if(!left){selected=q;q=terrainNext_[q];--right;}
                    else if(!right||q==TerrainEnd){selected=p;p=terrainNext_[p];--left;}
                    else if(terrain_->template dirtyOrder<Which>(p).before(terrain_->template dirtyOrder<Which>(q))){
                        selected=q;q=terrainNext_[q];--right;
                    }else{selected=p;p=terrainNext_[p];--left;}
                    if(tail==TerrainEnd)first=selected;else terrainNext_[tail]=TerrainIndex(selected);
                    tail=selected;
                }
                p=q;
            }
            if(tail!=TerrainEnd)terrainNext_[tail]=TerrainEnd;
            head=first;if(merges<=1)break;
        }
        terrainHead_=head;projected_=true;
    }
    unsigned terrainSuccessor(unsigned local)const{
        projectTerrain();unsigned n=terrainNext_[local];
        while(n!=TerrainEnd&&!terrain_->template dirtyContains<Which>(n))n=terrainNext_[n];
        return n;
    }
    unsigned olderTerrain(Order key)const{
        projectTerrain();unsigned n=terrainHead_;
        while(n!=TerrainEnd){
            if(terrain_->template dirtyContains<Which>(n)&&terrain_->template dirtyOrder<Which>(n).before(key))return sharedTerrain(n);
            n=terrainNext_[n];
        }
        return Capacity;
    }
    unsigned olderActor(Order key)const{
        unsigned n=actors_.first();
        while(n!=ActorEnd&&!actorOrders_[n].before(key))n=actors_.next(n);
        return sharedActor(n);
    }
    void advance(bool& inherited){
        if(clock_.low==0xffffffffu&&clock_.high==0xffffffffu){
            // The source has no order-counter failure. Preserve order on full
            // wrap instead of adding a new terminal condition. This scratch is
            // used only once per 2^64 marks, not on birth or normal traversal.
            Index slots[Capacity];unsigned count=0;
            for(unsigned n=first();n!=End;n=next(n))slots[count++]=Index(n);
            for(unsigned i=0;i<count;++i){const unsigned slot=slots[i];const Order key{count-i,0};
                if(isTerrain(slot))terrain_->template dirtyMark<Which>(slot-First,key);
                else actorOrders_[actorSlot(slot)]=key;
            }
            clock_={count,0};inherited=false;
        }
        clock_.advance();
    }
    bool markMode(unsigned slot,bool inherited){
        if(slot>=Capacity||(isTerrain(slot)&&!terrain_))return false;
        if(slot==head_)return true;
        advance(inherited);
        if(isTerrain(slot)){
            const unsigned local=slot-First;const bool present=terrain_->template dirtyContains<Which>(local);
            if(inherited)terrain_->template dirtyInherit<Which>(local);
            else if(!terrain_->template dirtyMark<Which>(local,clock_))return false;
            if(!present)++terrainCount_;
            terrainHead_=local;projected_=false;
        }else{
            const unsigned local=actorSlot(slot);actors_.mark(local);actorOrders_[local]=clock_;
        }
        head_=slot;return true;
    }
public:
    static constexpr unsigned End=Capacity;
    explicit RunnerObservedDirtyQueue(Order initial={}):clock_(initial){}
    bool bindTerrain(Terrain& terrain){if(terrain_)return false;terrain_=&terrain;return true;}
    unsigned first()const{return head_;}
    unsigned size()const{return terrainCount_+actors_.size();}
    bool contains(unsigned slot)const{
        return slot<Capacity&&(isTerrain(slot)?terrain_&&terrain_->template dirtyContains<Which>(slot-First):actors_.contains(actorSlot(slot)));
    }
    unsigned next(unsigned slot)const{
        if(!contains(slot))return End;
        const auto key=order(slot);
        const unsigned actor=isTerrain(slot)?olderActor(key):sharedActor(actors_.next(actorSlot(slot)));
        const unsigned terrain=isTerrain(slot)?sharedTerrain(terrainSuccessor(slot-First)):olderTerrain(key);
        return newer(actor,terrain);
    }
    bool mark(unsigned slot){return markMode(slot,false);}
    // Fresh constructor owns the membership publication. Reserve only the
    // queue chronology here; no observable callback may split this interval.
    Order generatedOrder(unsigned slot){
        bool inherited=false;advance(inherited);
        ++terrainCount_;terrainHead_=slot-First;head_=slot;projected_=false;
        return clock_;
    }
    // Producer exposed membership bits and inherited order in fresh cells.
    // No observations/other marks intervene; admission excludes clock wrap.
    void completeInheritedTerrain(unsigned last,unsigned count){
        if(!count)return;
        clock_=clock_.shifted(count);terrainCount_+=count;
        terrainHead_=last-First;head_=last;projected_=false;
    }
    void publishInheritedMembership(unsigned slot){terrain_->template dirtyInherit<Which>(slot-First);}
    // Admitted homogeneous birth continuation: the run's order base already
    // predicts this clock value. The cell is freshly allocated, or is the head
    // just marked by an earlier phase of this same birth. No callback can
    // interleave another queue mark. Do not dispatch through actor marking or
    // inspect a membership that this transaction has already established.
    bool markAllocatedTerrain(unsigned slot,bool inherited){
        if(slot==head_)return true;
        advance(inherited);
        const unsigned local=slot-First;
        if(inherited)terrain_->template dirtyInherit<Which>(local);
        else if(!terrain_->template dirtyMark<Which>(local,clock_))return false;
        ++terrainCount_;terrainHead_=local;projected_=false;head_=slot;return true;
    }
    bool remove(unsigned slot){
        if(!contains(slot))return false;
        if(isTerrain(slot)){
            const unsigned local=slot-First;
            if(local==terrainHead_)terrainHead_=terrainCount_==1?TerrainEnd:terrainSuccessor(local);
            terrain_->template dirtyRemove<Which>(local);--terrainCount_;
            if(!terrainCount_)projected_=true;
        }else actors_.remove(actorSlot(slot));
        if(slot==head_)head_=newer(sharedActor(actors_.first()),sharedTerrain(terrainHead_));
        return true;
    }
    void clear(){
        actors_.clear();if(terrain_)terrain_->template dirtyClear<Which>();
        head_=End;terrainHead_=TerrainEnd;terrainCount_=0;projected_=true;clock_={};
    }
    static bool shouldQueue(bool hasObject,std::uint32_t objectFlags,std::uint32_t flags){return Rules::shouldQueue(hasObject,objectFlags,flags);}
    static bool shouldRefreshQueued(std::uint32_t flags){return Rules::shouldRefreshQueued(flags);}
    static bool shouldPublishBounds(std::uint32_t objectFlags,std::uint32_t flags,bool allPrepared){return Rules::shouldPublishBounds(objectFlags,flags,allPrepared);}
    template<class Flags,class Refresh> bool flush(Flags flags,Refresh refresh){
        unsigned slot=first();
        while(slot!=End){
            if(shouldRefreshQueued(flags(slot))&&!refresh(slot))return false;
            if(!contains(slot))return false;
            const unsigned successor=next(slot);remove(slot);slot=successor;
        }
        return true;
    }
};
template<unsigned Capacity,unsigned First,unsigned Last,class Terrain,unsigned Which>
struct RunnerDirtyQueueStorage {using Type=RunnerObservedDirtyQueue<Capacity,First,Last,Terrain,Which>;};
template<unsigned Capacity,unsigned First,unsigned Last,unsigned Which>
struct RunnerDirtyQueueStorage<Capacity,First,Last,void,Which> {using Type=RunnerSpatialDirtyQueue<Capacity>;};
}
#endif

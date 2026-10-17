#ifndef DOWNWELL_CORE_RUNNER_SOURCE_ROOM_MEMBERSHIP_H
#define DOWNWELL_CORE_RUNNER_SOURCE_ROOM_MEMBERSHIP_H
#include "RunnerRunRoomMembership.h"
#include "RunnerRoomOrderState.h"
#include "TerrainPublicationClock.h"
namespace downwell {
// Ordinary room nodes remain the existing linked owner. Generated terrain
// has no node here: its initial room state is a view of the source segment,
// and exceptional room changes live in that segment's changed cell. No
// observed-room projection or second terrain membership table is built.
template<unsigned Capacity,unsigned First,unsigned Last,class Terrain>
class RunnerSourceRoomMembership:public RunnerRunRoomMembership<Capacity,First,Last> {
    using Base=RunnerRunRoomMembership<Capacity,First,Last>;
    using State=RunnerRoomOrderState;
    Terrain* terrain_=nullptr;
    TerrainPublicationClock clock_;
    RunnerRegistrationOrder ordinaryOrder_[Capacity]{};
    bool source(unsigned slot,State& out)const{
        return slot>=First&&slot<Last&&terrain_&&terrain_->sourceRoomState(slot-First,out);
    }
    unsigned publications()const{return terrain_?terrain_->sourcePublicationCount():0;}
    State state(unsigned slot)const{
        State value;if(source(slot,value))return value;
        if(!Base::contains(slot))return {};
        return {ordinaryOrder_[slot],Base::depth(slot),static_cast<std::int8_t>(Base::active(slot)?0:1),Base::marked(slot)};
    }
    static bool before(const State& a,const State& b){
        if(a.list!=b.list)return a.list<b.list;
        if(!a.list){
            if(RunnerSpatialSplit::less(a.depth,b.depth))return true;
            if(RunnerSpatialSplit::less(b.depth,a.depth))return false;
        }
        return a.order.before(b.order);
    }
    unsigned next(int list,const State* after,bool reverse,bool onlyMarked=false)const{
        unsigned found=End;State best;
        auto consider=[&](unsigned slot,const State& value){
            if(value.list!=list||(onlyMarked&&!value.marked)||
               (after&&!(reverse?before(value,*after):before(*after,value))))return true;
            if(found==End||(reverse?before(best,value):before(value,best))){found=slot;best=value;}
            return true;
        };
        // The ordinary lists are already ordered. This rare mixed observer
        // does not change them; the animation skip cursor below uses them
        // directly and never enters this scan for terrain it excludes.
        const_cast<Base&>(static_cast<const Base&>(*this)).visitCleanupChecked([&](unsigned slot){return consider(slot,state(slot));});
        if(terrain_)terrain_->visitSourceRoomStates([&](unsigned local,const State& value){return consider(First+local,value);});
        return found;
    }
    template<class Visit> bool walk(int list,bool reverse,Visit visit,const State* after=nullptr){
        unsigned slot=next(list,after,reverse);
        while(slot!=End){const State key=state(slot);const unsigned following=next(list,&key,reverse);
            if(!visit(slot))return false;slot=following;
        }
        return true;
    }
public:
    static constexpr unsigned End=Capacity;
    using AnimationProjection=typename Base::AnimationProjection;
    bool bindTerrain(Terrain& terrain){if(terrain_)return false;terrain_=&terrain;return true;}
    RunnerRegistrationOrder sourcePublicationBasis()const{return clock_.basis();}
    const TerrainPublicationClock& sourcePublicationClock()const{return clock_;}
    bool contains(unsigned slot)const{State value;return source(slot,value)?value.list>=0:Base::contains(slot);}
    bool active(unsigned slot)const{State value;return source(slot,value)?value.list==0:Base::active(slot);}
    bool marked(unsigned slot)const{State value;return source(slot,value)?value.list>=0&&value.marked:Base::marked(slot);}
    RunnerImageValue depth(unsigned slot)const{return state(slot).depth;}
    bool mark(unsigned slot){
        State value;if(!source(slot,value))return Base::mark(slot);
        if(value.list<0)return false;value.marked=true;return terrain_->setSourceRoomState(slot-First,value);
    }
    bool bind(unsigned slot,RunnerImageValue depth){
        if(slot>=Capacity||!RunnerSpatialSplit::valid(depth))return false;
        State value;
        if(source(slot,value))return terrain_->setSourceRoomState(slot-First,{clock_.advance(publications()),depth,0,false});
        if(!Base::bind(slot,depth))return false;
        ordinaryOrder_[slot]=clock_.advance(publications());return true;
    }
    bool bindTerrainSpan(unsigned first,unsigned count,RunnerImageValue depth){
        if(!Base::bindTerrainSpan(first,count,depth))return false;
        for(unsigned i=0;i<count;++i)ordinaryOrder_[first+i]=clock_.advance(publications());
        return true;
    }
    bool remove(unsigned slot){
        State value;if(source(slot,value))return value.list>=0&&terrain_->setSourceRoomState(slot-First,{});
        return Base::remove(slot);
    }
    void clear(){
        Base::clear();
        if(terrain_)terrain_->visitSourceRoomStates([&](unsigned slot,const State&){return terrain_->setSourceRoomState(slot,{});});
    }
    void clearReference(){clear();}
    class AnimationCursor {
        const RunnerSourceRoomMembership& room_;
        typename Base::AnimationCursor ordinary_;
        unsigned first_,last_,current_=End;
        bool skipSources_;
        void skip(){
            while(current_!=End&&current_>=first_&&current_<last_){const auto key=room_.state(current_);current_=room_.next(0,&key,false);}
        }
    public:
        AnimationCursor(const RunnerSourceRoomMembership& room,unsigned first,unsigned last):
            room_(room),ordinary_(room,first,last),first_(first),last_(last),skipSources_(first<=First&&last>=Last){
            if(!skipSources_){current_=room_.next(0,nullptr,false);skip();}
        }
        unsigned slot(){return skipSources_?ordinary_.slot():current_;}
        bool within(unsigned first,unsigned last){const auto value=slot();return value>=first&&value<last;}
        unsigned take(){
            if(skipSources_)return ordinary_.take();
            const unsigned value=current_;
            if(value!=End){const auto key=room_.state(value);current_=room_.next(0,&key,false);skip();}
            return value;
        }
    };
    template<class Visit> bool visitAnimationChecked(Visit visit)const{
        if(!terrain_||!terrain_->sourcePopulation())return Base::visitAnimationChecked(visit);
        return const_cast<RunnerSourceRoomMembership*>(this)->walk(0,false,visit);
    }
    template<class Visit> bool visitDrawChecked(Visit visit)const{
        if(!terrain_||!terrain_->sourcePopulation())return Base::visitDrawChecked(visit);
        return const_cast<RunnerSourceRoomMembership*>(this)->walk(0,true,visit);
    }
    template<class Visit> bool visitAnimationRetainedChecked(unsigned a,unsigned b,bool& dirty,Visit visit)const{
        if(a>b||b>Capacity)return false;
        if(!terrain_||!terrain_->sourcePopulation())return Base::visitAnimationRetainedChecked(a,b,dirty,visit);
        if(!dirty&&a<=First&&b>=Last){
            bool interrupted=false;State after;
            const bool okay=Base::visitAnimationRetainedChecked(a,b,dirty,[&](unsigned slot){
                after=state(slot);if(!visit(slot))return false;
                if(dirty){interrupted=true;return false;}return true;
            });
            if(!interrupted)return okay;
            return const_cast<RunnerSourceRoomMembership*>(this)->walk(0,false,visit,&after);
        }
        bool full=dirty;dirty=false;
        if(full){DW_PROFILE_COUNT(counterAnimationFullPass);}else{DW_PROFILE_COUNT(counterAnimationProjectedPass);}
        return visitAnimationChecked([&](unsigned slot){if(dirty)full=true;return (!full&&slot>=a&&slot<b)||visit(slot);});
    }
    template<class Visit> bool visitAnimationProjectedChecked(AnimationProjection&,unsigned a,unsigned b,bool& dirty,Visit visit)const{
        return visitAnimationRetainedChecked(a,b,dirty,visit);
    }
    template<class Visit> bool visitCleanupChecked(Visit visit){return walk(0,false,visit)&&walk(1,false,visit);}
    template<class Visit> bool visitMarkedCleanupOrdered(Visit visit){
        for(int list=0;list<2;++list){unsigned slot=next(list,nullptr,false,true);
            while(slot!=End){const State value=state(slot);
                if(!visit(slot,value.depth,list==0))return false;
                slot=next(list,&value,false,true);
            }
        }return true;
    }
    template<class Visit> bool visitMarkedCleanupChecked(Visit visit){
        return visitMarkedCleanupOrdered([&](unsigned slot,RunnerImageValue,bool){return visit(slot);});
    }
    template<class Select,class Inactive,class Publish> bool reconcileSelectedChecked(Select select,Inactive inactive,Publish publish){
        // No generated activation changes: preserve the linked-list fast path.
        if(select(First,Last)==Last)return Base::reconcileSelectedChecked(select,inactive,[&](unsigned slot,bool active){
            ordinaryOrder_[slot]=clock_.advance(publications());return publish(slot,active);
        });
        for(int list=1;list>=0;--list){
            if(!walk(list,false,[&](unsigned slot){
                if(select(slot,slot+1)!=slot||bool(inactive(slot))==bool(list))return true;
                State value;
                if(source(slot,value)){
                    value.list=list?0:1;value.order=clock_.advance(publications());
                    if(!terrain_->setSourceRoomState(slot-First,value))return false;
                    return publish(slot,list!=0);
                }
                return Base::reconcileSelectedChecked([&](unsigned first,unsigned end){return first<=slot&&slot<end?slot:end;},
                    [&](unsigned){return !bool(list);},[&](unsigned current,bool active){
                        ordinaryOrder_[current]=clock_.advance(publications());return publish(current,active);
                    });
            }))return false;
        }return true;
    }
    template<class Inactive,class Publish> bool reconcileChecked(Inactive inactive,Publish publish){
        return reconcileSelectedChecked([](unsigned first,unsigned){return first;},inactive,publish);
    }
    template<class Inactive,class Publish> void reconcile(Inactive inactive,Publish publish){
        reconcileChecked(inactive,[&](unsigned slot,bool active){publish(slot,active);return true;});
    }
};
}
#endif

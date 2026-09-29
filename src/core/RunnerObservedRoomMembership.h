#ifndef DOWNWELL_RUNNER_OBSERVED_ROOM_MEMBERSHIP_H
#define DOWNWELL_RUNNER_OBSERVED_ROOM_MEMBERSHIP_H
#include "RunnerRoomOrderState.h"
#include "RunnerSpatialSplit.h"
#include "ProfileCounters.h"
namespace downwell {
// No terrain room nodes or locator array. Two derived, independently invalidated
// domain orders are merged only when observed. Actor mutations do not sort the
// terrain domain. Visitors retain the scalar room owner's mutation contracts.
template<unsigned Capacity,unsigned First,unsigned Last,class Terrain>
class RunnerObservedRoomMembership {
    static_assert(First<Last&&Last<=Capacity&&Capacity<65534,"Room domains");
    static constexpr unsigned Actors=Capacity-(Last-First),Cells=Last-First;
    using Slot=std::uint16_t;
    using State=RunnerRoomOrderState;
    State actors_[Actors?Actors:1]{};
    Slot liveActors_[Actors?Actors:1]{};
    unsigned actorCount_=0,actorRevision_=0,markedCount_=0;
    Terrain* terrain_=nullptr;
    RunnerRegistrationOrder clock_;
    template<unsigned N> struct Projection {
        Slot slots[N?N:1]{};
        unsigned count=0,activeEnd=0,revision=0;
        bool valid=false;
    };
    mutable Projection<Actors> actorOrder_;
    mutable Projection<Cells> terrainOrder_;
    Projection<Capacity> markedOrder_;
    void compactMarks(){
        unsigned count=0;
        for(unsigned i=0;i<markedOrder_.count;++i){const unsigned s=markedOrder_.slots[i];
            if(s!=Capacity&&marked(s))markedOrder_.slots[count++]=Slot(s);
        }
        markedOrder_.count=count;markedCount_=count;
    }
    void unmark(unsigned s){
        if(!state(s)->marked)return;
        state(s)->marked=false;--markedCount_;
        // Leave a hole during cleanup: its callback may remove the current
        // member, but must not shift the saved successors underneath the walk.
        for(unsigned i=0;i<markedOrder_.count;++i)if(markedOrder_.slots[i]==s){
            markedOrder_.slots[i]=Slot(Capacity);break;
        }
    }
    static bool external(unsigned s){return s>=First&&s<Last;}
    static unsigned actorSlot(unsigned s){return s<First?s:s-Cells;}
    State* state(unsigned s){
        if(s>=Capacity)return nullptr;
        return external(s)?(terrain_?terrain_->roomState(s-First):nullptr):&actors_[actorSlot(s)];
    }
    const State* state(unsigned s)const{
        if(s>=Capacity)return nullptr;
        return external(s)?(terrain_?terrain_->roomState(s-First):nullptr):&actors_[actorSlot(s)];
    }
    void changed(unsigned s){if(external(s))terrain_->roomChanged();else ++actorRevision_;}
    bool before(unsigned a,unsigned b)const{
        const auto& x=*state(a);const auto& y=*state(b);
        if(x.list!=y.list)return x.list<y.list;
        if(!x.list){
            if(RunnerSpatialSplit::less(x.depth,y.depth))return true;
            if(RunnerSpatialSplit::less(y.depth,x.depth))return false;
        }
        return x.order.before(y.order);
    }
    template<unsigned N> void sort(Projection<N>& p,unsigned revision)const{
        // Birth-ordered same-depth terrain is already sorted. Do not turn the
        // common linear expansion into a depth-comparison sort per block.
        bool ordered=true;
        for(unsigned i=1;i<p.count;++i)if(before(p.slots[i],p.slots[i-1])){ordered=false;break;}
        if(!ordered){
            auto sift=[&](unsigned root,unsigned end){
                const Slot value=p.slots[root];
                while(root<end/2){unsigned child=root*2+1;
                    if(child+1<end&&before(p.slots[child],p.slots[child+1]))++child;
                    if(!before(value,p.slots[child]))break;
                    p.slots[root]=p.slots[child];root=child;
                }p.slots[root]=value;
            };
            for(unsigned i=p.count/2;i;--i)sift(i-1,p.count);
            for(unsigned end=p.count;end>1;){--end;const Slot v=p.slots[end];
                p.slots[end]=p.slots[0];p.slots[0]=v;sift(0,end);}
        }
        p.activeEnd=0;
        while(p.activeEnd<p.count&&state(p.slots[p.activeEnd])->list==0)++p.activeEnd;
        p.revision=revision;p.valid=true;
    }
    void observeActors()const{
        if(!actorOrder_.valid||actorOrder_.revision!=actorRevision_){
            actorOrder_.count=actorCount_;
            for(unsigned i=0;i<actorCount_;++i)actorOrder_.slots[i]=liveActors_[i];
            sort(actorOrder_,actorRevision_);
        }
    }
    void observe()const{
        observeActors();
        const unsigned revision=terrain_?terrain_->roomRevision():0;
        if(!terrainOrder_.valid||terrainOrder_.revision!=revision){
            terrainOrder_.count=0;
            if(terrain_)terrain_->visitRoomSlots([&](unsigned s){
                terrainOrder_.slots[terrainOrder_.count++]=Slot(First+s);return true;
            });
            sort(terrainOrder_,revision);
        }
    }
    // Do not observe/rebuild while consuming a pass. Cleanup may remove its
    // current slot; reconciliation changes lists before invoking publication.
    template<class Visit> bool walk(unsigned list,bool reverse,Visit visit)const{
        observe();
        unsigned a=list?actorOrder_.activeEnd:0,t=list?terrainOrder_.activeEnd:0;
        unsigned ae=list?actorOrder_.count:actorOrder_.activeEnd;
        unsigned te=list?terrainOrder_.count:terrainOrder_.activeEnd;
        while(a<ae||t<te){
            unsigned s;
            if(reverse){
                if(a==ae)s=terrainOrder_.slots[--te];
                else if(t==te)s=actorOrder_.slots[--ae];
                else if(before(actorOrder_.slots[ae-1],terrainOrder_.slots[te-1]))s=terrainOrder_.slots[--te];
                else s=actorOrder_.slots[--ae];
            }else{
                if(a==ae)s=terrainOrder_.slots[t++];
                else if(t==te)s=actorOrder_.slots[a++];
                else if(before(actorOrder_.slots[a],terrainOrder_.slots[t]))s=actorOrder_.slots[a++];
                else s=terrainOrder_.slots[t++];
            }
            if(!visit(s))return false;
        }return true;
    }
public:
    static constexpr unsigned End=Capacity;
    struct AnimationProjection {};
    bool bindTerrain(Terrain& terrain){if(terrain_)return false;terrain_=&terrain;return true;}
    bool contains(unsigned s)const{const auto* p=state(s);return p&&p->list>=0;}
    bool active(unsigned s)const{const auto* p=state(s);return p&&p->list==0;}
    bool marked(unsigned s)const{const auto* p=state(s);return p&&p->list>=0&&p->marked;}
    bool mark(unsigned s){if(!contains(s))return false;
        if(!state(s)->marked){
            if(markedOrder_.count==Capacity)compactMarks();
            state(s)->marked=true;++markedCount_;
            markedOrder_.slots[markedOrder_.count++]=Slot(s);
        }return true;}
    bool bind(unsigned s,RunnerImageValue key){
        auto* p=state(s);if(!p||!RunnerSpatialSplit::valid(key))return false;
        unmark(s);
        if(p->list<0&&!external(s))liveActors_[actorCount_++]=Slot(s);
        clock_.advance();*p={clock_,key,0,false};changed(s);return true;
    }
    bool remove(unsigned s){
        if(!contains(s))return false;
        unmark(s);
        *state(s)=State{};
        if(!external(s))for(unsigned i=0;i<actorCount_;++i)if(liveActors_[i]==s){
            liveActors_[i]=liveActors_[--actorCount_];break;
        }
        changed(s);return true;
    }
    void clear(){
        while(actorCount_)remove(liveActors_[actorCount_-1]);
        if(terrain_)terrain_->visitRoomSlots([&](unsigned s){remove(First+s);return true;});
    }
    bool prepareRetainedAnimation(unsigned a,unsigned b)const{return a<=b&&b<=Capacity;}
    template<class Visit> bool visitAnimationChecked(Visit visit)const{return walk(0,false,visit);}
    template<class Visit> bool visitDrawChecked(Visit visit)const{return walk(0,true,visit);}
    template<class Visit> bool visitAnimationRetainedChecked(unsigned a,unsigned b,bool& dirty,Visit visit)const{
        if(a>b||b>Capacity)return false;
        bool full=dirty;dirty=false;
        if(full){DW_PROFILE_COUNT(counterAnimationFullPass);}else{DW_PROFILE_COUNT(counterAnimationProjectedPass);}
        if(!full&&a==First&&b==Last){
            observeActors();
            for(unsigned i=0;i<actorOrder_.activeEnd;++i){
                const unsigned s=actorOrder_.slots[i];
                if(!visit(s))return false;
                // Resume from the raw successor, including terrain, only if
                // the actor wrote an omitted position during this callback.
                if(dirty)return walk(0,false,[&](unsigned next){return !before(s,next)||visit(next);});
            }
            return true;
        }
        return walk(0,false,[&](unsigned s){
            if(dirty)full=true;
            return (!full&&s>=a&&s<b)||visit(s);
        });
    }
    template<class Visit> bool visitAnimationProjectedChecked(AnimationProjection&,unsigned a,unsigned b,bool& dirty,Visit visit)const{
        return visitAnimationRetainedChecked(a,b,dirty,visit);
    }
    template<class Visit> bool visitCleanupChecked(Visit visit){return walk(0,false,visit)&&walk(1,false,visit);}
    template<class Visit> bool visitMarkedCleanupChecked(Visit visit){
        if(!markedCount_)return true;
        compactMarks();sort(markedOrder_,0);
        // Only the sparse destruction set needs room-order materialization.
        // Do not rebuild either complete domain for one destroyed block.
        for(unsigned i=0;i<markedOrder_.count;++i){const unsigned s=markedOrder_.slots[i];
            if(!visit(s)){compactMarks();return false;}
        }
        compactMarks();
        return true;
    }
    template<class Inactive,class Publish> bool reconcileChecked(Inactive inactive,Publish publish){
        for(unsigned list=1;;--list){
            if(!walk(list,false,[&](unsigned s){
                if(bool(inactive(s))==bool(list))return true;
                auto& p=*state(s);clock_.advance();p.order=clock_;p.list=std::int8_t(1-list);changed(s);
                return publish(s,list!=0);
            }))return false;
            if(!list)break;
        }return true;
    }
    template<class Inactive,class Publish> void reconcile(Inactive inactive,Publish publish){
        reconcileChecked(inactive,[&](unsigned s,bool registered){publish(s,registered);return true;});
    }
};
}
#endif

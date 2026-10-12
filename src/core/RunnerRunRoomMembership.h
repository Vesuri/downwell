#ifndef DOWNWELL_RUNNER_RUN_ROOM_MEMBERSHIP_H
#define DOWNWELL_RUNNER_RUN_ROOM_MEMBERSHIP_H
#include "RunnerSpatialSplit.h"
#include "ProfileCounters.h"
#include "TerrainReadCensus.h"
namespace downwell {
// Source room order represented by contiguous terrain-slot runs. Actor nodes
// are singletons. Descriptors reserve the full slot capacity: splitting an
// interior cell never adds a failure boundary. No game callback is deferred.
template<unsigned Capacity,unsigned First,unsigned Last> class RunnerRunRoomMembership {
    static_assert(First<=Last&&Last<=Capacity&&Capacity<65534,"Invalid room run domain");
    using Slot=std::uint16_t;
    struct Run {RunnerImageValue key{};Slot first=0,count=0,next=Capacity,previous=Capacity,marks=0,list=0;};
    Run runs_[Capacity?Capacity:1]{};
    Slot locator_[Capacity?Capacity:1]{};
    std::uint32_t marks_[(Capacity+31)/32?(Capacity+31)/32:1]{};
    unsigned head_[2]={Capacity,Capacity},tail_[2]={Capacity,Capacity};
    unsigned used_=0,free_=Capacity,markedCount_=0;
    bool terrain(unsigned s)const{return s>=First&&s<Last;}
    unsigned allocate(){if(free_!=End){const unsigned n=free_;free_=runs_[n].next;return n;}return used_++;}
    void release(unsigned n){runs_[n].next=Slot(free_);free_=n;}
    unsigned first(unsigned list)const{return head_[list]==End?End:runs_[head_[list]].first;}
    unsigned after(unsigned n)const{return runs_[n].next==End?End:runs_[runs_[n].next].first;}
    unsigned next(unsigned s)const{const auto& r=runs_[locator_[s]-1];
        return s+1<unsigned(r.first)+r.count?s+1:after(locator_[s]-1);}
    void insertAfter(unsigned slot,RunnerImageValue key,unsigned list,unsigned previous){
        if(previous!=End){auto& r=runs_[previous];
            if(terrain(slot)&&terrain(r.first)&&unsigned(r.first)+r.count==slot&&r.key.bits==key.bits){
                ++r.count;locator_[slot]=Slot(previous+1);return;
            }
        }
        const unsigned n=allocate(),following=previous==End?head_[list]:runs_[previous].next;
        runs_[n]={key,Slot(slot),1,Slot(following),Slot(previous),0,Slot(list)};
        if(previous==End)head_[list]=n;else runs_[previous].next=Slot(n);
        if(following==End)tail_[list]=n;else runs_[following].previous=Slot(n);
        locator_[slot]=Slot(n+1);
    }
    void insertActive(unsigned slot,RunnerImageValue key){
        unsigned previous=tail_[0],forward=head_[0];
        while(previous!=End&&RunnerSpatialSplit::less(key,runs_[previous].key)){
            if(RunnerSpatialSplit::less(key,runs_[forward].key)){previous=runs_[forward].previous;break;}
            forward=runs_[forward].next;previous=runs_[previous].previous;
        }
        insertAfter(slot,key,0,previous);
    }
public:
    static constexpr unsigned End=Capacity;
    struct AnimationProjection {}; // Runs themselves supply the skip projection.
    // A borrowed cursor, not a prepared list. Family consumers may process
    // consecutive entries without changing room order. Save next before each
    // handler; skip non-animated runs only after that handler has returned.
    class AnimationCursor {
        const RunnerRunRoomMembership& room_;
        unsigned current_,skipFirst_,skipLast_;
    public:
        AnimationCursor(const RunnerRunRoomMembership& room,unsigned first,unsigned last)
            :room_(room),current_(room.first(0)),skipFirst_(first),skipLast_(last){}
        unsigned slot(){
            while(current_!=End&&current_>=skipFirst_&&current_<skipLast_){
                DW_TERRAIN_SLOT_READ(terrainRoomAnimationSkip,current_,First,Last);
                const auto n=room_.locator_[current_]-1;
                const auto& run=room_.runs_[n];
                current_=skipLast_<unsigned(run.first)+run.count?skipLast_:room_.after(n);
            }
            return current_;
        }
        bool within(unsigned first,unsigned last){const auto s=slot();return s>=first&&s<last;}
        unsigned take(){const auto s=slot();if(s!=End)current_=room_.next(s);return s;}
    };
    bool contains(unsigned s)const{DW_TERRAIN_SLOT_READ(terrainRoomPresence,s,First,Last);return s<Capacity&&locator_[s]!=0;}
    bool active(unsigned s)const{DW_TERRAIN_SLOT_READ(terrainRoomActive,s,First,Last);return contains(s)&&runs_[locator_[s]-1].list==0;}
    bool marked(unsigned s)const{return contains(s)&&((marks_[s>>5]>>(s&31))&1u);}
    bool mark(unsigned s){
        if(!contains(s))return false;
        if(!marked(s)){marks_[s>>5]|=1u<<(s&31);++runs_[locator_[s]-1].marks;++markedCount_;}
        return true;
    }
    bool bind(unsigned s,RunnerImageValue key){
        if(s>=Capacity||!RunnerSpatialSplit::valid(key))return false;
        remove(s);insertActive(s,key);return true;
    }
    // A callback-free, freshly allocated terrain segment. One ordered
    // insertion supplies all its cells; the scalar bind remains the oracle.
    bool bindTerrainSpan(unsigned first,unsigned count,RunnerImageValue key){
        if(!count)return true;
        if(first<First||first>=Last||count>Last-first||!RunnerSpatialSplit::valid(key))return false;
        for(unsigned s=first;s<first+count;++s)if(contains(s))return false;
        insertActive(first,key);
        const unsigned n=locator_[first]-1;
        auto& run=runs_[n];
        run.count=Slot(unsigned(run.count)+count-1);
        for(unsigned s=first+1;s<first+count;++s)locator_[s]=Slot(n+1);
        return true;
    }
    bool remove(unsigned s){
        if(!contains(s))return false;
        const unsigned n=locator_[s]-1;auto& r=runs_[n];
        if(marked(s)){marks_[s>>5]&=~(1u<<(s&31));--r.marks;--markedCount_;}
        if(r.count==1){
            if(r.previous==End)head_[r.list]=r.next;else runs_[r.previous].next=r.next;
            if(r.next==End)tail_[r.list]=r.previous;else runs_[r.next].previous=r.previous;
            release(n);
        }else if(s==r.first){++r.first;--r.count;}
        else if(s==unsigned(r.first)+r.count-1)--r.count;
        else{
            const unsigned right=allocate(),offset=s-r.first+1;
            runs_[right]={r.key,Slot(s+1),Slot(r.count-offset),r.next,Slot(n),0,r.list};
            auto& rr=runs_[right];
            for(unsigned i=rr.first;i<unsigned(rr.first)+rr.count;++i){
                locator_[i]=Slot(right+1);if(marked(i))++rr.marks;
            }
            r.marks=Slot(r.marks-rr.marks);
            if(r.next==End)tail_[r.list]=right;else runs_[r.next].previous=Slot(right);
            r.next=Slot(right);r.count=Slot(offset-1);
        }
        locator_[s]=0;return true;
    }
    void clear(){
        // Whole-room teardown has no mutation callbacks. Keep descriptor
        // release order, but do not shrink/relink each cell of a terrain run.
        for(unsigned list=0;list<2;++list){
            unsigned n=head_[list];
            while(n!=End){
                const unsigned following=runs_[n].next;
                const unsigned end=unsigned(runs_[n].first)+runs_[n].count;
                for(unsigned s=runs_[n].first;s<end;++s)locator_[s]=0;
                release(n);n=following;
            }
            head_[list]=tail_[list]=End;
        }
        if(markedCount_)for(auto& word:marks_)word=0;
        markedCount_=0;
    }
    void clearReference(){for(unsigned list=0;list<2;++list)while(head_[list]!=End)remove(first(list));}
    unsigned runCount()const{
        unsigned count=0;
        for(unsigned list=0;list<2;++list){
            for(unsigned n=head_[list];n!=End;n=runs_[n].next)++count;
        }
        return count;
    }
    template<class Visit> bool visitAnimationChecked(Visit visit)const{
        for(unsigned s=first(0);s!=End;s=next(s))if(!visit(s))return false;return true;
    }
    template<class Visit> bool visitDrawChecked(Visit visit)const{
        for(unsigned n=tail_[0];n!=End;n=runs_[n].previous){const auto& r=runs_[n];
            for(unsigned i=r.count;i;--i)if(!visit(r.first+i-1))return false;
        }return true;
    }
    bool prepareRetainedAnimation(unsigned a,unsigned b)const{return a<=b&&b<=Capacity;}
    template<class Visit> bool visitAnimationProjectedChecked(AnimationProjection&,unsigned a,unsigned b,bool& dirty,Visit visit)const{
        return visitAnimationRetainedChecked(a,b,dirty,visit);
    }
    template<class Visit> bool visitAnimationRetainedChecked(unsigned a,unsigned b,bool& dirty,Visit visit)const{
        if(a>b||b>Capacity)return false;
        bool full=dirty;dirty=false;
        if(full){DW_PROFILE_COUNT(counterAnimationFullPass);}else{DW_PROFILE_COUNT(counterAnimationProjectedPass);}
        for(unsigned s=first(0);s!=End;){
            if(dirty)full=true;
            if(!full&&s>=a&&s<b){const auto& r=runs_[locator_[s]-1];
                s=b<unsigned(r.first)+r.count?b:after(locator_[s]-1);continue;
            }
            const unsigned following=next(s);if(!visit(s))return false;s=following;
        }return true;
    }
    template<class Visit> bool visitCleanupChecked(Visit visit){
        for(unsigned list=0;list<2;++list){
            for(unsigned s=first(list);s!=End;){
                const unsigned following=next(s);if(!visit(s))return false;s=following;
            }
        }
        return true;
    }
    template<class Visit> bool visitMarkedCleanupChecked(Visit visit){
        return visitMarkedCleanupOrdered([&](unsigned slot,RunnerImageValue,bool){return visit(slot);});
    }
    // Include the source room sort key and active-list boundary so native
    // family records can be merged without constructing room nodes at birth.
    template<class Visit> bool visitMarkedCleanupOrdered(Visit visit){
        if(!markedCount_)return true;
        for(unsigned list=0;list<2;++list){
            for(unsigned s=first(list);s!=End;){
                const unsigned n=locator_[s]-1;
                const unsigned end=unsigned(runs_[n].first)+runs_[n].count;
                DW_TERRAIN_SLOT_READ(terrainRoomCleanup,s,First,Last);
                if(!runs_[n].marks){s=after(n);continue;}
                // Marks already belong to this owner. Select the next marked
                // cell without expanding an otherwise unchanged terrain run.
                // Re-read after each callback: removal may split the run and
                // the callback can mark a later lifetime in source room order.
                while(s<end){
                    const unsigned limit=(s|31u)+1<end?(s|31u)+1:end;
                    auto word=marks_[s>>5]>>(s&31);
                    if(word){
                        while(s<limit&&!(word&1u)){++s;word>>=1;}
                        if(s<limit)break;
                    }
                    s=limit;
                }
                if(s==end){s=after(n);continue;}
                const unsigned following=next(s);
                if(!visit(s,runs_[n].key,list==0))return false;
                s=following;
            }
        }
        return true;
    }
    template<class Visit> bool visitMarkedCleanupOrderedReference(Visit visit){
        if(!markedCount_)return true;
        for(unsigned list=0;list<2;++list){
            for(unsigned s=first(list);s!=End;){
                const unsigned n=locator_[s]-1;
                if(!runs_[n].marks){s=after(n);continue;}
                const unsigned following=next(s);
                if(marked(s)&&!visit(s,runs_[n].key,list==0))return false;s=following;
            }
        }
        return true;
    }
    // Selection only removes unchanged flag reads; traversal/publication stays
    // inactive-room then active-room order. Callbacks must not mutate this owner
    // or activation flags. Promoted entries may be visited again in list zero.
    template<class Select,class Inactive,class Publish>
    bool reconcileSelectedChecked(Select select,Inactive inactive,Publish publish){
        for(unsigned list=1;;--list){
            for(unsigned s=first(list);s!=End;){
                const unsigned n=locator_[s]-1;
                const unsigned end=unsigned(runs_[n].first)+runs_[n].count;
                s=select(s,end);
                DW_TERRAIN_SLOT_READ(terrainRoomReconcile,runs_[n].first,First,Last);
                if(s==end){s=after(n);continue;}
                const unsigned following=next(s);
                if(bool(inactive(s))!=bool(list)){
                    const auto key=runs_[locator_[s]-1].key;const bool wasMarked=marked(s);
                    remove(s);
                    if(list)insertActive(s,key);else insertAfter(s,key,1,tail_[1]);
                    if(wasMarked)mark(s);
                    if(!publish(s,list!=0))return false;
                }
                s=following;
            }
            if(!list)break;
        }return true;
    }
    template<class Inactive,class Publish> bool reconcileChecked(Inactive inactive,Publish publish){
        return reconcileSelectedChecked([](unsigned first,unsigned){return first;},inactive,publish);
    }
    template<class Inactive,class Publish> void reconcile(Inactive inactive,Publish publish){
        reconcileChecked(inactive,[&](unsigned s,bool registered){publish(s,registered);return true;});
    }
};
}
#endif

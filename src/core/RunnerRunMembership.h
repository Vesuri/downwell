#ifndef DOWNWELL_RUNNER_RUN_MEMBERSHIP_H
#define DOWNWELL_RUNNER_RUN_MEMBERSHIP_H
#include "Profile.h"
#include <cstdint>
namespace downwell {
// Registration order, not Draw order. Consecutive terrain slots/generations
// share a run; actors remain singletons. The slot map is only a locator, not
// a second registration list. Reserve enough descriptors for total splitting
// so this representation introduces no new capacity/failure boundary.
template<unsigned Capacity,unsigned First,unsigned Last> class RunnerRunMembership {
    static_assert(First<=Last&&Last<=Capacity&&Capacity<65535,"Invalid run domain");
    using Slot=std::uint16_t;
    struct Run {std::uint32_t identity=0;int object=-1;Slot first=0,count=0,next=Capacity,previous=Capacity;};
    Run runs_[Capacity?Capacity:1]{};
    // Zero means absent; live locators store descriptor index + 1.
    Slot locator_[Capacity?Capacity:1]{};
    Slot actors_[Capacity-(Last-First)?Capacity-(Last-First):1]{};
    unsigned head_=Capacity,tail_=Capacity,free_=Capacity,used_=0,count_=0,actorsCount_=0,revision_=0;
    bool actor(unsigned slot)const{return slot<First||slot>=Last;}
    unsigned allocate(){
        if(free_!=Capacity){const unsigned n=free_;free_=runs_[n].next;return n;}
        return used_++;
    }
    void release(unsigned n){runs_[n].next=Slot(free_);free_=n;}
    const Run& at(unsigned slot)const{return runs_[locator_[slot]-1];}
    std::uint32_t identity(unsigned slot)const{const auto& r=at(slot);return r.identity+slot-r.first;}
public:
    static constexpr unsigned End=Capacity;
    template<unsigned OmitFirst,unsigned OmitLast> struct RegistrationProjection {
        static_assert(OmitFirst<=OmitLast&&OmitLast<=Capacity,"Invalid projection");
        static constexpr unsigned slots=Capacity-(OmitLast-OmitFirst);
        Slot order[slots?slots:1]{};
        const RunnerRunMembership* owner=nullptr;
        unsigned revision=0,count=0;
    };
    bool contains(unsigned slot)const{return slot<Capacity&&locator_[slot]!=0;}
    unsigned size()const{return count_;}
    unsigned runCount()const{unsigned n=0;for(unsigned r=head_;r!=End;r=runs_[r].next)++n;return n;}
    // Same contract as the scalar registry: only immediately after a fresh
    // successful bind, with no intervening membership writes or callbacks.
    void repeatFreshTailBinding(){revision_+=2;}
    bool bind(unsigned slot,int object,std::uint32_t id){
        if(slot>=Capacity||object<0)return false;
        if(tail_!=End){const auto& t=runs_[tail_];
            if(slot==unsigned(t.first)+t.count-1&&t.object==object&&identity(slot)==id){revision_+=2;return true;}}
        unbind(slot);
        if(!actor(slot)&&tail_!=End){auto& t=runs_[tail_];
            if(!actor(t.first)&&unsigned(t.first)+t.count==slot&&t.object==object&&t.identity+t.count==id){
                ++t.count;locator_[slot]=Slot(tail_+1);++count_;++revision_;return true;
            }
        }
        const unsigned n=allocate();
        runs_[n]={id,object,Slot(slot),1,Slot(End),Slot(tail_)};
        if(tail_==End)head_=n;else runs_[tail_].next=Slot(n);
        tail_=n;locator_[slot]=Slot(n+1);++count_;++revision_;
        if(actor(slot))actors_[actorsCount_++]=Slot(slot);
        return true;
    }
    bool unbind(unsigned slot){
        if(!contains(slot))return false;
        const unsigned n=locator_[slot]-1;auto& r=runs_[n];
        if(actor(slot)){
            unsigned i=0;while(i<actorsCount_&&actors_[i]!=slot)++i;
            for(++i;i<actorsCount_;++i)actors_[i-1]=actors_[i];
            --actorsCount_;
        }
        if(r.count==1){
            if(r.previous==End)head_=r.next;else runs_[r.previous].next=r.next;
            if(r.next==End)tail_=r.previous;else runs_[r.next].previous=r.previous;
            release(n);
        }else if(slot==r.first){++r.first;++r.identity;--r.count;}
        else if(slot==unsigned(r.first)+r.count-1)--r.count;
        else{
            const unsigned right=allocate(),offset=slot-r.first+1;
            runs_[right]={r.identity+offset,r.object,Slot(slot+1),Slot(r.count-offset),r.next,Slot(n)};
            if(r.next==End)tail_=right;else runs_[r.next].previous=Slot(right);
            r.next=Slot(right);r.count=Slot(offset-1);
            for(unsigned s=slot+1;s<unsigned(runs_[right].first)+runs_[right].count;++s)locator_[s]=Slot(right+1);
        }
        locator_[slot]=0;--count_;++revision_;return true;
    }
    void clear(){
        // No callbacks can observe intermediate removals. Release each run
        // once, retaining the scalar revision and descriptor free-list order.
        unsigned n=head_;
        while(n!=End){
            const unsigned following=runs_[n].next;
            const unsigned end=unsigned(runs_[n].first)+runs_[n].count;
            for(unsigned s=runs_[n].first;s<end;++s)locator_[s]=0;
            release(n);n=following;
        }
        revision_+=count_;head_=tail_=End;count_=actorsCount_=0;
    }
    void clearReference(){while(head_!=End)unbind(runs_[head_].first);}
    template<class Visit> bool visitRegistrations(Visit visit)const{return visitRegistrationRange(0,Capacity,visit);}
    template<class Visit> bool visitRegistrationRange(unsigned first,unsigned last,Visit visit)const{
        if(first>last||last>Capacity)return false;
        if(first==last)return true;
        for(unsigned n=head_;n!=End;n=runs_[n].next){const auto& r=runs_[n];
            const unsigned begin=r.first>first?r.first:first;
            const unsigned end=unsigned(r.first)+r.count<last?unsigned(r.first)+r.count:last;
            for(unsigned s=begin;s<end;++s)if(!visit(s,r.identity+s-r.first,r.object))return false;
        }
        return true;
    }
    template<unsigned A,unsigned B,class Visit>
    bool visitRegistrationRange(RegistrationProjection<A,B>& projection,unsigned first,unsigned last,Visit visit)const{
        if(first>last||last>Capacity)return false;
        if(first==last)return true;
        if(A<B&&first<B&&last>A)return false;
        if(projection.owner!=this||projection.revision!=revision_){
            projection.count=0;
            visitRegistrations([&](unsigned s,auto,auto){if(s<A||s>=B)projection.order[projection.count++]=Slot(s);return true;});
            projection.owner=this;projection.revision=revision_;
        }
        for(unsigned i=0;i<projection.count;++i){const unsigned s=projection.order[i];
            if(s>=first&&s<last&&(!visit(s,identity(s),at(s).object)||projection.revision!=revision_))return false;
        }
        return true;
    }
    template<class Visit> bool visitRetainedRegistrationRange(unsigned first,unsigned last,Visit visit)const{
        if(first>last||last>Capacity)return false;
        if(first==last)return true;
        if(first<Last&&last>First)return false;
        const unsigned revision=revision_;
        for(unsigned i=0;i<actorsCount_;++i){const unsigned s=actors_[i];
            if(s>=first&&s<last){const auto& r=at(s); // Actors are always singletons.
                if(!visit(s,r.identity,r.object)||revision!=revision_)return false;
            }
        }
        return true;
    }
    template<class IsA,class Visit> void visit(int object,IsA isA,Visit callback)const{
        // Ancestry is immutable during traversal; callbacks may inspect or
        // refresh instances, but must not change registration or ancestry.
        // A run has one object, so rejected families need no per-cell walk.
        if(object<0)return;
        for(unsigned n=head_;n!=End;n=runs_[n].next){const auto& r=runs_[n];
            if(r.object!=object&&!isA(r.object,object))continue;
            const unsigned end=unsigned(r.first)+r.count;
            for(unsigned s=r.first;s<end;++s)
                if(!callback(s,r.identity+s-r.first))return;
        }
    }
    template<class IsA,class Visit> void visitReference(int object,IsA isA,Visit callback)const{
        if(object<0)return;
        visitRegistrations([&](unsigned s,std::uint32_t id,int kind){
            return !(kind==object||isA(kind,object))||callback(s,id);
        });
    }
    template<class IsA> unsigned count(int object,IsA isA)const{
        DW_PROFILE_SCOPE(profileObjectPopulation);
        if(object<0)return 0;
        unsigned result=0;
        for(unsigned n=head_;n!=End;n=runs_[n].next){const auto& r=runs_[n];
            if(r.object==object||isA(r.object,object))result+=r.count;
        }
        return result;
    }
    template<class IsA> unsigned countReference(int object,IsA isA)const{
        unsigned result=0;visitReference(object,isA,[&](unsigned,auto){++result;return true;});return result;
    }
};
}
#endif

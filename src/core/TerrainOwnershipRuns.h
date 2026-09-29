#ifndef DOWNWELL_TERRAIN_OWNERSHIP_RUNS_H
#define DOWNWELL_TERRAIN_OWNERSHIP_RUNS_H
#include "InstanceHandle.h"
#include <cstdint>
namespace downwell {
// Authoritative immutable terrain identities, in birth order. Per-cell mutable
// Create results belong to the caller; no second identity/order list is needed.
// Retired cells are holes until their run becomes empty. Slot reuse changes the
// locator, never the historical geometry of surviving cells in that run.
template<unsigned Capacity> class TerrainOwnershipRuns {
    static_assert(Capacity>0&&Capacity<65535,"Terrain run locator domain");
    using Slot=std::uint16_t;
    static constexpr unsigned End=Capacity;
    struct Run {
        int object=0,x=0,y=0,step=0,creationCode=-1;
        std::uint32_t generation=0,source=0;
        Slot first=0,count=0,live=0,next=End,previous=End;
    };
    Run runs_[Capacity]{};
    Slot locator_[Capacity]{};
    unsigned head_=End,tail_=End,free_=End,used_=0,population_=0;
    unsigned allocate(){if(free_!=End){unsigned n=free_;free_=runs_[n].next;return n;}return used_++;}
public:
    struct Identity {
        InstanceHandle handle{};
        int object=0,x=0,y=0,creationCode=-1;
        std::uint32_t source=0;
    };
    unsigned size()const{return population_;}
    bool contains(InstanceHandle h)const{
        if(h.slot>=Capacity||!h.generation||!locator_[h.slot])return false;
        const auto& r=runs_[locator_[h.slot]-1];
        return h.generation==r.generation+h.slot-r.first;
    }
    bool occupied(unsigned slot)const{return slot<Capacity&&locator_[slot]!=0;}
    InstanceHandle currentHandle(unsigned slot)const{
        if(!occupied(slot))return {};
        const auto& r=runs_[locator_[slot]-1];return {slot,r.generation+slot-r.first};
    }
    int object(unsigned slot)const{return occupied(slot)?runs_[locator_[slot]-1].object:-1;}
    InstanceHandle handle(unsigned ordinal)const{
        if(ordinal>=population_)return {};
        // Live generation immediately reads the newest birth for Create.
        // Avoid traversing older runs for that operation.
        if(ordinal+1==population_){const auto& r=runs_[tail_];
            for(unsigned i=r.count;i;--i){const unsigned slot=r.first+i-1;
                if(locator_[slot]==tail_+1)return {slot,r.generation+i-1};
            }
        }
        for(unsigned n=head_;n!=End;n=runs_[n].next){const auto& r=runs_[n];
            if(ordinal>=r.live){ordinal-=r.live;continue;}
            for(unsigned i=0;i<r.count;++i){const unsigned slot=r.first+i;
                if(locator_[slot]==n+1){if(!ordinal)return {slot,r.generation+i};--ordinal;}
            }
        }
        return {};
    }
    bool read(InstanceHandle h,Identity& out)const{
        if(!contains(h))return false;
        const auto& r=runs_[locator_[h.slot]-1];
        const unsigned offset=h.slot-r.first;
        out={h,r.object,r.x+int(offset)*r.step,r.y,r.creationCode,r.source};return true;
    }
    bool append(InstanceHandle h,int object,int x,int y,std::uint32_t source=0,int creationCode=-1){
        if(h.slot>=Capacity||!h.generation||occupied(h.slot)||population_==Capacity||
            (object!=60&&object!=93&&object!=99))return false;
        // Only generated, adjacent horizontal cells share a descriptor.
        // Authored source identities and creation scripts remain singleton.
        if(tail_!=End&&!source&&creationCode==-1){auto& r=runs_[tail_];
            if(!r.source&&r.creationCode==-1&&r.object==object&&r.y==y&&
                unsigned(r.first)+r.count==h.slot&&r.generation+r.count==h.generation){
                // Avoid subtraction overflow, including unusual caller coordinates.
                const int last=r.x+int(r.count-1)*r.step;
                const bool right=last<=0x7fffffff-16&&x==last+16;
                const bool left=last>=(-2147483647-1)+16&&x==last-16;
                const int step=right?16:left?-16:0;
                if(step&&(r.count==1||r.step==step)){
                    r.step=step;++r.count;++r.live;locator_[h.slot]=Slot(tail_+1);++population_;return true;
                }
            }
        }
        const unsigned n=allocate();
        runs_[n]={object,x,y,0,creationCode,h.generation,source,Slot(h.slot),1,1,Slot(End),Slot(tail_)};
        if(tail_==End)head_=n;else runs_[tail_].next=Slot(n);
        tail_=n;locator_[h.slot]=Slot(n+1);++population_;return true;
    }
    bool retire(InstanceHandle h){
        if(!contains(h))return false;
        const unsigned n=locator_[h.slot]-1;auto& r=runs_[n];
        locator_[h.slot]=0;--population_;
        if(--r.live==0){
            if(r.previous==End)head_=r.next;else runs_[r.previous].next=r.next;
            if(r.next==End)tail_=r.previous;else runs_[r.next].previous=r.previous;
            r.next=Slot(free_);free_=n;
        }
        return true;
    }
private:
    template<bool Rows,class Visit> bool visitSelected(int first,int end,Visit visit)const{
        // No structural mutation during this borrowed traversal.
        for(unsigned n=head_;n!=End;n=runs_[n].next){const auto& r=runs_[n];
            // Select geometry before expanding identities: every cell in a run
            // shares Y. The margin admits the bounded wall artwork overhang.
            if(Rows&&((r.y<first&&unsigned(first)-unsigned(r.y)>=16u)||
                (r.y>=end&&unsigned(r.y)-unsigned(end)>=16u)))continue;
            int x=r.x;
            for(unsigned i=0;i<r.count;++i){const unsigned slot=r.first+i;
                if(locator_[slot]==n+1&&!visit(Identity{{slot,r.generation+i},r.object,x,r.y,r.creationCode,r.source}))return false;
                if(i+1<r.count)x+=r.step;
            }
        }
        return true;
    }
public:
    template<class Visit> bool visit(Visit visit)const{return visitSelected<false>(0,0,visit);}
    template<class Visit> bool visitRows(int first,int end,Visit visit)const{
        return first>=end||visitSelected<true>(first,end,visit);
    }
    unsigned runCount()const{unsigned count=0;for(unsigned n=head_;n!=End;n=runs_[n].next)++count;return count;}
};
}
#endif

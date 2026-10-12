#ifndef DOWNWELL_TERRAIN_OWNERSHIP_RUNS_H
#define DOWNWELL_TERRAIN_OWNERSHIP_RUNS_H
#include "InstanceHandle.h"
#include "TerrainReadCensus.h"
#include <cstdint>
namespace downwell {
struct TerrainIdentityOnly {
    TerrainIdentityOnly shifted(unsigned)const{return {};}
    bool same(const TerrainIdentityOnly&)const{return true;}
};
// Authoritative immutable terrain identities, in birth order. Per-cell mutable
// Create results belong to the caller; no second identity/order list is needed.
// Retired cells are holes until their run becomes empty. Slot reuse changes the
// locator, never the historical geometry of surviving cells in that run.
template<unsigned Capacity,class Publication=TerrainIdentityOnly> class TerrainOwnershipRuns {
    static_assert(Capacity>0&&Capacity<65535,"Terrain run locator domain");
    using Slot=std::uint16_t;
    static constexpr unsigned End=Capacity;
    struct Run:Publication {
        int object=0,x=0,y=0,step=0,creationCode=-1;
        std::uint32_t generation=0;
        // object == -2 borrows an immutable prepared-cell stream. Authored
        // identities instead retain their numeric source ID in the same space.
        union { std::uint32_t source=0; const unsigned char* cells; };
        Slot first=0,count=0,live=0,next=End,previous=End;
        int objectAt(unsigned i)const{return object==-2?((cells[2*i]&128)?93:60):object;}
        int xAt(unsigned i)const{return object==-2?(int(cells[2*i]&127)-32)*16:x+int(i)*step;}
        std::uint32_t sourceId()const{return object==-2?0:source;}
        void advance(unsigned n){if(object==-2)cells+=2*n;else x+=int(n)*step;}
    };
    Run runs_[Capacity]{};
    inline static const Publication initialPublication_{};
    Slot locator_[Capacity]{};
    unsigned head_=End,tail_=End,free_=End,used_=0,population_=0;
    unsigned reserved_=End;
    unsigned allocate(){if(free_!=End){unsigned n=free_;free_=runs_[n].next;return n;}return used_++;}
    // Split only at an actual mutation boundary. Geometry, lifetime and
    // publication share this descriptor and locator; no parallel per-cell
    // publication array is needed. Retired holes remain holes in both parts.
    unsigned split(unsigned n,unsigned offset){
        auto& left=runs_[n];
        const unsigned first=left.first+offset,end=left.first+left.count;
        unsigned rightLive=0;
        for(unsigned s=first;s<end;++s)if(locator_[s]==n+1)++rightLive;
        // Callers split around a live cell. An all-hole prefix/suffix can be
        // trimmed in place and must not consume a descriptor.
        if(!rightLive){left.count=Slot(offset);return End;}
        if(rightLive==left.live){
            static_cast<Publication&>(left)=static_cast<const Publication&>(left).shifted(offset);
            left.advance(offset);left.generation+=offset;
            left.first=Slot(first);left.count=Slot(end-first);return n;
        }
        const unsigned right=allocate();
        auto& r=runs_[right];r=left;
        static_cast<Publication&>(r)=static_cast<const Publication&>(left).shifted(offset);
        r.advance(offset);r.generation+=offset;
        r.first=Slot(first);r.count=Slot(end-first);r.live=Slot(rightLive);r.previous=Slot(n);
        if(r.next==End)tail_=right;else runs_[r.next].previous=Slot(right);
        left.next=Slot(right);left.count=Slot(offset);left.live-=Slot(rightLive);
        for(unsigned s=first;s<end;++s)if(locator_[s]==n+1)locator_[s]=Slot(right+1);
        return right;
    }
public:
    struct Identity {
        InstanceHandle handle{};
        int object=0,x=0,y=0,creationCode=-1;
        std::uint32_t source=0;
    };
    unsigned size()const{return population_;}
    bool reservationPending()const{return reserved_!=End;}
    bool contains(InstanceHandle h)const{
        DW_TERRAIN_READ(terrainIdentityRead);
        if(h.slot>=Capacity||!h.generation||!locator_[h.slot])return false;
        const auto& r=runs_[locator_[h.slot]-1];
        return h.generation==r.generation+h.slot-r.first;
    }
    bool occupied(unsigned slot)const{DW_TERRAIN_READ(terrainIdentityRead);return slot<Capacity&&locator_[slot]!=0;}
    InstanceHandle currentHandle(unsigned slot)const{
        if(!occupied(slot))return {};
        const auto& r=runs_[locator_[slot]-1];return {slot,r.generation+slot-r.first};
    }
    int object(unsigned slot)const{
        if(!occupied(slot))return -1;
        const auto& r=runs_[locator_[slot]-1];return r.objectAt(slot-r.first);
    }
    InstanceHandle handle(unsigned ordinal)const{
        DW_TERRAIN_READ(terrainIdentityRead);
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
        out={h,r.objectAt(offset),r.xAt(offset),r.y,r.creationCode,r.sourceId()};return true;
    }
    bool publication(InstanceHandle h,Publication& out)const{
        DW_TERRAIN_READ(terrainPublicationRead);
        if(!contains(h))return false;
        const auto& r=runs_[locator_[h.slot]-1];
        out=static_cast<const Publication&>(r).shifted(h.slot-r.first);return true;
    }
    // Exclusive constructor writer. Borrow only until the next structural
    // operation; a shared descriptor must keep the split-aware mutation path.
    Publication* singletonPublication(InstanceHandle h){
        if(reserved_!=End||!contains(h))return nullptr;
        auto& r=runs_[locator_[h.slot]-1];
        return r.count==1?static_cast<Publication*>(&r):nullptr;
    }
    bool changePublication(InstanceHandle h,const Publication& value){
        if(reserved_!=End||!contains(h))return false;
        unsigned n=locator_[h.slot]-1;
        const unsigned offset=h.slot-runs_[n].first;
        if(static_cast<const Publication&>(runs_[n]).shifted(offset).same(value))return true;
        if(offset)n=split(n,offset);
        if(runs_[n].count>1)split(n,1);
        static_cast<Publication&>(runs_[n])=value;return true;
    }
    template<class Part> bool publicationPart(InstanceHandle h,Part Publication::* member,Part& out)const{
        if(!contains(h))return false;
        const auto& r=runs_[locator_[h.slot]-1];
        out=(static_cast<const Publication&>(r).*member).shifted(h.slot-r.first);return true;
    }
    template<class Part> bool changePublicationPart(InstanceHandle h,Part Publication::* member,const Part& value){
        if(reserved_!=End||!contains(h))return false;
        unsigned n=locator_[h.slot]-1;const unsigned offset=h.slot-runs_[n].first;
        if((static_cast<const Publication&>(runs_[n]).*member).shifted(offset).same(value))return true;
        if(offset)n=split(n,offset);
        if(runs_[n].count>1)split(n,1);
        static_cast<Publication&>(runs_[n]).*member=value;return true;
    }
    bool append(InstanceHandle h,int object,int x,int y,std::uint32_t source=0,int creationCode=-1,
                const Publication& publication=initialPublication_,bool inheritPublication=false,bool* inherited=nullptr){
        if(inherited)*inherited=false;
        if(reserved_!=End||h.slot>=Capacity||!h.generation||occupied(h.slot)||population_==Capacity||
            (object!=60&&object!=93&&object!=99))return false;
        // Only generated, adjacent horizontal cells share a descriptor.
        // Authored source identities and creation scripts remain singleton.
        if(tail_!=End&&!source&&creationCode==-1){auto& r=runs_[tail_];
            if(r.object!=-2&&!r.source&&r.creationCode==-1&&r.object==object&&r.y==y&&
                unsigned(r.first)+r.count==h.slot&&r.generation+r.count==h.generation){
                // Avoid subtraction overflow, including unusual caller coordinates.
                const int last=r.x+int(r.count-1)*r.step;
                const bool right=last<=0x7fffffff-16&&x==last+16;
                const bool left=last>=(-2147483647-1)+16&&x==last-16;
                const int step=right?16:left?-16:0;
                if(step&&(r.count==1||r.step==step)&&
                    (inheritPublication||static_cast<const Publication&>(r).shifted(r.count).same(publication))){
                    if(inherited)*inherited=inheritPublication;
                    r.step=step;++r.count;++r.live;locator_[h.slot]=Slot(tail_+1);++population_;return true;
                }
            }
        }
        const unsigned n=allocate();
        // Initialize only an actual new descriptor. Do not construct and copy
        // an actor-sized run temporary for each inherited terrain cell.
        auto& r=runs_[n];static_cast<Publication&>(r)=publication;
        r.object=object;r.x=x;r.y=y;r.step=0;r.creationCode=creationCode;
        r.generation=h.generation;r.source=source;r.first=Slot(h.slot);
        r.count=r.live=1;r.next=End;r.previous=Slot(tail_);
        if(tail_==End)head_=n;else runs_[tail_].next=Slot(n);
        tail_=n;locator_[h.slot]=Slot(n+1);++population_;return true;
    }
    bool appendInherited(InstanceHandle h,int object,int x,int y,std::uint32_t source,int creationCode,bool* inherited){
        return append(h,object,x,y,source,creationCode,initialPublication_,true,inherited);
    }
    // The caller guarantees immutable bytes until every cell is retired or
    // this owner is cleared. This is a source-ordered identity span, not a
    // geometry index: exceptional publication changes still split it normally.
    bool appendRetainedCells(InstanceHandle first,const unsigned char* cells,int y,unsigned count,
                             const Publication& publication){
        if(reserved_!=End)return false;
        if(!count)return true;
        if(!cells||first.slot>=Capacity||count>Capacity-first.slot||count>Capacity-population_||
           !first.generation||count-1>0xffffffffu-first.generation)return false;
        for(unsigned i=0;i<count;++i)if(occupied(first.slot+i))return false;
        const unsigned n=allocate();auto& r=runs_[n];
        static_cast<Publication&>(r)=publication;
        r.object=-2;r.cells=cells;r.x=r.step=0;r.y=y;r.creationCode=-1;
        r.generation=first.generation;r.first=Slot(first.slot);r.count=r.live=Slot(count);
        r.next=End;r.previous=Slot(tail_);
        if(tail_==End)head_=n;else runs_[tail_].next=Slot(n);
        tail_=n;
        for(unsigned i=0;i<count;++i)locator_[first.slot+i]=Slot(n+1);
        population_+=count;return true;
    }
    // Reserve the authoritative descriptor before a callback-free birth
    // transaction. It is not live: no locator, population or traversal link
    // is published until commit. All structural mutations are excluded while
    // the writer is borrowed. The ordinary append above remains the oracle.
    Publication* reserveRetainedCells(InstanceHandle first,const unsigned char* cells,int y,unsigned count){
        if(reserved_!=End||!count||!cells||first.slot>=Capacity||count>Capacity-first.slot||
           count>Capacity-population_||!first.generation||count-1>0xffffffffu-first.generation)return nullptr;
        for(unsigned i=0;i<count;++i)if(occupied(first.slot+i))return nullptr;
        const unsigned n=allocate();auto& r=runs_[n];
        static_cast<Publication&>(r)=initialPublication_;
        r.object=-2;r.cells=cells;r.x=r.step=0;r.y=y;r.creationCode=-1;
        r.generation=first.generation;r.first=Slot(first.slot);r.count=Slot(count);r.live=0;
        r.next=End;r.previous=End;reserved_=n;return &r;
    }
    bool commitRetainedCells(unsigned completed){
        if(reserved_==End||completed>runs_[reserved_].count)return false;
        const unsigned n=reserved_;auto& r=runs_[n];reserved_=End;
        if(!completed){r.next=Slot(free_);free_=n;return true;}
        r.count=r.live=Slot(completed);r.previous=Slot(tail_);
        if(tail_==End)head_=n;else runs_[tail_].next=Slot(n);
        tail_=n;
        for(unsigned i=0;i<completed;++i)locator_[r.first+i]=Slot(n+1);
        population_+=completed;return true;
    }
    // An admitted callback-free constructor publishes one complete horizontal
    // span. Keep the scalar append as its oracle; don't rebuild a Publication
    // and revalidate the run geometry for every cell.
    bool appendSpan(InstanceHandle first,int object,int x,int y,int step,unsigned count,
                    const Publication& publication){
        if(reserved_!=End||!count||count>Capacity||first.slot>=Capacity||count>Capacity-first.slot||
           count>Capacity-population_||!first.generation||count-1>0xffffffffu-first.generation||
           (object!=60&&object!=93)||(step!=16&&step!=-16))return false;
        const int distance=int(count-1)*16;
        if((step>0&&x>0x7fffffff-distance)||(step<0&&x<(-2147483647-1)+distance))return false;
        for(unsigned i=0;i<count;++i)if(occupied(first.slot+i))return false;
        // Use the scalar first append to preserve descriptor merging exactly,
        // including singleton tails and exceptional publication boundaries.
        if(!append(first,object,x,y,0,-1,publication))return false;
        auto& r=runs_[tail_];
        if(count>1&&r.count>1&&r.step!=step){
            // The first cell joined an opposite-direction tail. The second
            // begins a new run, exactly as scalar append would.
            return appendSpan({first.slot+1,first.generation+1},object,x+step,y,step,count-1,
                              publication.shifted(1));
        }
        if(count>1)r.step=step;
        r.count=Slot(unsigned(r.count)+count-1);r.live=Slot(unsigned(r.live)+count-1);
        for(unsigned i=1;i<count;++i)locator_[first.slot+i]=Slot(tail_+1);
        population_+=count-1;return true;
    }
    bool retire(InstanceHandle h){
        if(reserved_!=End||!contains(h))return false;
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
                DW_TERRAIN_READ(terrainIdentityRead);
                if(locator_[slot]==n+1&&!visit(Identity{{slot,r.generation+i},r.objectAt(i),r.object==-2?r.xAt(i):x,r.y,r.creationCode,r.sourceId()}))return false;
                if(i+1<r.count)x+=r.step;
            }
        }
        return true;
    }
public:
    class PublicationRun {
        const TerrainOwnershipRuns& owner_;unsigned index_;
    public:
        PublicationRun(const TerrainOwnershipRuns& owner,unsigned index):owner_(owner),index_(index){}
        const Publication& publication()const{DW_TERRAIN_READ(terrainPublicationRead);return owner_.runs_[index_];}
        template<bool Reverse=false,class Visit> bool visitSlots(Visit visit)const{
            const auto& r=owner_.runs_[index_];
            for(unsigned i=0;i<r.count;++i){const unsigned offset=Reverse?r.count-1-i:i,slot=r.first+offset;
                if(owner_.locator_[slot]==index_+1&&!visit(slot,offset))return false;
            }return true;
        }
    };
    template<class Visit> bool visitPublicationRun(unsigned slot,Visit visit)const{
        return occupied(slot)&&visit(PublicationRun(*this,locator_[slot]-1));
    }
    template<class Visit> bool visitPublicationRuns(Visit visit)const{
        // Borrowed traversal; publication mutations can split runs and must
        // occur outside this walk, using stable slot/handle projections.
        for(unsigned n=head_;n!=End;n=runs_[n].next)if(!visit(PublicationRun(*this,n)))return false;
        return true;
    }
    template<class Visit> bool visit(Visit visit)const{return visitSelected<false>(0,0,visit);}
    template<class Visit> bool visitRows(int first,int end,Visit visit)const{
        return first>=end||visitSelected<true>(first,end,visit);
    }
    unsigned runCount()const{unsigned count=0;for(unsigned n=head_;n!=End;n=runs_[n].next)++count;return count;}
};
}
#endif

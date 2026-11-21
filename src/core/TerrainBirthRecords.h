#ifndef DOWNWELL_CORE_TERRAIN_BIRTH_RECORDS_H
#define DOWNWELL_CORE_TERRAIN_BIRTH_RECORDS_H
#include "InstanceHandle.h"
#include "PlacementNew.h"
#include "TerrainPublicationClock.h"
#include <cstdint>
namespace downwell {
// Source facts, not an eagerly constructed runner instance. All fields are
// supplied at the admitted birth boundary. Plain fields deliberately avoid
// constructing Capacity copies of default runner state at scene entry.
struct TerrainBirthFacts {
    const unsigned char* cells;
    std::uint32_t slot,generation,registration;
    int y,tile,layerId;
    unsigned layer;
    std::uint32_t layerLow,layerHigh,roomLow,roomHigh,roomDepth;
    unsigned char flags[2];
};
// Generated row segments only; authored and unsupported scalar construction
// retain their source path. A changed cell has exactly one mutable owner.
// Alive bits belong to the segment, so recycling a mutable slot cannot make
// an older retired identity live again. A derived slot locator indexes the
// record; it owns no duplicate identity or mutable runner state.
template<unsigned Capacity,class Mutable> class TerrainBirthRecords {
    static_assert(Capacity&&Capacity<65535,"Source record capacity");
    struct Record {
        TerrainBirthFacts birth;
        std::uint32_t ordinal,alive[2];
        unsigned count,live;
        std::uint16_t next,previous;
    };
    // Scalars first: 68000 displacements reach only 32 KiB past the base.
    unsigned used_,population_,highWater_,head_,tail_,free_,pending_=0,pendingCount_=0;
    std::uint32_t published_,lastGeneration_;
    Record* pendingRecord_=nullptr;
    Record records_[Capacity];
    struct Change {alignas(Mutable) unsigned char bytes[sizeof(Mutable)];};
    Change changes_[Capacity];
    std::uint32_t changedGeneration_[Capacity];
    std::uint16_t locator_[Capacity]{};
    static std::uint32_t mask(unsigned n){return n>=32?0xffffffffu:n?(1u<<n)-1:0;}
    static bool alive(const Record& r,unsigned offset){return (r.alive[offset>>5]>>(offset&31))&1u;}
    unsigned findReference(InstanceHandle h)const{
        if(!h.generation||h.slot>=Capacity)return used_;
        for(unsigned i=0;i<used_;++i){const auto& r=records_[i];
            if(!r.live||h.slot<r.birth.slot||h.slot-r.birth.slot>=r.count)continue;
            const unsigned offset=h.slot-r.birth.slot;
            if(r.birth.generation+offset==h.generation&&alive(r,offset))return i;
        }
        return used_;
    }
    unsigned find(InstanceHandle h)const{
        if(!h.generation||h.slot>=Capacity||!locator_[h.slot])return used_;
        const unsigned n=locator_[h.slot]-1;const auto& r=records_[n];
        return r.birth.generation+h.slot-r.birth.slot==h.generation?n:used_;
    }
    Mutable* changed(unsigned slot){return reinterpret_cast<Mutable*>(changes_[slot].bytes);}
    const Mutable* changed(unsigned slot)const{return reinterpret_cast<const Mutable*>(changes_[slot].bytes);}
public:
    // Intentional user-provided constructor: untouched record/state storage
    // is not value-initialized. Only the sparse-change generation map is live.
    TerrainBirthRecords():used_(0),population_(0),highWater_(0),head_(Capacity),tail_(Capacity),free_(Capacity),published_(0),lastGeneration_(0){
        for(auto& generation:changedGeneration_)generation=0;
    }
    ~TerrainBirthRecords(){clear();}
    TerrainBirthRecords(const TerrainBirthRecords&)=delete;
    TerrainBirthRecords& operator=(const TerrainBirthRecords&)=delete;
    unsigned size()const{return population_;}
    unsigned sourcePublicationCount()const{return published_;}
    bool canAppend(InstanceHandle first,unsigned count)const{
        return canAppendImpl<false>(first,count);
    }
    bool canAppendReference(InstanceHandle first,unsigned count)const{
        return canAppendImpl<true>(first,count);
    }
private:
    template<bool ScanRecords> bool canAppendImpl(InstanceHandle first,unsigned count)const{
        if(!count||count>64||first.generation<=lastGeneration_||first.slot>=Capacity||count>Capacity-first.slot||
           count>Capacity-population_||count-1>0xffffffffu-first.generation||
           count>0xffffffffu-published_)return false;
        if(first.slot>=highWater_)return true;
        const unsigned end=first.slot+count;
        if constexpr(!ScanRecords){
            // Commit/retire/clear already maintain this live-slot locator.
            // Historical records and retired holes need not be revisited to
            // admit a new segment. Pending reservations remain unpublished.
            for(unsigned slot=first.slot;slot<end;++slot)if(locator_[slot])return false;
            return true;
        }
        for(unsigned i=0;i<used_;++i){const auto& r=records_[i];
            if(!r.live||end<=r.birth.slot||first.slot>=r.birth.slot+r.count)continue;
            const unsigned begin=first.slot>r.birth.slot?first.slot-r.birth.slot:0;
            const unsigned limit=end-r.birth.slot<r.count?end-r.birth.slot:r.count;
            for(unsigned n=begin;n<limit;++n)if(alive(r,n))return false;
        }
        return true;
    }
public:
    bool append(const TerrainBirthFacts& birth,unsigned count){
        auto* facts=reserve({birth.slot,birth.generation},count);
        if(!facts)return false;
        *facts=birth;return commit();
    }
    // Admit one segment and lend its unpublished record, so the birth owner
    // writes the facts once in place. Nothing is observable until commit();
    // abandon() (or another reserve) discards it without side effects.
    TerrainBirthFacts* reserve(InstanceHandle first,unsigned count){
        if(!canAppend(first,count))return nullptr;
        pending_=free_!=Capacity?free_:used_;pendingCount_=count;
        pendingRecord_=&records_[pending_];
        auto& birth=pendingRecord_->birth;
        birth.slot=first.slot;birth.generation=first.generation;
        return &birth;
    }
    void abandon(){pendingCount_=0;}
    // The reserved, unpublished record; null when none is pending.
    TerrainBirthFacts* pendingFacts(){return pendingCount_?&pendingRecord_->birth:nullptr;}
    bool commit(){
        const unsigned count=pendingCount_,n=pending_;pendingCount_=0;
        if(!count)return false;
        auto& r=*pendingRecord_;const auto& birth=r.birth;
        if(!birth.cells||!birth.registration||count-1>0xffffffffu-birth.registration)return false;
        if(n==free_)free_=r.next;else ++used_;
        r.ordinal=published_+1;
        r.count=r.live=count;r.alive[0]=mask(count);r.alive[1]=mask(count>32?count-32:0);
        r.next=Capacity;r.previous=tail_;
        if(tail_!=Capacity)records_[tail_].next=n;else head_=n;
        tail_=n;lastGeneration_=birth.generation+count-1;
        for(unsigned i=0;i<count;++i)locator_[birth.slot+i]=std::uint16_t(n+1);
        published_+=count;population_+=count;
        if(birth.slot+count>highWater_)highWater_=birth.slot+count;
        return true;
    }
    struct Read {
        const TerrainBirthFacts* birth=nullptr;
        unsigned offset=0,ordinal=0;
        int object()const{return birth->cells[2*offset]&128?93:60;}
        int x()const{return (int(birth->cells[2*offset]&127)-32)*16;}
        RunnerRegistrationOrder layerSerial()const{
            return TerrainPublicationClock::serial({birth->layerLow,birth->layerHigh},ordinal);
        }
        RunnerRegistrationOrder roomSerial()const{
            return TerrainPublicationClock::serial({birth->roomLow,birth->roomHigh},ordinal);
        }
        unsigned registration()const{return birth->registration+offset;}
    };
    template<bool Rows> class SelectedCursor {
        const TerrainBirthRecords& owner_;
        unsigned record_,offset_=0;
        int first_,end_;
    public:
        explicit SelectedCursor(const TerrainBirthRecords& owner,int first=0,int end=0):
            owner_(owner),record_(owner.head_),first_(first),end_(end){}
        bool take(InstanceHandle& handle,Read& out){
            while(record_!=Capacity){const auto& r=owner_.records_[record_];
                if constexpr(Rows)if((r.birth.y<first_&&unsigned(first_)-unsigned(r.birth.y)>=16u)||
                    (r.birth.y>=end_&&unsigned(r.birth.y)-unsigned(end_)>=16u)){
                    record_=r.next;offset_=0;continue;
                }
                while(offset_<r.count){const unsigned i=offset_++;
                    if(!alive(r,i))continue;
                    handle={r.birth.slot+i,r.birth.generation+i};out={&r.birth,i,r.ordinal+i};return true;
                }
                record_=r.next;offset_=0;
            }
            return false;
        }
    };
    using Cursor=SelectedCursor<false>;
    unsigned runCount()const{
        unsigned count=0;for(unsigned n=head_;n!=Capacity;n=records_[n].next)++count;return count;
    }
    bool read(InstanceHandle h,Read& out)const{
        const unsigned n=find(h);if(n==used_)return false;
        const auto& r=records_[n];const unsigned offset=h.slot-r.birth.slot;
        out={&r.birth,offset,r.ordinal+offset};return true;
    }
    InstanceHandle currentHandleReference(unsigned slot)const{
        for(unsigned i=0;i<used_;++i){const auto& r=records_[i];
            if(!r.live||slot<r.birth.slot||slot-r.birth.slot>=r.count)continue;
            const unsigned offset=slot-r.birth.slot;
            if(alive(r,offset))return {slot,r.birth.generation+offset};
        }
        return {};
    }
    InstanceHandle currentHandle(unsigned slot)const{
        if(slot>=Capacity||!locator_[slot])return {};
        const auto& r=records_[locator_[slot]-1];
        return {slot,r.birth.generation+slot-r.birth.slot};
    }
    bool lookupMatchesReference(InstanceHandle h)const{
        const auto a=currentHandle(h.slot),b=currentHandleReference(h.slot);
        return a.slot==b.slot&&a.generation==b.generation&&find(h)==findReference(h);
    }
    const Mutable* mutation(InstanceHandle h)const{
        return find(h)!=used_&&changedGeneration_[h.slot]==h.generation?changed(h.slot):nullptr;
    }
    // Unordered, read-only access to the existing exceptional-state owner.
    // Retirement destroys the cell and clears its generation before reuse;
    // untouched births have no mutable state to visit. Callbacks must not
    // publish or retire cells while this traversal is in progress.
    template<class Visit> bool visitMutations(Visit visit)const{
        for(unsigned slot=0;slot<highWater_;++slot)
            if(changedGeneration_[slot]&&!visit(slot,*changed(slot)))return false;
        return true;
    }
    template<class Initial> Mutable* materialize(InstanceHandle h,Initial initial){
        // initial is a pure projection, not a game callback or a second birth.
        Read source;if(!read(h,source))return nullptr;
        if(changedGeneration_[h.slot]!=h.generation){
            if(changedGeneration_[h.slot])changed(h.slot)->~Mutable();
            new(changes_[h.slot].bytes) Mutable(initial(source));
            changedGeneration_[h.slot]=h.generation;
        }
        return changed(h.slot);
    }
    bool retire(InstanceHandle h){
        const unsigned n=find(h);if(n==used_)return false;
        auto& r=records_[n];const unsigned offset=h.slot-r.birth.slot;
        locator_[h.slot]=0;
        r.alive[offset>>5]&=~(1u<<(offset&31));--r.live;--population_;
        if(changedGeneration_[h.slot]==h.generation){changed(h.slot)->~Mutable();changedGeneration_[h.slot]=0;}
        if(!r.live){
            if(r.previous==Capacity)head_=r.next;else records_[r.previous].next=r.next;
            if(r.next==Capacity)tail_=r.previous;else records_[r.next].previous=r.previous;
            r.next=free_;free_=n;
        }
        return true;
    }
    // Read-only source chronology. Cell state may be materialized while
    // observing it, but structural births/retirements need their own cursor.
    template<class Visit> bool visit(Visit visit)const{
        for(unsigned n=head_;n!=Capacity;n=records_[n].next){const auto& r=records_[n];
            for(unsigned i=0;i<r.count;++i)if(alive(r,i))
                if(!visit(InstanceHandle{r.birth.slot+i,r.birth.generation+i},Read{&r.birth,i,r.ordinal+i}))return false;
        }
        return true;
    }
    void clear(){
        for(auto& locator:locator_)locator=0;
        for(unsigned n=0;n<Capacity;++n)if(changedGeneration_[n]){
            changed(n)->~Mutable();changedGeneration_[n]=0;
        }
        used_=population_=highWater_=0;
        head_=tail_=free_=Capacity;
        // The bound consumers' clocks retain their history across room clear.
    }
};
}
#endif

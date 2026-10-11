#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_JOURNAL_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_JOURNAL_H
#include "RunnerSpatialIndex.h"
#include "PreparedTerrainBirthBounds.h"
namespace downwell {
template<bool Prepared> struct RunnerSpatialGeometrySource {};
template<> struct RunnerSpatialGeometrySource<true> {
    const PreparedTerrainBirthBounds* geometry_=nullptr;
};
// Derived-tree scheduling only. The caller still publishes instance fields,
// flags, dirty queues and lifetime changes synchronously. Never collapse equal
// updates or reconstruct from current positions: tree order depends on history.
// All reads catch up. service() permits bounded work between publication bursts.
template<unsigned Nodes,unsigned Slots,unsigned Pending=512,bool PreparedGeometry=false,class Tree=RunnerSpatialIndex<Nodes>>
class RunnerSpatialJournal:private RunnerSpatialGeometrySource<PreparedGeometry> {
    static_assert(Pending && !(Pending&(Pending-1)),"Journal capacity must be a power of two");
    static_assert(!PreparedGeometry||Slots<0x80000000u,"Prepared journal identity tag");
    using Math=RunnerSpatialSplit;
    static constexpr unsigned depthLimit=[](){unsigned n=Nodes,bits=0;do{++bits;n>>=1;}while(n);return bits;}();
    // A valid tree has at most N leaves and N-1 internal nodes for N live
    // entries. Removal retains at most one detached node per level while
    // reinserting. Reserve a second depth allowance for transient root splits.
    // This intentionally loose bound keeps deferred operations away from pool
    // exhaustion; near the bound, run the original mutation synchronously.
    static constexpr unsigned reserve=2*depthLimit+1;
    static constexpr unsigned safePopulation=Nodes>reserve?(Nodes-reserve)/2:0;
public:
    using Box=typename Math::Box;
    template<class Provider> bool bindTree(Provider provider){return tree_.bind(provider);}
    // Scene dependency declaration, only before any publication. This is not
    // lazy query reconstruction: omitted history can never answer a query.
    // A later consumer must select the full index at scene admission.
    bool declareNoQueries(){
        if(!valid()||count_||population_||!deferred_)return false;
        queries_=false;return true;
    }
    bool queriesEnabled()const{return queries_;}
    // Read-only admission for a constructor with no intervening observation.
    // Do not cross pressure/service or eager-fallback boundaries in this path.
    bool admitsFreshGeometrySpan(unsigned first,unsigned count,const Math::PixelBox& envelope)const{
        if constexpr(!PreparedGeometry)return false;
        if(!valid()||!deferred_||first>=Slots||count>Slots-first||
           (queries_&&(count>Pending-count_||population_>safePopulation||count>safePopulation-population_)))return false;
        for(unsigned i=first;i<first+count;++i)if(contains(i))return false;
        if(!queries_)return true;
        Math::PixelBox combined;return admitPixels(envelope,combined);
    }
    bool bindGeometry(const PreparedTerrainBirthBounds& geometry){
        if constexpr(PreparedGeometry){
            if(this->geometry_||count_||population_||!valid())return false;
            this->geometry_=&geometry;return true;
        }else return false;
    }
    // A synchronous constructor may publish a whole admitted segment without
    // re-admitting its envelope for every cell. Retain one ordered operation
    // per cell; admission excludes pressure/service and eager boundaries.
    // Producer must not observe/mutate this journal. Its keys must lie within
    // the supplied normalized envelope, just as insertGeometry trusts pixels.
    // A failed producer leaves exactly the completed prefix, not a rollback.
    // Ineligible segments return success with admitted=false and invoke no
    // producer, allowing the caller to retain its scalar publication path.
    template<class Producer> bool insertGeometrySpan(unsigned first,unsigned count,
        const Math::PixelBox& envelope,unsigned& completed,bool& admitted,Producer produce){
        static_assert(PreparedGeometry,"Prepared geometry policy required");
        completed=0;admitted=false;
        Math::PixelBox combined;
        if(!this->geometry_||!valid())return false;
        if(!admitsFreshGeometrySpan(first,count,envelope))return true;
        if(queries_&&!admitPixels(envelope,combined))return false;
        admitted=true;
        const unsigned start=count_;
        for(unsigned i=0;i<count;++i){
            PreparedTerrainBirthBounds::Key key;
            if(!produce(i,key)||!this->geometry_->validKey(key)){
                // Normally only the final envelope is needed. On failure,
                // reconstruct the admitted prefix's historical union exactly.
                for(unsigned j=0;queries_&&j<completed;++j){
                    const auto& op=operations_[(head_+start+j)&(Pending-1)];
                    Math::PixelBox pixels;Box box;
                    if(!this->geometry_->bounds({op.box.left.bits},box,true)||
                       !Math::decodePixels(box,pixels)||!admitPixels(pixels,combined))return false;
                    bounds_=combined;haveBounds_=true;
                }
                return false;
            }
            const unsigned instance=first+i;
            if(queries_){
            auto& operation=operations_[(head_+count_)&(Pending-1)];
            operation.box.left.bits=key.value;
            operation.instance=instance|0x80000000u;operation.insert=true;++count_;
            DW_PROFILE_COUNT(counterSpatialJournalQueued);
            }
            present_[instance>>5]|=1u<<(instance&31);++population_;++completed;
        }
        if(queries_&&count){bounds_=combined;haveBounds_=true;}
        return true;
    }
    // Immutable resource key, not a live instance lookup. Pixel endpoints
    // obey the same trusted/normalized contract as insertPrepared. Preserve
    // one logical operation per publication, including pressure/service timing.
    bool insertGeometry(PreparedTerrainBirthBounds::Key key,std::uint32_t instance,
                        const Math::PixelBox& pixels){
        static_assert(PreparedGeometry,"Prepared geometry policy required");
        if(!valid()||!this->geometry_||!this->geometry_->validKey(key))return false;
        if(!queries_)return insertIdentity(instance);
        Math::PixelBox next;
        if(!deferred_||instance>=Slots||contains(instance)||population_>=safePopulation||
            !admitPixels(pixels,next)){
            if(!eager())return false;
            Box box;
            return this->geometry_->bounds(key,box,true)&&tree_.insert(box,instance);
        }
        if(!makeRoom())return false;
        auto& operation=operations_[(head_+count_)&(Pending-1)];
        operation.box.left.bits=key.value;
        operation.instance=instance|0x80000000u;operation.insert=true;++count_;
        DW_PROFILE_COUNT(counterSpatialJournalQueued);
        present_[instance>>5]|=1u<<(instance&31);++population_;
        bounds_=next;haveBounds_=true;return true;
    }
    bool valid()const{return good_&&tree_.valid();}
    bool deferred()const{return deferred_;}
    unsigned pending()const{return count_;}
    void clear(){
        tree_.clear();head_=count_=population_=0;good_=deferred_=true;haveBounds_=false;
        for(auto& word:present_)word=0;
    }
    unsigned nodeCount()const{if(!queries_){good_=false;return 0;}catchUp();return tree_.nodeCount();}
    int height()const{if(!queries_){good_=false;return 0;}catchUp();return tree_.height();}
    // Borrow during admission, then take the journal's own snapshot. Neither
    // admission nor catch-up mutates caller storage; no temporary value copy
    // is needed at the API boundary (notably on the 68000 stack).
    bool insert(const Box& box,std::uint32_t instance){
        return insertPrepared(box,instance,nullptr);
    }
    // Optional decoded geometry from the same immutable preparation that
    // supplies box. Caller must supply the exact integer endpoints (no signed
    // zero/fractional values); no cached tree membership or admission decision
    // is accepted here. Population, historical union and pressure stay live.
    bool insertPrepared(const Box& box,std::uint32_t instance,const Math::PixelBox* pixels){
        if(!valid())return false;
        if(!queries_)return Math::valid(box)&&insertIdentity(instance);
        Math::PixelBox next;
        if(!deferred_ || instance>=Slots || contains(instance) ||
            population_>=safePopulation || !(pixels?admitPixels(*pixels,next):admit(box,next))){
            if(!eager())return false;
            return tree_.insert(box,instance);
        }
        if(!makeRoom())return false;
        operations_[(head_+count_)&(Pending-1)]={box,instance,true};++count_;
        DW_PROFILE_COUNT(counterSpatialJournalQueued);
        present_[instance>>5]|=1u<<(instance&31);++population_;
        bounds_=next;haveBounds_=true;return true;
    }
    bool remove(const Box& oldBounds,std::uint32_t instance,bool retry,bool& removed){
        if(!valid())return false;
        if(!queries_){
            if(instance>=Slots||!retry||!Math::valid(oldBounds))return good_=false;
            removed=contains(instance);
            if(removed){present_[instance>>5]&=~(1u<<(instance&31));--population_;}
            return true;
        }
        // Full-root retry with a unique identity has an unambiguous result.
        // Unbounded IDs, duplicates and no-retry searches use the eager tree.
        if(!deferred_ || instance>=Slots || !retry || !Math::valid(oldBounds)){
            if(!eager())return false;
            return tree_.remove(oldBounds,instance,retry,removed);
        }
        if(!contains(instance)){removed=false;return true;}
        if(!makeRoom())return false;
        operations_[(head_+count_)&(Pending-1)]={oldBounds,instance,false};++count_;
        DW_PROFILE_COUNT(counterSpatialJournalQueued);
        present_[instance>>5]&=~(1u<<(instance&31));--population_;
        removed=true;return true;
    }
    template<class Visit> bool query(Box box,Visit visit)const{
        if(!queries_)return good_=false;
        return catchUp()&&tree_.query(box,visit);
    }
    template<class Visit> bool queryLine(const RunnerLineArguments& args,Visit visit)const{
        if(!queries_)return good_=false;
        return catchUp()&&tree_.queryLine(args,visit);
    }
    bool service(unsigned budget)const{
        if(!valid())return false;
        while(count_&&budget--){
            const auto& operation=operations_[head_];
            bool removed=false;
            bool ok;
            if constexpr(PreparedGeometry){
                if(operation.instance&0x80000000u){
                    Box box;
                    ok=this->geometry_&&this->geometry_->bounds({operation.box.left.bits},box,true)&&
                        tree_.insert(box,operation.instance&0x7fffffffu);
                }else ok=operation.insert?tree_.insert(operation.box,operation.instance):
                    tree_.remove(operation.box,operation.instance,true,removed)&&removed;
            }else ok=operation.insert?tree_.insert(operation.box,operation.instance):
                    tree_.remove(operation.box,operation.instance,true,removed)&&removed;
            if(!ok){good_=false;return false;}
            DW_PROFILE_COUNT(counterSpatialJournalServiced);
            head_=(head_+1)&(Pending-1);--count_;
        }
        return true;
    }
private:
    struct Operation{Box box;std::uint32_t instance;bool insert;};
    mutable Tree tree_;
    Operation operations_[Pending]{};
    std::uint32_t present_[(Slots+31)/32? (Slots+31)/32:1]{};
    mutable unsigned head_=0,count_=0;
    unsigned population_=0;
    mutable bool good_=true;
    bool deferred_=true,haveBounds_=false,queries_=true;
    Math::PixelBox bounds_{};
    bool contains(unsigned instance)const{return (present_[instance>>5]>>(instance&31))&1u;}
    bool insertIdentity(unsigned instance){
        if(instance>=Slots||contains(instance))return good_=false;
        present_[instance>>5]|=1u<<(instance&31);++population_;return true;
    }
    bool catchUp()const{return service(count_);}
    bool makeRoom(){
        if(count_<Pending)return true;
        DW_PROFILE_COUNT(counterSpatialJournalPressure);
        return catchUp();
    }
    bool eager(){
        if(!catchUp())return false;
        if(deferred_)DW_PROFILE_COUNT(counterSpatialJournalFallback);
        deferred_=false;return true;
    }
    bool admit(const Box& box,Math::PixelBox& combined)const{
        // These conditions also exclude numeric failures during later replay:
        // every historical union has exact finite binary32 area arithmetic.
        if(box.left.bits==0x80000000u||box.top.bits==0x80000000u||
           box.right.bits==0x80000000u||box.bottom.bits==0x80000000u||
           !Math::decodePixels(box,combined))return false;
        return admitPixels(combined,combined);
    }
    bool admitPixels(const Math::PixelBox& pixels,Math::PixelBox& combined)const{
        combined=pixels;
        if(combined.left>combined.right||combined.top>combined.bottom)return false;
        if(haveBounds_){
            if(bounds_.left<combined.left)combined.left=bounds_.left;
            if(bounds_.top<combined.top)combined.top=bounds_.top;
            if(bounds_.right>combined.right)combined.right=bounds_.right;
            if(bounds_.bottom>combined.bottom)combined.bottom=bounds_.bottom;
        }
        const unsigned width=unsigned(combined.right-combined.left),height=unsigned(combined.bottom-combined.top);
        return width<=65535&&height<=65535&&
            std::uint32_t(std::uint16_t(width))*std::uint32_t(std::uint16_t(height))<=0x1000000u;
    }
};
}
#endif

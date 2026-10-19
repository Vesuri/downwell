#ifndef DOWNWELL_CORE_NATIVE_TERRAIN_SOURCE_RUNS_H
#define DOWNWELL_CORE_NATIVE_TERRAIN_SOURCE_RUNS_H
#include "TerrainOwnershipRuns.h"
#include "TerrainBirthRecords.h"
#include "PackedWallState.h"
#include "RunnerLogicalLayerOrder.h"
#include "RunnerRoomOrderState.h"
namespace downwell {
template<class Publication> struct NativeTerrainChangedCell {
    Publication publication;
    unsigned char flags=0;
    std::int16_t sprite=-2;
    PackedWallState wall;
    RunnerRoomOrderState room;
};
// The retained owner is for authored and unsupported constructors only. A
// prepared generated birth never enters it: its identity, initial Create
// result and runner publication all come from the same source segment.
template<unsigned Capacity,class Publication> class NativeTerrainSourceRuns:
    public TerrainOwnershipRuns<Capacity,Publication> {
    using Base=TerrainOwnershipRuns<Capacity,Publication>;
public:
    using Identity=typename Base::Identity;
    using Cell=NativeTerrainChangedCell<Publication>;
    using Sources=TerrainBirthRecords<Capacity,Cell>;
    using SourceRead=typename Sources::Read;
private:
    Sources sources_;
    static Publication initialPublication(const SourceRead& source){
        return {source.registration(),{{source.layerSerial(),false},
            source.birth->layerId,source.birth->layer,false}};
    }
    static Cell initialCell(const SourceRead& source){
        return {initialPublication(source),source.birth->flags[source.object()==93],
            static_cast<std::int16_t>(source.birth->tile),PackedWallState{},
            {source.roomSerial(),{source.birth->roomDepth},0,false}};
    }
    static Identity identity(InstanceHandle h,const SourceRead& source){
        return {h,source.object(),source.x(),source.birth->y,-1,0};
    }
public:
    Sources& sources(){return sources_;}
    const Sources& sources()const{return sources_;}
    unsigned sourcePublicationCount()const{return sources_.sourcePublicationCount();}
    bool sourceOwned(InstanceHandle h)const{SourceRead read;return sources_.read(h,read);}
    unsigned sourcePopulation()const{return sources_.size();}
    bool nextSourceRegistration(unsigned after,InstanceHandle& handle,int& object,unsigned& order,bool walls=false)const{
        bool found=false;
        sources_.visit([&](InstanceHandle h,const SourceRead& source){
            if(!walls&&source.object()!=93)return true;
            const auto* changed=sources_.mutation(h);
            if(changed&&changed->room.list!=0)return true;
            const unsigned stamp=changed?changed->publication.stamp:source.registration();
            if(stamp>after&&(!found||stamp<order)){found=true;handle=h;object=source.object();order=stamp;}
            return true;
        });return found;
    }
    bool sourceRoomState(unsigned slot,RunnerRoomOrderState& out)const{
        const auto h=sources_.currentHandle(slot);SourceRead source;
        if(!sources_.read(h,source))return false;
        if(const auto* changed=sources_.mutation(h))out=changed->room;
        else out={source.roomSerial(),{source.birth->roomDepth},0,false};
        return true;
    }
    bool setSourceRoomState(unsigned slot,const RunnerRoomOrderState& value){
        auto* cell=sources_.materialize(sources_.currentHandle(slot),initialCell);
        if(!cell)return false;cell->room=value;return true;
    }
    template<class Visit> bool visitSourceRoomStates(Visit visit)const{
        return sources_.visit([&](InstanceHandle h,const SourceRead& source){
            if(const auto* changed=sources_.mutation(h))return visit(h.slot,changed->room);
            return visit(h.slot,RunnerRoomOrderState{source.roomSerial(),{source.birth->roomDepth},0,false});
        });
    }
    template<class Visit> bool visitMarkedSourceRoomStates(Visit visit)const{
#ifdef DOWNWELL_REFERENCE_SOURCE_CLEANUP_SCAN
        return visitSourceRoomStates([&](unsigned slot,const RunnerRoomOrderState& room){
            return room.list<0||!room.marked||visit(slot,room);
        });
#else
        // Initial source room state is never marked. Only the changed cell
        // can own a cleanup mark; do not reconstruct every source birth.
        return sources_.visitMutations([&](unsigned slot,const Cell& cell){
            return cell.room.list<0||!cell.room.marked||visit(slot,cell.room);
        });
#endif
    }
    unsigned size()const{return Base::size()+sources_.size();}
    unsigned runCount()const{return Base::runCount()+sources_.runCount();}
    bool occupied(unsigned slot)const{return Base::occupied(slot)||sources_.currentHandle(slot).generation;}
    InstanceHandle currentHandle(unsigned slot)const{
        const auto scalar=Base::currentHandle(slot);return scalar.generation?scalar:sources_.currentHandle(slot);
    }
    bool contains(InstanceHandle h)const{
        if(Base::contains(h))return true;
        SourceRead source;return sources_.read(h,source);
    }
    int object(unsigned slot)const{
        const int scalar=Base::object(slot);if(scalar>=0)return scalar;
        SourceRead source;return sources_.read(sources_.currentHandle(slot),source)?source.object():-1;
    }
    bool read(InstanceHandle h,Identity& out)const{
        if(Base::read(h,out))return true;
        SourceRead source;if(!sources_.read(h,source))return false;
        out=identity(h,source);return true;
    }
    bool publication(InstanceHandle h,Publication& out)const{
        if(Base::publication(h,out))return true;
        SourceRead source;if(!sources_.read(h,source))return false;
        if(const auto* changed=sources_.mutation(h))out=changed->publication;
        else out=initialPublication(source);
        return true;
    }
    bool changePublication(InstanceHandle h,const Publication& value){
        if(Base::contains(h))return Base::changePublication(h,value);
        auto* cell=sources_.materialize(h,initialCell);if(!cell)return false;
        cell->publication=value;return true;
    }
    template<class T,T Cell::* Member> bool readField(unsigned slot,T& out)const{
        const auto h=sources_.currentHandle(slot);SourceRead source;
        if(!sources_.read(h,source))return false;
        if(const auto* changed=sources_.mutation(h))out=changed->*Member;
        else out=initialCell(source).*Member;
        return true;
    }
    bool readCollisionFields(InstanceHandle h,unsigned char& flags,std::int16_t& sprite,
        PackedWallState& wall)const{
        SourceRead source;if(!sources_.read(h,source))return false;
        if(const auto* changed=sources_.mutation(h)){
            flags=changed->flags;sprite=changed->sprite;wall=changed->wall;
        }else{
            flags=source.birth->flags[source.object()==93];
            sprite=static_cast<std::int16_t>(source.birth->tile);wall=PackedWallState{};
        }
        return true;
    }
    template<class T,T Cell::* Member> T* writeField(unsigned slot){
        const auto h=sources_.currentHandle(slot);if(!h.generation)return nullptr;
        auto* changed=sources_.materialize(h,initialCell);
        return changed?&(changed->*Member):nullptr;
    }
    template<bool Rows> class SelectedCursor {
        typename Base::template SelectedCursor<Rows> scalar_;
        typename Sources::template SelectedCursor<Rows> source_;
        Identity scalarNext_,sourceNext_;
        bool scalarLive_,sourceLive_;
        bool takeSource(){
            InstanceHandle h;SourceRead source;
            if(!source_.take(h,source))return false;
            sourceNext_=identity(h,source);return true;
        }
    public:
        explicit SelectedCursor(const NativeTerrainSourceRuns& owner,int first=0,int end=0):
            scalar_(owner,first,end),source_(owner.sources_,first,end),
            scalarLive_(scalar_.take(scalarNext_)),sourceLive_(takeSource()){}
        bool take(Identity& out){
            if(!scalarLive_&&!sourceLive_)return false;
            if(scalarLive_&&(!sourceLive_||scalarNext_.handle.generation<sourceNext_.handle.generation)){
                out=scalarNext_;scalarLive_=scalar_.take(scalarNext_);
            }else{out=sourceNext_;sourceLive_=takeSource();}
            return true;
        }
    };
    using Cursor=SelectedCursor<false>;
    InstanceHandle handle(unsigned ordinal)const{
        Cursor cursor(*this);Identity id;
        while(cursor.take(id))if(!ordinal--)return id.handle;
        return {};
    }
    template<class Visit> bool visit(Visit visit)const{
        Cursor cursor(*this);Identity id;while(cursor.take(id))if(!visit(id))return false;return true;
    }
    template<class Visit> bool visitRows(int first,int end,Visit visit)const{
#ifdef DOWNWELL_REFERENCE_SOURCE_ROW_SCAN
        return visitRowsReference(first,end,visit);
#else
        if(first>=end)return true;
        // Reject whole source/retained runs before expanding identities, then
        // merge the remaining cursors in the same immutable birth order.
        SelectedCursor<true> cursor(*this,first,end);Identity id;
        while(cursor.take(id))if(!visit(id))return false;
        return true;
#endif
    }
    template<class Visit> bool visitRowsReference(int first,int end,Visit visit)const{
        if(first>=end)return true;
        return this->visit([&](const Identity& id){
            return ((id.y<first&&unsigned(first)-unsigned(id.y)>=16u)||
                (id.y>=end&&unsigned(id.y)-unsigned(end)>=16u))||visit(id);
        });
    }
    bool retire(InstanceHandle h){return Base::contains(h)?Base::retire(h):sources_.retire(h);}
    void clear(){
        // Layer and room consumers keep their clocks across teardown; source
        // publication counts therefore survive along with lifetime counters.
        sources_.clear();Base::clear();
    }
};
// Existing lifetime methods use the same field interface. Reads of source
// records project their initial value; a write materializes one changed cell,
// shared by flags, wall state, sprite, registration and layer state.
template<unsigned Capacity,class Runs,class Publication,class T,T NativeTerrainChangedCell<Publication>::* Member,bool Source>
class NativeTerrainField {
    Runs& owner_;
    T scalar_[Capacity]{};
public:
    explicit NativeTerrainField(Runs& owner):owner_(owner){}
    const T& retained(unsigned slot)const{return scalar_[slot];}
    T operator[](unsigned slot)const{
        if constexpr(Source){T value;if(owner_.template readField<T,Member>(slot,value))return value;}
        return scalar_[slot];
    }
    T& operator[](unsigned slot){
        if constexpr(Source)if(auto* value=owner_.template writeField<T,Member>(slot))return *value;
        return scalar_[slot];
    }
};
template<unsigned Capacity,class Runs,class Publication,class T,T NativeTerrainChangedCell<Publication>::* Member>
class NativeTerrainField<Capacity,Runs,Publication,T,Member,false> {
    T scalar_[Capacity]{};
public:
    explicit NativeTerrainField(Runs&){}
    const T& retained(unsigned slot)const{return scalar_[slot];}
    const T& operator[](unsigned slot)const{return scalar_[slot];}
    T& operator[](unsigned slot){return scalar_[slot];}
};
template<unsigned Capacity,class Publication,bool Source> struct NativeTerrainRunsStorage {
    using Type=TerrainOwnershipRuns<Capacity,Publication>;
};
template<unsigned Capacity,class Publication> struct NativeTerrainRunsStorage<Capacity,Publication,true> {
    using Type=NativeTerrainSourceRuns<Capacity,Publication>;
};
}
#endif

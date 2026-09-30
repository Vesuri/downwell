#ifndef DOWNWELL_RUNNER_SPATIAL_STATES_H
#define DOWNWELL_RUNNER_SPATIAL_STATES_H
#include "RunnerSpatialState.h"
#include "PreparedTerrainBirthBounds.h"
#include "RunnerValueRead.h"
namespace downwell {
template<class State,unsigned Capacity,unsigned First,unsigned Last,class Terrain=void>
class RunnerSpatialStates;
template<class State,unsigned Capacity,unsigned First,unsigned Last>
class RunnerSpatialStates<State,Capacity,First,Last,void> {
    State values_[Capacity]{};
public:
    static constexpr bool runOwned=false;
    using Read=const State*;
    struct Edit {State& value;State& get(){return value;}};
    Read read(unsigned slot)const{return &values_[slot];}
    Edit edit(unsigned slot){return {values_[slot]};}
    template<bool> Edit editDomain(unsigned slot){return edit(slot);}
    std::uint32_t flags(unsigned slot)const{return values_[slot].flags;}
    bool special(unsigned slot)const{return values_[slot].special;}
    unsigned kind(unsigned slot)const{return values_[slot].kind;}
    bool capture(unsigned slot){auto& s=values_[slot];s.previous={s.x,s.y};return true;}
    void fresh(unsigned slot,const State& state){values_[slot]=state;values_[slot].previous={state.x,state.y};}
    void cache(unsigned slot,const decltype(State{}.bounds)& box,PreparedTerrainBirthBounds::Key){values_[slot].bounds=box;}
    template<class T> bool bind(T&){return true;}
    void geometry(const PreparedTerrainBirthBounds&){}
    template<unsigned N> void prepare(const PreparedObjectDefaults<N>&){}
    bool valid()const{return true;}
    void clear(){}
};
// Generated terrain owns no per-cell spatial record here. Publication stages
// select immutable flags and constructor bounds; coordinates come from the
// authoritative identity run. A real mutation splits that run and retains an
// exact literal state. No later query replays constructors or reads current art
// to reconstruct historical bounds.
template<class State,unsigned Capacity,unsigned First,unsigned Last,class Terrain>
class RunnerSpatialStates {
    State actors_[Capacity-(Last-First)]{};
    Terrain* terrain_=nullptr;
    const PreparedTerrainBirthBounds* geometry_=nullptr;
    std::uint32_t initialFlags_[3]{};
    bool precise_[3]{};
    bool good_=true;
    struct Borrow {unsigned slot;State* value;Borrow* next;};
    Borrow* borrowed_=nullptr;
    static bool terrain(unsigned slot){return slot>=First&&slot<Last;}
    static unsigned family(int object){return object==60?0:object==93?1:2;}
    State& actor(unsigned slot){return actors_[slot<First?slot:slot-(Last-First)];}
    const State& actor(unsigned slot)const{return actors_[slot<First?slot:slot-(Last-First)];}
    State* borrowed(unsigned slot)const{
        for(auto* b=borrowed_;b;b=b->next)if(b->slot==slot)return b->value;
        return nullptr;
    }
    std::uint32_t generatedFlags(unsigned local,unsigned stage)const{
        const unsigned mode=stage&127u,inactive=(stage&128u)?2u:0u;
        if(mode<2)return 8u|inactive;
        const unsigned f=family(terrain_->spatialObject(local));
        auto flags=initialFlags_[f]|inactive;
        if(mode>=3)flags=(flags&~0x208u)|(precise_[f]?0x200u:0u);
        if(mode==4||mode==6)flags|=0x8000u;
        if(mode==5||mode==6)flags|=8u;
        return flags;
    }
    State stored(unsigned slot)const{
        const unsigned local=slot-First,stage=terrain_->spatialStage(local),mode=stage&127u;
        if(mode==7)return terrain_->spatialLiteral(local);
        State s;if(!mode)return s;
        int object,x,y;
        if(!terrain_->spatialPosition(local,object,x,y))return s;
        s.x.assignInteger(x);s.y.assignInteger(y);s.previous={s.x,s.y};
        s.flags=generatedFlags(local,stage);
        if(mode>=3){PreparedTerrainBirthBounds::Key key;
            if(geometry_->key(object,x,y,key))geometry_->bounds(key,s.bounds);
        }
        return s;
    }
    void save(unsigned slot,const State& s){
        if(!stored(slot).same(s))terrain_->spatialLiteral(slot-First,s);
    }
public:
    static constexpr bool runOwned=true;
    using Read=RunnerValueRead<State>;
    class Edit {
        RunnerSpatialStates& owner_;unsigned slot_;
        union Value {State state;Value(){}~Value(){}} value_;
        State* state_;Borrow borrow_;bool owns_=false;
    public:
        Edit(RunnerSpatialStates& owner,unsigned slot):owner_(owner),slot_(slot){
            if(!terrain(slot)){state_=&owner.actor(slot);return;}
            if((state_=owner.borrowed(slot)))return;
            new(&value_.state)State(owner.stored(slot));state_=&value_.state;owns_=true;
            borrow_={slot,state_,owner.borrowed_};owner.borrowed_=&borrow_;
        }
        Edit(const Edit&)=delete;
        ~Edit(){if(owns_){owner_.borrowed_=borrow_.next;owner_.save(slot_,*state_);value_.state.~State();}}
        State& get(){return *state_;}
    };
    Read read(unsigned slot)const{
        if(!terrain(slot))return Read(&actor(slot));
        if(const auto* live=borrowed(slot))return Read(*live);
        return Read(stored(slot));
    }
    Edit edit(unsigned slot){return Edit(*this,slot);}
    struct ActorEdit {State& state;State& get(){return state;}};
    // Called only after the family binding encoded/validated its handle.
    // Actors never require terrain reconstruction or scoped publication.
    template<bool MayBeTerrain> auto editDomain(unsigned slot){
        if constexpr(MayBeTerrain)return edit(slot);
        else return ActorEdit{actor(slot)};
    }
    std::uint32_t flags(unsigned slot)const{
        if(!terrain(slot))return actor(slot).flags;
        if(const auto* s=borrowed(slot))return s->flags;
        const unsigned stage=terrain_->spatialStage(slot-First);
        return (stage&127u)==7?terrain_->spatialLiteral(slot-First).flags:generatedFlags(slot-First,stage);
    }
    bool special(unsigned slot)const{
        if(!terrain(slot))return actor(slot).special;
        if(const auto* s=borrowed(slot))return s->special;
        return (terrain_->spatialStage(slot-First)&127u)==7&&terrain_->spatialLiteral(slot-First).special;
    }
    unsigned kind(unsigned slot)const{
        if(!terrain(slot))return actor(slot).kind;
        if(const auto* s=borrowed(slot))return s->kind;
        return (terrain_->spatialStage(slot-First)&127u)==7?terrain_->spatialLiteral(slot-First).kind:1;
    }
    bool capture(unsigned slot){
        if(!terrain(slot)){auto& s=actor(slot);s.previous={s.x,s.y};return true;}
        if(terrain(slot)&&!borrowed(slot)&&(terrain_->spatialStage(slot-First)&127u)!=7)return true;
        auto e=edit(slot);auto& s=e.get();s.previous={s.x,s.y};return true;
    }
    void fresh(unsigned slot,const State& state){
        if(!terrain(slot)){auto& s=actor(slot);s=state;s.previous={state.x,state.y};return;}
        auto e=edit(slot);e.get()=state;e.get().previous={state.x,state.y};
    }
    void cache(unsigned slot,const decltype(State{}.bounds)& box,PreparedTerrainBirthBounds::Key){
        if(!terrain(slot)){actor(slot).bounds=box;return;}
        auto e=edit(slot);e.get().bounds=box;
    }
    // These stage changes are restricted to the admitted generated-birth path.
    // Nothing reads or copies coordinate/bounds records at these boundaries.
    void freshGenerated(unsigned slot,bool inactive){terrain_->spatialStage(slot-First,1u|(inactive?128u:0u));}
    void inheritedConstructor(unsigned slot,bool inactive,bool spatial,bool collision){
        terrain_->inheritedConstructor(slot-First,inactive,spatial,collision);
    }
    void stageGenerated(unsigned slot,unsigned mode){
        terrain_->spatialStage(slot-First,(terrain_->spatialStage(slot-First)&128u)|mode);
    }
    bool dirtyGenerated(unsigned slot){
        if(!terrain(slot)||borrowed(slot))return false;
        const auto stage=terrain_->spatialStage(slot-First);const unsigned mode=stage&127u;
        if(mode!=3&&mode!=4)return false;
        terrain_->spatialStage(slot-First,stage+2);return true;
    }
    bool bind(Terrain& value){if(terrain_)return false;terrain_=&value;return true;}
    void geometry(const PreparedTerrainBirthBounds& value){geometry_=&value;}
    template<unsigned N> void prepare(const PreparedObjectDefaults<N>& defaults){
        const int objects[]={60,93,99};
        for(unsigned f=0;f<3;++f)if(const auto* d=defaults.get(objects[f])){
            initialFlags_[f]=d->initialFlags;
            RunnerImageValue x,y;RunnerSpatialSplit::Box box;bool precise=false;
            if(geometry_&&geometry_->at(objects[f],0,0,x,y,box,precise))precise_[f]=precise;
        }
    }
    bool valid()const{return good_;}
    void clear(){good_=true;}
};
}
#endif

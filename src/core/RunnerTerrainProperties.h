#ifndef DOWNWELL_RUNNER_TERRAIN_PROPERTIES_H
#define DOWNWELL_RUNNER_TERRAIN_PROPERTIES_H
#include "PreparedObjectDefaults.h"
#include "TerrainRunnerPublication.h"
#include "RunnerValueRead.h"
namespace downwell {
// Native terrain appearance belongs to the authoritative identity run. Only
// actors retain full records here; immutable constructor variants are shared.
template<class Properties,unsigned Capacity,unsigned First,unsigned Last,class Terrain>
class RunnerTerrainProperties {
    Properties actors_[Capacity-(Last-First)]{};
    Properties models_[7]{};
    Terrain* terrain_=nullptr;
    bool good_=true;
    static bool terrain(unsigned slot){return slot>=First&&slot<Last;}
    static unsigned type(int object){return object==60?0:object==93?1:2;}
    Properties& actor(unsigned slot){return actors_[slot<First?slot:slot-(Last-First)];}
    const Properties& actor(unsigned slot)const{return actors_[slot<First?slot:slot-(Last-First)];}
    Properties expand(unsigned slot)const{
        const auto c=terrain_->appearance(slot-First);
        if(c.model>=8)return models_[2+2*(c.model-8)];
        auto p=models_[c.model];p.sprite.spriteId=c.sprite;p.sprite.imageIndex=c.index;return p;
    }
    static bool sameModel(const Properties& a,const Properties& b){
        return a.sprite.maskId==b.sprite.maskId&&a.sprite.scaleX.bits==b.sprite.scaleX.bits&&
            a.sprite.scaleY.bits==b.sprite.scaleY.bits&&a.sprite.angle.bits==b.sprite.angle.bits&&
            a.depth.bits==b.depth.bits&&a.timeline.bits==b.timeline.bits&&a.cachedFrame.bits==b.cachedFrame.bits;
    }
    void save(unsigned slot,const Properties& p){
        auto c=terrain_->appearance(slot-First);
        unsigned model=c.model>=8?2+2*(c.model-8):c.model;
        if(!sameModel(p,models_[model])){
            for(model=0;model<7&&!sameModel(p,models_[model]);++model){}
            if(model==7){good_=false;return;}
        }
        c.sprite=p.sprite.spriteId;c.index=p.sprite.imageIndex;c.model=static_cast<unsigned char>(model);
        terrain_->setAppearance(slot-First,c);
    }
public:
    using Read=RunnerValueRead<Properties>;
    class Edit {
        RunnerTerrainProperties& owner_;unsigned slot_;
        union Value {Properties value;Value(){}~Value(){}} value_;
        Properties* p_;
    public:
        Edit(RunnerTerrainProperties& owner,unsigned slot):owner_(owner),slot_(slot){
            if(terrain(slot)){new(&value_.value)Properties(owner.expand(slot));p_=&value_.value;}
            else p_=&owner.actor(slot);
        }
        Edit(const Edit&)=delete;
        ~Edit(){if(terrain(slot_)){owner_.save(slot_,*p_);value_.value.~Properties();}}
        Properties& get(){return *p_;}
        void publish(){if(terrain(slot_))owner_.save(slot_,*p_);}
    };
    bool bind(Terrain& terrain){if(terrain_)return false;terrain_=&terrain;return true;}
    template<unsigned N> void prepare(const PreparedObjectDefaults<N>& defaults){
        const int objects[]={60,93,99};
        for(int object:objects)if(const auto* d=defaults.get(object)){
            auto& p=models_[1+2*type(object)];p.sprite.spriteId=d->spriteId;p.sprite.maskId=d->maskId;p.depth=d->depth;
            models_[2+2*type(object)]=p;models_[2+2*type(object)].depth=d->creationDepth;
        }
    }
    bool valid()const{return good_;}
    Read read(unsigned slot)const{return terrain(slot)?Read(expand(slot)):Read(&actor(slot));}
    Edit edit(unsigned slot){return Edit(*this,slot);}
    struct ActorEdit {Properties& p;Properties& get(){return p;}void publish(){}};
    template<bool MayBeTerrain> auto editDomain(unsigned slot){
        if constexpr(MayBeTerrain)return edit(slot);
        else return ActorEdit{actor(slot)};
    }
    void birth(unsigned slot){terrain_->constructorAppearance(slot-First);}
    void created(unsigned slot,int sprite,RunnerImageValue index){
        auto c=terrain_->appearance(slot-First);
        if(c.model>=8)c.model=static_cast<unsigned char>(2+2*(c.model-8));
        c.sprite=sprite;c.index=index;terrain_->setAppearance(slot-First,c);
    }
};
template<class Properties,unsigned Capacity> class RunnerFullProperties {
    Properties records_[Capacity]{};
public:
    using Read=const Properties*;
    struct Edit {Properties& p;Properties& get(){return p;}void publish(){}};
    Read read(unsigned slot)const{return &records_[slot];}
    Edit edit(unsigned slot){return {records_[slot]};}
    template<bool> Edit editDomain(unsigned slot){return edit(slot);}
    bool valid()const{return true;}
};
}
#endif

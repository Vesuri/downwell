#ifndef DOWNWELL_CORE_GAMEPLAY_COLLISION_OWNER_H
#define DOWNWELL_CORE_GAMEPLAY_COLLISION_OWNER_H
#include "GameplayCollisionView.h"
#include "RunnerSpatialInstances.h"
#include "RunnerObjectFamilies.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
namespace downwell {
// Resource-backed ownership at the native world's existing publication points.
// No queries are exposed until all participating families and setters exist.
// Install bindings before constructing the world, then attach before Step.
class GameplayCollisionOwner {
public:
    using View=GameplayCollisionView;
    using Owner=RunnerSpatialInstances<View::capacity,View::capacity*2>;
private:
    Owner owner_;
    Owner::Properties properties_[View::capacity]{};
    RunnerObjectFamilies<386> families_;
    const ObjectPack& objects_;
    const SpriteCollisionPack& sprites_;
    const GameplayWorld* world_=nullptr;
    const GameplayWorld::PlayerInstance* initialPlayer_=nullptr;
    InstanceHandle initialPlayerKey_{};
    bool good_=false;
    bool check(bool ok){if(!ok)good_=false;return ok;}
    static bool real(int n,RunnerImageValue& out){return out.assign(MotionBinary64::fromInteger(n));}
    bool refresh(InstanceHandle key,bool publish){
        const auto* flags=families_.flags(unsigned(owner_.objectId(key)));
        return flags&&owner_.refreshSprite(key,*flags,false,sprites_,properties_[key.slot].sprite,true,
            [](auto angle,auto& cosine,auto& sine){
                RunnerImageValue radians;MotionBinary64 c,s;
                return SmokeDirection::radians(angle,radians)&&smokeTrig(radians.promoted(),c,s)&&
                    cosine.assign(c)&&sine.assign(s);
            },[](auto v,std::int32_t& n){return v.promoted().roundPixel(n);},publish);
    }
    template<class Record,View::Kind Kind> struct Binding {
        GameplayCollisionOwner& parent;
        static int object(const Record& r){
            if constexpr(Kind==View::Kind::enemy||Kind==View::Kind::terrain)return r.objectId;
            return Kind==View::Kind::player?35:92;
        }
        bool key(InstanceHandle local,const Record& r,InstanceHandle& out,bool birth=false){
            if(!parent.valid()||local.generation!=r.lifetimeId||!View::encode(Kind,local,out))return parent.check(false);
            if(birth&&!parent.world_)return parent.check(Kind==View::Kind::player);
            View::Record actual;
            if(!parent.world_||!View(*parent.world_).resolve(out,actual)||
                (!birth&&(!parent.owner_.resolve(out)||parent.owner_.objectId(out)!=object(r))))return parent.check(false);
            const void* pointer;
            if constexpr(Kind==View::Kind::player)pointer=actual.player;
            else if constexpr(Kind==View::Kind::enemy)pointer=actual.enemy;
            else if constexpr(Kind==View::Kind::terrain)pointer=actual.terrain;
            else pointer=actual.breakable;
            return parent.check(pointer==&r);
        }
        bool birth(InstanceHandle local,const Record& r){
            InstanceHandle h;if(!key(local,r,h,true))return false;
            constexpr bool roomCoordinates=Kind==View::Kind::player||Kind==View::Kind::terrain;
            constexpr int ox=roomCoordinates?0:240,oy=roomCoordinates?0:176;
            if(r.x>0x7fffffff-ox||r.y>0x7fffffff-oy)return parent.check(false);
            Owner::State state;
            if(!real(r.x+ox,state.x)||!real(r.y+oy,state.y))return parent.check(false);
            if(!r.active)state.flags|=2u;
            if(!parent.check(parent.owner_.attach(h,object(r),state)))return false;
            auto& p=parent.properties_[h.slot];p={};
            if(!parent.check(parent.owner_.bindObjectFromPacks(h,object(r),true,p,
                parent.objects_,parent.sprites_,parent.families_,false)))return false;
            PackedObject definition;
            if(!parent.objects_.findObject(object(r),definition)||!real(definition.creationDepth,p.depth))return parent.check(false);
            if constexpr(Kind==View::Kind::enemy){
                p.sprite.spriteId=r.spriteId;p.sprite.maskId=r.maskId;
                if(!real(r.objectId==117?r.motionState:r.facing,p.sprite.scaleX)||
                    !real(r.imageScaleY(),p.sprite.scaleY)||r.imagePhase>0x7fffffffu||
                    !real(int(r.imagePhase),p.sprite.imageIndex))return parent.check(false);
                if(p.sprite.imageIndex.bits)p.sprite.imageIndex.bits-=8u<<23;
            }else if constexpr(Kind==View::Kind::player){
                if(!real(r.scaleX,p.sprite.scaleX))return parent.check(false);
            }
            if(!parent.check(parent.owner_.publishBirth(h,parent.families_,
                [&](InstanceHandle live,bool publish){return parent.refresh(live,publish);})))return false;
            if constexpr(Kind==View::Kind::player){
                if(!parent.world_){parent.initialPlayer_=&r;parent.initialPlayerKey_=local;}
            }
            return true;
        }
        bool retire(InstanceHandle local,const Record& r){InstanceHandle h;
            return key(local,r,h)&&parent.check(parent.owner_.retire(h));}
        bool membership(InstanceHandle local,const Record& r,bool member){InstanceHandle h;
            return key(local,r,h)&&parent.check(parent.owner_.registerObject(h,member));}
        bool activation(InstanceHandle local,const Record& r,bool active){InstanceHandle h;
            return key(local,r,h)&&parent.check(parent.owner_.setActive(h,active,*parent.families_.flags(object(r))));}
        bool move(InstanceHandle local,const Record& r,RunnerImageValue x,RunnerImageValue y){InstanceHandle h;
            return key(local,r,h)&&parent.check(parent.owner_.move(h,x,y,*parent.families_.flags(object(r))));}
        bool sprite(InstanceHandle local,const Record& r,int sprite){InstanceHandle h;
            if(!key(local,r,h))return false;auto& p=parent.properties_[h.slot];
            return parent.check(parent.owner_.changeSpriteFromPack(h,p.sprite,p.timeline,p.cachedFrame,
                sprite,parent.sprites_,*parent.families_.flags(object(r))));}
        bool imageIndex(InstanceHandle local,const Record& r,RunnerImageValue value){InstanceHandle h;
            return key(local,r,h)&&parent.check(parent.owner_.writeImageIndex(h,parent.properties_[h.slot].sprite,value));}
        bool mask(InstanceHandle local,const Record& r,int value){InstanceHandle h;
            return key(local,r,h)&&parent.check(parent.owner_.changeMask(h,parent.properties_[h.slot].sprite,
                value,*parent.families_.flags(object(r))));}
        bool transform(InstanceHandle local,const Record& r,RunnerTransformField field,RunnerImageValue value,
            RunnerImageValue alpha,std::uint32_t blend){InstanceHandle h;
            return key(local,r,h)&&parent.check(parent.owner_.changeTransform(h,parent.properties_[h.slot].sprite,
                field,value,alpha,blend,*parent.families_.flags(object(r))));}
    };
    Binding<GameplayWorld::PlayerInstance,View::Kind::player> player_{*this};
    Binding<GameplayWorld::EnemyState,View::Kind::enemy> enemies_{*this};
    Binding<GameplayWorld::Breakable,View::Kind::breakable> boxes_{*this};
    Binding<GameplayTerrainInstances::Record,View::Kind::terrain> terrainBinding_{*this};
    GameplayTerrainInstances terrain_{GameplayTerrainInstances::Binding::bindAnimation(terrainBinding_)};
public:
    GameplayCollisionOwner(const ObjectPack& objects,const SpriteCollisionPack& sprites):objects_(objects),sprites_(sprites){
        good_=sprites.valid()&&families_.bindInstanceFlags(objects);
    }
    GameplayCollisionOwner(const GameplayCollisionOwner&)=delete;
    GameplayCollisionOwner& operator=(const GameplayCollisionOwner&)=delete;
    bool valid()const{return good_&&owner_.valid()&&terrain_.valid();}
    bool attach(const GameplayWorld& world){
        if(!valid()||(world_&&world_!=&world)||(!world_&&
            (!initialPlayer_||world.findPlayer(initialPlayerKey_)!=initialPlayer_)))return false;
        world_=&world;return true;
    }
    auto playerBinding(){return GameplayWorld::PlayerLifecycleBinding::bindTransforms(player_);}
    auto enemyBinding(){return GameplayWorld::EnemyLifecycleBinding::bindMasks(enemies_);}
    auto breakableBinding(){return GameplayWorld::BreakableLifecycleBinding::bindMovement(boxes_);}
    GameplayTerrainInstances& terrain(){return terrain_;}
    const Owner::State* state(InstanceHandle key)const{return valid()?owner_.resolve(key):nullptr;}
    const Owner::Properties* properties(InstanceHandle key)const{return state(key)?&properties_[key.slot]:nullptr;}
    bool population(int object,unsigned& count)const{return valid()&&owner_.instanceNumber(object,families_,count);}
};
}
#endif

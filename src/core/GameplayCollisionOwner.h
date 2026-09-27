#ifndef DOWNWELL_CORE_GAMEPLAY_COLLISION_OWNER_H
#define DOWNWELL_CORE_GAMEPLAY_COLLISION_OWNER_H
#include "GameplayCollisionView.h"
#include "RunnerSpatialInstances.h"
#include "RunnerObjectFamilies.h"
#include "RunnerRoomMembership.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
namespace downwell {
// Resource-backed ownership at the native world's existing publication points.
// Terrain-only queries require complete registered terrain families. Other
// selectors remain unavailable until their families and setters are registered.
// Install bindings before constructing the world, then attach before Step.
class GameplayCollisionOwner {
public:
    using View=GameplayCollisionView;
    using Owner=RunnerSpatialInstances<View::capacity,View::capacity*2>;
private:
    Owner owner_;
    RunnerRoomMembership<View::capacity> room_;
    InstanceHandle roomKeys_[View::capacity]{};
    Owner::Properties properties_[View::capacity]{};
    RunnerObjectFamilies<386> families_;
    const ObjectPack& objects_;
    const SpriteCollisionPack& sprites_;
    GameplayWorld* world_=nullptr;
    const GameplayWorld::PlayerInstance* initialPlayer_=nullptr;
    InstanceHandle initialPlayerKey_{};
    bool good_=false;
    bool activationPending_=false;
    bool destructionPending_=false;
    bool terrainFamilies_[2]={false,false};
    bool check(bool ok){if(!ok)good_=false;return ok;}
    bool terrainQuery(int object)const{
        if(!valid()||!world_||(object!=60&&object!=93))return false;
        return terrainFamilies_[object==60?0:1];
    }
    bool resolves(InstanceHandle h)const{
        View::Record r;return world_&&owner_.resolve(h)&&View(*world_).resolve(h,r);
    }
    static bool inverseTrig(RunnerImageValue angle,RunnerImageValue& cosine,RunnerImageValue& sine){
        angle.bits^=0x80000000u;
        RunnerImageValue radians;MotionBinary64 c,s;
        return SmokeDirection::radians(angle,radians)&&smokeTrig(radians.promoted(),c,s)&&
            cosine.assign(c)&&sine.assign(s);
    }
    static bool roundPixel(RunnerImageValue v,std::int32_t& n){return v.promoted().roundPixel(n);}
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
            if(!parent.check(parent.room_.bind(h.slot,p.depth)))return false;
            parent.roomKeys_[h.slot]=h;
            if(!r.active)parent.activationPending_=true;
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
            if(!key(local,r,h)||!parent.check(parent.room_.contains(h.slot)))return false;
            if(!parent.check(parent.owner_.retire(h)))return false;
            parent.roomKeys_[h.slot]={};
            return parent.check(parent.room_.remove(h.slot));}
        bool markDestroyed(InstanceHandle local,const Record& r){InstanceHandle h;
            if(!key(local,r,h))return false;
            const auto* state=parent.owner_.resolve(h);
            if(!state||!parent.check(parent.owner_.setExclusions(h,(state->flags&0x100003u)|1u)))return false;
            parent.destructionPending_=true;return true;}
        bool membership(InstanceHandle local,const Record& r,bool member){InstanceHandle h;
            return key(local,r,h)&&parent.check(parent.owner_.registerObject(h,member));}
        bool activation(InstanceHandle local,const Record& r,bool active){InstanceHandle h;
            if(!key(local,r,h)||!parent.check(parent.owner_.setActive(h,active,*parent.families_.flags(object(r)))))return false;
            parent.activationPending_=true;return true;}
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
    GameplayTerrainInstances terrain_;
public:
    GameplayCollisionOwner(const ObjectPack& objects,const SpriteCollisionPack& sprites,
        GameplayTerrainInstances::Presentation presentation={},GameplayTerrainInstances::WallCollision collision={}):objects_(objects),sprites_(sprites),
        terrain_(GameplayTerrainInstances::Binding::bindDestruction(terrainBinding_),presentation,collision){
        good_=sprites.valid()&&families_.bindInstanceFlags(objects);
        if(good_)for(unsigned selector=0;selector<2;++selector){
            terrainFamilies_[selector]=true;
            for(unsigned i=0;i<families_.size();++i)
                if(families_.isA(int(i),selector==0?60:93)&&i!=60&&i!=93&&i!=99)
                    terrainFamilies_[selector]=false;
        }
    }
    GameplayCollisionOwner(const GameplayCollisionOwner&)=delete;
    GameplayCollisionOwner& operator=(const GameplayCollisionOwner&)=delete;
    bool valid()const{return good_&&owner_.valid()&&terrain_.valid();}
    bool attach(GameplayWorld& world){
        if(!valid()||(world_&&world_!=&world)||(!world_&&
            (!initialPlayer_||world.findPlayer(initialPlayerKey_)!=initialPlayer_)))return false;
        world_=&world;
        world.configurePhaseBoundary([](void* context,GameplayWorld::PhaseBoundary){
            return static_cast<GameplayCollisionOwner*>(context)->reconcileActivation();
        },this);
        world.configureEnemyStepOrder([](void* context,GameplayWorld::EnemyStepOrder& snapshot){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            return self.check(self.owner_.visitRegistrations([&](InstanceHandle shared,int object,const Owner::State& state){
                View::Kind kind;InstanceHandle local;
                if(!View::decode(shared,kind,local))return false;
                if(kind!=View::Kind::enemy||(state.flags&0x100003u))return true;
                return self.world_->resolveEnemyLifetime(local)&&snapshot.append(local.slot,local.generation,object);
            }));
        },this);
        return true;
    }
    auto playerBinding(){return GameplayWorld::PlayerLifecycleBinding::bindTransforms(player_);}
    auto enemyBinding(){return GameplayWorld::EnemyLifecycleBinding::bindMaskDestruction(enemies_);}
    auto breakableBinding(){return GameplayWorld::BreakableLifecycleBinding::bindDestruction(boxes_);}
    // Caller owns the phase workspace; a full native snapshot must not live on
    // the 8 KiB task stack. Capture all registered families, then resolve again
    // immediately before each event. An empty own event is still dispatched.
    template<unsigned Capacity,class Execute>
    bool dispatchStepPhase(unsigned subtype,RunnerStepSnapshot<Capacity>& snapshot,Execute execute){
        if(!valid()||!world_||subtype>2)return check(false);
        snapshot.clear();
        if(!check(owner_.visitRegistrations([&](InstanceHandle h,int object,const Owner::State& state){
            return resolves(h)&&((state.flags&0x100003u)||snapshot.append(h.slot,h.generation,object));
        })))return false;
        snapshot.sort();
        const unsigned count=snapshot.size();
        for(unsigned i=0;i<count;++i){
            const auto h=snapshot.handle(i);
            const auto* current=owner_.resolve(h);
            // Destruction/retirement by a preceding callback suppresses this
            // lifetime; a new occupant of the slot cannot inherit its turn.
            if(!current||(current->flags&0x100003u))continue;
            if(!check(resolves(h)))return false;
            PackedObjectEvent event;
            if(!objects_.findEvent(owner_.objectId(h),3,int(subtype),event))continue;
            if(!check(execute(h,event))||!valid())return false;
        }
        return true;
    }
    GameplayTerrainInstances& terrain(){return terrain_;}
    const Owner::State* state(InstanceHandle key)const{return valid()?owner_.resolve(key):nullptr;}
    const Owner::Properties* properties(InstanceHandle key)const{return state(key)?&properties_[key.slot]:nullptr;}
    bool population(int object,unsigned& count)const{return valid()&&owner_.instanceNumber(object,families_,count);}
    bool terrainPositionMeeting(int object,MotionBinary64 x,MotionBinary64 y,const SpriteMaskPack& masks,bool& hit){
        RunnerImageValue px,py;bool result=false;
        if(!terrainQuery(object)||!masks.valid()||!px.assign(x)||!py.assign(y))return check(false);
        auto update=[&](InstanceHandle h,bool publish){return resolves(h)&&refresh(h,publish);};
        if(!owner_.point(object,px,py,true,false,families_,update,
            [&](InstanceHandle h,RunnerImageValue rawX,RunnerImageValue rawY,bool& found){
                return resolves(h)&&owner_.pointCandidate(h,properties_[h.slot].sprite,rawX,rawY,
                    true,true,sprites_,masks,update,inverseTrig,roundPixel,found);
            },result))return check(false);
        hit=result;return true;
    }
    bool terrainPlaceMeeting(InstanceHandle local,int object,MotionBinary64 x,MotionBinary64 y,
        const SpriteMaskPack& masks,bool& hit){
        InstanceHandle self;Owner::Result result;
        if(!terrainQuery(object)||!masks.valid()||!View::encode(View::Kind::terrain,local,self)||
            !resolves(self))return check(false);
        auto update=[&](InstanceHandle h,bool publish){return resolves(h)&&refresh(h,publish);};
        if(!owner_.placeVm(self,object,x,y,false,families_,update,
            [&](InstanceHandle a,InstanceHandle b,bool publish,bool& found){
                return resolves(a)&&resolves(b)&&owner_.collideCompatibility(a,b,
                    properties_[a.slot].sprite,properties_[b.slot].sprite,sprites_,masks,
                    RunnerMaskDispatch{},publish,update,inverseTrig,roundPixel,found);
            },result))return check(false);
        hit=result.found;return true;
    }
    bool reconcileActivation(){
        if(!valid()||!world_)return check(false);
        if(!activationPending_)return true;
        const bool result=room_.reconcileChecked([&](unsigned slot){
            const auto* state=owner_.resolve(roomKeys_[slot]);
            if(!state){good_=false;return true;}
            return (state->flags&2u)!=0;
        },[&](unsigned slot,bool registered){
            return good_&&owner_.registerObject(roomKeys_[slot],registered);
        });
        if(!check(result&&good_))return false;
        activationPending_=false;return true;
    }
    // Dispatch a scheduler-selected wall, not a terrain-only phase. Effects
    // owns tile publication, synchronous flower Create and area-five lifetime
    // policy; none may physically retire this record during the callback.
    template<class Effects> bool wallEndStepEvent(InstanceHandle local,int area,GameMakerRandom& random,
        const SpriteMaskPack& masks,Effects& effects){
        if(!valid()||!world_||!terrain_.find(local))return check(false);
        struct Ops {
            GameplayCollisionOwner& owner;InstanceHandle local;const SpriteMaskPack& masks;Effects& effects;
            bool coordinates(int dx,int dy,MotionBinary64& x,MotionBinary64& y){
                const auto* r=owner.terrain_.find(local);if(!r)return false;
                x=MotionBinary64::fromInteger(r->x);y=MotionBinary64::fromInteger(r->y);
                return x.add(MotionBinary64::fromInteger(dx))&&y.add(MotionBinary64::fromInteger(dy));
            }
            bool place(int dx,int dy,int object,bool& hit){MotionBinary64 x,y;
                return coordinates(dx,dy,x,y)&&owner.terrainPlaceMeeting(local,object,x,y,masks,hit);}
            bool point(int dx,int dy,int object,bool& hit){MotionBinary64 x,y;
                return coordinates(dx,dy,x,y)&&owner.terrainPositionMeeting(object,x,y,masks,hit);}
            bool image(int frame){return owner.terrain_.imageIndex(local,frame);}
            bool tile(int resource,int sx,int sy,int w,int h,int dx,int dy,int depth){MotionBinary64 x,y;
                return resource==739&&sx==48&&sy==48&&w==16&&h==16&&depth==100&&
                    coordinates(dx,dy,x,y)&&effects.tile(x,y);}
            bool destroy(){return owner.terrain_.requestDestroy(local);}
            bool deactivate(){return owner.terrain_.activation(local,false);}
            bool outOfView(){return effects.outOfView(local);}
            bool create(int object,int dx,int dy){MotionBinary64 x,y;
                return coordinates(dx,dy,x,y)&&effects.create(object,x,y);}
        } ops{*this,local,masks,effects};
        return check(terrain_.wallEvent(local,area,random,ops));
    }
    // Source room cleanup visits active entries before inactive entries.
    // Retire may remove only the supplied lifetime, never create/rebind; it
    // must dispatch to the corresponding typed store before this call returns.
    template<class Retire> bool cleanupDestroyed(Retire retire){
        if(!valid()||!world_)return check(false);
        if(!destructionPending_)return true;
        const bool cleaned=room_.visitCleanupChecked([&](unsigned slot){
            const auto h=roomKeys_[slot];const auto* state=owner_.resolve(h);
            View::Record record;
            if(!state||!View(*world_).resolve(h,record))return false;
            if(!(state->flags&1u))return true;
            if(!retire(h,record)||!valid()||owner_.resolve(h))return false;
            View::Record remaining;
            return !View(*world_).resolve(h,remaining);
        });
        if(!check(cleaned))return false;
        destructionPending_=false;return true;
    }
    // Invoke at Draw entry, not at an inter-Step reconciliation boundary.
    // Retire through the typed stores, retaining identity across compaction.
    // Player destruction remains unsupported rather than silently discarded.
    bool prepareDraw(){
        return cleanupDestroyed([&](InstanceHandle shared,const View::Record& record){
            View::Kind kind;InstanceHandle local;
            if(!View::decode(shared,kind,local))return false;
            switch(kind){
                case View::Kind::terrain:return record.terrain==terrain_.find(local)&&terrain_.retireMarked(local);
                case View::Kind::enemy:return record.enemy==world_->resolveEnemyLifetime(local)&&world_->retireMarkedEnemy(local);
                case View::Kind::breakable:return record.breakable==world_->findBreakable(local)&&world_->retireMarkedBreakable(local);
                default:return false;
            }
        });
    }
};
}
#endif

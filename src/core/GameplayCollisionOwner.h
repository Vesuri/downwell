#ifndef DOWNWELL_CORE_GAMEPLAY_COLLISION_OWNER_H
#define DOWNWELL_CORE_GAMEPLAY_COLLISION_OWNER_H
#include "GameplayCollisionView.h"
#include "BoxDestructionScript.h"
#include "RunnerSpatialInstances.h"
#include "RunnerObservedRegistration.h"
#include "RunnerObservedRoomMembership.h"
#include "RunnerObjectFamilies.h"
#include "RunnerRoomMembership.h"
#include "RunnerRunRoomMembership.h"
#include "SmokeDirection.h"
#include "SmokeTrig.h"
#include "DebrisDraw.h"
#include "TimeStopBound.h"
#include "GameplayRoomLayers.h"
#include "ObjectDrawMetadata.h"
#include "ObjectAlarmMetadata.h"
#include "Profile.h"
#include "PreparedUnitSpriteBounds.h"
#include "PreparedTerrainBirthBounds.h"
namespace downwell {
// Resource-backed ownership at the native world's existing publication points.
// Terrain-only queries retain their explicit resource-family gate. Projectile
// line queries cover the currently registered solid/enemy lifetimes; other
// selectors and unimplemented actors are not synthesized by these adapters.
// Install bindings before constructing the world, then attach before Step.
class GameplayCollisionOwner {
public:
    using View=GameplayCollisionView;
    // Verification builds keep the complete scalar membership pipeline alive,
    // not just isolated container tests. Spatial journal scheduling is the
    // same on both sides so query timing is not changed by this comparison.
#ifdef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
    using Owner=RunnerSpatialInstances<View::capacity,View::capacity*2,true,View::terrainBase,View::timeFieldBase,false>;
#elif defined(DOWNWELL_TERRAIN_RUN_STORE)
    using Owner=RunnerSpatialInstances<View::capacity,View::capacity*2,true,View::terrainBase,View::timeFieldBase,true,
        RunnerSpatialLifetimes<View::capacity,View::terrainBase,View::timeFieldBase,GameplayTerrainInstances>,
        RunnerObservedRegistration<View::capacity,View::terrainBase,View::timeFieldBase,GameplayTerrainInstances>,true>;
#else
    using Owner=RunnerSpatialInstances<View::capacity,View::capacity*2,true,View::terrainBase,View::timeFieldBase>;
#endif
private:
    Owner owner_;
    static constexpr unsigned drawCapacity=1+GameplayWorld::enemyCapacity+GameplayWorld::bulletCapacity+
        GameplayScriptSpawnInstances::capacity+GameplayDebrisInstances::capacity+GameplayEntryDithers::capacity;
    struct DrawEntry {GameplayWorld::DrawActor kind;InstanceHandle local;RunnerImageValue depth;};
    mutable DrawEntry drawOrder_[drawCapacity]{};
    mutable unsigned drawCount_=0;
    mutable std::uint32_t drawCaptureRevision_=0,drawLayerRevision_=0;
    mutable bool drawOrderValid_=false;
    RunnerStepSnapshot<GameplayWorld::enemyCapacity+GameplayWorld::breakableCapacity+GameplayDebrisInstances::capacity+8+GameplayEntryDithers::capacity+GameplayWorld::droneBulletCapacity> combatAlarmPhase_;
    bool combatAlarmCaptureDirty_=true;
    unsigned combatAlarmMask_=0;
    bool combatAlarmCaptureValid_=false;
#ifdef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
    RunnerRoomMembership<View::capacity> room_;
#elif defined(DOWNWELL_TERRAIN_RUN_STORE)
    RunnerObservedRoomMembership<View::capacity,View::terrainBase,View::timeFieldBase,GameplayTerrainInstances> room_;
#else
    RunnerRunRoomMembership<View::capacity,View::terrainBase,View::timeFieldBase> room_;
#endif
    bool terrainSnapshotDirty_=true;
    Owner::Properties properties_[View::capacity]{};
    RunnerObjectFamilies<386> families_;
    const ObjectPack& objects_;
    // The pinned original has 741 sprite descriptors. Decode once before
    // any births/queries; keep preparation storage off the target task stack.
    PackedSpriteCollision preparedSprites_[741];
    SpriteCollisionPack sprites_;
    PreparedObjectDefaults<386> objectDefaults_;
    PreparedSpriteGeometry<741> spriteGeometry_;
    PreparedTerrainBirthBounds terrainBirthBounds_;
#if defined(DOWNWELL_TERRAIN_RUN_STORE) && !defined(DOWNWELL_SCALAR_TERRAIN_OWNERSHIP)
    PreparedTerrainCreate terrainCreate_;
#endif
#ifdef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
    GameplayRoomLayers<View::capacity> layers_;
#elif defined(DOWNWELL_TERRAIN_RUN_STORE)
    GameplayRoomLayers<View::capacity,true,GameplayTerrainInstances> layers_;
#else
    GameplayRoomLayers<View::capacity,true> layers_;
#endif
    ObjectDrawMetadata<386> drawMetadata_;
    ObjectAlarmMetadata<386> alarmMetadata_;
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
        return world_&&owner_.contains(h)&&View(*world_).contains(h);
    }
    bool eligibilityFlags(InstanceHandle h,std::uint32_t& out)const{
#ifdef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
        const auto* state=owner_.resolve(h);if(!state)return false;
        out=state->flags;return true;
#else
        return owner_.instanceFlags(h,out);
#endif
    }
    template<class Visit> bool visitEligibility(Visit visit)const{
#ifdef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
        return owner_.visitRegistrations([&](InstanceHandle h,int object,const Owner::State& state){
            return visit(h,object,state.flags);
        });
#else
        return owner_.visitRegistrationFlags(visit);
#endif
    }
    static bool inverseTrig(RunnerImageValue angle,RunnerImageValue& cosine,RunnerImageValue& sine){
        angle.bits^=0x80000000u;
        RunnerImageValue radians;MotionBinary64 c,s;
        return SmokeDirection::radians(angle,radians)&&smokeTrig(radians.promoted(),c,s)&&
            cosine.assign(c)&&sine.assign(s);
    }
    static bool roundPixel(RunnerImageValue v,std::int32_t& n){return v.roundPixel(n);}
    static bool real(int n,RunnerImageValue& out){return out.assignInteger(n);}
    bool refresh(InstanceHandle key,bool publish){
        DW_PROFILE_SCOPE(profileInstanceBounds);
        const auto* flags=families_.flags(unsigned(owner_.objectId(key)));
        const auto* state=owner_.resolve(key);
        if(flags&&state){
            const auto& sprite=properties_[key.slot].sprite;
            if(!(state->special&&sprite.maskId<0)&&!(sprite.angle.bits&0x7fffffffu)&&
               (sprite.scaleX.bits&0x7fffffffu)==0x3f800000u&&
               (sprite.scaleY.bits&0x7fffffffu)==0x3f800000u){
                const int id=sprite.maskId<0?sprite.spriteId:sprite.maskId;
                Owner::Box bounds;bool prepared;
                {
                    DW_PROFILE_SCOPE(profileBoundsGeometry);
                    prepared=spriteGeometry_.at(id,state->x,state->y,bounds,
                        (sprite.scaleX.bits>>31)!=0,(sprite.scaleY.bits>>31)!=0);
                }
                if(prepared)return owner_.refresh(key,*flags,false,&bounds,
                    preparedSprites_[id].collisionMode==1,publish);
            }
        }
        return flags&&owner_.refreshSprite(key,*flags,false,sprites_,properties_[key.slot].sprite,true,
            [](auto angle,auto& cosine,auto& sine){
                DW_PROFILE_SCOPE(profileInstanceBoundsTrig);
                RunnerImageValue radians;MotionBinary64 c,s;
                return SmokeDirection::radians(angle,radians)&&smokeTrig(radians.promoted(),c,s)&&
                    cosine.assign(c)&&sine.assign(s);
            },[](auto v,std::int32_t& n){return v.roundPixel(n);},publish);
    }
    template<class Record,View::Kind Kind> struct Binding {
        GameplayCollisionOwner& parent;
        static int object(const Record& r){
            if constexpr(Kind==View::Kind::enemy||Kind==View::Kind::terrain)return r.objectId;
            if constexpr(Kind==View::Kind::timeField)return 82;
            if constexpr(Kind==View::Kind::bullet)return 261;
            if constexpr(Kind==View::Kind::scriptEffect)return 219;
            if constexpr(Kind==View::Kind::debris)return 370;
            if constexpr(Kind==View::Kind::timeChange)return 211;
            if constexpr(Kind==View::Kind::entryDither)return 73;
            if constexpr(Kind==View::Kind::drone)return 266;
            if constexpr(Kind==View::Kind::goalField||Kind==View::Kind::exitController)return r.objectId;
            return Kind==View::Kind::player?35:92;
        }
        bool key(InstanceHandle local,const Record& r,InstanceHandle& out,bool birth=false){
            if(!parent.valid()||!View::encode(Kind,local,out))return parent.check(false);
            if constexpr(Kind!=View::Kind::timeField)
                if(local.generation!=r.lifetimeId)return parent.check(false);
            if(birth&&!parent.world_)return parent.check(Kind==View::Kind::player);
            if(!parent.world_)return parent.check(false);
            if(!birth&&(!parent.owner_.contains(out)||parent.owner_.objectId(out)!=object(r)))return parent.check(false);
#ifndef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
            if constexpr(Kind==View::Kind::terrain){
                const auto* store=parent.world_->terrainInstances();
                return parent.check(store&&store->ownsIdentity(local,r));
            }else
#endif
            {
                const auto* actual=View(*parent.world_).template resolveLocal<Kind>(local);
                return parent.check(actual==&r);
            }
        }
        bool birth(InstanceHandle local,const Record& r){return birthMode<false>(local,r);}
        template<bool AllocatedTerrain> bool birthMode(InstanceHandle local,const Record& r){
            DW_PROFILE_SCOPE(profileInstanceBirth);
            InstanceHandle h;
            if constexpr(AllocatedTerrain){
                static_assert(Kind==View::Kind::terrain,"Owned allocation transaction only");
                // appendInitialized supplied this record directly from the
                // allocation it just admitted. The spatial birth still checks
                // that identity once; do not deserialize the terrain run again
                // merely to compare the record with its own source.
                if(parent.world_&&parent.world_->terrainInstances()==&parent.terrain_){
                    if(!parent.valid()||!View::encode(Kind,local,h)||local.generation!=r.lifetimeId)
                        return parent.check(false);
                }else if(!key(local,r,h,true))return false;
            }else if(!key(local,r,h,true))return false;
            if constexpr(Kind==View::Kind::player)if(!parent.check(parent.layers_.prepare()))return false;
            constexpr bool roomCoordinates=Kind==View::Kind::player||Kind==View::Kind::terrain||Kind==View::Kind::timeField||Kind==View::Kind::goalField||Kind==View::Kind::exitController;
            constexpr int ox=roomCoordinates?0:240,oy=roomCoordinates?0:176;
            Owner::State state;
            Owner::Box terrainBounds;bool preparedTerrain=false,terrainPrecise=false;
            RunnerSpatialSplit::PixelBox terrainPixels;
            PreparedTerrainBirthBounds::Key terrainGeometry;
            if constexpr(Kind==View::Kind::terrain)
                preparedTerrain=parent.terrainBirthBounds_.at(r.objectId,r.x,r.y,
                    state.x,state.y,terrainBounds,terrainPrecise,&terrainPixels,&terrainGeometry);
            if constexpr(Kind==View::Kind::scriptEffect){
                if(!r.effect||!r.effect->valid)return parent.check(false);
                state.x=r.effect->x;state.y=r.effect->y;
            }else if constexpr(Kind==View::Kind::debris){
                if(!r.effect)return parent.check(false);
                state.x=r.effect->x;state.y=r.effect->y;
            }else if constexpr(Kind==View::Kind::timeChange||Kind==View::Kind::entryDither||Kind==View::Kind::drone){
                state.x=r.x;state.y=r.y;
            }else{
                if(r.x>0x7fffffff-ox||r.y>0x7fffffff-oy)return parent.check(false);
                if(!preparedTerrain&&(!real(r.x+ox,state.x)||!real(r.y+oy,state.y)))return parent.check(false);
            }
            if constexpr(Kind!=View::Kind::timeField)if(!r.active)state.flags|=2u;
            auto& p=parent.properties_[h.slot];
#if defined(DOWNWELL_TERRAIN_RUN_STORE) && !defined(DOWNWELL_SCALAR_TERRAIN_OWNERSHIP)
            if constexpr(Kind==View::Kind::terrain)if(preparedTerrain){
                const bool admitted=AllocatedTerrain&&parent.world_&&parent.world_->terrainInstances()==&parent.terrain_;
                auto between=[&](RunnerImageValue depth){
                        if(admitted)parent.room_.bindAllocatedTerrain(h.slot,depth);
                        else if(!parent.check(parent.room_.bind(h.slot,depth)))return false;
                        if(!r.active)parent.activationPending_=true;
                        return true;
                    };
                bool born;
                if constexpr(AllocatedTerrain){
                    // The typed allocator just admitted and reset this slot.
                    // Only that exact owner may bypass a second identity walk.
                    if(admitted)
                        born=parent.owner_.template attachPreparedTerrainBirth<true>(h,object(r),state,p,
                            parent.objectDefaults_,parent.families_,terrainBounds,terrainPixels,terrainPrecise,
                            between,terrainGeometry);
                    else born=parent.owner_.attachPreparedTerrainBirth(h,object(r),state,p,
                        parent.objectDefaults_,parent.families_,terrainBounds,terrainPixels,terrainPrecise,
                        between,terrainGeometry);
                }else born=parent.owner_.attachPreparedTerrainBirth(h,object(r),state,p,
                    parent.objectDefaults_,parent.families_,terrainBounds,terrainPixels,terrainPrecise,
                    between,terrainGeometry);
                if(!parent.check(born))return false;
                if(admitted)return parent.check(parent.layers_.birthAllocatedTerrain(h,p.depth,!r.active));
                return parent.check(parent.layers_.birth(h,r.sourceInstanceId,p.depth,!r.active,false));
            }
#endif
            {
                DW_PROFILE_SCOPE(profileBirthMetadata);
                RunnerImageValue creationDepth;
                if(!parent.check(parent.owner_.attachObjectPrepared(h,object(r),state,true,p,
                    parent.objectDefaults_,parent.families_,&creationDepth)))return false;
                p.depth=creationDepth;
            }
            if constexpr(Kind==View::Kind::debris){
                // instance_create uses the compatibility depth table, not the
                // GMS2 OBJT default; Create also selects area-specific art.
                if(!real(DebrisDraw::sourceDepth,p.depth))return parent.check(false);
                p.sprite.spriteId=r.effect->spriteId;p.sprite.imageIndex=r.effect->imageIndex;
            }
            if constexpr(Kind==View::Kind::entryDither){
                if(!real(-200000,p.depth))return parent.check(false);
            }
            {
                DW_PROFILE_SCOPE(profileBirthRoomOrder);
                if(!parent.check(parent.room_.bind(h.slot,p.depth)))return false;
            }
            if constexpr(Kind!=View::Kind::timeField)if(!r.active)parent.activationPending_=true;
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
                [&](InstanceHandle live,bool publish){
                    if constexpr(Kind==View::Kind::terrain)if(preparedTerrain)
                        return parent.owner_.refreshPreparedTerrain(live,*parent.families_.flags(object(r)),
                            terrainBounds,terrainPixels,terrainPrecise,publish);
                    return parent.refresh(live,publish);
                })))return false;
            unsigned sourceId=0;
            if constexpr(Kind==View::Kind::terrain)sourceId=r.sourceInstanceId;
            if constexpr(Kind==View::Kind::player)sourceId=parent.layers_.playerSourceId();
            if constexpr(Kind==View::Kind::timeChange)sourceId=parent.pendingTimeChangeId_;
            if constexpr(Kind==View::Kind::entryDither)sourceId=r.sourceId;
            if constexpr(Kind==View::Kind::drone)sourceId=r.sourceId;
            constexpr bool projected=Kind==View::Kind::player||Kind==View::Kind::enemy||
                Kind==View::Kind::bullet||Kind==View::Kind::scriptEffect||Kind==View::Kind::debris||Kind==View::Kind::entryDither;
            {
                DW_PROFILE_SCOPE(profileInstanceLayerAttach);
                if(!parent.check(parent.layers_.birth(h,sourceId,p.depth,(state.flags&2u)!=0,projected)))return false;
            }
            if constexpr(Kind==View::Kind::player){
                if(!parent.world_){parent.initialPlayer_=&r;parent.initialPlayerKey_=local;}
            }
            return true;
        }
        bool birthAt(InstanceHandle local,const Record& r,RunnerImageValue x,RunnerImageValue y){
            // The store has already published its narrowed constructor fields.
            // Never emulate this as integer birth followed by a move.
            return parent.check(r.x.bits==x.bits&&r.y.bits==y.bits)&&birth(local,r);
        }
        bool retire(InstanceHandle local,const Record& r){InstanceHandle h;
            if(!key(local,r,h)||!parent.check(parent.room_.contains(h.slot)))return false;
            if(!parent.check(parent.owner_.retire(h)))return false;
            return parent.check(parent.room_.remove(h.slot)&&parent.layers_.retire(h));}
        bool markDestroyed(InstanceHandle local,const Record& r){InstanceHandle h;
            if(!key(local,r,h))return false;
            std::uint32_t flags;
            if(!parent.eligibilityFlags(h,flags)||!parent.check(parent.owner_.setExclusions(h,(flags&0x100003u)|1u)))return false;
            if(!parent.check(parent.room_.mark(h.slot)))return false;
            parent.destructionPending_=true;return true;}
        bool membership(InstanceHandle local,const Record& r,bool member){InstanceHandle h;
            return key(local,r,h)&&parent.check(parent.owner_.registerObject(h,member));}
        bool activation(InstanceHandle local,const Record& r,bool active){InstanceHandle h;
            if(!key(local,r,h)||!parent.check(parent.owner_.setActive(h,active,*parent.families_.flags(object(r)))))return false;
            if(!parent.check(parent.layers_.activation(h,active)))return false;
            parent.activationPending_=true;return true;}
        bool move(InstanceHandle local,const Record& r,RunnerImageValue x,RunnerImageValue y){InstanceHandle h;
            return key(local,r,h)&&parent.check(parent.owner_.move(h,x,y,*parent.families_.flags(object(r))));}
        bool depth(InstanceHandle local,const Record& r,MotionBinary64 value){InstanceHandle h;RunnerImageValue narrowed;
            if(!key(local,r,h)||!narrowed.assign(value))return parent.check(false);
            if(!parent.layers_.enabled())return parent.check(RunnerSpatialSplit::equal(narrowed,parent.properties_[h.slot].depth));
            if(!parent.check(parent.layers_.writeDepth(h,value)))return false;
            parent.properties_[h.slot].depth=narrowed;return true;
        }
        bool sprite(InstanceHandle local,const Record& r,int sprite){InstanceHandle h;
            return key(local,r,h)&&publishSprite(h,object(r),sprite);
        }
        bool publishSprite(InstanceHandle h,int object,int sprite){
            auto& p=parent.properties_[h.slot];
            if(p.sprite.spriteId!=sprite)parent.drawOrderValid_=false;
            return parent.check(parent.owner_.changeSpriteFromPack(h,p.sprite,p.timeline,p.cachedFrame,
                sprite,parent.sprites_,*parent.families_.flags(object)));}
#ifdef DOWNWELL_TERRAIN_RUN_STORE
        bool initializedTerrainRun(const AreaOneBuilder::TerrainRun& run,int tile,GameMakerRandom& random,
            AreaOneBuilder::EmitFunction backend,void* context){
            static_assert(Kind==View::Kind::terrain,"Generated terrain transaction only");
#ifndef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
            const auto* d=parent.objectDefaults_.get(run.objectId);
            const auto* flags=parent.families_.flags(unsigned(run.objectId));
            const auto* create=parent.terrainCreate_.get(run.objectId,tile);
            PreparedTerrainBirthBounds::Key first,last;
            // Bound count before computing the endpoint, including malformed
            // external resource fixtures. Unsupported geometry uses cell birth.
            const bool geometry=run.count&&run.count<=GameplayTerrainInstances::capacity&&
                (run.stepX==16||run.stepX==-16)&&
                parent.terrainBirthBounds_.key(run.objectId,run.x,run.y,first)&&
                parent.terrainBirthBounds_.key(run.objectId,run.x+int(run.count-1)*run.stepX,run.y,last);
            if(parent.valid()&&parent.world_&&parent.world_->terrainInstances()==&parent.terrain_&&
                d&&flags&&create&&(*flags&0x87u)==d->propertyFlags&&
                (d->spriteId==-1||d->frames<=65535u)&&geometry){
                typename decltype(parent.layers_)::TerrainRunBirth layer;
                bool prepared=false;
                return run.executeEach([&](const DeferredCreateInstance& command){
                    if(!backend(context,command))return false;
                    return parent.terrain_.appendInitializedCell(command.objectId,command.x,command.y,command.active,
                        [&](InstanceHandle local,GameplayTerrainInstances::CreateValues& values,bool& unbounded){
                            const InstanceHandle h{View::terrainBase+local.slot,local.generation};
                            Owner::State state;Owner::Box bounds;RunnerSpatialSplit::PixelBox pixels;
                            bool precise;PreparedTerrainBirthBounds::Key key;
                            if(!parent.terrainBirthBounds_.at(run.objectId,command.x,command.y,
                                state.x,state.y,bounds,precise,&pixels,&key))return parent.check(false);
                            if(!command.active)state.flags|=2u;
                            auto& p=parent.properties_[h.slot];
                            if(!parent.check(parent.owner_.attachTerrainRunCell(h,run.objectId,state,p,*d,
                                parent.families_,flags,bounds,pixels,precise,[&](RunnerImageValue depth){
                                    parent.room_.bindAllocatedTerrain(h.slot,depth);
                                    if(!command.active)parent.activationPending_=true;
                                    return true;
                                },key,prepared)))return false;
                            if(!parent.check(parent.layers_.birthAllocatedTerrain(h,p.depth,!command.active,&layer)))return false;
                            DW_PROFILE_SCOPE(profileTerrainCreate);
                            if(create->changedSprite)parent.drawOrderValid_=false;
                            parent.owner_.finishAdmittedTerrainCreate(h,p,*create,*flags);
                            values.spriteId=create->sprite;
                            if(create->writeIndex)values.imageIndex=create->index;
                            values.imageSpeed=0;
                            if(run.objectId==60&&(wallRadius(create->sprite)<0||wallRadius(create->sprite)>16))unbounded=true;
                            return true;
                        });
                });
            }
#endif
            return run.executeEach([&](const DeferredCreateInstance& command){
                return backend(context,command)&&parent.terrain_.appendInitialized(command.objectId,
                    command.x,command.y,command.active,tile,random);
            });
        }
        bool initializedTerrainBirth(InstanceHandle local,const Record& r,int tile,GameMakerRandom& random,
            GameplayTerrainInstances::CreateValues& values,bool& unbounded){
            static_assert(Kind==View::Kind::terrain,"Generated terrain transaction only");
            // birth admits the typed identity and synchronously publishes every
            // original ownership boundary. No callbacks can retire/reuse it
            // between that success and these source Create property writes.
            if(!birthMode<true>(local,r))return false;
            DW_PROFILE_SCOPE(profileTerrainCreate);
            if(r.objectId==99)return true;
            const InstanceHandle h{View::terrainBase+local.slot,local.generation};
#ifndef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
            if(const auto* plan=parent.terrainCreate_.get(r.objectId,tile)){
                if(plan->changedSprite)parent.drawOrderValid_=false;
                parent.owner_.finishAdmittedTerrainCreate(h,parent.properties_[h.slot],*plan,
                    *parent.families_.flags(r.objectId));
                values.spriteId=plan->sprite;
                if(plan->writeIndex)values.imageIndex=plan->index;
                values.imageSpeed=0;
                if(r.objectId==60&&(wallRadius(plan->sprite)<0||wallRadius(plan->sprite)>16))unbounded=true;
                return true;
            }
#endif
            return terrainCreateProperties(r.objectId,tile,false,random,[&](int sprite){
                if(!publishSprite(h,r.objectId,sprite))return false;
                values.spriteId=sprite;
                if(r.objectId==60&&(wallRadius(sprite)<0||wallRadius(sprite)>16))unbounded=true;
                return true;
            },[&](int index){
                RunnerImageValue value;
                if(!value.assignInteger(index)||!parent.check(parent.owner_.writeImageIndex(h,
                    parent.properties_[h.slot].sprite,value)))return false;
                values.imageIndex=index;return true;
            },[&](int speed){values.imageSpeed=speed;return true;});
        }
#endif
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
    Binding<OrdinaryTimeField,View::Kind::timeField> fields_{*this};
    Binding<GameplayGoalFields::Record,View::Kind::goalField> goals_{*this};
    Binding<GameplayExitControllers::Record,View::Kind::exitController> exits_{*this};
    Binding<GameplayWorld::BulletState,View::Kind::bullet> bullets_{*this};
    Binding<GameplayScriptSpawnInstances::Record,View::Kind::scriptEffect> scriptEffectsBinding_{*this};
    GameplayScriptSpawnInstances scriptEffects_{GameplayScriptSpawnInstances::Binding::bindMaskDestructionDepth(scriptEffectsBinding_)};
    Binding<GameplayDebrisInstances::Record,View::Kind::debris> debrisBinding_{*this};
    GameplayDebrisInstances debris_{GameplayDebrisInstances::Binding::bindDestruction(debrisBinding_)};
    Binding<NativeTimeChangeRegistration::Record,View::Kind::timeChange> timeChangeBinding_{*this};
    NativeTimeChangeRegistration timeChanges_{*this};
    Binding<GameplayEntryDithers::Record,View::Kind::entryDither> entryBinding_{*this};
    GameplayEntryDithers entries_{GameplayEntryDithers::Binding::bindDestruction(entryBinding_)};
    Binding<GameplayWorld::DroneBullets::Record,View::Kind::drone> droneBinding_{*this};
    GameplayWorld::DroneBullets drones_{GameplayWorld::DroneBullets::Binding::bindPositionedActor(droneBinding_)};
    std::uint32_t pendingTimeChangeId_=0;
    GameplayTerrainInstances terrain_;
public:
    GameplayCollisionOwner(const ObjectPack& objects,const SpriteCollisionPack& sprites,
        GameplayTerrainInstances::Presentation presentation={},GameplayTerrainInstances::WallCollision collision={},
        const RoomPack* rooms=nullptr):objects_(objects),sprites_(sprites),layers_(rooms),
        terrain_(GameplayTerrainInstances::Binding::bindDestruction(terrainBinding_),presentation,collision){
#if defined(DOWNWELL_TERRAIN_RUN_STORE) && !defined(DOWNWELL_SCALAR_TERRAIN_OWNERSHIP)
        if(!owner_.bindTerrainLifetimes(terrain_))return;
        if(!room_.bindTerrain(terrain_))return;
        if(!layers_.bindTerrain(terrain_,View::terrainBase))return;
#endif
#ifdef DOWNWELL_TERRAIN_RUN_STORE
        if(!terrain_.bindInitializedBirth(terrainBinding_)||!terrain_.bindInitializedRun(terrainBinding_))return;
#endif
        good_=sprites_.prepare(preparedSprites_,741)&&objectDefaults_.prepare(objects,sprites_)&&
            spriteGeometry_.prepare(sprites_)&&
            terrainBirthBounds_.prepare(objectDefaults_,sprites_)&&
            families_.bindInstanceFlags(objects)&&drawMetadata_.bind(objects)&&alarmMetadata_.bind(objects);
#if defined(DOWNWELL_TERRAIN_RUN_STORE) && !defined(DOWNWELL_SCALAR_TERRAIN_OWNERSHIP)
        if(good_)good_=terrainCreate_.prepare(objectDefaults_,sprites_);
        if(good_)good_=owner_.bindTerrainGeometry(terrainBirthBounds_);
#endif
        if(good_)for(unsigned selector=0;selector<2;++selector){
            terrainFamilies_[selector]=true;
            for(unsigned i=0;i<families_.size();++i)
                if(families_.isA(int(i),selector==0?60:93)&&i!=60&&i!=93&&i!=99)
                    terrainFamilies_[selector]=false;
        }
    }
    GameplayCollisionOwner(const GameplayCollisionOwner&)=delete;
    GameplayCollisionOwner& operator=(const GameplayCollisionOwner&)=delete;
    bool valid()const{return good_&&owner_.valid()&&terrain_.valid()&&scriptEffects_.valid()&&debris_.valid()&&timeChanges_.valid()&&entries_.valid()&&drones_.valid()&&layers_.valid();}
    unsigned pendingSpatialIndex()const{return owner_.pendingSpatialIndex();}
    bool serviceSpatialIndex(unsigned budget){return check(owner_.serviceSpatialIndex(budget));}
    bool attach(GameplayWorld& world){
        combatAlarmCaptureValid_=false;
        if(!owner_.watchCaptureRanges(View::enemyBase,View::terrainBase,
            View::debrisBase,View::capacity,combatAlarmCaptureDirty_))return check(false);
        if(!valid()||(world_&&world_!=&world)||(!world_&&
            (!initialPlayer_||world.findPlayer(initialPlayerKey_)!=initialPlayer_)))return false;
        if(!world_&&!world.bindTimeFieldLifecycle(OrdinaryTimeFields<64>::Binding::bind(fields_)))return check(false);
        if(!world_&&!world.bindGoalFieldLifecycle(GameplayGoalFields::Binding::bindMaskDestruction(goals_)))return check(false);
        if(!world_&&!world.bindExitControllerLifecycle(GameplayExitControllers::Binding::bind(exits_)))return check(false);
        if(!world_&&!world.bindBulletLifecycle(GameplayWorld::BulletLifecycleBinding::bindMaskDestruction(bullets_)))return check(false);
        if(!world_&&!world.bindScriptSpawnInstances(scriptEffects_))return check(false);
        if(!world_&&!world.bindDebrisInstances(debris_))return check(false);
        if(!world_&&!world.bindTimeChangeInstances(timeChanges_))return check(false);
        if(!world_&&!world.bindEntryDithers(entries_))return check(false);
        if(!world_&&!world.bindDroneBullets(drones_))return check(false);
        world_=&world;
        world.configureDroneEndStep([](void* context,long topQ12){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            if(!self.drones_.size())return true;
            MotionBinary64 top;
            if(!multiplyBinary64(MotionBinary64::fromInteger(topQ12),{0x3f300000u,0},top))return self.check(false);
            return self.check(self.owner_.visitRetainedRegistrationRange(View::droneBase,View::capacity,
                [&](InstanceHandle shared,int object,const Owner::State& state){
                    if(state.flags&0x100003u)return true;
                    View::Kind kind;InstanceHandle local;
                    return object==266&&View::decode(shared,kind,local)&&kind==View::Kind::drone&&
                        self.drones_.endStep(local,top,MotionBinary64::fromInteger(284),true);
                }));
        },this);
        world.configureRoomActorDraw([](const void* context,GameplayWorld::DrawActorVisitor visit,void* visitor){
            const auto& self=*static_cast<const GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            const auto draw=[&](InstanceHandle shared,RunnerImageValue depth){
                View::Kind kind;InstanceHandle local;
                if(!View::decode(shared,kind,local))return false;
                // Retained terrain/field composition is not part of this
                // visitor. Do not resolve or copy their records per frame.
                if(kind!=View::Kind::enemy&&kind!=View::Kind::bullet&&kind!=View::Kind::scriptEffect&&kind!=View::Kind::debris&&kind!=View::Kind::player&&kind!=View::Kind::entryDither)return true;
                std::uint32_t flags;
                if(!self.eligibilityFlags(shared,flags)||!View(*self.world_).contains(shared))return false;
                if(flags&0x100003u)return true;
                switch(kind){
                    case View::Kind::player:return visit(visitor,GameplayWorld::DrawActor::player,local,depth);
                    case View::Kind::enemy:return visit(visitor,GameplayWorld::DrawActor::enemy,local,depth);
                    case View::Kind::bullet:return visit(visitor,GameplayWorld::DrawActor::bullet,local,depth);
                    case View::Kind::scriptEffect:return visit(visitor,GameplayWorld::DrawActor::scriptEffect,local,depth);
                    case View::Kind::debris:return visit(visitor,GameplayWorld::DrawActor::debris,local,depth);
                    case View::Kind::entryDither:return visit(visitor,GameplayWorld::DrawActor::entryDither,local,depth);
                    default:return true;
                }
            };
            if(self.layers_.enabled())return self.layers_.visit([&](InstanceHandle h,RunnerLayerDraw::Element& out){
                if(!self.eligibilityFlags(h,out.flags))return false;
                View::Kind kind;InstanceHandle local;if(!View::decode(h,kind,local))return false;
                // Retained scenery/field passes own these draws. Preserve the
                // inactive flag (which terminates a layer), but avoid event
                // and sprite lookups for every terrain tile each frame.
                if(kind!=View::Kind::enemy&&kind!=View::Kind::bullet&&kind!=View::Kind::scriptEffect&&
                    kind!=View::Kind::debris&&kind!=View::Kind::player&&kind!=View::Kind::entryDither)return true;
                const unsigned metadata=self.drawMetadata_.get(self.owner_.objectId(h));
                out.object=(metadata&ObjectDrawMetadata<386>::exists)!=0;
                out.drawEvent=(metadata&ObjectDrawMetadata<386>::drawEvent)!=0;
                out.sprite=self.properties_[h.slot].sprite.spriteId>=0;return true;
            },[&](InstanceHandle h,int depth){RunnerImageValue value;return real(depth,value)&&draw(h,value);});
            return self.room_.visitDrawChecked([&](unsigned slot){return draw(self.owner_.currentHandle(slot),self.properties_[slot].depth);});
        },this,[](const void* context,GameplayWorld::DrawActorVisitor visit,void* visitor){
            const auto& self=*static_cast<const GameplayCollisionOwner*>(context);
            if(!self.valid()||!self.world_)return false;
#ifdef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
            return self.world_->visitRoomActors([&](auto kind,auto key,auto depth){return visit(visitor,kind,key,depth);});
#else
            const auto capture=self.owner_.captureRevision(),layers=self.layers_.drawRevision();
            if(self.drawOrderValid_&&capture!=0xffffffffu&&layers!=0xffffffffu&&
               capture==self.drawCaptureRevision_&&layers==self.drawLayerRevision_){
                for(unsigned i=0;i<self.drawCount_;++i){const auto& e=self.drawOrder_[i];
                    if(!visit(visitor,e.kind,e.local,e.depth))return false;
                }
                return true;
            }
            self.drawOrderValid_=false;self.drawCount_=0;
            // Fill during the original live traversal, not before dispatch:
            // failed callbacks retain exactly the same successful prefix.
            const bool ok=self.world_->visitRoomActors([&](auto kind,auto key,auto depth){
                if(self.drawCount_==drawCapacity)return false;
                self.drawOrder_[self.drawCount_++]={kind,key,depth};
                return visit(visitor,kind,key,depth);
            });
            self.drawCaptureRevision_=capture;self.drawLayerRevision_=layers;
            self.drawOrderValid_=ok&&self.valid()&&capture==self.owner_.captureRevision()&&
                layers==self.layers_.drawRevision();
            return ok;
#endif
        });
        if(!owner_.watchSnapshotRange(View::terrainBase,View::timeFieldBase,terrainSnapshotDirty_))return check(false);
        if(!room_.prepareRetainedAnimation(View::terrainBase,View::timeFieldBase))return check(false);
        world.configureRoomAnimation([](void* context){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            return self.check(self.room_.visitAnimationRetainedChecked(
                View::terrainBase,View::timeFieldBase,self.terrainSnapshotDirty_,[&](unsigned slot){
                const auto shared=self.owner_.currentHandle(slot);
                // The conservative dirty fallback keeps terrain snapshots in
                // room order. Unchanged terrain has no animation dispatch in
                // this adapter and already retains the identical snapshot.
                if(shared.slot>=View::terrainBase&&shared.slot<View::timeFieldBase){
                    DW_PROFILE_COUNT(counterTerrainSnapshots);
                    const auto* terrain=self.world_->terrainInstances();
                    return terrain&&terrain->find({shared.slot-View::terrainBase,shared.generation})&&
                        self.owner_.capturePrevious(shared);
                }
                View::Kind kind;InstanceHandle local;
                DW_PROFILE_COUNT(counterActorSnapshots);
                if(!View::decode(shared,kind,local)||!View(*self.world_).contains(shared)||
                    !self.owner_.capturePrevious(shared))return false;
                switch(kind){
                    case View::Kind::enemy:{
                        DW_PROFILE_SCOPE(profileAnimationDispatch);
                        return self.world_->advanceEnemyRoomAnimation(local);
                    }
                    case View::Kind::bullet:{
                        DW_PROFILE_SCOPE(profileAnimationDispatch);
                        return self.world_->advanceBulletRoomAnimation(local);
                    }
                    case View::Kind::scriptEffect:{
                        DW_PROFILE_SCOPE(profileAnimationDispatch);
                        return self.world_->advanceScriptRoomAnimation(local);
                    }
                    case View::Kind::debris:{
                        DW_PROFILE_SCOPE(profileAnimationDispatch);
                        return self.world_->advanceDebrisRoomAnimation(local);
                    }
                    case View::Kind::drone:return self.drones_.advanceAnimation(local);
                    default:return true;
                }
            }));
        },this);
        world.configurePhaseBoundary([](void* context,GameplayWorld::PhaseBoundary phase){
            auto& owner=*static_cast<GameplayCollisionOwner*>(context);
            return owner.reconcileActivation()&&owner.layers_.serviceDepths()&&
                (phase!=GameplayWorld::PhaseBoundary::roomEntry||owner.prepareDraw());
        },this);
        world.configureEnemyStepOrder([](void* context,GameplayWorld::EnemyStepOrder& snapshot){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            return self.check(self.owner_.visitRetainedRegistrationRange(View::enemyBase,View::breakableBase,
                [&](InstanceHandle shared,int object,const Owner::State& state){
                View::Kind kind;InstanceHandle local;
                if(!View::decode(shared,kind,local))return false;
                if(kind!=View::Kind::enemy||(state.flags&0x100003u))return true;
                return self.world_->resolveEnemyLifetime(local)&&snapshot.append(local.slot,local.generation,object);
            }));
        },this,[](void* context){
            const auto& self=*static_cast<GameplayCollisionOwner*>(context);
            return self.valid()?self.owner_.captureRevision():0xffffffffu;
        });
        world.configureRoomAlarms([](void* context){
            DW_PROFILE_SCOPE(profileAlarmCapture);
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            auto& phase=self.combatAlarmPhase_;
            const bool reused=self.combatAlarmCaptureValid_&&!self.combatAlarmCaptureDirty_;
            if(!reused){
                phase.clear();
                auto capture=[&](InstanceHandle h,int object,const Owner::State& state){
                    return self.resolves(h)&&((state.flags&0x100003u)||phase.append(h.slot,h.generation,object));
                };
                // Only implemented families are admitted. Avoid scanning every
                // terrain record for this partial combat alarm traversal.
                if(!self.check(self.owner_.visitRetainedRegistrationRange(View::enemyBase,View::terrainBase,capture)&&
                    self.owner_.visitRetainedRegistrationRange(View::debrisBase,View::capacity,capture)))return false;
                // Save before callbacks: mutation during dispatch invalidates
                // reuse next tick without changing this phase's captured set.
                self.combatAlarmCaptureDirty_=false;
                self.combatAlarmCaptureValid_=true;
            }
            return self.dispatchCapturedAlarms<true>(phase,[&](InstanceHandle h,unsigned alarm,const PackedObjectEvent&){
                View::Kind kind;InstanceHandle local;if(!View::decode(h,kind,local))return false;
                return kind==View::Kind::enemy?self.world_->advanceEnemyRoomAlarm(local,alarm):
                    kind==View::Kind::breakable?self.world_->advanceBoxRoomAlarm(local,alarm):
                    kind==View::Kind::debris?self.world_->advanceBoundDebrisRoomAlarm(local,alarm):
                    kind==View::Kind::timeChange?self.world_->advanceTimeChangeRoomAlarm(local,alarm):
                    kind==View::Kind::drone?(alarm==0&&self.drones_.advanceAlarm(local)):
                    kind==View::Kind::entryDither&&self.entries_.advanceAlarm(local,alarm);
            },reused,&self.combatAlarmMask_);
        },this);
        world.configureDebrisStepOrder([](void* context,GameplayWorld::DebrisStepOrder& phase){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            return self.check(self.owner_.visitRetainedRegistrationRange(View::debrisBase,View::timeChangeBase,
                [&](InstanceHandle h,int object,const Owner::State& state){
                    if(state.flags&0x100003u)return true;
                    View::Kind kind;InstanceHandle local;
                    return View::decode(h,kind,local)&&kind==View::Kind::debris&&self.resolves(h)&&
                        phase.append(local.slot,local.generation,object);
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
        if(!check(visitEligibility([&](InstanceHandle h,int object,std::uint32_t flags){
            return resolves(h)&&((flags&0x100003u)||snapshot.append(h.slot,h.generation,object));
        })))return false;
        snapshot.sort();
        const unsigned count=snapshot.size();
        for(unsigned i=0;i<count;++i){
            const auto h=snapshot.handle(i);
            std::uint32_t flags;
            // Destruction/retirement by a preceding callback suppresses this
            // lifetime; a new occupant of the slot cannot inherit its turn.
            if(!eligibilityFlags(h,flags)||(flags&0x100003u))continue;
            if(!check(resolves(h)))return false;
            PackedObjectEvent event;
            if(!objects_.findEvent(owner_.objectId(h),3,int(subtype),event))continue;
            if(!check(execute(h,event))||!valid())return false;
        }
        return true;
    }
    GameplayTerrainInstances& terrain(){return terrain_;}
    GameplayWorld::DroneBullets& droneBullets(){return drones_;}
    // Caller-resident phase snapshot, shared by all twelve alarm indices.
    // advanceAndDispatch owns the actor's alarm storage: decrement once and
    // execute the selected event synchronously only on reaching zero. Objects
    // without that alarm event are absent from the original event table.
    // Like dispatchStepPhase, this is a captured-lifetime adapter; remaining
    // original same-phase creation/list-mutation cases require separate proof.
    template<unsigned Capacity,class AdvanceAndDispatch>
    bool dispatchAlarmPhase(RunnerStepSnapshot<Capacity>& snapshot,AdvanceAndDispatch advanceAndDispatch){
        DW_PROFILE_SCOPE(profileAlarmCapture);
        if(!valid()||!world_)return check(false);
        snapshot.clear();
        if(!check(visitEligibility([&](InstanceHandle h,int object,std::uint32_t flags){
            return resolves(h)&&((flags&0x100003u)||snapshot.append(h.slot,h.generation,object));
        })))return false;
        return dispatchCapturedAlarms(snapshot,advanceAndDispatch);
    }
private:
    // The original phase leaves negative counters untouched (0x509e15).
    // Read the live compact stores, not a mask captured before callbacks:
    // an earlier alarm can arm a later counter in this very phase. Zero is
    // still work: it must retire to -1. Other families retain their full
    // adapter path, including effect-owner validity checks.
    bool combatAlarmPending(InstanceHandle h,unsigned alarm,bool& pending)const{
        pending=true;
        if(!world_)return false;
        if(h.slot>=View::enemyBase&&h.slot<View::breakableBase){
            const auto* enemy=world_->resolveEnemyLifetime({h.slot-View::enemyBase,h.generation});
            if(!enemy)return false;
            if(alarm==0)pending=enemy->alarm0>=0;
            else if(alarm==1)pending=enemy->alarm1>=0;
            return true;
        }
        if(h.slot>=View::breakableBase&&h.slot<View::terrainBase){
            const auto* box=world_->findBreakable({h.slot-View::breakableBase,h.generation});
            if(!box)return false;
            if(alarm==0)pending=box->alarm0>=0;
            return true;
        }
        return resolves(h);
    }
    template<bool SkipInactive=false,unsigned Capacity,class AdvanceAndDispatch>
    bool dispatchCapturedAlarms(RunnerStepSnapshot<Capacity>& snapshot,AdvanceAndDispatch advanceAndDispatch,
        bool alreadySorted=false,unsigned* retainedMask=nullptr){
        DW_PROFILE_SCOPE(profileAlarmScan);
        if(!alreadySorted){DW_PROFILE_SCOPE(profileAlarmSort);snapshot.sort();}
        const unsigned count=snapshot.size();
        const auto bindingRevision=owner_.bindingRevision();
        unsigned capturedAlarms=alreadySorted&&retainedMask?*retainedMask:0;
        // Capture admitted resource-backed objects and sort grouped them.
        // No callbacks occur here, so one lookup per group safely identifies
        // entire event-table columns that cannot dispatch any captured entry.
        // A reused capture has the same ordered objects and immutable resource
        // masks. Retain this union with it instead of rescanning every tick.
        // Binding changes during dispatch still disable absent-column skips.
        if(!alreadySorted||!retainedMask){
            int previousObject=-1;
            for(unsigned i=0;i<count;++i){
                const int object=snapshot.object(i);
                if(object==previousObject)continue;
                unsigned mask;
                if(!check(alarmMetadata_.mask(object,mask)))return false;
                capturedAlarms|=mask;previousObject=object;
            }
            if(retainedMask)*retainedMask=capturedAlarms;
        }
        for(unsigned alarm=0;alarm<AlarmBank::count;++alarm){
          if(!(capturedAlarms&(1u<<alarm))&&owner_.bindingsUnchanged(bindingRevision))continue;
          int cachedObject=-1;unsigned alarmMask=0;
          PackedObjectEvent event;bool eventCached=false,eventFound=false;
          for(unsigned i=0;i<count;++i){
            const auto h=snapshot.handle(i);
            bool haveFlags=false;std::uint32_t flags;
            int object=snapshot.object(i);
            // A captured object's absent event needs no instance lookup while
            // bindings are unchanged. Any birth, retire, rebind or clear makes
            // the rest of this pass use live lookup, including later alarms.
            if(!owner_.bindingsUnchanged(bindingRevision)){
                haveFlags=eligibilityFlags(h,flags);
                if(!haveFlags||(flags&0x100003u))continue;
                object=owner_.objectId(h);
            }
            if(object!=cachedObject){
                if(!check(alarmMetadata_.mask(object,alarmMask)))return false;
                cachedObject=object;eventCached=false;
            }
            if(!(alarmMask&(1u<<alarm))){
                // No callbacks occur in an absent event group. While bindings
                // match capture, every remaining member has the same resource
                // and can be skipped together. After mutation retain live reads.
                if(owner_.bindingsUnchanged(bindingRevision))i=snapshot.objectGroupEnd(i)-1;
                continue;
            }
            if(!haveFlags)haveFlags=eligibilityFlags(h,flags);
            if(!haveFlags||(flags&0x100003u))continue;
            // Capture validated every admitted lifetime. Revalidate the typed
            // store before each real event, not for absent event-table entries.
            if constexpr(SkipInactive){
                bool pending;
                if(!check(combatAlarmPending(h,alarm,pending)))return false;
                if(!pending)continue;
            }else if(!check(resolves(h)))return false;
            // Only immutable resource lookup is shared. A callback may retire,
            // deactivate or rebind another captured instance, so all live
            // checks and the current object read remain inside this loop.
            if(!eventCached){
                eventFound=alarmMetadata_.findEvent(object,alarm,event);eventCached=true;
            }
            if(!eventFound)continue;
            {DW_PROFILE_SCOPE(profileAlarmCallback);
                if(!check(advanceAndDispatch(h,alarm,event))||!valid())return false;}
          }
        }
        return true;
    }
public:
    GameplayScriptSpawnInstances& scriptEffects(){return scriptEffects_;}
    GameplayDebrisInstances& debris(){return debris_;}
    NativeTimeChangeRegistration& timeChanges(){return timeChanges_;}
    GameplayEntryDithers& entryDithers(){return entries_;}
    // Until every native dynamic family participates in the source ID stream,
    // this entry point accepts only an explicitly supplied original ID. Never
    // allocate a family-local substitute. The caller owns effects/speed until
    // registered lifetimes have been retired or the room has been cleared.
    template<class Pause> bool createTimeChangeWithSourceId(TimeChangeEffects<8>& effects,
        std::uint32_t id,MotionBinary64 x,MotionBinary64 y,int& speed,Pause pause){
        if(!valid()||!world_||pendingTimeChangeId_||!id||id>0x7fffffffu)return check(false);
        if(!effects.boundTo(timeChanges_)&&!effects.bind(timeChanges_))return check(false);
        pendingTimeChangeId_=id;
        const bool ok=world_->createTimeChange(effects,x,y,speed,pause);
        pendingTimeChangeId_=0;return check(ok);
    }
    bool birth(InstanceHandle key,const NativeTimeChangeRegistration::Record& record,std::uint32_t& id){
        if(!pendingTimeChangeId_)return check(false);
        id=pendingTimeChangeId_;
        const bool ok=timeChangeBinding_.birth(key,record);
        pendingTimeChangeId_=0;return ok;
    }
    bool markDestroyed(InstanceHandle key,const NativeTimeChangeRegistration::Record& record){
        return timeChangeBinding_.markDestroyed(key,record);
    }
    bool activation(InstanceHandle key,const NativeTimeChangeRegistration::Record& record,bool active){
        return timeChangeBinding_.activation(key,record,active);
    }
    bool retire(InstanceHandle key,const NativeTimeChangeRegistration::Record& record){
        return timeChangeBinding_.retire(key,record);
    }
    const Owner::State* state(InstanceHandle key)const{return valid()?owner_.resolve(key):nullptr;}
    const Owner::Properties* properties(InstanceHandle key)const{return state(key)?&properties_[key.slot]:nullptr;}
    // These take selected shared collision handles, not box-array indices.
    // Game fields can still be written after a destruction mark; only actual
    // room cleanup invalidates the handle. No activation/retirement happens.
    bool projectileWallParent(InstanceHandle h,int& parent)const{
        PackedObject object;
        if(!valid()||!resolves(h)||!objects_.findObject(owner_.objectId(h),object))return false;
        parent=object.parentId;return true;
    }
    bool projectileBoxHealth(InstanceHandle h,MotionBinary64& health)const{
        View::Record record;
        if(!valid()||!resolves(h)||!View(*world_).resolve(h,record)||!record.breakable)return false;
        health=record.breakable->state.wallHp;return true;
    }
    bool readBoxDestruction(InstanceHandle h,BoxDestructionScript::Caller& out)const{
        View::Record record;const auto* position=state(h);
        if(!position||!resolves(h)||!View(*world_).resolve(h,record)||!record.breakable)return false;
        out={record.breakable->state,position->x.promoted(),position->y.promoted()};return true;
    }
    bool projectileBoxHealthWrite(InstanceHandle h,MotionBinary64 health){
        View::Kind kind;InstanceHandle local;
        return valid()&&resolves(h)&&View::decode(h,kind,local)&&kind==View::Kind::breakable&&
            world_->publishBoxHealth(local,health);
    }
    bool projectileBoxHit(InstanceHandle h){
        View::Kind kind;InstanceHandle local;
        return valid()&&resolves(h)&&View::decode(h,kind,local)&&kind==View::Kind::breakable&&
            world_->publishBoxHit(local);
    }
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
        InstanceHandle self;
        if(!terrainQuery(object)||!masks.valid()||!View::encode(View::Kind::terrain,local,self)||
            !resolves(self))return check(false);
        return registeredPlaceMeeting(self,object,x,y,masks,hit);
    }
    // Object 370 uses sParentSolid (56), including thin platforms and boxes.
    // This queries registered lifetimes only; it is not evidence that every
    // member of family 56 is already admitted by the live scene generator.
    bool debrisPlaceMeeting(InstanceHandle local,MotionBinary64 x,MotionBinary64 y,
        const SpriteMaskPack& masks,bool& hit){
        InstanceHandle self;
        if(!valid()||!world_||!masks.valid()||!View::encode(View::Kind::debris,local,self)||
           !resolves(self))return check(false);
        return registeredPlaceMeeting(self,56,x,y,masks,hit);
    }
    bool debrisFrozen(InstanceHandle local,bool pTimeStop,int roomWidth,const SpriteMaskPack& masks,bool& frozen){
        InstanceHandle self;
        const auto* record=debris_.find(local);
        if(!valid()||!world_||!record||!record->effect||!View::encode(View::Kind::debris,local,self)||!resolves(self))return check(false);
        const auto& effect=*record->effect;bool allowed=false;
        if(!TimeStopBound::evaluate(pTimeStop,effect.x.promoted(),effect.y.promoted(),roomWidth,
            [&](auto x,auto y,bool& hit){
                return masks.valid()&&registeredPlaceMeeting(self,82,x,y,masks,hit);
            },allowed))return check(false);
        frozen=!allowed;return true;
    }
    // Box Step's final alarmStop(0), including after a destroy request. Use
    // the retained published shape, not a point or a global blanket freeze.
    bool stopBoxAlarm0(InstanceHandle local,bool pTimeStop,int roomWidth,const SpriteMaskPack& masks){
        InstanceHandle self;
        if(!valid()||!world_||!View::encode(View::Kind::breakable,local,self)||!resolves(self))return check(false);
        const auto* state=owner_.resolve(self);if(!state)return check(false);
        return check(world_->stopBoxAlarm0(local,[&](bool& allowed){
            return TimeStopBound::evaluate(pTimeStop,state->x.promoted(),state->y.promoted(),roomWidth,
                [&](auto x,auto y,bool& hit){return masks.valid()&&registeredPlaceMeeting(self,82,x,y,masks,hit);},allowed);
        }));
    }
    // objPlayer_n Begin Step queries parentTimeField using private xx/yy,
    // while collision bounds/mask/transforms belong to the registered player.
    // placeVm temporarily publishes the requested position and restores it.
    bool playerTimeFieldMeeting(MotionBinary64 x,MotionBinary64 y,const SpriteMaskPack& masks,bool& hit){
        InstanceHandle self;
        if(!valid()||!world_||!masks.valid()||
            !View::encode(View::Kind::player,world_->playerHandle(),self)||!resolves(self))return check(false);
        return registeredPlaceMeeting(self,82,x,y,masks,hit);
    }
    struct RegisteredFieldContact {
        bool found=false;
        InstanceHandle handle{}; // Internal generation key, NOT a VM instance ID.
        RunnerImageValue x{},y{};
    };
    bool playerTimeFieldSelection(MotionBinary64 x,MotionBinary64 y,const SpriteMaskPack& masks,
        RegisteredFieldContact& result){
        InstanceHandle self;
        if(!valid()||!world_||!masks.valid()||
            !View::encode(View::Kind::player,world_->playerHandle(),self)||!resolves(self))return check(false);
        Owner::Result selected;
        if(!registeredPlaceSelection(self,82,x,y,masks,selected))return false;
        RegisteredFieldContact next;
        if(selected.found){
            const auto* state=owner_.resolve(selected.handle);
            if(!state||!resolves(selected.handle))return check(false);
            next={true,selected.handle,state->x,state->y};
        }
        result=next;return true;
    }
    // Selected global Step entry, not a pool-local sweep. The caller supplies
    // the original per-instance TimeStopBound result through a checked adapter.
    template<unsigned Capacity,class Stopped>
    bool stepDebris(DebrisEffects<Capacity>& effects,InstanceHandle shared,
        typename DebrisEffects<Capacity>::Phase phase,const SpriteMaskPack& masks,Stopped stopped){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||!View::decode(shared,kind,local)||kind!=View::Kind::debris||!resolves(shared))return check(false);
        return check(world_->stepDebrisRoomEffect(effects,local,phase,stopped,
            [&](InstanceHandle key,const DebrisAnimation&,MotionBinary64 x,MotionBinary64 y,bool& hit){
                return debrisPlaceMeeting(key,x,y,masks,hit);
            }));
    }
private:
    bool registeredPlaceMeeting(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        const SpriteMaskPack& masks,bool& hit){
        Owner::Result result;
        if(!registeredPlaceSelection(self,object,x,y,masks,result))return false;
        hit=result.found;return true;
    }
    bool registeredPlaceSelection(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        const SpriteMaskPack& masks,Owner::Result& result){
        auto update=[&](InstanceHandle h,bool publish){return resolves(h)&&refresh(h,publish);};
        if(!owner_.placeVm(self,object,x,y,false,families_,update,
            [&](InstanceHandle a,InstanceHandle b,bool publish,bool& found){
                return resolves(a)&&resolves(b)&&owner_.collideCompatibility(a,b,
                    properties_[a.slot].sprite,properties_[b.slot].sprite,sprites_,masks,
                    RunnerMaskDispatch{},publish,update,inverseTrig,roundPixel,found);
            },result))return check(false);
        return true;
    }
public:
    // self is a shared collision handle, not a terrain-local storage slot.
    // Keep the completed-family gate: projectile parent families must not be
    // queried until all of their lifetimes and setters are owned here.
    bool terrainCollisionPoint(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        bool precise,bool notMe,const SpriteMaskPack& masks,Owner::Result& result){
        if(!terrainQuery(object))return check(false);
        return registeredCollisionPoint(self,object,x,y,precise,notMe,masks,result);
    }
    bool projectileSolidPoint(InstanceHandle self,MotionBinary64 x,MotionBinary64 y,
        bool notMe,const SpriteMaskPack& masks,Owner::Result& result){
        if(!valid()||!world_)return check(false);
        return registeredCollisionPoint(self,59,x,y,false,notMe,masks,result);
    }
private:
    bool registeredCollisionPoint(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        bool precise,bool notMe,const SpriteMaskPack& masks,Owner::Result& result){
        if(!masks.valid())return check(false);
        auto update=[&](InstanceHandle h,bool publish){return resolves(h)&&refresh(h,publish);};
        return check(owner_.collisionPointVm(self,object,x,y,notMe,families_,
            [&](InstanceHandle h,RunnerImageValue px,RunnerImageValue py,bool& hit){
                return resolves(h)&&owner_.pointCandidate(h,properties_[h.slot].sprite,px,py,
                    true,precise,sprites_,masks,update,inverseTrig,roundPixel,hit);
            },result));
    }
public:
    bool terrainNonPreciseLine(InstanceHandle self,int object,MotionBinary64 x1,MotionBinary64 y1,
        MotionBinary64 x2,MotionBinary64 y2,bool notMe,Owner::Result& result){
        if(!terrainQuery(object))return check(false);
        return registeredNonPreciseLine(self,object,x1,y1,x2,y2,notMe,result);
    }
    // scrBulCheckSolid uses parent solid 59 and parent enemy 83, not nearest
    // distance or an enemy-first sweep. Query registered wall, box and enemy
    // lifetimes through the original family/list/tree dispatcher. This does
    // not run impact events or admit parentShootableThing shape queries.
    bool projectileNonPreciseLine(InstanceHandle self,int object,MotionBinary64 x1,MotionBinary64 y1,
        MotionBinary64 x2,MotionBinary64 y2,bool notMe,Owner::Result& result){
        if(!valid()||!world_||(object!=59&&object!=83))return check(false);
        return registeredNonPreciseLine(self,object,x1,y1,x2,y2,notMe,result);
    }
private:
    bool registeredNonPreciseLine(InstanceHandle self,int object,MotionBinary64 x1,MotionBinary64 y1,
        MotionBinary64 x2,MotionBinary64 y2,bool notMe,Owner::Result& result){
        RunnerLineArguments args;
        if(!args.assign(x1,y1,x2,y2))return check(false);
        auto update=[&](InstanceHandle h,bool publish){return resolves(h)&&refresh(h,publish);};
        return check(owner_.collisionLine(self,object,args,notMe,false,families_,update,
            [&](InstanceHandle h,const RunnerLineArguments& line,bool& hit){
                return resolves(h)&&owner_.nonPreciseLineCandidate(h,properties_[h.slot].sprite,
                    line,true,sprites_,update,hit);
            },result));
    }
public:
    bool reconcileActivation(){
        if(!valid()||!world_)return check(false);
        if(!activationPending_)return true;
        const bool result=room_.reconcileChecked([&](unsigned slot){
            std::uint32_t flags;
            if(!eligibilityFlags(owner_.currentHandle(slot),flags)){good_=false;return true;}
            return (flags&2u)!=0;
        },[&](unsigned slot,bool registered){
            // Writes while inactive may have been consumed by an intervening
            // active-room pass. Promotion must refresh that retained snapshot.
            if(registered&&slot>=View::terrainBase&&slot<View::timeFieldBase)terrainSnapshotDirty_=true;
            const auto h=owner_.currentHandle(slot);
            return good_&&owner_.registerObject(h,registered)&&layers_.reconcile(h);
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
                const auto r=owner.terrain_.find(local);if(!r)return false;
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
        DW_PROFILE_SCOPE(profileDrawCleanup);
        const bool cleaned=room_.visitMarkedCleanupChecked([&](unsigned slot){
            const auto h=owner_.currentHandle(slot);std::uint32_t flags;
            if(!eligibilityFlags(h,flags)||!(flags&1u))return false;
            View::Record record;
            if(!View(*world_).resolve(h,record))return false;
            if(!retire(h,record)||!valid()||owner_.contains(h))return false;
            return !View(*world_).contains(h);
        });
        if(!check(cleaned))return false;
        destructionPending_=false;return true;
    }
    // Invoke at Draw entry or after room-entry reconciliation, never at an
    // ordinary inter-Step reconciliation boundary.
    // Retire through the typed stores, retaining identity across compaction.
    // Player destruction remains unsupported rather than silently discarded.
    bool prepareDraw(){return prepareDrawWithEffects([&](InstanceHandle key,const View::Record&){return world_->retireScriptRoomEffect(key);});}
    template<unsigned Capacity> bool prepareDraw(ScriptSpawnEffects<Capacity>& effects){
        if(!effects.boundTo(scriptEffects_))return check(false);
        return prepareDrawWithEffects([&](InstanceHandle local,const View::Record& record){
            return record.scriptEffect&&scriptEffects_.retireMarked(local,*record.scriptEffect,effects);
        });
    }
private:
    template<class RetireEffect> bool prepareDrawWithEffects(RetireEffect retireEffect){
        return cleanupDestroyed([&](InstanceHandle shared,const View::Record& record){
            View::Kind kind;InstanceHandle local;
            if(!View::decode(shared,kind,local))return false;
            switch(kind){
                case View::Kind::terrain:return record.terrain==terrain_.find(local)&&terrain_.retireMarked(local);
                case View::Kind::enemy:return record.enemy==world_->resolveEnemyLifetime(local)&&world_->retireMarkedEnemy(local);
                case View::Kind::breakable:return record.breakable==world_->findBreakable(local)&&world_->retireMarkedBreakable(local);
                case View::Kind::goalField:return record.goalField==world_->goalFieldInstances().find(local)&&world_->retireMarkedGoalField(local);
                case View::Kind::bullet:return record.bullet==world_->resolveBulletLifetime(local)&&world_->retireMarkedBullet(local);
                case View::Kind::scriptEffect:return retireEffect(local,record);
                case View::Kind::debris:return record.debris&&world_->retireDebrisRoomEffect(local);
                case View::Kind::timeChange:return record.timeChange&&world_->retireTimeChangeRoomEffect(local);
                case View::Kind::entryDither:return record.entryDither&&entries_.retire(local);
                case View::Kind::drone:return record.drone&&drones_.retire(local);
                default:return false;
            }
        });
    }
};
}
#endif

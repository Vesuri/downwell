#ifndef DOWNWELL_CORE_GAMEPLAY_COLLISION_OWNER_H
#define DOWNWELL_CORE_GAMEPLAY_COLLISION_OWNER_H
#include "GameplayCollisionView.h"
#include "BlastSkeletonContact.h"
#include "BlastWallContact.h"
#include "BlastShootableContact.h"
#include "BoxDestructionScript.h"
#include "MainRoomLifetime.h"
#include "RunnerSpatialInstances.h"
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
#include "RunnerFullProperties.h"
#include "NativeInstanceTable.h"
#include "RunnerCollisionEventContext.h"
#include "RunnerCollisionDispatch.h"
#include "NativeEnemyAlarmRoster.h"
namespace downwell {
#ifdef DOWNWELL_DRAW_CACHE_AUDIT
// Optional diagnostic observer; absent from normal code and simulation state.
void auditActorDrawCache(unsigned outcome,unsigned actors,bool captureChanged,bool layersChanged);
#endif
#ifdef DOWNWELL_LOCAL_DRAW_CAPTURE
extern volatile unsigned localDrawCaptureMode;
#endif
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
#else
    using Owner=RunnerSpatialInstances<View::capacity,View::capacity*2,true,View::terrainBase,View::timeFieldBase>;
#endif
private:
    Owner owner_;
    // Published binary32 setters are not the actors' integer drawing positions.
    // Terrain owns immutable coordinates in runs, not duplicate slot records.
    struct NativePosition {
        RunnerImageValue x,y;
        Owner::Sprite sprite;
        Owner::Box bounds{};
        // +0x180 caches special-sprite frame count, not the collision frame.
        // Ordinary sprites bypass its getter path (runner-image-fields.md).
        RunnerImageValue timeline{},cachedFrame{0xbf800000u};
        std::uint32_t appearanceFlags=8u;
    };
    NativePosition nativePositions_[View::capacity-(View::timeFieldBase-View::terrainBase)]{};
    struct NativeWallQuery {
        InstanceHandle handle;Owner::State state;Owner::Sprite sprite;
        int dx=0,dy=0,cacheX=0,cacheY=0;
    };
    NativeWallQuery* wallQuery_=nullptr; // Borrowed only inside a non-reentrant query.
    using NativeDirtyQueue=RunnerSpatialDirtyQueue<View::capacity>;
    NativeDirtyQueue nativeSpatialDirty_,nativeCollisionDirty_;
    unsigned nativeCollisionRetained_[View::capacity]{};
    bool nativeDirty(unsigned slot,int object,std::uint32_t flags){
        const auto* objectFlags=families_.flags(unsigned(object));
        if(!objectFlags||slot>=View::capacity)return check(false);
        if(NativeDirtyQueue::shouldQueue(true,*objectFlags,flags)){
            nativeSpatialDirty_.mark(slot);
            if(*objectFlags&8u)nativeCollisionDirty_.mark(slot);
        }
        return true;
    }
    bool nativeBirthQueues(unsigned slot,int object){
        const auto* defaults=objectDefaults_.get(object);
        // Binding setters precede ancestor preparation. Repeated prepends
        // during that synchronous binding leave the same queue head.
        if(!defaults||!nativeDirty(slot,object,defaults->initialFlags)||
            !families_.prepareBirth(unsigned(object)))return check(false);
        if(*families_.flags(unsigned(object))&8u)nativeCollisionDirty_.mark(slot);
        return true;
    }
    bool nativeRetireQueues(unsigned slot){
        if(auto* state=nativePosition(slot))if(state->appearanceFlags&0x8000u){
            auto box=state->bounds;
            if(RunnerSpatialSplit::less(box.right,box.left)){const auto v=box.left;box.left=box.right;box.right=v;}
            if(RunnerSpatialSplit::less(box.bottom,box.top)){const auto v=box.top;box.top=box.bottom;box.bottom=v;}
            if(!check(owner_.externalSpatialPublication([&](auto& index){
                bool removed;return index.remove(box,slot,true,removed);
            })))return false;
            state->appearanceFlags&=~0x8000u;
        }
        nativeSpatialDirty_.remove(slot);nativeCollisionDirty_.remove(slot);
        return true;
    }
    NativePosition* nativePosition(unsigned slot){
        if(slot>=View::capacity||(slot>=View::terrainBase&&slot<View::timeFieldBase))return nullptr;
        return &nativePositions_[slot<View::terrainBase?slot:slot-(View::timeFieldBase-View::terrainBase)];
    }
    const NativePosition* nativePosition(unsigned slot)const{
        if(slot>=View::capacity||(slot>=View::terrainBase&&slot<View::timeFieldBase))return nullptr;
        return &nativePositions_[slot<View::terrainBase?slot:slot-(View::timeFieldBase-View::terrainBase)];
    }
    static constexpr unsigned drawCapacity=2+GameplayWorld::enemyCapacity+GameplayWorld::bulletCapacity+
        GameplayScriptSpawnInstances::capacity+GameplayDebrisInstances::capacity+GameplayEntryDithers::capacity+GameplayWorld::boneCapacity+GameplayWorld::skeletonCapacity+64+32+32+32+32;
    struct DrawEntry {GameplayWorld::DrawActor kind;InstanceHandle local;RunnerImageValue depth;};
    struct DrawIdentity {GameplayWorld::DrawActor kind;InstanceHandle local;};
    static constexpr unsigned drawCapacityWithTitles=drawCapacity+4+32+32+8;
    DrawIdentity preparedDrawIdentities_[drawCapacityWithTitles]{};
    mutable DrawEntry drawOrder_[drawCapacityWithTitles]{};
    mutable unsigned drawCount_=0;
    mutable std::uint32_t drawCaptureRevision_=0,drawLayerRevision_=0;
    mutable bool drawOrderValid_=false;
#ifdef DOWNWELL_DRAW_CACHE_AUDIT
    mutable bool drawAuditHasSnapshot_=false;
#endif
    RunnerStepSnapshot<1+GameplayWorld::enemyCapacity+GameplayWorld::breakableCapacity+GameplayDebrisInstances::capacity+8+GameplayEntryDithers::capacity+GameplayWorld::droneBulletCapacity+GameplayWorld::boneCapacity+GameplayWorld::skeletonCapacity+GameplayWorld::currencyCapacity+32+32+4+4+8> combatAlarmPhase_;
    RunnerStepSnapshot<GameplayWorld::enemyCapacity+GameplayWorld::bulletCapacity+GameplayWorld::droneBulletCapacity+GameplayWorld::skeletonCapacity+GameplayPlayerControllers::capacity+32> actorEndStepPhase_;
    bool actorEndStepExecuting_=false;
    bool combatAlarmPhaseInUse_=false;
    bool combatAlarmCaptureDirty_=true;
    unsigned combatAlarmMask_=0;
    bool combatAlarmCaptureValid_=false;
    bool nativeBoxAlarmPrefix_=false;
    using EnemyAlarmRoster=NativeEnemyAlarmRoster<GameplayWorld::enemyCapacity>;
    EnemyAlarmRoster enemyAlarmRoster_;
    bool enemyAlarmRosterEligible_=true;
    bool nativeEnemyAlarms()const{
#ifdef DOWNWELL_REFERENCE_ENEMY_ALARM_ROSTER
        return false;
#else
        // Only this owner has the shared monotonic registration chronology.
        // Other ownership modes keep their original captured list.
        return native_&&sceneryLayers_&&(nativeBoxes_||nativeFields_)&&enemyAlarmRosterEligible_;
#endif
    }
    bool updateEnemyAlarmRoster(InstanceHandle h){
        if(!nativeEnemyAlarms())return true;
        std::uint32_t flags;
        if(!table_.flags(h,flags)||(flags&0x100003u)||!table_.registered(h))return enemyAlarmRoster_.erase(h.slot);
        const int object=table_.objectId(h);
        unsigned alarms;
        // The other implemented alarm families are 73, 92, 211, 266 and
        // 370. Unsupported objects retain the complete registered capture.
        if(object<=92||object>=211||!alarmMetadata_.mask(object,alarms)){
            enemyAlarmRosterEligible_=false;combatAlarmCaptureValid_=false;return true;
        }
        return check(enemyAlarmRoster_.put({h,object,table_.registrationOrder(h),alarms}));
    }
#ifdef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
    RunnerRoomMembership<View::capacity> room_;
#else
    RunnerRunRoomMembership<View::capacity,View::terrainBase,View::timeFieldBase> room_;
#endif
    bool terrainSnapshotDirty_=true;
    using PropertyStore=RunnerFullProperties<Owner::Properties,View::capacity>;
    PropertyStore properties_;
    RunnerObjectFamilies<386> families_;
    const ObjectPack& objects_;
    // The pinned original has 741 sprite descriptors. Decode once before
    // any births/queries; keep preparation storage off the target task stack.
    PackedSpriteCollision preparedSprites_[741];
    SpriteCollisionPack sprites_;
    PreparedObjectDefaults<386> objectDefaults_;
    PreparedSpriteGeometry<741> spriteGeometry_;
    PreparedTerrainBirthBounds terrainBirthBounds_;
    PreparedTerrainCreate nativeTerrainCreate_;
#ifdef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
    GameplayRoomLayers<View::capacity> layers_;
#else
    GameplayRoomLayers<View::capacity,true> layers_;
#endif
    ObjectDrawMetadata<386> drawMetadata_;
    ObjectAlarmMetadata<386> alarmMetadata_;
    GameplayWorld* world_=nullptr;
    const GameplayWorld::PlayerInstance* initialPlayer_=nullptr;
    InstanceHandle initialPlayerKey_{};
    bool good_=false;
    bool failureObserved_=false;
    bool activationPending_=false;
    bool destructionPending_=false;
    bool terrainFamilies_[2]={false,false};
    bool timeFieldFamily_=false;
    bool check(bool ok){if(!ok)good_=false;return ok;}
    bool preparedDrawEnabled()const{
#if defined(DOWNWELL_REFERENCE_DRAW_LIST) || defined(DOWNWELL_SCALAR_TERRAIN_OWNERSHIP)
        return false;
#else
        return native_&&layers_.enabled();
#endif
    }
    bool publishPreparedDraw(InstanceHandle h){
        if(!preparedDrawEnabled())return true;
        // These always-active native families borrow layer population but
        // have no physical actor element, including the scalar-family oracle.
        if(sceneryLayers_&&((h.slot>=View::breakableBase&&h.slot<View::terrainBase)||
            (h.slot>=View::timeFieldBase&&h.slot<View::goalFieldBase)))return true;
        View::Kind kind;InstanceHandle local;
        RunnerLayerDraw::Element element;
        if(!View::decode(h,kind,local)||!eligibilityFlags(h,element.flags))return check(false);
        element.type=2;element.instance=true;
        unsigned token=0;bool actor=true;
        GameplayWorld::DrawActor drawKind=GameplayWorld::DrawActor::player;
        switch(kind){
            case View::Kind::player:break;
            case View::Kind::enemy:drawKind=GameplayWorld::DrawActor::enemy;token=h.slot;break;
            case View::Kind::bullet:drawKind=GameplayWorld::DrawActor::bullet;
                token=1+GameplayWorld::enemyCapacity+local.slot;break;
            case View::Kind::scriptEffect:drawKind=GameplayWorld::DrawActor::scriptEffect;
                token=1+GameplayWorld::enemyCapacity+GameplayWorld::bulletCapacity+local.slot;break;
            case View::Kind::debris:drawKind=GameplayWorld::DrawActor::debris;
                token=1+GameplayWorld::enemyCapacity+GameplayWorld::bulletCapacity+
                    GameplayScriptSpawnInstances::capacity+local.slot;break;
            case View::Kind::ditherAbove:drawKind=GameplayWorld::DrawActor::ditherAbove;
                token=1+GameplayWorld::enemyCapacity+GameplayWorld::bulletCapacity+
                    GameplayScriptSpawnInstances::capacity+GameplayDebrisInstances::capacity;break;
            case View::Kind::entryDither:drawKind=GameplayWorld::DrawActor::entryDither;
                token=drawCapacity-32-32-64-64-GameplayWorld::skeletonCapacity-GameplayWorld::boneCapacity-GameplayEntryDithers::capacity+local.slot;break;
            case View::Kind::bone:drawKind=GameplayWorld::DrawActor::bone;
                token=drawCapacity-32-32-64-64-GameplayWorld::skeletonCapacity-GameplayWorld::boneCapacity+local.slot;break;
            case View::Kind::skeleton:drawKind=GameplayWorld::DrawActor::skeleton;
                token=drawCapacity-32-32-64-64-GameplayWorld::skeletonCapacity+local.slot;break;
            case View::Kind::timeShard:drawKind=GameplayWorld::DrawActor::timeShard;
                token=drawCapacity-32-32-64-64+local.slot;break;
            case View::Kind::corpseExpiry:drawKind=GameplayWorld::DrawActor::corpseExpiry;
                token=drawCapacity-32-32-64+local.slot;break;
            case View::Kind::consumptionBlood:drawKind=GameplayWorld::DrawActor::consumptionBlood;
                token=drawCapacity-32+local.slot;break;
            case View::Kind::corpse:drawKind=GameplayWorld::DrawActor::corpse;
                token=drawCapacity-32-32+local.slot;break;
            case View::Kind::smoke:drawKind=GameplayWorld::DrawActor::smoke;
                token=drawCapacity-32-32-32+local.slot;break;
            case View::Kind::levelNumber:drawKind=GameplayWorld::DrawActor::levelNumber;
                token=drawCapacity+local.slot;break;
            case View::Kind::limboShard:drawKind=GameplayWorld::DrawActor::limboShard;
                token=drawCapacity+4+local.slot;break;
            case View::Kind::blast:drawKind=GameplayWorld::DrawActor::blast;
                token=drawCapacity+4+32+32+local.slot;break;
            case View::Kind::flashBall:drawKind=GameplayWorld::DrawActor::flashBall;
                token=drawCapacity+4+32+local.slot;break;
            default:actor=false;break;
        }
        if(actor){
            if(token>=drawCapacityWithTitles)return check(false);
            preparedDrawIdentities_[token]={drawKind,local};
            const unsigned metadata=drawMetadata_.get(table_.objectId(h));
            element.object=(metadata&ObjectDrawMetadata<386>::exists)!=0;
            element.drawEvent=(metadata&ObjectDrawMetadata<386>::drawEvent)!=0;
            element.sprite=table_.sprite(h.slot)>=0;
        }
        return check(layers_.publishPreparedDraw(h,token,RunnerLayerDraw::action(element)));
    }
    bool terrainQuery(int object)const{
        if(!valid()||!world_||!terrainRegistered_||(object!=60&&object!=93))return false;
        return terrainFamilies_[object==60?0:1];
    }
    // Unregistered terrain must not silently vanish from a selector whose
    // family contains walls or thin platforms (including all/other selectors).
    bool selectorAvailable(int object)const{
        if(native_)return false;
        if(terrainRegistered_)return true;
        if(object<0)return false;
        return !families_.isA(60,object)&&!families_.isA(93,object)&&!families_.isA(99,object);
    }
    // Walls share depth 100 (rmMain's placed layer and objWall_n/objThinPlatform
    // creation depth). An inactive wall ends that layer's Draw, so an actor
    // there would need registered terrain elements to keep its draw fidelity.
    bool actorDepthAvailable(RunnerImageValue depth)const{
        RunnerImageValue terrainDepth;
        return terrainRegistered_||(terrainDepth.assignInteger(100)&&!RunnerSpatialSplit::equal(depth,terrainDepth));
    }
    bool nativeBoxSlot(unsigned slot)const{return nativeBoxes_&&slot>=View::breakableBase&&slot<View::terrainBase;}
    bool nativeFieldSlot(unsigned slot)const{return nativeFields_&&slot>=View::timeFieldBase&&slot<View::goalFieldBase;}
    const GameplayWorld::Breakable* nativeBox(InstanceHandle h)const{
        if(!world_||!nativeBoxSlot(h.slot))return nullptr;
        const auto* box=world_->findBreakable({h.slot-View::breakableBase,h.generation});
        return box&&box->nativeRegistration?box:nullptr;
    }
    const OrdinaryTimeField* nativeField(InstanceHandle h)const{
        if(!world_||!nativeFieldSlot(h.slot))return nullptr;
        const auto& fields=world_->timeFields();const unsigned index=h.slot-View::timeFieldBase;
        if(!h.generation||fields.identityAt(index)!=h.generation)return nullptr;
        const auto* field=fields.at(index);
        return field&&field->nativeRegistration?field:nullptr;
    }
    bool resolves(InstanceHandle h)const{
        if(!world_)return false;
        if(native_){
            // Registered actors take their original path. Only absent keys
            // need delegation to native families; adding a family must not
            // turn every actor lookup into a walk of all family predicates.
            if(table_.contains(h))return View(*world_).contains(h);
            return nativeBox(h)||nativeField(h)||
                (h.slot>=View::terrainBase&&h.slot<View::timeFieldBase&&world_->nativeTerrainInstances()&&
                 world_->nativeTerrainInstances()->contains({h.slot-View::terrainBase,h.generation}));
        }
        return owner_.contains(h)&&View(*world_).contains(h);
    }
    unsigned animationSkipFirst()const{return native_?View::breakableBase:View::terrainBase;}
    unsigned animationSkipLast()const{return native_?View::bulletBase:View::timeFieldBase;}
    int objectOf(InstanceHandle h)const{
        if(!native_)return owner_.objectId(h);
        const int object=table_.objectId(h);if(object>=0)return object;
        if(nativeBox(h))return 92;
        if(nativeField(h))return 82;
        if(world_&&world_->nativeTerrainInstances()){
            GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;
            if(View(*world_).readTerrain(h,id,active,destroyed))return id.object;
        }
        return -1;
    }
    InstanceHandle handleAt(unsigned slot)const{
        if(!native_)return owner_.currentHandle(slot);
        const auto registered=table_.currentHandle(slot);if(registered.generation)return registered;
        if(nativeBoxSlot(slot)){
            const unsigned index=slot-View::breakableBase;
            if(!world_||index>=world_->breakableSlots())return {};
            const auto& b=world_->breakableSlot(index);
            return b.nativeRegistration?InstanceHandle{slot,b.lifetimeId}:InstanceHandle{};
        }
        if(world_&&nativeFieldSlot(slot)){
            const unsigned i=slot-View::timeFieldBase;
            const auto* f=world_->timeFields().at(i);
            if(f&&f->nativeRegistration)return {slot,world_->timeFields().identityAt(i)};
        }
        if(world_&&world_->nativeTerrainInstances()&&slot>=View::terrainBase&&slot<View::timeFieldBase){
            const auto local=world_->nativeTerrainInstances()->currentHandle(slot-View::terrainBase);
            if(local.generation)return {slot,local.generation};
        }
        return {};
    }
    std::uint32_t captureRevision()const{return native_?table_.captureRevision():owner_.captureRevision();}
    std::uint32_t bindingRevision()const{return native_?table_.bindingRevision():owner_.bindingRevision();}
    bool bindingsUnchanged(std::uint32_t revision)const{
        return native_?table_.bindingsUnchanged(revision):owner_.bindingsUnchanged(revision);
    }
    bool capturePrevious(InstanceHandle h){return native_?(table_.contains(h)||nativeBox(h)||nativeField(h)):owner_.capturePrevious(h);}
    int drawSprite(unsigned slot)const{return native_?table_.sprite(slot):properties_.read(slot)->sprite.spriteId;}
    // Registration order restricted to one owner domain, with live flags.
    template<class Visit> bool visitRegistered(unsigned first,unsigned last,Visit visit)const{
        if(native_)return visitNativeRegistered(first,last,visit);
        return owner_.visitRetainedRegistrationRange(first,last,[&](InstanceHandle h,int object,const Owner::State& state){
            return visit(h,object,state.flags);
        });
    }
    bool eligibilityFlags(InstanceHandle h,std::uint32_t& out)const{
        if(native_){
            bool actor=table_.flags(h,out);
            if(!actor)if(const auto* b=nativeBox(h)){out=nativeBoxFlags_|(b->destroyRequested?1u:0u);actor=true;}
            if(!actor&&nativeField(h)){out=nativeFieldFlags_;actor=true;}
            if(actor){
                const auto* state=nativePosition(h.slot);if(!state)return false;
                out=(out&~0xc208u)|(state->appearanceFlags&0xc208u);return true;
            }
            if(world_&&world_->nativeTerrainInstances()){
                GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;
                if(View(*world_).readTerrain(h,id,active,destroyed)){
                    const auto* definition=objectDefaults_.get(id.object);if(!definition)return false;
                    std::uint32_t spatial;
                    if(!world_->nativeTerrainInstances()->spatialFlags(id.handle,spatial))return false;
                    out=(definition->initialFlags&~8u)|spatial|
                        (table_.externalRegistered(h)?0u:0x80000u);return true;
                }
            }
            return false;
        }
#ifdef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
        const auto state=owner_.resolve(h);if(!state)return false;
        out=state->flags;return true;
#else
        return owner_.instanceFlags(h,out);
#endif
    }
    template<class Visit> bool visitEligibility(Visit visit)const{
        if(native_)return visitNativeRegistered(0,View::capacity,visit);
#ifdef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
        return owner_.visitRegistrations([&](InstanceHandle h,int object,const Owner::State& state){
            return visit(h,object,state.flags);
        });
#else
        return owner_.visitRegistrationFlags(visit);
#endif
    }
    template<class Visit> bool visitNativeRegistered(unsigned first,unsigned last,Visit visit)const{
        const bool boxes=nativeBoxes_&&last>View::breakableBase&&first<View::terrainBase;
        const bool fields=nativeFields_&&last>View::timeFieldBase&&first<View::goalFieldBase;
        auto flagsOf=[&](InstanceHandle h,std::uint32_t& flags){return eligibilityFlags(h,flags);};
        if(!boxes&&!fields)
            return table_.visitRegistrationRange(first,last,flagsOf,visit);
        if(!world_||first>last||last>View::capacity)return false;
        unsigned box=first>View::breakableBase?first-View::breakableBase:0;
        const unsigned end=boxes?(last<View::terrainBase?last-View::breakableBase:world_->breakableSlots()):0;
        unsigned field=first>View::timeFieldBase?first-View::timeFieldBase:0;
        const unsigned fieldEnd=fields?(last<View::goalFieldBase?last-View::timeFieldBase:world_->timeFields().size()):0;
        // Both families append in birth order without compaction. Merge their
        // native sequences with actor registrations only for requested ranges.
        auto emitBefore=[&](std::uint32_t order){
            for(;;){
                const GameplayWorld::Breakable* b=nullptr;
                const OrdinaryTimeField* f=nullptr;
                while(box<end&&box<world_->breakableSlots()){
                    b=&world_->breakableSlot(box);if(b->nativeRegistration)break;
                    b=nullptr;++box;
                }
                while(field<fieldEnd){
                    f=world_->timeFields().at(field);if(f&&f->nativeRegistration)break;
                    f=nullptr;++field;
                }
                if(!b&&!f)return true;
                const bool fieldFirst=f&&(!b||f->nativeRegistration<b->nativeRegistration);
                const auto stamp=fieldFirst?f->nativeRegistration:b->nativeRegistration;
                if(order&&stamp>=order)return true;
                if(fieldFirst){
                    const InstanceHandle h{View::timeFieldBase+field,world_->timeFields().identityAt(field)};
                    ++field;if(!visit(h,82,nativeFieldFlags_))return false;
                }else{
                    const InstanceHandle h{View::breakableBase+box++,b->lifetimeId};
                    if(!visit(h,92,nativeBoxFlags_|(b->destroyRequested?1u:0u)))return false;
                }
            }
        };
        return table_.visitRegistrationRange(first,last,flagsOf,[&](InstanceHandle h,int object,std::uint32_t flags){
            const auto order=h.slot>=View::terrainBase&&h.slot<View::timeFieldBase?
                world_->nativeTerrainInstances()->registrationOrder({h.slot-View::terrainBase,h.generation}):
                table_.registrationOrder(h);
            return emitBefore(order)&&visit(h,object,flags);
        })&&emitBefore(0);
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
        const auto state=owner_.resolve(key);
        const auto appearance=state?properties_.read(key.slot):typename PropertyStore::Read{};
        if(flags&&state){
            const auto& sprite=appearance->sprite;
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
        return flags&&owner_.refreshSprite(key,*flags,false,sprites_,appearance->sprite,true,
            [](auto angle,auto& cosine,auto& sine){
                DW_PROFILE_SCOPE(profileInstanceBoundsTrig);
                RunnerImageValue radians;MotionBinary64 c,s;
                return SmokeDirection::radians(angle,radians)&&smokeTrig(radians.promoted(),c,s)&&
                    cosine.assign(c)&&sine.assign(s);
            },[](auto v,std::int32_t& n){return v.roundPixel(n);},publish);
    }
    bool refreshNativeActor(InstanceHandle h,bool publish){
        if(!native_||!valid())return false;
        auto* state=nativePosition(h.slot);std::uint32_t flags;
        if(!state||!eligibilityFlags(h,flags))return check(false);
        const auto* objectFlags=families_.flags(unsigned(objectOf(h)));if(!objectFlags)return check(false);
        const auto& sprite=state->sprite;
        PackedSpriteCollision resource;Owner::Box bounds;
        const bool found=sprites_.collisionSprite(sprite.spriteId,sprite.maskId,resource);
        if(found){
            if(!RunnerSpatialSplit::valid(sprite.angle))return check(false);
            const bool rotated=(sprite.angle.bits&0x7fffffffu)!=0;
            RunnerImageValue cosine{},sine{};
            if(rotated){
                RunnerImageValue radians;MotionBinary64 c,s;
                if(!SmokeDirection::radians(sprite.angle,radians)||!smokeTrig(radians.promoted(),c,s)||
                    !cosine.assign(c)||!sine.assign(s))return check(false);
            }
            if(!RunnerSpriteBounds::fromSprite(resource,state->x,state->y,sprite.scaleX,sprite.scaleY,
                rotated,cosine,sine,true,[](auto v,std::int32_t& n){return v.roundPixel(n);},bounds))return check(false);
        }
        const bool okay=owner_.externalSpatialPublication([&](auto& index){
            return RunnerSpatialBoundsUpdate::finishRefresh(index,h.slot,*objectFlags,flags,false,state->bounds,
                found?&bounds:nullptr,state->x,state->y,found&&resource.collisionMode==1,publish);
        });
        state->appearanceFlags=flags&0xc208u;return check(okay);
    }
    bool nativeTerrainBirthBounds(const GameplayWorld::NativeTerrain::Identity& id,
        RunnerImageValue& x,RunnerImageValue& y,Owner::Box& bounds,bool& found,bool& precise,int sprite=-2){
        if(sprite==-2&&terrainBirthBounds_.at(id.object,id.x,id.y,x,y,bounds,precise)){found=true;return true;}
        const auto* definition=objectDefaults_.get(id.object);
        if(!definition||!x.assignInteger(id.x)||!y.assignInteger(id.y))return check(false);
        PackedSpriteCollision resource;
        found=sprites_.collisionSprite(sprite==-2?definition->spriteId:sprite,definition->maskId,resource);
        precise=found&&resource.collisionMode==1;
        if(!found)bounds={x,y,x,y};
        return !found||check(RunnerSpriteBounds::fromSprite(resource,x,y,
            RunnerImageValue{0x3f800000u},RunnerImageValue{0x3f800000u},false,RunnerImageValue{},RunnerImageValue{},true,
            [](auto value,std::int32_t& pixel){return value.roundPixel(pixel);},bounds));
    }
    bool nativeTerrainCachedBounds(const GameplayWorld::NativeTerrain::Identity& id,
        RunnerImageValue& x,RunnerImageValue& y,Owner::Box& bounds,bool& found,bool& precise,int sprite){
        int dx,dy;
        if(!world_||!world_->nativeTerrainInstances()||!world_->nativeTerrainInstances()->cachedOffset(id.handle,dx,dy))return false;
        if(!dx&&!dy)return nativeTerrainBirthBounds(id,x,y,bounds,found,precise,sprite);
        const auto* definition=objectDefaults_.get(id.object);
        RunnerImageValue ox,oy,cx,cy;
        if(!definition||!x.assignInteger(id.x)||!y.assignInteger(id.y)||!ox.assignInteger(-dx)||!oy.assignInteger(-dy)||
            !RunnerSpatialSplit::sub(x,ox,cx)||!RunnerSpatialSplit::sub(y,oy,cy))return false;
        PackedSpriteCollision resource;
        found=sprites_.collisionSprite(sprite==-2?definition->spriteId:sprite,definition->maskId,resource);
        precise=found&&resource.collisionMode==1;
        if(!found){bounds={cx,cy,cx,cy};return true;}
        return RunnerSpriteBounds::fromSprite(resource,cx,cy,RunnerImageValue{0x3f800000u},
            RunnerImageValue{0x3f800000u},false,RunnerImageValue{},RunnerImageValue{},true,
            [](auto value,std::int32_t& pixel){return value.roundPixel(pixel);},bounds);
    }
    bool refreshNativeTerrain(InstanceHandle h,bool publish=true){
        if(!valid()||!world_||!world_->nativeTerrainInstances()||h.slot<View::terrainBase||h.slot>=View::timeFieldBase)return check(false);
        if(wallQuery_&&wallQuery_->handle.slot==h.slot&&wallQuery_->handle.generation==h.generation){
            auto& q=*wallQuery_;Owner::Box bounds;PackedSpriteCollision resource;
            const auto* objectFlags=families_.flags(unsigned(objectOf(h)));if(!objectFlags)return check(false);
            const bool found=sprites_.collisionSprite(q.sprite.spriteId,q.sprite.maskId,resource);
            if(found&&!RunnerSpriteBounds::fromSprite(resource,q.state.x,q.state.y,q.sprite.scaleX,q.sprite.scaleY,
                false,{}, {},true,[](auto v,std::int32_t& n){return v.roundPixel(n);},bounds))return check(false);
            const bool okay=owner_.externalSpatialPublication([&](auto& index){
                return RunnerSpatialBoundsUpdate::finishRefresh(index,h.slot,*objectFlags,q.state.flags,false,q.state.bounds,
                    found?&bounds:nullptr,q.state.x,q.state.y,found&&resource.collisionMode==1,publish);
            });
            q.cacheX=q.dx;q.cacheY=q.dy;
            const InstanceHandle local{h.slot-View::terrainBase,h.generation};
            const bool stored=world_->publishNativeTerrainBounds(local,q.state.flags)&&
                world_->publishNativeTerrainQueryCache(local,q.cacheX,q.cacheY,false);
            return check(okay&&stored);
        }
        const auto* terrain=world_->nativeTerrainInstances();
        const InstanceHandle local{h.slot-View::terrainBase,h.generation};
        GameplayWorld::NativeTerrain::Identity id;bool active,destroyed,created;
        std::uint32_t flags;int current,cached;unsigned imageIndex;
        if(!terrain->read(local,id,active,destroyed)||!eligibilityFlags(h,flags)||
            !terrain->appearance(local,current,imageIndex,created)||!terrain->cachedSprite(local,cached))return check(false);
        const auto* objectFlags=families_.flags(unsigned(id.object));if(!objectFlags)return check(false);
        RunnerImageValue x,y;Owner::Box oldBounds,newBounds;bool found,precise;
        if(!nativeTerrainCachedBounds(id,x,y,oldBounds,found,precise,cached)||
            !nativeTerrainBirthBounds(id,x,y,newBounds,found,precise,current))return false;
        const bool okay=owner_.externalSpatialPublication([&](auto& index){
            return RunnerSpatialBoundsUpdate::finishRefresh(index,h.slot,*objectFlags,flags,false,oldBounds,
                found?&newBounds:nullptr,x,y,precise,publish);
        });
        // finishRefresh publishes cache/flags before index mutation. Preserve
        // that prefix even if the journal operation failed terminally.
        const bool recorded=world_->publishNativeTerrainBounds(local,flags);
        return check(okay&&recorded);
    }
    bool nativeCollisionVisual(InstanceHandle h,Owner::State& state,Owner::Sprite& sprite){
        if(wallQuery_&&wallQuery_->handle.slot==h.slot&&wallQuery_->handle.generation==h.generation){
            state=wallQuery_->state;sprite=wallQuery_->sprite;return true;
        }
        if(!native_||!resolves(h)||!eligibilityFlags(h,state.flags))return false;
        if(const auto* actor=nativePosition(h.slot)){
            state.x=actor->x;state.y=actor->y;state.bounds=actor->bounds;sprite=actor->sprite;return true;
        }
        if(!world_||!world_->nativeTerrainInstances())return false;
        const auto* terrain=world_->nativeTerrainInstances();
        const InstanceHandle local{h.slot-View::terrainBase,h.generation};
        GameplayWorld::NativeTerrain::Identity id;bool active,destroyed,created,found,precise;
        int cached,current;unsigned index;
        if(!terrain->read(local,id,active,destroyed)||!terrain->cachedSprite(local,cached)||
            !terrain->appearance(local,current,index,created)||
            !nativeTerrainCachedBounds(id,state.x,state.y,state.bounds,found,precise,cached))return false;
        const auto* definition=objectDefaults_.get(id.object);if(!definition)return false;
        sprite.spriteId=current==-2?definition->spriteId:current;sprite.maskId=definition->maskId;
        return sprite.imageIndex.assignInteger(index);
    }
    bool nativeTerrainBirth(const GameplayWorld::NativeTerrain::Identity& id,bool active,
        std::uint32_t& order,std::uint32_t& flags){
        const unsigned slot=View::terrainBase+id.handle.slot;
        if(!valid()||!table_.externalBirth(slot,order)||!nativeBirthQueues(slot,id.object))return check(false);
        if(!check(table_.registerExternalObject({slot,id.handle.generation},id.object,true)))return false;
        const auto* definition=objectDefaults_.get(id.object);const auto* objectFlags=families_.flags(unsigned(id.object));
        if(!definition||!objectFlags)return check(false);
        if(!check(room_.bind(slot,definition->creationDepth)))return false;
        if(!active)activationPending_=true;
        flags=definition->initialFlags|(active?0u:2u);
        RunnerImageValue x,y;Owner::Box bounds,cached{};bool found,precise;
        if(!nativeTerrainBirthBounds(id,x,y,bounds,found,precise))return false;
        if(!check(owner_.externalSpatialPublication([&](auto& index){
            return RunnerSpatialBoundsUpdate::finishRefresh(index,slot,*objectFlags,flags,false,cached,
                found?&bounds:nullptr,x,y,precise,true);
        })))return false;
        if(!check(layers_.birth({slot,id.handle.generation},id.source,definition->creationDepth,!active,false)))return false;
        // The birth callback precedes run publication. Publish the scenery
        // cutoff directly; resolving this handle through the run is premature.
        return !preparedDrawEnabled()||check(layers_.publishPreparedDraw({slot,id.handle.generation},0,
            active?RunnerLayerDraw::Action::skip:RunnerLayerDraw::Action::endLayer));
    }
    bool nativeTerrainRetire(unsigned local){
        if(!valid()||!world_||!world_->nativeTerrainInstances())return check(false);
        const auto* terrain=world_->nativeTerrainInstances();
        GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;std::uint32_t flags;
        const auto h=terrain->currentHandle(local);
        if(!terrain->read(h,id,active,destroyed)||!terrain->spatialFlags(h,flags))return check(false);
        const unsigned slot=View::terrainBase+local;
        if(flags&0x8000u){
            // Cached geometry may still be constructor state after Create.
            // Never remove using the current appearance before it is flushed.
            RunnerImageValue x,y;Owner::Box bounds;bool found,precise;
            int cached;if(!terrain->cachedSprite(h,cached)||
                !nativeTerrainCachedBounds(id,x,y,bounds,found,precise,cached))return check(false);
            if(RunnerSpatialSplit::less(bounds.right,bounds.left)){const auto v=bounds.left;bounds.left=bounds.right;bounds.right=v;}
            if(RunnerSpatialSplit::less(bounds.bottom,bounds.top)){const auto v=bounds.top;bounds.top=bounds.bottom;bounds.bottom=v;}
            if(!check(owner_.externalSpatialPublication([&](auto& index){
                bool removed;return index.remove(bounds,slot,true,removed);
            })))return false;
        }
        if(!check(room_.remove(slot)))return false;
        if(!check(layers_.retire({slot,h.generation})))return false;
        if(!check(table_.registerExternalObject({slot,h.generation},id.object,false)))return false;
        table_.externalRetire(slot);return nativeRetireQueues(slot);
    }
    template<class Record,View::Kind Kind> struct Binding {
        GameplayCollisionOwner& parent;
        static int object(const Record& r){
            if constexpr(Kind==View::Kind::result)return 325;
            if constexpr(Kind==View::Kind::levelNumber)return 372;
            if constexpr(Kind==View::Kind::limboShard)return 379;
            if constexpr(Kind==View::Kind::flashBall)return 224;
            if constexpr(Kind==View::Kind::runController)return 37;
            if constexpr(Kind==View::Kind::playerController)return r.objectId;
            if constexpr(Kind==View::Kind::enemy||Kind==View::Kind::terrain)return r.objectId;
            if constexpr(Kind==View::Kind::timeField)return 82;
            if constexpr(Kind==View::Kind::bullet)return 261;
            if constexpr(Kind==View::Kind::scriptEffect)return 219;
            if constexpr(Kind==View::Kind::corpseExpiry)return 242;
            if constexpr(Kind==View::Kind::debris)return 370;
            if constexpr(Kind==View::Kind::timeChange)return 211;
            if constexpr(Kind==View::Kind::timeShard)return 238;
            if constexpr(Kind==View::Kind::entryDither)return 73;
            if constexpr(Kind==View::Kind::ditherAbove)return 61;
            if constexpr(Kind==View::Kind::builder)return 65;
            if constexpr(Kind==View::Kind::drone)return 266;
            if constexpr(Kind==View::Kind::bone)return 269;
            if constexpr(Kind==View::Kind::skeleton)return 120;
            if constexpr(Kind==View::Kind::blood)return 220;
            if constexpr(Kind==View::Kind::smoke)return 225;
            if constexpr(Kind==View::Kind::blast)return 276;
            if constexpr(Kind==View::Kind::corpse)return 263;
            if constexpr(Kind==View::Kind::consumptionBlood)return 223;
            if constexpr(Kind==View::Kind::currency)return 248;
            if constexpr(Kind==View::Kind::goalField||Kind==View::Kind::exitController)return r.objectId;
            return Kind==View::Kind::player?35:92;
        }
        bool key(InstanceHandle local,const Record& r,InstanceHandle& out,bool birth=false){
            if(!parent.valid()||!View::encode(Kind,local,out))return parent.check(false);
            if constexpr(Kind!=View::Kind::timeField)
                if(local.generation!=r.lifetimeId)return parent.check(false);
            if(birth&&!parent.world_)return parent.check(Kind==View::Kind::player);
            if(!parent.world_)return parent.check(false);
            if(!birth&&parent.objectOf(out)!=object(r))return parent.check(false);
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
        bool birth(InstanceHandle local,const Record& r){
            DW_PROFILE_SCOPE(profileInstanceBirth);
            InstanceHandle h;
            if(!key(local,r,h,true))return false;
            if constexpr(Kind==View::Kind::player||Kind==View::Kind::builder)if(!parent.check(parent.layers_.prepareFirstBirth()))return false;
            if constexpr(Kind!=View::Kind::terrain)if(parent.native_)return nativeBirth(local,h,r);
            Owner::State state;
            Owner::Box terrainBounds;bool preparedTerrain=false,terrainPrecise=false;
            RunnerSpatialSplit::PixelBox terrainPixels;
            PreparedTerrainBirthBounds::Key terrainGeometry;
            if constexpr(Kind==View::Kind::terrain)
                preparedTerrain=parent.terrainBirthBounds_.at(r.objectId,r.x,r.y,
                    state.x,state.y,terrainBounds,terrainPrecise,&terrainPixels,&terrainGeometry);
            if(!preparedTerrain){
                NativePosition position;
                if(!birthPosition(r,position))return false;
                state.x=position.x;state.y=position.y;
            }
            if constexpr(Kind!=View::Kind::timeField)if(!r.active)state.flags|=2u;
            auto edit=parent.properties_.template editDomain<Kind==View::Kind::terrain>(h.slot);auto& p=edit.get();
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
            edit.publish();
            if(!parent.check(parent.owner_.publishBirth(h,parent.families_,
                [&](InstanceHandle live,bool publish){
                    if constexpr(Kind==View::Kind::terrain)if(preparedTerrain)
                        return parent.owner_.refreshPreparedTerrain(live,*parent.families_.flags(object(r)),
                            terrainBounds,terrainPixels,terrainPrecise,publish);
                    return parent.refresh(live,publish);
                })))return false;
            unsigned sourceId=0;
            if constexpr(Kind==View::Kind::terrain)sourceId=r.sourceInstanceId;
            if constexpr(Kind==View::Kind::breakable)sourceId=r.sourceId;
            if constexpr(Kind==View::Kind::player)sourceId=parent.layers_.playerSourceId();
            if constexpr(Kind==View::Kind::timeField)sourceId=r.sourceId;
            if constexpr(Kind==View::Kind::debris||Kind==View::Kind::scriptEffect)sourceId=r.sourceId;
            if constexpr(Kind==View::Kind::timeChange)sourceId=parent.pendingTimeChangeId_;
            if constexpr(Kind==View::Kind::flashBall||Kind==View::Kind::limboShard||Kind==View::Kind::levelNumber||Kind==View::Kind::result||Kind==View::Kind::runController||Kind==View::Kind::consumptionBlood||Kind==View::Kind::corpse||Kind==View::Kind::corpseExpiry||Kind==View::Kind::blast||Kind==View::Kind::smoke)sourceId=r.sourceId;
            if constexpr(Kind==View::Kind::timeShard)sourceId=parent.pendingTimeShardId_;
            if constexpr(Kind==View::Kind::playerController||Kind==View::Kind::entryDither||Kind==View::Kind::ditherAbove||Kind==View::Kind::builder||Kind==View::Kind::goalField)sourceId=r.sourceId;
            if constexpr(Kind==View::Kind::drone||Kind==View::Kind::bone||Kind==View::Kind::skeleton||Kind==View::Kind::blood||Kind==View::Kind::currency)sourceId=r.sourceId;
            constexpr bool projected=Kind==View::Kind::flashBall||Kind==View::Kind::limboShard||Kind==View::Kind::levelNumber||Kind==View::Kind::consumptionBlood||Kind==View::Kind::corpse||Kind==View::Kind::blast||Kind==View::Kind::smoke||Kind==View::Kind::corpseExpiry||Kind==View::Kind::player||Kind==View::Kind::enemy||
                Kind==View::Kind::bullet||Kind==View::Kind::scriptEffect||Kind==View::Kind::debris||Kind==View::Kind::entryDither||Kind==View::Kind::bone||Kind==View::Kind::skeleton||Kind==View::Kind::ditherAbove||Kind==View::Kind::timeShard;
            if(projected&&!parent.actorDepthAvailable(p.depth))return parent.check(false);
            {
                DW_PROFILE_SCOPE(profileInstanceLayerAttach);
                if(!parent.check(parent.layers_.birth(h,sourceId,p.depth,(state.flags&2u)!=0,projected)))return false;
            }
            if constexpr(Kind==View::Kind::player){
                if(!parent.world_){parent.initialPlayer_=&r;parent.initialPlayerKey_=local;}
            }
            return true;
        }
        bool birthPosition(const Record& r,NativePosition& result){
            auto* position=&result;
            if constexpr(Kind==View::Kind::scriptEffect)if(!r.effect||!r.effect->valid)return parent.check(false);
            if constexpr(Kind==View::Kind::scriptEffect||Kind==View::Kind::debris){
                if(!r.effect)return parent.check(false);
                position->x=r.effect->x;position->y=r.effect->y;
            }else if constexpr(Kind==View::Kind::flashBall||Kind==View::Kind::limboShard||Kind==View::Kind::levelNumber||Kind==View::Kind::result||Kind==View::Kind::runController||Kind==View::Kind::consumptionBlood||Kind==View::Kind::corpse||Kind==View::Kind::blast||Kind==View::Kind::smoke||Kind==View::Kind::corpseExpiry||Kind==View::Kind::playerController||Kind==View::Kind::timeShard||Kind==View::Kind::timeChange||Kind==View::Kind::entryDither||Kind==View::Kind::ditherAbove||Kind==View::Kind::builder||Kind==View::Kind::drone||Kind==View::Kind::bone||Kind==View::Kind::skeleton||Kind==View::Kind::blood||Kind==View::Kind::currency){
                position->x=r.x;position->y=r.y;
            }else{
                constexpr bool roomCoordinates=Kind==View::Kind::player||Kind==View::Kind::terrain||Kind==View::Kind::timeField||Kind==View::Kind::goalField||Kind==View::Kind::exitController;
                constexpr int ox=roomCoordinates?0:240,oy=roomCoordinates?0:176;
                if(r.x>0x7fffffff-ox||r.y>0x7fffffff-oy||
                    !real(r.x+ox,position->x)||!real(r.y+oy,position->y))return parent.check(false);
            }
            return parent.check(RunnerSpatialSplit::valid(position->x)&&RunnerSpatialSplit::valid(position->y));
        }
        bool nativeAppearance(InstanceHandle h,const Record& r){
            auto* state=parent.nativePosition(h.slot);const auto* definition=parent.objectDefaults_.get(object(r));
            if(!state||!definition)return parent.check(false);
            state->sprite={};state->sprite.spriteId=definition->spriteId;state->sprite.maskId=definition->maskId;
            state->timeline={};state->cachedFrame={0xbf800000u};state->appearanceFlags=8u;state->bounds={};
            if constexpr(Kind==View::Kind::enemy){
                state->sprite.spriteId=r.spriteId;state->sprite.maskId=r.maskId;
                if(!real(r.objectId==117?r.motionState:r.facing,state->sprite.scaleX)||
                    !real(r.imageScaleY(),state->sprite.scaleY)||r.imagePhase>0x7fffffffu||
                    !real(int(r.imagePhase),state->sprite.imageIndex))return parent.check(false);
                if(state->sprite.imageIndex.bits)state->sprite.imageIndex.bits-=8u<<23;
            }else if constexpr(Kind==View::Kind::player){
                if(!real(r.scaleX,state->sprite.scaleX))return parent.check(false);
            }else if constexpr(Kind==View::Kind::debris){
                if(!r.effect)return parent.check(false);
                state->sprite.spriteId=r.effect->spriteId;state->sprite.imageIndex=r.effect->imageIndex;
            }
            return true;
        }
        // The same order publications as the reference birth, without
        // bounds or full properties: the flag word and depth come from
        // the prepared object default that attachObjectPrepared applies.
        bool nativeBirth(InstanceHandle local,InstanceHandle h,const Record& r){
            const auto* definition=parent.objectDefaults_.get(object(r));
            if(!definition)return parent.check(false);
            auto* position=parent.nativePosition(h.slot);
            if(!position||!birthPosition(r,*position)||!nativeAppearance(h,r))return parent.check(false);
            bool inactive=false;
            if constexpr(Kind!=View::Kind::timeField)inactive=!r.active;
            RunnerImageValue depth=definition->creationDepth;
            int sprite=definition->spriteId;
            if constexpr(Kind==View::Kind::debris){
                if(!r.effect||!real(DebrisDraw::sourceDepth,depth))return parent.check(false);
                sprite=r.effect->spriteId;
            }
            if constexpr(Kind==View::Kind::scriptEffect)if(!r.effect||!r.effect->valid)return parent.check(false);
            if constexpr(Kind==View::Kind::entryDither)if(!real(-200000,depth))return parent.check(false);
            if constexpr(Kind==View::Kind::enemy)sprite=r.spriteId;
            if constexpr(Kind==View::Kind::breakable)if(parent.nativeBoxes_){
#ifndef DOWNWELL_REFERENCE_BREAKABLES
                // Ordinary callback births (e.g. an added layer observer)
                // may reacquire an empty family's layer independently.
                parent.boxAllocation_.leased=false;
#endif
                return parent.check(!inactive&&parent.table_.externalBirth(h.slot,r.nativeRegistration)&&parent.nativeBirthQueues(h.slot,object(r))&&parent.refreshNativeActor(h,true)&&
#ifndef DOWNWELL_REFERENCE_BREAKABLES
                    parent.layers_.birthSceneryFamily(parent.boxLayer_));
#else
                    parent.layers_.birthScenery(depth));
#endif
            }
            if constexpr(Kind==View::Kind::timeField)if(parent.nativeFields_){
                return parent.check(parent.table_.externalBirth(h.slot,r.nativeRegistration)&&parent.nativeBirthQueues(h.slot,object(r))&&parent.refreshNativeActor(h,true)&&
                    parent.layers_.birthScenery(depth));
            }
            if(!parent.check(parent.table_.birth(h,object(r),definition->initialFlags|(inactive?2u:0u),sprite)))return false;
            if(!parent.nativeBirthQueues(h.slot,object(r)))return false;
            if constexpr(Kind==View::Kind::enemy)if(!parent.updateEnemyAlarmRoster(h))return false;
            if(!parent.check(parent.room_.bind(h.slot,depth)))return false;
            if(!parent.refreshNativeActor(h,true))return false;
            if(inactive)parent.activationPending_=true;
            if constexpr(Kind==View::Kind::breakable||Kind==View::Kind::timeField)
                if(parent.sceneryLayers_){
                    // These native stores only birth active scenery and
                    // retire it; they expose no source depth/activation writes.
                    return parent.check(!inactive&&parent.layers_.birthScenery(depth));
                }
            unsigned sourceId=0;
            if constexpr(Kind==View::Kind::player)sourceId=parent.layers_.playerSourceId();
            if constexpr(Kind==View::Kind::breakable)sourceId=r.sourceId;
            if constexpr(Kind==View::Kind::timeField)sourceId=r.sourceId;
            if constexpr(Kind==View::Kind::debris||Kind==View::Kind::scriptEffect)sourceId=r.sourceId;
            if constexpr(Kind==View::Kind::timeChange)sourceId=parent.pendingTimeChangeId_;
            if constexpr(Kind==View::Kind::flashBall||Kind==View::Kind::limboShard||Kind==View::Kind::levelNumber||Kind==View::Kind::result||Kind==View::Kind::runController||Kind==View::Kind::consumptionBlood||Kind==View::Kind::corpse||Kind==View::Kind::corpseExpiry||Kind==View::Kind::blast||Kind==View::Kind::smoke)sourceId=r.sourceId;
            if constexpr(Kind==View::Kind::timeShard)sourceId=parent.pendingTimeShardId_;
            if constexpr(Kind==View::Kind::playerController||Kind==View::Kind::entryDither||Kind==View::Kind::ditherAbove||Kind==View::Kind::builder||Kind==View::Kind::goalField)sourceId=r.sourceId;
            if constexpr(Kind==View::Kind::drone||Kind==View::Kind::bone||Kind==View::Kind::skeleton||Kind==View::Kind::blood||Kind==View::Kind::currency)sourceId=r.sourceId;
            constexpr bool projected=Kind==View::Kind::flashBall||Kind==View::Kind::limboShard||Kind==View::Kind::levelNumber||Kind==View::Kind::consumptionBlood||Kind==View::Kind::corpse||Kind==View::Kind::blast||Kind==View::Kind::smoke||Kind==View::Kind::corpseExpiry||Kind==View::Kind::player||Kind==View::Kind::enemy||
                Kind==View::Kind::bullet||Kind==View::Kind::scriptEffect||Kind==View::Kind::debris||Kind==View::Kind::entryDither||Kind==View::Kind::bone||Kind==View::Kind::skeleton||Kind==View::Kind::ditherAbove||Kind==View::Kind::timeShard;
            if(projected&&!parent.actorDepthAvailable(depth))return parent.check(false);
            if(!parent.check(parent.layers_.birth(h,sourceId,depth,inactive,projected)))return false;
            if(!parent.publishPreparedDraw(h))return false;
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
        bool placeMeeting(InstanceHandle local,const Record& r,int object,MotionBinary64 x,MotionBinary64 y,bool& hit){
            static_assert(Kind==View::Kind::corpse,"corpse query binding");
            InstanceHandle shared;
            if(!parent.corpseMasks_||!key(local,r,shared))return parent.check(false);
            return parent.registeredPlaceMeeting(shared,object,x,y,*parent.corpseMasks_,hit);
        }
        bool retire(InstanceHandle local,const Record& r){InstanceHandle h;
            if constexpr(Kind==View::Kind::timeField)if(parent.nativeFields_){
                if(!key(local,r,h)||!r.nativeRegistration)return parent.check(false);
                const auto* d=parent.objectDefaults_.get(82);
                if(!parent.check(d&&parent.layers_.retireScenery(d->creationDepth)))return false;
                if(!parent.nativeRetireQueues(h.slot))return false;
                r.nativeRegistration=0;parent.table_.externalRetire(h.slot);return true;
            }
            if constexpr(Kind==View::Kind::breakable)if(parent.nativeBoxes_){
                if(!key(local,r,h)||!r.nativeRegistration)return parent.check(false);
#ifndef DOWNWELL_REFERENCE_BREAKABLES
                if(!parent.check(parent.layers_.retireSceneryFamily(parent.boxLayer_)))return false;
#else
                const auto* d=parent.objectDefaults_.get(92);
                if(!parent.check(d&&parent.layers_.retireScenery(d->creationDepth)))return false;
#endif
                if(!parent.nativeRetireQueues(h.slot))return false;
                r.nativeRegistration=0;parent.table_.externalRetire(h.slot);return true;
            }
            if(!key(local,r,h)||!parent.check(parent.room_.contains(h.slot)))return false;
            if(!parent.check(parent.native_?parent.table_.retire(h):parent.owner_.retire(h)))return false;
            if(parent.native_&&!parent.nativeRetireQueues(h.slot))return false;
            if constexpr(Kind==View::Kind::enemy)if(!parent.updateEnemyAlarmRoster(h))return false;
            if(!parent.check(parent.room_.remove(h.slot)))return false;
            if constexpr(Kind==View::Kind::breakable||Kind==View::Kind::timeField)
                if(parent.sceneryLayers_){
                    const auto* d=parent.objectDefaults_.get(object(r));
                    return parent.check(d&&parent.layers_.retireScenery(d->creationDepth));
                }
            return parent.check(parent.layers_.retire(h));}
        bool markDestroyed(InstanceHandle local,const Record& r){InstanceHandle h;
            if(!key(local,r,h))return false;
            if constexpr(Kind==View::Kind::breakable)if(parent.nativeBoxes_){
                unsigned i=0;
                while(i<parent.markedBoxCount_&&parent.markedBoxes_[i].slot<local.slot)++i;
                if(i<parent.markedBoxCount_&&parent.markedBoxes_[i].slot==local.slot){
                    parent.markedBoxes_[i]=local;parent.table_.externalFlagsChanged(h.slot);return true;
                }
                if(parent.markedBoxCount_==GameplayWorld::breakableCapacity)return parent.check(false);
                for(unsigned n=parent.markedBoxCount_;n>i;--n)parent.markedBoxes_[n]=parent.markedBoxes_[n-1];
                parent.markedBoxes_[i]=local;++parent.markedBoxCount_;
                parent.table_.externalFlagsChanged(h.slot);
                parent.destructionPending_=true;return true;
            }
            std::uint32_t flags;
            if(!parent.eligibilityFlags(h,flags)||!parent.check(parent.native_?parent.table_.setExclusions(h,(flags&0x100003u)|1u):
                parent.owner_.setExclusions(h,(flags&0x100003u)|1u)))return false;
            if constexpr(Kind==View::Kind::enemy)if(!parent.updateEnemyAlarmRoster(h))return false;
            if(!parent.check(parent.room_.mark(h.slot)))return false;
            parent.destructionPending_=true;return parent.publishPreparedDraw(h);}
        bool membership(InstanceHandle local,const Record& r,bool member){InstanceHandle h;
            if constexpr(Kind==View::Kind::timeField)if(parent.nativeFields_)
                return parent.check(member)&&key(local,r,h);
            if constexpr(Kind==View::Kind::breakable)if(parent.nativeBoxes_)
                return parent.check(member)&&key(local,r,h);
            if(!key(local,r,h)||!parent.check(parent.native_?parent.table_.registerObject(h,member):parent.owner_.registerObject(h,member)))return false;
            if constexpr(Kind==View::Kind::enemy)if(!parent.updateEnemyAlarmRoster(h))return false;
            return parent.publishPreparedDraw(h);}
        bool activation(InstanceHandle local,const Record& r,bool active){InstanceHandle h;
            if constexpr(Kind==View::Kind::breakable||Kind==View::Kind::timeField)
                if(parent.sceneryLayers_)return parent.check(active)&&key(local,r,h);
            if(!key(local,r,h))return false;
            if(parent.native_&&active){
                std::uint32_t flags;if(!parent.table_.flags(h,flags))return parent.check(false);
                if((flags&2u)&&!parent.nativeDirty(h.slot,object(r),flags))return false;
            }
            if(!parent.check(parent.native_?parent.table_.setActive(h,active):
                parent.owner_.setActive(h,active,*parent.families_.flags(object(r)))))return false;
            if constexpr(Kind==View::Kind::enemy)if(!parent.updateEnemyAlarmRoster(h))return false;
            if(!parent.check(parent.layers_.activation(h,active)))return false;
            parent.activationPending_=true;return parent.publishPreparedDraw(h);}
        // Retain actual narrowed setter values before admitting spatial queries.
        // Bounds/queue publication remains separate; integer draw positions
        // cannot establish whether a later source setter changed the position.
        bool move(InstanceHandle local,const Record& r,RunnerImageValue x,RunnerImageValue y){InstanceHandle h;
            if(parent.native_){
                if constexpr(Kind==View::Kind::breakable){
                    RunnerImageValue px,py;auto xx=MotionBinary64::fromInteger(r.x),yy=MotionBinary64::fromInteger(r.y);
                    // The static native box index has no fractional/moving
                    // geometry. Never report an ignored setter as published.
                    return key(local,r,h)&&parent.check(xx.add(MotionBinary64::fromInteger(240))&&
                        yy.add(MotionBinary64::fromInteger(176))&&px.assign(xx)&&py.assign(yy)&&
                        px.bits==x.bits&&py.bits==y.bits);
                }
                if(!key(local,r,h))return false;
                auto* position=parent.nativePosition(h.slot);
                if(!position||!RunnerSpatialSplit::valid(x)||!RunnerSpatialSplit::valid(y))return parent.check(false);
                // Ordinary setter preserves the existing sign of zero on a
                // numeric no-op, just as RunnerSpatialPosition::assign does.
                if(!RunnerSpatialSplit::equal(position->x,x)||!RunnerSpatialSplit::equal(position->y,y)){
                    position->x=x;position->y=y;
                    position->appearanceFlags|=8u;
                    std::uint32_t flags;
                    if(!parent.eligibilityFlags(h,flags)||!parent.nativeDirty(h.slot,object(r),flags))return parent.check(false);
                }
                return true;
            }
            return key(local,r,h)&&parent.check(parent.owner_.template moveDomain<Kind==View::Kind::terrain>(h,x,y,*parent.families_.flags(object(r))));}
        bool depth(InstanceHandle local,const Record& r,MotionBinary64 value){InstanceHandle h;RunnerImageValue narrowed;
            if(!key(local,r,h)||!narrowed.assign(value)||!parent.actorDepthAvailable(narrowed))return parent.check(false);
            if(parent.native_)return parent.check(parent.layers_.enabled()&&parent.layers_.writeDepth(h,value));
            auto edit=parent.properties_.template editDomain<Kind==View::Kind::terrain>(h.slot);auto& p=edit.get();
            if(!parent.layers_.enabled())return parent.check(RunnerSpatialSplit::equal(narrowed,p.depth));
            if(!parent.check(parent.layers_.writeDepth(h,value)))return false;
            p.depth=narrowed;return true;
        }
        bool sprite(InstanceHandle local,const Record& r,int sprite){InstanceHandle h;
            if(!key(local,r,h))return false;
            if(parent.native_){
                auto* state=parent.nativePosition(h.slot);if(!state)return parent.check(false);
                if(state->sprite.spriteId==sprite)return true;
                std::uint32_t frames=0;if(sprite>=0)parent.sprites_.frameCount(unsigned(sprite),frames);
                RunnerSpriteChange::Result change;
                if(!parent.check(RunnerSpriteChange::plan(state->sprite.spriteId,sprite,state->sprite.imageIndex,frames,change)))return false;
                state->sprite.spriteId=sprite;state->cachedFrame={0xbf800000u};state->appearanceFlags|=8u;
                if(change.resetIndex)state->sprite.imageIndex={};
                if(change.resetTimeline)state->timeline={};
                bool changed;if(!parent.check(parent.table_.setSprite(h,sprite,changed)))return false;
                if(changed){
                    std::uint32_t flags;
                    if(!parent.eligibilityFlags(h,flags)||!parent.nativeDirty(h.slot,object(r),flags))return parent.check(false);
                    if(parent.preparedDrawEnabled())return parent.publishPreparedDraw(h);
                    parent.drawOrderValid_=false;
                }
                return true;
            }
            return publishSprite(h,object(r),sprite);
        }
        bool publishSprite(InstanceHandle h,int object,int sprite){
            auto edit=parent.properties_.template editDomain<Kind==View::Kind::terrain>(h.slot);auto& p=edit.get();
            if(p.sprite.spriteId!=sprite)parent.drawOrderValid_=false;
            return parent.check(parent.owner_.changeSpriteFromPack(h,p.sprite,p.timeline,p.cachedFrame,
                sprite,parent.sprites_,*parent.families_.flags(object)));}
        bool imageIndex(InstanceHandle local,const Record& r,RunnerImageValue value){InstanceHandle h;
            if(parent.native_){
                if(!key(local,r,h)||!parent.check(RunnerSpatialSplit::valid(value)))return false;
                auto* state=parent.nativePosition(h.slot);if(!state)return parent.check(false);
                state->sprite.imageIndex=value;return true;
            }
            if(!key(local,r,h))return false;auto edit=parent.properties_.template editDomain<Kind==View::Kind::terrain>(h.slot);
            return parent.check(parent.owner_.writeImageIndex(h,edit.get().sprite,value));}
        bool mask(InstanceHandle local,const Record& r,int value){InstanceHandle h;
            if(parent.native_){
                if(!key(local,r,h))return false;
                auto* state=parent.nativePosition(h.slot);if(!state)return parent.check(false);
                if(state->sprite.maskId==value)return true;
                state->sprite.maskId=value;state->appearanceFlags|=8u;
                std::uint32_t flags;
                return parent.eligibilityFlags(h,flags)&&parent.nativeDirty(h.slot,object(r),flags);
            }
            if(!key(local,r,h))return false;auto edit=parent.properties_.template editDomain<Kind==View::Kind::terrain>(h.slot);
            return parent.check(parent.owner_.changeMask(h,edit.get().sprite,
                value,*parent.families_.flags(object(r))));}
        bool transform(InstanceHandle local,const Record& r,RunnerTransformField field,RunnerImageValue value,
            RunnerImageValue alpha,std::uint32_t blend){InstanceHandle h;
            if(parent.native_){
                if(!key(local,r,h))return false;
                auto* state=parent.nativePosition(h.slot);if(!state)return parent.check(false);
                bool changed;
                if(!parent.check(RunnerSpriteSetters::transform(state->sprite,field,value,alpha,blend,state->appearanceFlags,changed)))return false;
                if(!changed)return true;
                std::uint32_t flags;
                return parent.eligibilityFlags(h,flags)&&parent.nativeDirty(h.slot,object(r),flags);
            }
            if(!key(local,r,h))return false;auto edit=parent.properties_.template editDomain<Kind==View::Kind::terrain>(h.slot);
            return parent.check(parent.owner_.changeTransform(h,edit.get().sprite,
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
    Binding<GameplayWorld::RegisteredExpiry::Record,View::Kind::corpseExpiry> expiryBinding_{*this};
    GameplayWorld::RegisteredExpiry expiry_{GameplayWorld::RegisteredExpiry::Binding::bindPositionedActor(expiryBinding_)};
    Binding<GameplayWorld::RegisteredBlasts::Record,View::Kind::blast> blastBinding_{*this};
    GameplayWorld::RegisteredBlasts blasts_{GameplayWorld::RegisteredBlasts::Binding::bindPositionedActor(blastBinding_)};
    Binding<GameplayWorld::RegisteredSmoke::Record,View::Kind::smoke> smokeBinding_{*this};
    GameplayWorld::RegisteredSmoke smoke_{GameplayWorld::RegisteredSmoke::Binding::bindPositionedActor(smokeBinding_)};
    Binding<GameplayWorld::RegisteredCorpse::Record,View::Kind::corpse> corpseBinding_{*this};
    GameplayWorld::RegisteredCorpse corpses_{GameplayWorld::RegisteredCorpse::Binding::bindPositionedQueryActor(corpseBinding_)};
    Binding<GameplayWorld::ConsumptionParticles::Record,View::Kind::consumptionBlood> consumptionBinding_{*this};
    GameplayWorld::ConsumptionParticles consumption_{GameplayWorld::ConsumptionParticles::Binding::bindPositionedActor(consumptionBinding_)};
    Binding<GameplayWorld::LimboShards::Record,View::Kind::limboShard> limboBinding_{*this};
    GameplayWorld::LimboShards limbo_{GameplayWorld::LimboShards::Binding::bindPositionedActor(limboBinding_)};
    Binding<GameplayWorld::RegisteredFlash::Record,View::Kind::flashBall> flashBinding_{*this};
    GameplayWorld::RegisteredFlash flash_{GameplayWorld::RegisteredFlash::Binding::bindPositionedActor(flashBinding_)};
    Binding<GameplayWorld::RunControllers::Record,View::Kind::runController> controllerBinding_{*this};
    GameplayWorld::RunControllers controllers_{GameplayWorld::RunControllers::Binding::bindPositionedLifetime(controllerBinding_)};
    Binding<GameplayResultInstance::Record,View::Kind::result> resultBinding_{*this};
    GameplayResultInstance result_{GameplayResultInstance::Binding::bindMovement(resultBinding_)};
    Binding<GameplayWorld::LevelNumbers::Record,View::Kind::levelNumber> levelNumberBinding_{*this};
    GameplayWorld::LevelNumbers levelNumbers_{GameplayWorld::LevelNumbers::Binding::bindPositionedDestruction(levelNumberBinding_)};
    const SpriteMaskPack* corpseMasks_=nullptr;
    bool (*corpseChildId_)(void*,std::uint32_t&)=nullptr;
    void* corpseChildIdContext_=nullptr;
    GameplayScriptSpawnInstances scriptEffects_{GameplayScriptSpawnInstances::Binding::bindMaskDestructionDepth(scriptEffectsBinding_)};
    Binding<GameplayDebrisInstances::Record,View::Kind::debris> debrisBinding_{*this};
    GameplayDebrisInstances debris_{GameplayDebrisInstances::Binding::bindDestruction(debrisBinding_)};
    Binding<NativeTimeChangeRegistration::Record,View::Kind::timeChange> timeChangeBinding_{*this};
    NativeTimeChangeRegistration timeChanges_{*this};
    Binding<NativeTimeShardRegistration::Record,View::Kind::timeShard> timeShardBinding_{*this};
    NativeTimeShardRegistration timeShards_{*this};
    Binding<GameplayEntryDithers::Record,View::Kind::entryDither> entryBinding_{*this};
    GameplayEntryDithers entries_{GameplayEntryDithers::Binding::bindDestruction(entryBinding_)};
    Binding<GameplayDitherAbove::Record,View::Kind::ditherAbove> aboveBinding_{*this};
    GameplayDitherAbove above_{GameplayDitherAbove::Binding::bindDestruction(aboveBinding_)};
    Binding<GameplayBuilderController::Record,View::Kind::builder> builderBinding_{*this};
    GameplayBuilderController builderController_{GameplayBuilderController::Binding::bindMovement(builderBinding_)};
    Binding<GameplayPlayerControllers::Record,View::Kind::playerController> playerControllerBinding_{*this};
    GameplayPlayerControllers playerControllers_{GameplayPlayerControllers::Binding::bindDestruction(playerControllerBinding_)};
    Binding<GameplayWorld::DroneBullets::Record,View::Kind::drone> droneBinding_{*this};
    GameplayWorld::DroneBullets drones_{GameplayWorld::DroneBullets::Binding::bindPositionedActor(droneBinding_)};
    Binding<GameplayWorld::Bones::Record,View::Kind::bone> boneBinding_{*this};
    GameplayWorld::Bones bones_{GameplayWorld::Bones::Binding::bindPositionedActor(boneBinding_)};
    Binding<GameplayWorld::Skeletons::Record,View::Kind::skeleton> skeletonBinding_{*this};
    GameplayWorld::Skeletons skeletons_{GameplayWorld::Skeletons::Binding::bindPositionedActor(skeletonBinding_)};
    Binding<GameplayWorld::Blood::Record,View::Kind::blood> bloodBinding_{*this};
    GameplayWorld::Blood blood_{GameplayWorld::Blood::Binding::bindPositionedActor(bloodBinding_)};
    Binding<GameplayWorld::Currency::Record,View::Kind::currency> currencyBinding_{*this};
    GameplayWorld::Currency currency_{GameplayWorld::Currency::Binding::bindPositionedActor(currencyBinding_)};
    std::uint32_t pendingTimeChangeId_=0;
    std::uint32_t pendingTimeShardId_=0;
    void* timeEffectIdContext_=nullptr;
    bool (*timeEffectIdAllocate_)(void*,std::uint32_t&)=nullptr;
    const SpriteMaskPack* timeShardStepMasks_=nullptr; // Borrowed only during selected Step.
    const bool native_;
    const bool sceneryLayers_;
    const bool nativeBoxes_;
    const bool nativeFields_;
    const bool terrainRegistered_;
    NativeInstanceTable<View::capacity> table_;
    std::uint32_t nativeBoxFlags_=0;
    std::uint32_t nativeFieldFlags_=0;
    InstanceHandle markedBoxes_[GameplayWorld::breakableCapacity]{};
    unsigned markedBoxCount_=0;
    decltype(layers_)::SceneryFamily boxLayer_{};
    NativeFamilyBirth boxAllocation_{};
    GameplayTerrainInstances terrain_;
    GameplayTerrainInstances::Presentation nativeTerrainPresentation_;
    GameplayTerrainInstances::WallCollision nativeTerrainCollision_;
public:
    // Registered terrain is the complete runner reference: every wall owns a
    // lifetime, room/layer entry, object membership and spatial history, as
    // required by wall End Step and terrain-selector queries. Native gameplay
    // dispatches neither; its occupancy grid answers collision and its painter
    // draws walls. Terrain is sorted out of every actor ordering that it does
    // observe (room lists are stable by depth, and object lists are per
    // object), so unregistered terrain changes no actor order. Queries and
    // events that would need registered walls fail rather than miss them.
    //
    // Native ownership does not yet answer spatial queries. Published actor
    // positions, appearance and cached bounds are retained separately. Actor
    // and terrain constructors publish to the shared journal allocation;
    // complete animation and collision-pass scheduling remain integration work. A compact table keeps
    // what the actor orders observe (object, flag word, sprite index and
    // registration order); room lists and layers remain the order owners.
    enum class Ownership {reference,nativeTerrain,native,nativeScenery};
    GameplayCollisionOwner(const ObjectPack& objects,const SpriteCollisionPack& sprites,
        GameplayTerrainInstances::Presentation presentation={},GameplayTerrainInstances::WallCollision collision={},
        const RoomPack* rooms=nullptr,Ownership ownership=Ownership::reference,bool nativeBoxes=true,bool nativeFields=true):
        objects_(objects),sprites_(sprites),layers_(rooms),
        native_(ownership==Ownership::native||ownership==Ownership::nativeScenery),
        sceneryLayers_(ownership==Ownership::nativeScenery),
        nativeBoxes_(ownership==Ownership::nativeScenery&&nativeBoxes),
        nativeFields_(ownership==Ownership::nativeScenery&&nativeFields),
        terrainRegistered_(ownership==Ownership::reference),
        table_(ownership==Ownership::native||ownership==Ownership::nativeScenery),
        terrain_(terrainRegistered_?GameplayTerrainInstances::Binding::bindDestruction(terrainBinding_):
            GameplayTerrainInstances::Binding{},presentation,collision),
        nativeTerrainPresentation_(presentation),nativeTerrainCollision_(collision){
        if(preparedDrawEnabled())layers_.enablePreparedDraw();
        good_=sprites_.prepare(preparedSprites_,741)&&objectDefaults_.prepare(objects,sprites_)&&
            spriteGeometry_.prepare(sprites_)&&
            terrainBirthBounds_.prepare(objectDefaults_,sprites_)&&
            nativeTerrainCreate_.prepare(objectDefaults_,sprites_)&&
            families_.bindInstanceFlags(objects)&&drawMetadata_.bind(objects)&&alarmMetadata_.bind(objects);
        if(good_){const auto* box=objectDefaults_.get(92);if(!box)good_=false;else {
            nativeBoxFlags_=box->initialFlags;
            good_=RunnerDepthService::integerDepth(box->creationDepth,boxLayer_.depth);
        }}
        if(good_){const auto* field=objectDefaults_.get(82);if(!field)good_=false;else nativeFieldFlags_=field->initialFlags;}
#ifndef DOWNWELL_REFERENCE_BOX_ALARMS
        if(good_&&nativeBoxes_){
            unsigned mask=0;
            // The prefix adapter implements the pinned box's Alarm 0 only.
            // Other resource metadata retains generic captured dispatch.
            nativeBoxAlarmPrefix_=!(nativeBoxFlags_&0x100003u)&&alarmMetadata_.mask(92,mask)&&mask==1;
        }
#endif
        if(good_)for(unsigned selector=0;selector<2;++selector){
            terrainFamilies_[selector]=true;
            for(unsigned i=0;i<families_.size();++i)
                if(families_.isA(int(i),selector==0?60:93)&&i!=60&&i!=93&&i!=99)
                    terrainFamilies_[selector]=false;
        }
        if(good_){
            timeFieldFamily_=true;
            for(unsigned i=0;i<families_.size();++i)
                if(families_.isA(int(i),82)&&i!=82&&i!=255&&i!=371)timeFieldFamily_=false;
        }
#ifndef DOWNWELL_REFERENCE_VALIDATION
        if(native_&&valid()){
            // Native ownership never populates the private spatial/property
            // stores. Check their initial state once, then observe every
            // independently mutable family and the nested layer/depth owners.
            // Child failures remain immediate, including failures through the
            // public family accessors, not merely the enclosing callbacks.
            terrain_.observeFailure(good_);scriptEffects_.observeFailure(good_);
            debris_.observeFailure(good_);timeChanges_.observeFailure(good_);
            timeShards_.observeFailure(good_);
            entries_.observeFailure(good_);
            layers_.observeFailure(good_);failureObserved_=true;
        }
#endif
    }
    GameplayCollisionOwner(const GameplayCollisionOwner&)=delete;
    GameplayCollisionOwner& operator=(const GameplayCollisionOwner&)=delete;
    // Registration order with live flags, for differential verification.
    template<class Visit> bool visitRegistrationOrder(Visit visit)const{return valid()&&visitEligibility(visit);}
    // Drone dispatch deliberately exposes bool& status to arbitrary callbacks;
    // retain its live read rather than delaying a callback's failure to return.
    bool valid()const{return good_&&blasts_.valid()&&flash_.valid()&&limbo_.valid()&&levelNumbers_.valid()&&result_.valid()&&controllers_.valid()&&consumption_.valid()&&corpses_.valid()&&smoke_.valid()&&expiry_.valid()&&playerControllers_.valid()&&builderController_.valid()&&blood_.valid()&&skeletons_.valid()&&(failureObserved_?(drones_.valid()&&bones_.valid()):
        (owner_.valid()&&properties_.valid()&&terrain_.valid()&&scriptEffects_.valid()&&debris_.valid()&&
         timeShards_.valid()&&timeChanges_.valid()&&entries_.valid()&&drones_.valid()&&bones_.valid()&&layers_.valid()));}
    unsigned pendingSpatialIndex()const{return owner_.pendingSpatialIndex();}
    // Explicit source dirty pass. It is not yet scheduled as a gameplay
    // collision pass: complete animation/scene publication remains required.
    bool flushNativeSpatial(){
        if(!native_||!valid())return false;
        return check(nativeSpatialDirty_.flush([&](unsigned slot){
            std::uint32_t flags=0;
            if(!eligibilityFlags(handleAt(slot),flags))check(false);
            return flags;
        },[&](unsigned slot){
            const auto h=handleAt(slot);
            return slot>=View::terrainBase&&slot<View::timeFieldBase?refreshNativeTerrain(h):refreshNativeActor(h,true);
        }));
    }
    // Explicit collection only. Scheduling and precise event execution are
    // separate: this must not dispatch partial family passes in gameplay.
    template<unsigned Pairs> bool collectCollisionPairs(
        RunnerCollisionPairs<View::capacity,Pairs>& pairs,bool indexAvailable=true){
        if(!valid())return false;
        if(!native_)return check(owner_.collectCollisionPairs(pairs,indexAvailable,
            [&](InstanceHandle h,bool publish){return refresh(h,publish);},
            [&](InstanceHandle self,InstanceHandle other,bool& found){
                const int a=objectOf(self),b=objectOf(other);
                return a>=0&&b>=0&&objects_.collisionEventExists(a,b,found);
            },[&](InstanceHandle h,unsigned& frames){
                const auto p=properties_.read(h.slot);if(!p)return false;
                PackedSpriteCollision resource;
                frames=sprites_.collisionSprite(p->sprite.spriteId,p->sprite.maskId,resource)?resource.frameCount:0;return true;
            }));
        return check(RunnerCollisionCollect::run(pairs,nativeCollisionDirty_,nativeCollisionRetained_,indexAvailable,
            [&]{return flushNativeSpatial();},[&](unsigned slot){return handleAt(slot);},
            [&](InstanceHandle h,std::uint32_t& flags){return eligibilityFlags(h,flags);},
            [&](InstanceHandle h,Owner::Box& bounds){
                if(const auto* actor=nativePosition(h.slot)){bounds=actor->bounds;return true;}
                if(!world_||!world_->nativeTerrainInstances())return false;
                const auto* terrain=world_->nativeTerrainInstances();
                const InstanceHandle local{h.slot-View::terrainBase,h.generation};
                GameplayWorld::NativeTerrain::Identity id;bool active,destroyed,found,precise;int cached;
                RunnerImageValue x,y;
                return terrain->read(local,id,active,destroyed)&&terrain->cachedSprite(local,cached)&&
                    nativeTerrainCachedBounds(id,x,y,bounds,found,precise,cached);
            },
            [&](Owner::Box box,auto visit){return owner_.externalSpatialPublication([&](auto& index){return index.query(box,visit);});},
            [&](InstanceHandle self,InstanceHandle other,bool& found){
                const int a=objectOf(self),b=objectOf(other);
                return a>=0&&b>=0&&objects_.collisionEventExists(a,b,found);
            },
            [&](InstanceHandle h,unsigned& frames){
                int sprite,mask;
                if(const auto* actor=nativePosition(h.slot)){sprite=actor->sprite.spriteId;mask=actor->sprite.maskId;}
                else {
                    if(!world_||!world_->nativeTerrainInstances())return false;
                    const auto* definition=objectDefaults_.get(objectOf(h));if(!definition)return false;
                    unsigned index;bool created;
                    if(!world_->nativeTerrainInstances()->appearance({h.slot-View::terrainBase,h.generation},sprite,index,created))return false;
                    if(sprite==-2)sprite=definition->spriteId;mask=definition->maskId;
                }
                PackedSpriteCollision resource;
                frames=sprites_.collisionSprite(sprite,mask,resource)?resource.frameCount:0;return true;
            }));
    }
    // Candidate geometry only; callers must retain source family traversal
    // and inactive gates. Refresh precedes destruction rejection in 0x465520.
    bool collisionPointCandidate(InstanceHandle h,RunnerImageValue x,RunnerImageValue y,
        bool compatibility,bool precise,const SpriteMaskPack& masks,bool& hit){
        if(!valid()||!resolves(h))return check(false);
        if(!native_)return check(owner_.pointCandidate(h,properties_.read(h.slot)->sprite,x,y,
            compatibility,precise,sprites_,masks,[&](InstanceHandle key,bool publish){return refresh(key,publish);},
            inverseTrig,roundPixel,hit));
        Owner::State state;Owner::Sprite sprite;
        if(!RunnerSpatialSplit::valid(x)||!RunnerSpatialSplit::valid(y)||!nativeCollisionVisual(h,state,sprite))return check(false);
        if(state.flags&8u){
            if(!(h.slot>=View::terrainBase&&h.slot<View::timeFieldBase?refreshNativeTerrain(h):refreshNativeActor(h,true))||
                !nativeCollisionVisual(h,state,sprite))return check(false);
        }
        return check(RunnerSpritePoint::test(state,sprite,x,y,compatibility,precise,sprites_,masks,
            inverseTrig,roundPixel,hit));
    }
    // Pair test only, after ordered broad-phase collection. This does not
    // select candidates, dispatch events or approximate unsupported selectors.
    bool collisionOverlap(InstanceHandle self,InstanceHandle other,const SpriteMaskPack& masks,
        bool publish,bool& hit){
        if(!valid()||!resolves(self)||!resolves(other))return check(false);
        if(!native_)return check(owner_.collideCompatibility(self,other,
            properties_.read(self.slot)->sprite,properties_.read(other.slot)->sprite,sprites_,masks,
            RunnerMaskDispatch{},publish,[&](InstanceHandle h,bool update){return refresh(h,update);},
            inverseTrig,roundPixel,hit));
        Owner::State first,second;Owner::Sprite a,b;
        if(!nativeCollisionVisual(self,first,a)||!nativeCollisionVisual(other,second,b))return check(false);
        if(self.slot==other.slot||((first.flags|second.flags)&0x100001u)){hit=false;return true;}
        if(publish){
            const auto update=[&](InstanceHandle h){return h.slot>=View::terrainBase&&h.slot<View::timeFieldBase?
                refreshNativeTerrain(h):refreshNativeActor(h,true);};
            // OTHER's dirty bounds are published first, even if SELF later
            // rejects the pair. Preserve the spatial journal's mutation order.
            if((second.flags&8u)&&!update(other))return check(false);
            if((first.flags&8u)&&!update(self))return check(false);
            if(!nativeCollisionVisual(self,first,a)||!nativeCollisionVisual(other,second,b))return check(false);
        }
        return check(RunnerSpriteOverlap::test(first,second,a,b,sprites_,masks,RunnerMaskDispatch{},inverseTrig,roundPixel,hit));
    }
    bool queueCollision(InstanceHandle h){
        if(!valid()||!resolves(h))return check(false);
        const auto* flags=families_.flags(unsigned(objectOf(h)));if(!flags)return check(false);
        return check(native_?(!(*flags&8u)||nativeCollisionDirty_.mark(h.slot)):owner_.queueCollision(h,*flags));
    }
    template<class Execute> bool dispatchCollisionEvent(RunnerCollisionEventContext& context,
        InstanceHandle self,InstanceHandle other,RunnerCollisionEventContext::RoomState room,Execute execute){
        if(!valid())return false;
        return check(context.dispatch(self,other,room,
            [&](InstanceHandle a,InstanceHandle b,int& subtype,bool& found,PackedObjectEvent& event){
                if(!resolves(a)||!resolves(b))return false;
                const int object=objectOf(a);subtype=objectOf(b);
                return object>=0&&subtype>=0&&objects_.collisionEvent(object,subtype,found,event);
            },execute));
    }
    // Execute an already sealed, mixed-family pass; never filter it into
    // separate per-family passes. Motion owns source position/path rollback
    // and engine motion, not an integer-position approximation. Room guards
    // and current object IDs are reread separately for each event direction.
    // Physical retirement is forbidden until this pass has finished.
    template<unsigned Pairs,class Motion,class RoomState,class Execute>
    bool executeCollisionPairs(RunnerCollisionPairs<View::capacity,Pairs>& pairs,
        const SpriteMaskPack& masks,RunnerCollisionEventContext& context,
        Motion& motion,RoomState roomState,Execute execute){
        if(!valid()||!world_||!world_->effectsSucceeded()||!context.succeeded())return check(false);
        struct Ops {
            GameplayCollisionOwner& owner;const SpriteMaskPack& masks;RunnerCollisionEventContext& context;
            Motion& motion;RoomState& roomState;Execute& execute;
            bool overlap(InstanceHandle a,InstanceHandle b,bool& hit){return owner.collisionOverlap(a,b,masks,false,hit);}
            bool flags(InstanceHandle h,std::uint32_t& flags){return owner.eligibilityFlags(h,flags);}
            bool queueCollision(InstanceHandle h){return owner.queueCollision(h);}
            bool rollback(InstanceHandle h){return motion.rollback(h);}
            bool prepareMotion(InstanceHandle h){return motion.prepareMotion(h);}
            bool advanceMotion(InstanceHandle h){return motion.advanceMotion(h);}
            bool dispatch(InstanceHandle a,InstanceHandle b){
                return owner.dispatchCollisionEvent(context,a,b,roomState(),execute)&&owner.world_->effectsSucceeded();
            }
        } ops{*this,masks,context,motion,roomState,execute};
        RunnerCollisionDispatch dispatch;
        return check(pairs.dispatch([&](InstanceHandle a,InstanceHandle b){return dispatch.pair(a,b,ops);}));
    }
    // Diagnostic traversal only: this is not spatial candidate selection.
    // Spatial refresh is explicit; collision consumption still requires
    // complete scene publication and ordered animation/image writes. The
    // ordinary sprite getter has no collision-frame cache to invalidate.
    template<class Visit> bool visitNativeDirty(bool collision,Visit visit)const{
        if(!native_||!valid())return false;
        const auto& queue=collision?nativeCollisionDirty_:nativeSpatialDirty_;
        for(unsigned slot=queue.first();slot!=NativeDirtyQueue::End;slot=queue.next(slot)){
            const auto h=handleAt(slot);
            if(!h.generation||!visit(h))return false;
        }
        return true;
    }
    bool nativePublishedPosition(InstanceHandle h,RunnerImageValue& x,RunnerImageValue& y){
        if(!native_||!valid()||!table_.contains(h))return false;
        const auto* position=nativePosition(h.slot);if(!position)return false;
        x=position->x;y=position->y;return true;
    }
    bool nativePublishedSprite(InstanceHandle h,Owner::Sprite& sprite,std::uint32_t& flags)const{
        if(!native_||!valid()||!eligibilityFlags(h,flags))return false;
        const auto* state=nativePosition(h.slot);if(!state)return false;
        sprite=state->sprite;return true;
    }
    // Explicit publication for integration tests and future lazy collision
    // refresh. This does not flush the mixed queue or admit native selectors.
    bool nativeActorBounds(InstanceHandle h,Owner::Box& bounds,bool refreshBounds=false){
        if(!native_||!valid()||(refreshBounds&&!refreshNativeActor(h,true)))return false;
        std::uint32_t flags;if(!eligibilityFlags(h,flags))return false;
        const auto* state=nativePosition(h.slot);if(!state)return false;
        bounds=state->bounds;return true;
    }
    // Inspect already-published entries without flushing the dirty queues.
    // Deliberately not an instance_place/meeting implementation: terrain
    // and complete scene/animation publication are not yet admitted to queries.
    template<class Visit> bool inspectNativeIndex(Owner::Box box,Visit visit){
        if(!native_||!valid())return false;
        return check(owner_.externalSpatialPublication([&](auto& index){
            return index.query(box,[&](unsigned slot){
                const auto h=handleAt(slot);return h.generation&&visit(h);
            });
        }));
    }
    bool serviceSpatialIndex(unsigned budget){return check(owner_.serviceSpatialIndex(budget));}
    bool attach(GameplayWorld& world){
        combatAlarmCaptureValid_=false;
        if(!(native_?table_.watchCaptureRanges(0,nativeBoxAlarmPrefix_?View::breakableBase:View::terrainBase,View::debrisBase,View::capacity,combatAlarmCaptureDirty_):
            owner_.watchCaptureRanges(0,View::terrainBase,View::debrisBase,View::capacity,combatAlarmCaptureDirty_)))
            return check(false);
        if(!valid()||(world_&&world_!=&world)||(!world_&&
            (world.awaitingPlayerBirth()?initialPlayer_!=nullptr:
             (!initialPlayer_||world.findPlayer(initialPlayerKey_)!=initialPlayer_))))return false;
        if(!world_&&!world.bindTimeFieldLifecycle(OrdinaryTimeFields<64>::Binding::bind(fields_)))return check(false);
        if(!world_&&!world.bindGoalFieldLifecycle(GameplayGoalFields::Binding::bindMaskDestruction(goals_)))return check(false);
        if(!world_&&!world.bindExitControllerLifecycle(GameplayExitControllers::Binding::bind(exits_)))return check(false);
        if(!world_&&!world.bindBulletLifecycle(GameplayWorld::BulletLifecycleBinding::bindMaskDestruction(bullets_)))return check(false);
        if(!world_&&!world.bindScriptSpawnInstances(scriptEffects_))return check(false);
        if(!world_&&!world.bindRegisteredExpiry(expiry_))return check(false);
        if(!world_&&!world.bindRegisteredSmoke(smoke_))return check(false);
        if(!world_&&!world.bindRegisteredBlasts(blasts_))return check(false);
        if(!world_&&!world.bindRegisteredCorpses(corpses_))return check(false);
        if(!world_&&!world.bindConsumptionParticles(consumption_))return check(false);
        if(!world_&&!world.bindRunControllers(controllers_))return check(false);
        if(!world_&&!world.bindResultInstance(result_))return check(false);
        if(!world_&&!world.bindLevelNumbers(levelNumbers_))return check(false);
        if(!world_&&!world.bindLimboShards(limbo_))return check(false);
        if(!world_&&!world.bindRegisteredFlash(flash_))return check(false);
        if(!world_&&!world.bindDebrisInstances(debris_))return check(false);
        if(!world_&&!world.bindTimeChangeInstances(timeChanges_))return check(false);
        if(!world_&&!world.bindTimeShardInstances(timeShards_))return check(false);
        if(!world_&&!world.bindEntryDithers(entries_))return check(false);
        if(!world_&&!world.bindDitherAbove(above_))return check(false);
        if(!world_&&!world.bindBuilderController(builderController_))return check(false);
        if(!world_&&!world.bindPlayerControllers(playerControllers_))return check(false);
        if(!world_&&!world.bindDroneBullets(drones_))return check(false);
        if(!world_&&!world.bindBones(bones_))return check(false);
        if(!world_&&!world.bindSkeletons(skeletons_))return check(false);
        if(!world_&&!world.bindBlood(blood_))return check(false);
        if(!world_&&!world.bindCurrency(currency_))return check(false);
        world_=&world;
        if(native_&&!world.configureNativeTerrainBinding({this,
            [](void* p,unsigned count){auto& self=*static_cast<GameplayCollisionOwner*>(p);
                return self.valid()&&self.table_.canExternalBirth(count);},
            [](void* p,const GameplayWorld::NativeTerrain::Identity& id,bool active,std::uint32_t& order,std::uint32_t& flags){
                return static_cast<GameplayCollisionOwner*>(p)->nativeTerrainBirth(id,active,order,flags);},
            [](void* p,unsigned slot){auto& self=*static_cast<GameplayCollisionOwner*>(p);
                return self.nativeTerrainRetire(slot);},
            [](void* p,unsigned slot){auto& self=*static_cast<GameplayCollisionOwner*>(p);
                if(!self.valid())return false;
                if(!self.check(self.room_.mark(View::terrainBase+slot)))return false;
                self.destructionPending_=true;
                self.table_.externalFlagsChanged(View::terrainBase+slot);return true;},
            [](void* p,const GameplayWorld::NativeTerrain::Identity& id,int tile,std::uint32_t& flags){
                auto& self=*static_cast<GameplayCollisionOwner*>(p);
                const auto* plan=self.nativeTerrainCreate_.get(id.object,tile);
                if(!self.valid()||!plan||plan->sprite!=tile)return self.check(false);
                if(plan->changedSprite){
                    flags|=8u;
                    if(!self.nativeDirty(View::terrainBase+id.handle.slot,id.object,flags))return false;
                }
                // The admitted source Create has no RNG. Image index 23 for
                // platforms and speed zero are retained by the typed owner;
                // raw image-index writes do not dirty bounds or queues.
                return true;
            },
            [](void* p,const GameplayWorld::NativeTerrain::Identity& id,bool enabled){
                auto& self=*static_cast<GameplayCollisionOwner*>(p);
                return self.check(self.nativeTerrainCollision_.notify(id.x,id.y,enabled));
            },
            [](void* p,const GameplayWorld::NativeTerrain::Identity& id,int before,int after){
                auto& self=*static_cast<GameplayCollisionOwner*>(p);
                return self.check(self.nativeTerrainPresentation_.notify(id.x,id.y,before,after));
            },
            [](void* p,const GameplayWorld::NativeTerrain::Identity& id,bool active,std::uint32_t flags){
                auto& self=*static_cast<GameplayCollisionOwner*>(p);
                if(!self.valid())return false;
                const unsigned slot=View::terrainBase+id.handle.slot;
                if(active&&(flags&2u)&&!self.nativeDirty(slot,id.object,flags))return false;
                if(bool(flags&2u)==active)self.table_.externalFlagsChanged(slot);
                if(!self.check(self.layers_.activation({slot,id.handle.generation},active)))return false;
                if(self.preparedDrawEnabled()&&!self.check(self.layers_.publishPreparedDraw(
                    {slot,id.handle.generation},0,active?RunnerLayerDraw::Action::skip:RunnerLayerDraw::Action::endLayer)))return false;
                self.activationPending_=true;
                // Reconcile room membership at the phase boundary, not here.
                // Object-family and layer-element publication stay separate.
                return true;
            }
        }))return check(false);
#ifndef DOWNWELL_REFERENCE_BREAKABLES
        if(nativeBoxes_){
            if(!table_.bindExternalFamily(View::breakableBase,View::terrainBase,boxAllocation_))return check(false);
            boxAllocation_.good=&good_;boxAllocation_.context=this;
            boxAllocation_.observeBirth=[](void* context,unsigned slot){
                auto& self=*static_cast<GameplayCollisionOwner*>(context);
                if(!self.world_||slot>=self.world_->breakableSlots())return false;
                const auto& r=self.world_->breakableSlot(slot);const InstanceHandle h{View::breakableBase+slot,r.lifetimeId};
                auto* state=self.nativePosition(h.slot);
                return state&&self.boxes_.birthPosition(r,*state)&&self.boxes_.nativeAppearance(h,r)&&
                    self.nativeBirthQueues(h.slot,92)&&self.refreshNativeActor(h,true);
            };
            boxAllocation_.acquire=[](void* context,NativeFamilyBirth& birth){
                auto& self=*static_cast<GameplayCollisionOwner*>(context);
                return self.check(self.layers_.borrowSceneryFamily(self.boxLayer_,birth));
            };
            boxAllocation_.active=true;
        }
#endif
        world.configurePlayerDamageSlowdown([](void* context){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid()||!self.world_)return self.check(false);
            return self.check(self.world_->applyPlayerDamageSlowdown([&](auto apply){
                return self.visitEligibility([&](InstanceHandle h,int object,std::uint32_t flags){
                    if(!self.families_.isA(object,37)||(flags&0x140003u))return true;
                    View::Kind kind;InstanceHandle local;
                    return View::decode(h,kind,local)&&kind==View::Kind::runController&&apply(local);
                });
            }));
        },this);
        world.configureRegisteredActorEndStep([](void* context,long topQ12){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            const auto* nativeTerrain=self.world_->nativeTerrainInstances();
            const unsigned terrainCount=nativeTerrain?nativeTerrain->size():self.terrain_.size();
            const auto terrainCutoff=nativeTerrain?nativeTerrain->lastLifetime():self.terrain_.lastLifetime();
            if(!self.world_->playerHandle().generation&&!terrainCount&&!self.world_->breakableSlots()&&!self.world_->enemyCount()&&!self.world_->bulletCount()&&!self.drones_.size()&&!self.skeletons_.size()&&!self.playerControllers_.size()&&!self.corpses_.size())return true;
            MotionBinary64 top;
            if(!multiplyBinary64(MotionBinary64::fromInteger(topQ12),{0x3f300000u,0},top))return self.check(false);
            return self.dispatchActorEndStep([&](InstanceHandle shared,int object){
                View::Kind kind;InstanceHandle local;
                if(!View::decode(shared,kind,local))return false;
                if(object==35&&kind==View::Kind::player){self.world_->playerDamageEndStep();return true;}
                if(object==92&&kind==View::Kind::breakable){
                    const auto* box=self.world_->findBreakable(local);
                    if(!box||!retireMainRoomObjectsChecked(1,11,1,topQ12,[&](unsigned){
                        return LifetimeInstance{92,box->y+176,box->active&&!box->destroyRequested};
                    },[&](unsigned){return self.world_->markBreakableDestroyed(local);}))return false;
                    // Pinned original bytecode initializes autoTile to false
                    // and never writes it again. Reject an unsupported external
                    // state instead of silently omitting its image publication.
                    return !box->state.autoTile||box->state.checked;
                }
                if(kind==View::Kind::enemy){
                    const auto* enemy=self.world_->resolveEnemyLifetime(local);
                    return enemy&&hasSimpleOutOfViewEndStep(object)&&
                        retireMainRoomObjectsChecked(1,11,1,topQ12,[&](unsigned){
                            return LifetimeInstance{object,enemy->y+176,enemy->active&&!enemy->destroyRequested};
                        },[&](unsigned){return self.world_->markEnemyDestroyed(local);});
                }
                if(object==261&&kind==View::Kind::bullet)return self.endStepBullet(shared,top);
                if(object==120&&kind==View::Kind::skeleton)return self.skeletons_.endStep(local,top,true);
                if(object==263&&kind==View::Kind::corpse)return self.corpses_.endStep(local,top,true);
                if(object==256&&kind==View::Kind::playerController)
                    return self.playerControllers_.balloonEndStep(local,self.world_->upgrades().levels[15]!=0);
                return object==266&&kind==View::Kind::drone&&self.drones_.endStep(local,top,MotionBinary64::fromInteger(284),true);
            },[&]{return !terrainCount||self.retireThinPlatforms(topQ12,terrainCutoff);});
        },this,true);
        world.configureRoomActorDraw([](const void* context,GameplayWorld::DrawActorVisitor visit,void* visitor){
            const auto& self=*static_cast<const GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            const auto draw=[&](InstanceHandle shared,RunnerImageValue depth){
                View::Kind kind;InstanceHandle local;
                if(!View::decode(shared,kind,local))return false;
                // Retained terrain/field composition is not part of this
                // visitor. Do not resolve or copy their records per frame.
                if(kind!=View::Kind::flashBall&&kind!=View::Kind::limboShard&&kind!=View::Kind::levelNumber&&kind!=View::Kind::consumptionBlood&&kind!=View::Kind::corpse&&kind!=View::Kind::blast&&kind!=View::Kind::smoke&&kind!=View::Kind::corpseExpiry&&kind!=View::Kind::enemy&&kind!=View::Kind::bullet&&kind!=View::Kind::scriptEffect&&kind!=View::Kind::debris&&kind!=View::Kind::player&&kind!=View::Kind::entryDither&&kind!=View::Kind::bone&&kind!=View::Kind::skeleton&&kind!=View::Kind::ditherAbove&&kind!=View::Kind::timeShard)return true;
                std::uint32_t flags;
                if(!self.eligibilityFlags(shared,flags)||!View(*self.world_).contains(shared))return false;
                if(flags&0x100003u)return true;
                switch(kind){
                    case View::Kind::player:return visit(visitor,GameplayWorld::DrawActor::player,local,depth);
                    case View::Kind::levelNumber:return visit(visitor,GameplayWorld::DrawActor::levelNumber,local,depth);
                    case View::Kind::limboShard:return visit(visitor,GameplayWorld::DrawActor::limboShard,local,depth);
                    case View::Kind::flashBall:return visit(visitor,GameplayWorld::DrawActor::flashBall,local,depth);
                    case View::Kind::enemy:return visit(visitor,GameplayWorld::DrawActor::enemy,local,depth);
                    case View::Kind::bullet:return visit(visitor,GameplayWorld::DrawActor::bullet,local,depth);
                    case View::Kind::scriptEffect:return visit(visitor,GameplayWorld::DrawActor::scriptEffect,local,depth);
                    case View::Kind::debris:return visit(visitor,GameplayWorld::DrawActor::debris,local,depth);
                    case View::Kind::entryDither:return visit(visitor,GameplayWorld::DrawActor::entryDither,local,depth);
                    case View::Kind::ditherAbove:return visit(visitor,GameplayWorld::DrawActor::ditherAbove,local,depth);
                    case View::Kind::bone:return visit(visitor,GameplayWorld::DrawActor::bone,local,depth);
                    case View::Kind::skeleton:return visit(visitor,GameplayWorld::DrawActor::skeleton,local,depth);
                    case View::Kind::timeShard:return visit(visitor,GameplayWorld::DrawActor::timeShard,local,depth);
                    case View::Kind::corpseExpiry:return visit(visitor,GameplayWorld::DrawActor::corpseExpiry,local,depth);
                    case View::Kind::blast:return visit(visitor,GameplayWorld::DrawActor::blast,local,depth);
                    case View::Kind::smoke:return visit(visitor,GameplayWorld::DrawActor::smoke,local,depth);
                    case View::Kind::corpse:return visit(visitor,GameplayWorld::DrawActor::corpse,local,depth);
                    case View::Kind::consumptionBlood:return visit(visitor,GameplayWorld::DrawActor::consumptionBlood,local,depth);
                    default:return true;
                }
            };
            if(self.layers_.enabled())return self.layers_.visit([&](InstanceHandle h,RunnerLayerDraw::Element& out){
                if(!self.eligibilityFlags(h,out.flags))return false;
                View::Kind kind;InstanceHandle local;if(!View::decode(h,kind,local))return false;
                // Retained scenery/field passes own these draws. Preserve the
                // inactive flag (which terminates a layer), but avoid event
                // and sprite lookups for every terrain tile each frame.
                if(kind!=View::Kind::flashBall&&kind!=View::Kind::limboShard&&kind!=View::Kind::levelNumber&&kind!=View::Kind::consumptionBlood&&kind!=View::Kind::corpse&&kind!=View::Kind::blast&&kind!=View::Kind::smoke&&kind!=View::Kind::corpseExpiry&&kind!=View::Kind::enemy&&kind!=View::Kind::bullet&&kind!=View::Kind::scriptEffect&&
                    kind!=View::Kind::debris&&kind!=View::Kind::player&&kind!=View::Kind::entryDither&&kind!=View::Kind::bone&&kind!=View::Kind::skeleton&&kind!=View::Kind::ditherAbove&&kind!=View::Kind::timeShard)return true;
                const unsigned metadata=self.drawMetadata_.get(self.objectOf(h));
                out.object=(metadata&ObjectDrawMetadata<386>::exists)!=0;
                out.drawEvent=(metadata&ObjectDrawMetadata<386>::drawEvent)!=0;
                out.sprite=self.drawSprite(h.slot)>=0;return true;
            },[&](InstanceHandle h,int depth){RunnerImageValue value;return real(depth,value)&&draw(h,value);});
            if(self.native_)return false; // Native ownership requires room layers.
            return self.room_.visitDrawChecked([&](unsigned slot){return draw(self.owner_.currentHandle(slot),self.properties_.read(slot)->depth);});
        },this,[](const void* context,GameplayWorld::DrawActorVisitor visit,void* visitor){
            const auto& self=*static_cast<const GameplayCollisionOwner*>(context);
            if(!self.valid()||!self.world_)return false;
            if(self.preparedDrawEnabled()){
                const auto revision=self.layers_.preparedDrawRevision();
                if(self.drawOrderValid_&&revision!=0xffffffffu&&revision==self.drawLayerRevision_){
#ifdef DOWNWELL_DRAW_CACHE_AUDIT
                    auditActorDrawCache(0,self.drawCount_,false,false);
#endif
                    for(unsigned i=0;i<self.drawCount_;++i){const auto& e=self.drawOrder_[i];
                        if(!visit(visitor,e.kind,e.local,e.depth))return false;
                    }
                    return true;
                }
#ifdef DOWNWELL_DRAW_CACHE_AUDIT
                const unsigned previousCount=self.drawCount_;
                bool same=self.drawAuditHasSnapshot_;
                const bool orderChanged=revision!=self.drawLayerRevision_;
#endif
                self.drawOrderValid_=false;self.drawCount_=0;
                const bool ok=self.layers_.visitPreparedDraw([&](unsigned token,RunnerImageValue depth){
                    if(token>=drawCapacityWithTitles||self.drawCount_==drawCapacityWithTitles)return false;
                    const auto& identity=self.preparedDrawIdentities_[token];
#ifdef DOWNWELL_DRAW_CACHE_AUDIT
                    if(self.drawCount_>=previousCount)same=false;
                    else{const auto& old=self.drawOrder_[self.drawCount_];
                        if(old.kind!=identity.kind||old.local.slot!=identity.local.slot||
                            old.local.generation!=identity.local.generation||old.depth.bits!=depth.bits)same=false;
                    }
#endif
                    self.drawOrder_[self.drawCount_++]={identity.kind,identity.local,depth};
                    return visit(visitor,identity.kind,identity.local,depth);
                });
                self.drawLayerRevision_=revision;
                self.drawOrderValid_=ok&&revision==self.layers_.preparedDrawRevision();
#ifdef DOWNWELL_DRAW_CACHE_AUDIT
                auditActorDrawCache(!self.drawOrderValid_?3:same&&previousCount==self.drawCount_?1:2,
                    self.drawCount_,false,orderChanged);
                self.drawAuditHasSnapshot_=self.drawOrderValid_;
#endif
                return ok;
            }
#ifdef DOWNWELL_SCALAR_TERRAIN_OWNERSHIP
            return self.world_->visitRoomActors([&](auto kind,auto key,auto depth){return visit(visitor,kind,key,depth);});
#else
            const auto capture=
#ifdef DOWNWELL_LOCAL_DRAW_CAPTURE
                self.native_&&localDrawCaptureMode==2?self.table_.localCaptureRevision():
#endif
                self.captureRevision();
            const auto layers=self.layers_.drawRevision();
            if(self.drawOrderValid_&&capture!=0xffffffffu&&layers!=0xffffffffu&&
               capture==self.drawCaptureRevision_&&layers==self.drawLayerRevision_){
#ifdef DOWNWELL_DRAW_CACHE_AUDIT
                auditActorDrawCache(0,self.drawCount_,false,false);
#endif
                for(unsigned i=0;i<self.drawCount_;++i){const auto& e=self.drawOrder_[i];
                    if(!visit(visitor,e.kind,e.local,e.depth))return false;
                }
                return true;
            }
#ifdef DOWNWELL_DRAW_CACHE_AUDIT
            const unsigned previousCount=self.drawCount_;
            bool same=self.drawAuditHasSnapshot_;
            const bool captureChanged=capture!=self.drawCaptureRevision_;
            const bool layersChanged=layers!=self.drawLayerRevision_;
#endif
            self.drawOrderValid_=false;self.drawCount_=0;
            // Fill during the original live traversal, not before dispatch:
            // failed callbacks retain exactly the same successful prefix.
            const bool ok=self.world_->visitRoomActors([&](auto kind,auto key,auto depth){
                if(self.drawCount_==drawCapacityWithTitles)return false;
#ifdef DOWNWELL_DRAW_CACHE_AUDIT
                if(self.drawCount_>=previousCount)same=false;
                else {
                    const auto& old=self.drawOrder_[self.drawCount_];
                    if(old.kind!=kind||old.local.slot!=key.slot||old.local.generation!=key.generation||old.depth.bits!=depth.bits)same=false;
                }
#endif
                self.drawOrder_[self.drawCount_++]={kind,key,depth};
                return visit(visitor,kind,key,depth);
            });
            self.drawCaptureRevision_=capture;self.drawLayerRevision_=layers;
            self.drawOrderValid_=ok&&self.valid()&&capture==(
#ifdef DOWNWELL_LOCAL_DRAW_CAPTURE
                self.native_&&localDrawCaptureMode==2?self.table_.localCaptureRevision():
#endif
                self.captureRevision())&&
                layers==self.layers_.drawRevision();
#ifdef DOWNWELL_DRAW_CACHE_AUDIT
            auditActorDrawCache(!self.drawOrderValid_?3:same&&previousCount==self.drawCount_?1:2,
                               self.drawCount_,captureChanged,layersChanged);
            self.drawAuditHasSnapshot_=self.drawOrderValid_;
#endif
            return ok;
#endif
        });
        if(!native_&&!owner_.watchSnapshotRange(View::terrainBase,View::timeFieldBase,terrainSnapshotDirty_))return check(false);
        // Native ownership keeps no previous positions, so boxes, walls, fields
        // and controllers (contiguous slots, no Animation dispatch) are skipped
        // entirely instead of being decoded and resolved every frame.
        if(native_)terrainSnapshotDirty_=false;
        if(!room_.prepareRetainedAnimation(animationSkipFirst(),animationSkipLast()))return check(false);
        world.configureRoomAnimation([](void* context){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            auto visitOne=[&](unsigned slot){
                const auto shared=self.handleAt(slot);
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
                    !self.capturePrevious(shared))return false;
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
                    case View::Kind::corpseExpiry:return self.expiry_.advanceAnimation(local);
                    case View::Kind::smoke:return self.smoke_.advanceAnimation(local);
                    case View::Kind::blast:return self.blasts_.advanceAnimation(local);
                    case View::Kind::corpse:return self.corpses_.advanceAnimation(local);
                    case View::Kind::consumptionBlood:return self.consumption_.advanceAnimation(local);
                    case View::Kind::limboShard:return self.limbo_.advanceAnimation(local);
                    case View::Kind::flashBall:return self.flash_.advanceAnimation(local);
                    case View::Kind::timeShard:return self.world_->advanceTimeShardRoomAnimation(local);
                    case View::Kind::playerController:return self.playerControllers_.advanceAnimation(local);
                    case View::Kind::bone:return self.bones_.advanceAnimation(local);
                    case View::Kind::skeleton:return self.skeletons_.advanceAnimation(local);
                    case View::Kind::blood:return self.blood_.advanceAnimation(local);
                    case View::Kind::currency:return self.currency_.advanceAnimation(local);
                    default:return true;
                }
            };
#if !defined(DOWNWELL_REFERENCE_ANIMATION) && !defined(DOWNWELL_SCALAR_TERRAIN_OWNERSHIP)
            if(self.native_){
                // There are no native previous-position snapshots. Keep the
                // existing room links, but let each pure image-state family
                // consume its consecutive records without the generic adapter.
                decltype(self.room_)::AnimationCursor cursor(self.room_,self.animationSkipFirst(),self.animationSkipLast());
                const auto identity=[&](unsigned slot){return self.table_.currentHandle(slot).generation;};
                while(cursor.slot()!=View::capacity){
                    if(cursor.within(View::enemyBase,View::breakableBase)){
                        DW_PROFILE_SCOPE(profileAnimationDispatch);
                        if(!self.check(self.world_->advanceNativeEnemyAnimations(cursor,identity,View::enemyBase,View::breakableBase)))return false;
                    }else if(cursor.within(View::bulletBase,View::scriptEffectBase)){
                        DW_PROFILE_SCOPE(profileAnimationDispatch);
                        if(!self.check(self.world_->advanceNativeBulletAnimations(cursor,identity,View::bulletBase,View::scriptEffectBase)))return false;
                    }else if(!self.check(visitOne(cursor.take())))return false;
                }
                return true;
            }
#endif
            return self.check(self.room_.visitAnimationRetainedChecked(
                self.animationSkipFirst(),self.animationSkipLast(),self.terrainSnapshotDirty_,visitOne));
        },this);
        world.configurePhaseBoundary([](void* context,GameplayWorld::PhaseBoundary phase){
            auto& owner=*static_cast<GameplayCollisionOwner*>(context);
            if(owner.failureObserved_){
                // Mutations publish pending work; service it only at the
                // original boundary, never eagerly inside another handler.
                return owner.check(owner.valid()&&
                    (!owner.activationPending_||owner.reconcileActivation())&&
                    (!owner.layers_.pendingDepths()||owner.layers_.serviceDepths())&&
                    (phase!=GameplayWorld::PhaseBoundary::roomEntry||owner.prepareDraw()));
            }
            return owner.reconcileActivation()&&owner.layers_.serviceDepths()&&
                (phase!=GameplayWorld::PhaseBoundary::roomEntry||owner.prepareDraw());
        },this);
        world.configureEnemyStepOrder([](void* context,GameplayWorld::EnemyStepOrder& snapshot){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            return self.check(self.visitRegistered(View::enemyBase,View::breakableBase,
                [&](InstanceHandle shared,int object,std::uint32_t flags){
                View::Kind kind;InstanceHandle local;
                if(!View::decode(shared,kind,local))return false;
                if(kind!=View::Kind::enemy||(flags&0x100003u))return true;
                return self.world_->resolveEnemyLifetime(local)&&snapshot.append(local.slot,local.generation,object);
            }));
        },this,[](void* context){
            const auto& self=*static_cast<GameplayCollisionOwner*>(context);
            return self.valid()?self.captureRevision():0xffffffffu;
        });
        world.configureResultRoomAlarms([](void* context,GameplayWorld::ResultAlarmEmit emit,void* emitContext){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid()||!self.world_||!self.world_->levelClearActive())return false;
            const auto* result=self.result_.find(self.result_.handle());
            if(!result||!result->active||result->destroyRequested)return self.check(false);
            RunnerStepSnapshot<5> phase;
            if(!self.check(self.visitRegistered(View::runControllerBase,View::levelNumberBase,
                [&](InstanceHandle h,int object,std::uint32_t flags){
                    return self.resolves(h)&&((flags&0x100003u)||phase.append(h.slot,h.generation,object));
                })))return false;
            return self.dispatchCapturedAlarms(phase,[&](InstanceHandle h,unsigned alarm,const PackedObjectEvent&){
                View::Kind kind;InstanceHandle local;
                if(!View::decode(h,kind,local))return false;
                return kind==View::Kind::runController?self.world_->advanceControllerRoomAlarm(local,alarm):
                    kind==View::Kind::result&&self.world_->advanceResultRoomAlarm(local,alarm,emit,emitContext);
            });
        },this);
        world.configureRoomAlarms([](void* context){
            DW_PROFILE_SCOPE(profileAlarmCapture);
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid()||self.combatAlarmPhaseInUse_||self.actorEndStepExecuting_)return self.check(false);
            self.combatAlarmPhaseInUse_=true;
            struct ReleasePhase {bool& flag;~ReleasePhase(){flag=false;}} releasePhase{self.combatAlarmPhaseInUse_};
            auto& phase=self.combatAlarmPhase_;
            const auto boxes=self.nativeBoxAlarmPrefix_?self.world_->captureBoxAlarms():GameplayWorld::BoxAlarmPrefix{};
            EnemyAlarmRoster::Phase enemies;
            struct Release {EnemyAlarmRoster* owner=nullptr;~Release(){if(owner)owner->end();}} release;
            if(self.nativeEnemyAlarms()){
                if(!self.check(self.enemyAlarmRoster_.begin(enemies)))return false;
                release.owner=&self.enemyAlarmRoster_;
            }
            const bool reused=self.combatAlarmCaptureValid_&&!self.combatAlarmCaptureDirty_;
            if(!reused){
                phase.clear();
                auto capture=[&](InstanceHandle h,int object,std::uint32_t flags){
                    return self.resolves(h)&&(object==220||(flags&0x100003u)||phase.append(h.slot,h.generation,object));
                };
                // Only implemented families are admitted. Avoid scanning every
                // terrain record for this partial combat alarm traversal.
                const unsigned first=enemies.entries?View::breakableBase:View::enemyBase;
                const unsigned last=self.nativeBoxAlarmPrefix_?View::breakableBase:View::terrainBase;
                if(!self.check(self.visitRegistered(0,View::enemyBase,capture)&&
                    (first==last||self.visitRegistered(first,last,capture))&&
                    self.visitRegistered(View::debrisBase,View::capacity,capture)))return false;
                // Save before callbacks: mutation during dispatch invalidates
                // reuse next tick without changing this phase's captured set.
                self.combatAlarmCaptureDirty_=false;
                self.combatAlarmCaptureValid_=true;
            }
            return self.dispatchCapturedAlarms<true>(phase,[&](InstanceHandle h,unsigned alarm,const PackedObjectEvent&){
                View::Kind kind;InstanceHandle local;if(!View::decode(h,kind,local))return false;
                return kind==View::Kind::player?self.world_->advancePlayerDamageAlarm(local,alarm):
                    kind==View::Kind::enemy?self.world_->advanceEnemyRoomAlarm(local,alarm):
                    kind==View::Kind::breakable?self.world_->advanceBoxRoomAlarm(local,alarm):
                    kind==View::Kind::debris?self.world_->advanceBoundDebrisRoomAlarm(local,alarm):
                    kind==View::Kind::timeChange?self.world_->advanceTimeChangeRoomAlarm(local,alarm):
                    kind==View::Kind::timeShard?self.world_->advanceTimeShardRoomAlarm(local,alarm):
                    kind==View::Kind::playerController?self.playerControllers_.advanceAlarm(local,alarm):
                    kind==View::Kind::drone?(alarm==0&&self.drones_.advanceAlarm(local)):
                    kind==View::Kind::skeleton?self.skeletons_.advanceAlarm(local,alarm):
                    kind==View::Kind::smoke?self.smoke_.advanceAlarm(local,alarm):
                    kind==View::Kind::blast?self.blasts_.alarm(local,alarm):
                    kind==View::Kind::corpse?self.advanceCorpseAlarm(local,alarm):
                    kind==View::Kind::consumptionBlood?self.consumption_.advanceAlarm(local,alarm):
                    kind==View::Kind::runController?self.world_->advanceControllerRoomAlarm(local,alarm):
                    kind==View::Kind::currency?self.world_->advanceCurrencyRoomAlarm(local,alarm):
                    kind==View::Kind::levelNumber?self.levelNumbers_.advanceAlarm(local,alarm):
                    kind==View::Kind::entryDither&&self.entries_.advanceAlarm(local,alarm);
            },reused,&self.combatAlarmMask_,boxes
#ifndef DOWNWELL_REFERENCE_ENEMY_ALARM_ROSTER
            ,enemies
#endif
            );
        },this);
        world.configureDebrisStepOrder([](void* context,GameplayWorld::DebrisStepOrder& phase){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            return self.check(self.visitRegistered(View::debrisBase,View::timeChangeBase,
                [&](InstanceHandle h,int object,std::uint32_t flags){
                    if(flags&0x100003u)return true;
                    View::Kind kind;InstanceHandle local;
                    return View::decode(h,kind,local)&&kind==View::Kind::debris&&self.resolves(h)&&
                        phase.append(local.slot,local.generation,object);
                }));
        },this);
        world.configureCurrencyStepOrder([](void* context,GameplayWorld::CurrencyStepOrder& phase){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);
            if(!self.valid())return false;
            return self.check(self.visitRegistered(View::currencyBase,View::ditherAboveBase,
                [&](InstanceHandle shared,int object,std::uint32_t flags){
                    if(flags&0x100003u)return true;
                    View::Kind kind;InstanceHandle local;
                    return View::decode(shared,kind,local)&&kind==View::Kind::currency&&self.resolves(shared)&&
                        phase.append(local.slot,local.generation,object);
                }));
        },this,[](void* context,InstanceHandle local,bool& eligible){
            auto& self=*static_cast<GameplayCollisionOwner*>(context);InstanceHandle shared;std::uint32_t flags;
            if(!self.valid()||!View::encode(View::Kind::currency,local,shared)||!self.eligibilityFlags(shared,flags))return false;
            eligible=self.resolves(shared)&&!(flags&0x100003u);return true;
        });
        return true;
    }
    auto playerBinding(){return GameplayWorld::PlayerLifecycleBinding::bindMasks(player_);}
    auto enemyBinding(){return GameplayWorld::EnemyLifecycleBinding::bindMaskDestruction(enemies_);}
    auto breakableBinding(){
        auto binding=GameplayWorld::BreakableLifecycleBinding::bindDestruction(boxes_);
#ifndef DOWNWELL_REFERENCE_BREAKABLES
        if(nativeBoxes_){boxAllocation_.good=&good_;binding.bindNativeAllocation(boxAllocation_);}
#endif
        return binding;
    }
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
            if(!objects_.findEvent(objectOf(h),3,int(subtype),event))continue;
            if(!check(execute(h,event))||!valid())return false;
        }
        return true;
    }
    GameplayTerrainInstances& terrain(){return terrain_;}
    // Production handoff for the currently owned End Step actors. Keep its
    // actor capture off the Amiga task stack. Reference boxes borrow the idle
    // alarm workspace; native boxes borrow their lifetime prefix instead.
    // This does not stand in for the complete mixed world phase.
    template<class Execute> bool dispatchActorEndStep(Execute execute){
        return dispatchActorEndStep(execute,[]{return true;});
    }
    template<class Execute,class BetweenBoxesAndActors>
    bool dispatchActorEndStep(Execute execute,BetweenBoxesAndActors betweenBoxesAndActors){
        if(!valid()||!world_||actorEndStepExecuting_||combatAlarmPhaseInUse_)return check(false);
        actorEndStepExecuting_=true;
        struct Reset {bool& value;~Reset(){value=false;}} reset{actorEndStepExecuting_};
        InstanceHandle player;
        if(!check(visitRegistered(0,View::enemyBase,[&](InstanceHandle h,int object,std::uint32_t flags){
            if(flags&0x100003u)return true;
            if(object!=35||!resolves(h))return false;
            player=h;return true;
        })))return false;
        const auto boxes=world_->captureBoxAlarms(); // Stable slots and a phase-wide birth ceiling.
        // Reference box registration can be rebound and callbacks may mutate
        // it. Reuse the inactive alarm workspace for its snapshot; native
        // append-only box ownership needs only the prefix above.
        if(!nativeBoxes_){
            combatAlarmCaptureValid_=false;combatAlarmPhase_.clear();
            if(!check(visitRegistered(View::breakableBase,View::terrainBase,
                [&](InstanceHandle h,int object,std::uint32_t flags){
                    return (flags&0x100003u)||(object==92&&resolves(h)&&combatAlarmPhase_.append(h.slot,h.generation,object));
                })))return false;
        }
        actorEndStepPhase_.clear();
        if(!check(visitRegistered(View::enemyBase,View::breakableBase,
            [&](InstanceHandle h,int object,std::uint32_t flags){
                if((flags&0x100003u)||!hasSimpleOutOfViewEndStep(object))return true;
                return resolves(h)&&actorEndStepPhase_.append(h.slot,h.generation,object);
            })))return false;
        if(!check(visitRegistered(View::bulletBase,View::scriptEffectBase,
            [&](InstanceHandle h,int object,std::uint32_t flags){
                return (flags&0x100003u)||(object==261&&resolves(h)&&actorEndStepPhase_.append(h.slot,h.generation,object));
            })))return false;
        if(!check(visitRegistered(View::droneBase,View::bloodBase,
            [&](InstanceHandle h,int object,std::uint32_t flags){
                if((flags&0x100003u)||object==269)return true;
                return (object==120||object==266)&&resolves(h)&&
                    actorEndStepPhase_.append(h.slot,h.generation,object);
            })))return false;
        if(!check(visitRegistered(View::playerControllerBase,View::capacity,
            [&](InstanceHandle h,int object,std::uint32_t flags){
                if((flags&0x100003u)||(object!=256&&object!=263))return true;
                return resolves(h)&&actorEndStepPhase_.append(h.slot,h.generation,object);
            })))return false;
        actorEndStepPhase_.sort();
        // Player 35 precedes boxes 92. Capture ALL families before invoking
        // it: synchronous callbacks must not admit newborns to this phase.
        if(player.generation){
            std::uint32_t flags;
            if(eligibilityFlags(player,flags)&&!(flags&0x100003u)&&
                (!check(resolves(player)&&execute(player,35))||!valid()))return false;
        }
        // Object 92 precedes every actor admitted below. Borrow its stable
        // native prefix instead of allocating a per-box snapshot. Reference
        // ownership uses the captured registration order above. Capture actors
        // BEFORE callbacks so box-born actors wait.
        if(nativeBoxes_){
            const unsigned end=boxes.end;
            for(unsigned i=0;i<end&&i<world_->breakableSlots();++i){
                const auto& box=world_->breakableSlot(i);
                if(!box.nativeRegistration||box.lifetimeId>boxes.lastLifetime||!box.active||box.destroyRequested||
                    (nativeBoxFlags_&0x100003u))continue;
                const InstanceHandle h{View::breakableBase+i,box.lifetimeId};
                if(!check(execute(h,92))||!valid())return false;
            }
        }else{
            for(unsigned i=0;i<combatAlarmPhase_.size();++i){
                const auto h=combatAlarmPhase_.handle(i);std::uint32_t flags;
                if(!eligibilityFlags(h,flags)||(flags&0x100003u))continue;
                if(!check(resolves(h)&&execute(h,92))||!valid())return false;
            }
        }
        if(!check(betweenBoxesAndActors())||!valid())return false;
        for(unsigned i=0;i<actorEndStepPhase_.size();++i){
            const auto h=actorEndStepPhase_.handle(i);std::uint32_t flags;
            if(!eligibilityFlags(h,flags)||(flags&0x100003u))continue;
            if(!check(resolves(h)&&execute(h,actorEndStepPhase_.object(i)))||!valid())return false;
        }
        return true;
    }
private:
    bool retireThinPlatforms(long topQ12,std::uint32_t cutoff){
        // Pinned object 93 End Step has only scrOutofview, with no inherited
        // Destroy/Clean Up. Marking leaves registration intact, so this known
        // event body may borrow registration order without a terrain snapshot.
        PackedObjectEvent event;
        if(objects_.findEvent(93,1,0,event)||objects_.findEvent(93,12,0,event))return check(false);
        const auto visit=[&](InstanceHandle h,int object,std::uint32_t flags){
                if(object!=93||h.generation>cutoff||(flags&0x100003u))return true;
                GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;
                if(!View(*world_).readTerrain(h,id,active,destroyed))return false;
                return retireMainRoomObjectsChecked(1,11,1,topQ12,[&](unsigned){
                    return LifetimeInstance{93,id.y,active&&!destroyed};
                },[&](unsigned){
                    return world_->nativeTerrainInstances()?world_->destroyNativeTerrain(id.handle):terrain_.requestDestroy(id.handle);
                });
            };
        // The retained actor projection deliberately excludes terrain.
        return check(native_?visitNativeRegistered(View::terrainBase,View::timeFieldBase,visit):
            owner_.visitRegistrationRange(View::terrainBase,View::timeFieldBase,
                [&](InstanceHandle h,int object,const Owner::State& state){return visit(h,object,state.flags);}));
    }
public:
    GameplayWorld::DroneBullets& droneBullets(){return drones_;}
    GameplayWorld::Bones& bones(){return bones_;}
    GameplayWorld::Skeletons& skeletons(){return skeletons_;}
    GameplayWorld::Blood& blood(){return blood_;}
    GameplayWorld::Currency& currency(){return currency_;}
    // Selected code 991 destroys OTHER, not the dither. Pair collection and
    // wrapper ordering belong to the caller; retain the wall until Draw-entry
    // cleanup so later event directions can still resolve its identity.
    bool dispatchDitherAboveCollision(const RunnerCollisionEventContext::Frame& event){
        View::Kind selfKind,otherKind;InstanceHandle self,other;
        PackedObjectEvent binding{},destroy{};bool found=false;
        if(!valid()||!world_||(!terrainRegistered_&&!(native_&&world_->nativeTerrainInstances()))||
            !View::decode(event.self,selfKind,self)||selfKind!=View::Kind::ditherAbove||
            !View::decode(event.other,otherKind,other)||otherKind!=View::Kind::terrain||
            !resolves(event.self)||!resolves(event.other)||objectOf(event.other)!=60||
            event.type!=4||event.subtype!=60||event.owner!=61||event.code!=991||
            !objects_.collisionEvent(61,60,found,binding)||!found||
            binding.ownerObjectId!=61||binding.codeId!=991||objects_.findEvent(60,1,0,destroy))return check(false);
        return check(native_?world_->destroyNativeTerrain(other):terrain_.requestDestroy(other));
    }
    template<class Audio> bool dispatchTimeFieldWind(const RunnerCollisionEventContext::Frame& event,
        TimeFieldWind& wind,int& channel,MotionBinary64 volume,Audio& audio){
        return dispatchTimeFieldWindContact(event,[&](InstanceHandle local){
            return world_->contactGoalFieldWind(local,wind,channel,volume,audio);
        });
    }
    template<class Audio> bool dispatchTimeFieldWind(const RunnerCollisionEventContext::Frame& event,
        MotionBinary64 volume,Audio& audio){
        return dispatchTimeFieldWindContact(event,[&](InstanceHandle local){
            return world_->contactGoalFieldWind(local,volume,audio);
        });
    }
private:
    template<class Contact> bool dispatchTimeFieldWindContact(const RunnerCollisionEventContext::Frame& event,Contact contact){
        View::Kind kind;InstanceHandle local;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||!resolves(event.other)||
           !View::decode(event.self,kind,local)||kind!=View::Kind::goalField||
           event.type!=4||event.subtype!=objectOf(event.other)||
           !objects_.collisionEvent(objectOf(event.self),event.subtype,found,binding)||!found||
           binding.ownerObjectId!=255||event.owner!=binding.ownerObjectId||event.code!=binding.codeId)
            return check(false);
        return check(contact(local));
    }
public:
    // Exact other.xsp/ysp belong to the selected projectile lifetime. The
    // default binding reads retained source motion; legacy/uninitialized shots
    // fail rather than supplying a conversion of the Q20/Q32 trajectory.
    bool dispatchLimboShardBulletCollision(const RunnerCollisionEventContext::Frame& event){
        return dispatchLimboShardBulletCollision(event,[&](InstanceHandle shared,MotionBinary64& x,MotionBinary64& y){
            View::Kind kind;InstanceHandle local;
            return world_&&View::decode(shared,kind,local)&&kind==View::Kind::bullet&&
                world_->sourceBulletVelocity(local,x,y);
        });
    }
    template<class ReadVelocity> bool dispatchLimboShardBulletCollision(
        const RunnerCollisionEventContext::Frame& event,ReadVelocity readVelocity){
        View::Kind kind;InstanceHandle local;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||!resolves(event.other)||
           !View::decode(event.self,kind,local)||kind!=View::Kind::limboShard||
           event.type!=4||event.subtype!=objectOf(event.other)||event.owner!=379||event.code!=2771||
           !objects_.collisionEvent(379,event.subtype,found,binding)||!found||
           binding.ownerObjectId!=379||binding.codeId!=2771)return check(false);
        const auto* shard=limbo_.find(local);if(!shard)return check(false);
        if(!shard->active||shard->destroyRequested||shard->state.noTouch)return true;
        MotionBinary64 xsp,ysp;
        if(!readVelocity(event.other,xsp,ysp)||!xsp.valid()||!ysp.valid())return check(false);
        return check(world_->collideLimboShard(local,xsp,ysp));
    }
    bool dispatchTimeShardCollision(const RunnerCollisionEventContext::Frame& event){
        View::Kind kind;InstanceHandle local;PackedObjectEvent binding{};bool found=false;
        InstanceHandle player;
        if(!valid()||!world_||!resolves(event.self)||!resolves(event.other)||
           !View::decode(event.self,kind,local)||kind!=View::Kind::timeShard||
           !View::encode(View::Kind::player,world_->playerHandle(),player)||
           player.slot!=event.other.slot||player.generation!=event.other.generation||
           event.type!=4||event.subtype!=35||event.owner!=238||event.code!=2081||
           !objects_.collisionEvent(238,35,found,binding)||!found||
           binding.ownerObjectId!=238||binding.codeId!=2081)return check(false);
        return check(world_->collideTimeShardRoomEffect(local));
    }
    // One event router for the sealed mixed pass. Never collects another
    // family pass: the enclosing dispatcher retains pair order and lifetime.
    // Other is mandatory, so unsupported live events cannot silently vanish.
    template<unsigned Capacity,class Frames,class Audio,class Atan,class MakePlayer,class Damage,class Other>
    bool dispatchSkeletonEncounterCollision(const RunnerCollisionEventContext::Frame& event,
        const TimeStopEffects<Capacity>& stop,Frames frames,SoundPlayEnvironment& environment,
        Audio& audio,Atan atan,MakePlayer makePlayer,Damage damage,Other other){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||!stop.succeeded||!View::decode(event.self,kind,local)||!resolves(event.self))return check(false);
        if(kind==View::Kind::bone)return dispatchBoneCollision(event,stop.player.stopped,frames,damage);
        if(kind==View::Kind::skeleton)return dispatchSkeletonCollision(event,makePlayer);
        if(kind==View::Kind::bullet&&event.code==2224)
            return dispatchSkeletonBulletCollision(event,frames,environment,audio,atan);
        if(kind==View::Kind::smoke)return dispatchSmokeCollision(event);
        if(kind==View::Kind::consumptionBlood)return dispatchConsumptionBloodCollision(event);
        if(kind==View::Kind::ditherAbove)return dispatchDitherAboveCollision(event);
        if(event.owner==255)return dispatchTimeFieldWind(event,environment.gameVolume,audio);
        return check(other(event));
    }
    template<unsigned Capacity,class Allocate,class Frames,class Audio,class Atan,class MakePlayer,class Damage,class Other>
    bool dispatchSkeletonEncounterCollisionWithBlasts(const RunnerCollisionEventContext::Frame& event,
        const TimeStopEffects<Capacity>& stop,Allocate allocate,Frames frames,SoundPlayEnvironment& environment,
        Audio& audio,Atan atan,MakePlayer makePlayer,Damage damage,Other other){
        return dispatchSkeletonEncounterCollision(event,stop,frames,environment,audio,atan,makePlayer,damage,
            [&](const auto& unhandled){
                View::Kind kind;InstanceHandle local;
                if(!View::decode(unhandled.self,kind,local))return false;
                if(kind==View::Kind::corpse&&unhandled.subtype==276)return dispatchCorpseBlastCollision(unhandled);
                if(kind==View::Kind::blast&&unhandled.subtype==35)return dispatchBlastPlayerCollision(unhandled);
                if(kind==View::Kind::blast&&unhandled.code==2330)return dispatchBlastWallCollision(unhandled);
                if(kind==View::Kind::blast&&(unhandled.subtype==269||unhandled.subtype==263))return dispatchBlastShootableCollision(unhandled,allocate);
                return kind==View::Kind::blast&&unhandled.subtype==120?
                    dispatchBlastSkeletonCollisionWithEffects(unhandled,allocate,frames,environment,audio):other(unhandled);
            });
    }
    template<class MakePlayer> bool dispatchSkeletonCollision(const RunnerCollisionEventContext::Frame& event,MakePlayer makePlayer){
        View::Kind kind;InstanceHandle local;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||!resolves(event.other)||
           !View::decode(event.self,kind,local)||kind!=View::Kind::skeleton||event.type!=4||
           event.subtype!=objectOf(event.other)||!objects_.collisionEvent(120,event.subtype,found,binding)||!found||
           event.owner!=binding.ownerObjectId||event.code!=binding.codeId||
           !((event.owner==120&&event.code==1266)||(event.owner==83&&event.code==1066)))return check(false);
        return check(world_->collideSkeletonRoomEffect(local,event.code==1066,makePlayer));
    }
    template<class Frames,class Audio,class Atan>
    bool dispatchSkeletonBulletCollision(const RunnerCollisionEventContext::Frame& event,
        Frames frames,SoundPlayEnvironment& environment,Audio& audio,Atan atan){
        return dispatchSkeletonBulletCollision(event,frames,[&]{return requestMomentDelay();},environment,audio,atan);
    }
    template<class Frames,class Delay,class Audio,class Atan>
    bool dispatchSkeletonBulletCollision(const RunnerCollisionEventContext::Frame& event,
        Frames frames,Delay delay,SoundPlayEnvironment& environment,Audio& audio,Atan atan){
        View::Kind selfKind,otherKind;InstanceHandle shot,target;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||!resolves(event.other)||
           !View::decode(event.self,selfKind,shot)||selfKind!=View::Kind::bullet||
           !View::decode(event.other,otherKind,target)||otherKind!=View::Kind::skeleton||
           event.type!=4||event.subtype!=120||event.owner!=261||event.code!=2224||
           !objects_.collisionEvent(261,120,found,binding)||!found||
           binding.ownerObjectId!=261||binding.codeId!=2224)return check(false);
        return check(world_->hitSkeletonWithSourceBullet(shot,target,frames,delay,environment,audio,atan));
    }
    bool dispatchCorpseBlastCollision(const RunnerCollisionEventContext::Frame& event){
        View::Kind kind,targetKind;InstanceHandle local,target;PackedObjectEvent binding{};bool found=false;
        return check(valid()&&world_&&resolves(event.self)&&resolves(event.other)&&
            View::decode(event.self,kind,local)&&kind==View::Kind::corpse&&corpses_.find(local)&&
            View::decode(event.other,targetKind,target)&&targetKind==View::Kind::blast&&blasts_.find(target)&&
            event.type==4&&event.subtype==276&&event.owner==263&&event.code==2244&&
            objects_.collisionEvent(263,276,found,binding)&&found&&binding.ownerObjectId==263&&binding.codeId==2244);
    }
    template<class Allocate> bool dispatchBlastShootableCollision(const RunnerCollisionEventContext::Frame& event,Allocate allocate){
        View::Kind kind,targetKind;InstanceHandle local,target;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||!resolves(event.other)||
           !View::decode(event.self,kind,local)||kind!=View::Kind::blast||
           !View::decode(event.other,targetKind,target)||(targetKind!=View::Kind::bone&&targetKind!=View::Kind::corpse)||
           event.type!=4||event.subtype!=objectOf(event.other)||event.owner!=276||event.code!=2329||
           !objects_.collisionEvent(276,event.subtype,found,binding)||!found||binding.ownerObjectId!=276||binding.codeId!=2329)return check(false);
        const auto* caller=blasts_.find(local);std::uint32_t id=0;
        if(targetKind==View::Kind::bone){const auto* victim=bones_.find(target);if(victim)id=victim->sourceId;}
        else{const auto* victim=corpses_.find(target);if(victim)id=victim->sourceId;}
        if(!caller||!id)return check(false);
        const auto x=caller->x.promoted(),y=caller->y.promoted();
        return check(blasts_.dispatch(local,[&](auto& actor,auto&,bool& good){
            return blastShootableContact(actor,id,x,y,[&](MotionBinary64& hp){bool unused;return projectileShootableFields(event.other,hp,unused);},
                [&](MotionBinary64 hp){return projectileShootableHealthWrite(event.other,hp);},
                [&](auto& state,auto px,auto py){return world_->emitBlastContactFlash(state,px,py,allocate);},good);
        }));
    }
    bool dispatchBlastWallCollision(const RunnerCollisionEventContext::Frame& event){
        View::Kind kind;InstanceHandle local;View::Record target;
        PackedObjectEvent binding{};PackedObject definition{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||!resolves(event.other)||
           !View::decode(event.self,kind,local)||kind!=View::Kind::blast||!View(*world_).resolve(event.other,target)||
           event.type!=4||event.subtype!=objectOf(event.other)||event.owner!=276||event.code!=2330||
           !objects_.collisionEvent(276,event.subtype,found,binding)||!found||binding.ownerObjectId!=276||binding.codeId!=2330||
           !objects_.findObject(event.subtype,definition))return check(false);
        const unsigned id=target.breakable?target.breakable->sourceId:0;
        return check(blasts_.dispatch(local,[&](auto& actor,auto&,bool& good){
            return blastWallContact(actor,definition.parentId,id,
                [&](MotionBinary64& hp){return projectileBoxHealth(event.other,hp);},
                [&](MotionBinary64 hp){return projectileBoxHealthWrite(event.other,hp);},good);
        }));
    }
    bool dispatchBlastPlayerCollision(const RunnerCollisionEventContext::Frame& event){
        View::Kind kind;InstanceHandle local;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||!resolves(event.other)||
           !View::decode(event.self,kind,local)||kind!=View::Kind::blast||objectOf(event.other)!=35||
           event.type!=4||event.subtype!=35||event.owner!=276||event.code!=2332||
           !objects_.collisionEvent(276,35,found,binding)||!found||binding.ownerObjectId!=276||binding.codeId!=2332)return check(false);
        return check(blasts_.dispatch(local,[](auto& actor,auto&,bool& good){return actor.playerContact(good);}));
    }
    template<class Allocate,class Frames,class Audio>
    bool dispatchBlastSkeletonCollisionWithEffects(const RunnerCollisionEventContext::Frame& event,
        Allocate allocate,Frames frames,SoundPlayEnvironment& environment,Audio& audio){
        return dispatchBlastSkeletonCollision(event,[&](auto& caller,auto x,auto y,int sprite,auto speed,auto angle,int depth){
            unsigned id=0;
            return allocate(id)&&world_->spawnScriptEffectWithSourceId(id,x,y,sprite,speed,angle,
                MotionBinary64::fromInteger(depth),frames,caller.myFx);
        },environment,audio);
    }
    template<class Impact,class Audio>
    bool dispatchBlastSkeletonCollision(const RunnerCollisionEventContext::Frame& event,Impact impact,
        SoundPlayEnvironment& environment,Audio& audio){
        View::Kind kind,targetKind;InstanceHandle local,target;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||!resolves(event.other)||
           !View::decode(event.self,kind,local)||kind!=View::Kind::blast||
           !View::decode(event.other,targetKind,target)||targetKind!=View::Kind::skeleton||
           event.type!=4||event.subtype!=120||event.owner!=276||event.code!=2331||
           !objects_.collisionEvent(276,120,found,binding)||!found||binding.ownerObjectId!=276||binding.codeId!=2331)return check(false);
        const auto* caller=blasts_.find(local);const auto* enemy=skeletons_.find(target);
        if(!caller||!enemy)return check(false);
        const auto x=caller->x.promoted(),y=caller->y.promoted();const unsigned id=enemy->sourceId;
        return check(blasts_.dispatch(local,[&](auto& blast,auto& publish,bool& good){
            return skeletons_.dispatch(target,[&](auto& actor,auto&,bool& targetGood){
                const bool ok=blastSkeletonContact(blast,actor,id,x,y,publish,[&]{return requestMomentDelay();},impact,environment,audio,good);
                return targetGood=targetGood&&ok;
            });
        }));
    }
    bool dispatchSmokeCollision(const RunnerCollisionEventContext::Frame& event){
        View::Kind kind;InstanceHandle local;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||
           !View::decode(event.self,kind,local)||kind!=View::Kind::smoke||event.type!=4)return check(false);
        View::Record opposite;
        if(!View(*world_).resolve(event.other,opposite))return check(false);
        const int otherObject=!terrainRegistered_&&opposite.terrain?opposite.terrain->objectId:
            (resolves(event.other)?objectOf(event.other):-1);
        if(otherObject<0||event.subtype!=otherObject||
           !objects_.collisionEvent(225,otherObject,found,binding)||!found||
           event.owner!=225||binding.ownerObjectId!=225||event.code!=binding.codeId||
           (event.code!=2016&&event.code!=2017))return check(false);
        return event.code==2016||check(smoke_.wallContact(local));
    }
    bool dispatchConsumptionBloodCollision(const RunnerCollisionEventContext::Frame& event){
        View::Kind kind;InstanceHandle local;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||
           !View::decode(event.self,kind,local)||kind!=View::Kind::consumptionBlood||event.type!=4)return check(false);
        View::Record opposite;
        if(!View(*world_).resolve(event.other,opposite))return check(false);
        const int otherObject=!terrainRegistered_&&opposite.terrain?opposite.terrain->objectId:
            (resolves(event.other)?objectOf(event.other):-1);
        if(otherObject<0||event.subtype!=otherObject||
           !objects_.collisionEvent(223,otherObject,found,binding)||!found||
           event.owner!=223||binding.ownerObjectId!=223||event.code!=binding.codeId||
           (event.code!=2007&&event.code!=2008))return check(false);
        return check(consumption_.kill(local));
    }
    template<class Ops> bool dispatchCurrencyCollision(const RunnerCollisionEventContext::Frame& event,Ops& ops){
        View::Kind kind;InstanceHandle local;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||!resolves(event.other)||
            !View::decode(event.self,kind,local)||kind!=View::Kind::currency||event.type!=4||
            event.subtype!=objectOf(event.other)||!objects_.collisionEvent(248,event.subtype,found,binding)||!found||
            event.owner!=248||binding.ownerObjectId!=248||event.code!=2127||binding.codeId!=2127)return check(false);
        return check(world_->collectRegisteredCurrency(local,ops));
    }
    template<class Effects> bool dispatchCorpsePlayerCollision(const RunnerCollisionEventContext::Frame& event,
        bool knifeFork,bool playerDead,Effects& effects){
        View::Kind kind;InstanceHandle local,player;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_||!resolves(event.self)||!resolves(event.other)||
           !View::decode(event.self,kind,local)||kind!=View::Kind::corpse||
           !View::encode(View::Kind::player,world_->playerHandle(),player)||
           event.other.slot!=player.slot||event.other.generation!=player.generation||
           event.type!=4||event.subtype!=35||event.owner!=263||
           !objects_.collisionEvent(263,35,found,binding)||!found||
           binding.ownerObjectId!=263||binding.codeId!=event.code)return check(false);
        return check(corpses_.dispatchPublished(local,[&](auto& state,auto& publish,bool& okay){
            return state.collidePlayerPublished(knifeFork,playerDead,publish,effects,okay);
        }));
    }
    template<class Frames,class Damage> bool dispatchBoneCollision(
        const RunnerCollisionEventContext::Frame& event,bool stopped,Frames frames,Damage damage){
        View::Kind kind;InstanceHandle local;PackedObjectEvent binding{};bool found=false;
        if(!valid()||!world_)return check(false);
        View::Record opposite;
        if(!View(*world_).resolve(event.other,opposite))return check(false);
        // Retained scenery has a generation-checked run owner, not a duplicate
        // generic table entry. This validates a selected event, not its query.
        const int otherObject=!terrainRegistered_&&opposite.terrain?opposite.terrain->objectId:
            (resolves(event.other)?objectOf(event.other):-1);
        if(!resolves(event.self)||otherObject<0||
           !View::decode(event.self,kind,local)||kind!=View::Kind::bone||event.type!=4||
           event.subtype!=otherObject||!objects_.collisionEvent(269,event.subtype,found,binding)||!found||
           event.owner!=269||binding.ownerObjectId!=269||event.code!=binding.codeId||
           (event.code!=2291&&event.code!=2292))return check(false);
        return check(world_->collideBoneRoomEffect(local,event.code==2292,stopped,frames,damage));
    }
    template<unsigned Capacity,class MakeEffects> bool stepSkeleton(InstanceHandle shared,
        const TimeStopEffects<Capacity>& stop,MakeEffects makeEffects){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||!View::decode(shared,kind,local)||kind!=View::Kind::skeleton||
           !resolves(shared))return check(false);
        return check(world_->stepSkeletonRoomEffect(local,stop,makeEffects));
    }
    template<class MakeEffects> bool stepSkeleton(InstanceHandle shared,bool stopped,
        MotionBinary64 playerX,MotionBinary64 playerY,MakeEffects makeEffects){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||!View::decode(shared,kind,local)||kind!=View::Kind::skeleton||
           !resolves(shared))return check(false);
        return check(world_->stepSkeletonRoomEffect(local,stopped,playerX,playerY,makeEffects));
    }
    // Complete registered skeleton Step composition. The enclosing room owns
    // the numeric-ID stream, tile queries, settings and audio device; missing
    // dependencies are failures, never successful placeholder effects.
    template<unsigned Capacity,class Allocate,class Tile,class CurrencyQueries,class Audio>
    bool stepRegisteredSkeleton(InstanceHandle shared,const TimeStopEffects<Capacity>& stop,
        bool cutGunshot,Allocate allocate,Tile tile,CurrencyQueries& currencyQueries,
        const LevelNumberTextContext& text,SoundPlayEnvironment& environment,Audio& audio){
        View::Kind kind;InstanceHandle local;MotionBinary64 px,py;
        if(!valid()||!world_||!stop.succeeded||!View::decode(shared,kind,local)||
           kind!=View::Kind::skeleton||!resolves(shared)||
           !ContactCoordinates::playerGlobals(world_->player().frameStartXFixed(),
               world_->player().frameStartYFixed(),240,176,px,py))return check(false);
        return check(world_->stepSkeletonRoomEffectPublished(local,stop.player.stopped,px,py,
            [&](auto& actor,auto& throwing,auto& publish){
                struct Effects {
                    GameplayCollisionOwner& owner;GameplayWorld& world;SkeletonActor& actor;
                    RegisteredSkeletonThrow& throwing;decltype(publish)& publication;
                    const TimeStopEffects<Capacity>& stop;bool cut;
                    Allocate& allocate;Tile& tile;CurrencyQueries& queries;
                    const LevelNumberTextContext& text;SoundPlayEnvironment& environment;Audio& audio;
                    bool next(unsigned& id){std::uint32_t source=0;
                        if(!allocate(source)||!source||source>0x7fffffffu)return false;
                        id=source;return true;
                    }
                    bool hurtSound(int sound){return world.playSkeletonSound(actor,sound,environment,audio);}
                    bool throwBone(int){return world.throwSkeletonBoneAllocated(actor,throwing,allocate,environment,audio);}
                    bool enemyDeath(){return owner.recordSkeletonDeath(actor,cut,stop,
                        [&](unsigned& id){return next(id);},text,environment,audio);}
                    bool blood(int count,int spread){return count==10&&spread==3&&
                        world.emitSkeletonBlood(actor,[&](unsigned& id){return next(id);});}
                    bool smoke(MotionBinary64,MotionBinary64,int count,int spread){return count==5&&spread==1&&
                        world.emitSkeletonSmoke(actor,[&](unsigned& id){return next(id);});}
                    bool flash(MotionBinary64,MotionBinary64,int count,int spread,int radius){return count==6&&spread==2&&radius==10&&
                        world.emitSkeletonFlash(actor,[&](unsigned& id){return next(id);});}
                    bool currency(int amount){return amount==actor.money&&
                        world.emitSkeletonCurrency(owner.objects_,actor,allocate,queries);}
                    bool deathSound(int sound){return world.playSkeletonSound(actor,sound,environment,audio);}
                    bool deadBody(int sprite,MotionBinary64 impact,int explosion){return sprite==206&&explosion==102&&
                        world.emitSkeletonCorpse(publication,impact,allocate,tile);}
                    bool debris(int x,int y){std::uint32_t id=0;InstanceHandle child;
                        return allocate(id)&&world.spawnDebrisWithSourceId(id,MotionBinary64::fromInteger(x),
                            MotionBinary64::fromInteger(y),world.runStage().area,child);
                    }
                };
                return Effects{*this,*world_,actor,throwing,publish,stop,cutGunshot,allocate,tile,currencyQueries,
                    text,environment,audio};
            }));
    }
    // Called for a selected ordinary Step event, never as a separate family
    // walk. Preserve the shared handle until the resource owner resolves it.
    template<class Frames> bool stepBone(InstanceHandle shared,bool stopped,Frames frames){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||!View::decode(shared,kind,local)||kind!=View::Kind::bone||
           !resolves(shared))return check(false);
        return check(world_->stepBoneRoomEffect(local,stopped,frames));
    }
    // GameplayWorld owns rmMain; invoke only at the selected End Step slot.
    // Read the published builtin again after each destruction request.
    bool endStepBullet(InstanceHandle shared,MotionBinary64 top){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||!resolves(shared)||!View::decode(shared,kind,local)||kind!=View::Kind::bullet)
            return check(false);
        struct Ops {
            GameplayCollisionOwner& owner;InstanceHandle shared,local;MotionBinary64 top;
            bool readY(MotionBinary64& y){
                if(!owner.resolves(shared))return false;
                if(owner.native_){const auto* p=owner.nativePosition(shared.slot);if(!p)return false;y=p->y.promoted();}
                else {const auto* p=owner.state(shared);if(!p)return false;y=p->y.promoted();}
                return y.valid();
            }
            bool viewY(MotionBinary64& y){y=top;return true;}
            bool viewHeight(MotionBinary64& h){h=MotionBinary64::fromInteger(284);return true;}
            bool mainRoom(bool& main){main=true;return true;}
            bool destroySelf(){return owner.world_->markBulletDestroyed(local);}
        } ops{*this,shared,local,top};
        bool good=true;return check(ProjectileViewRetirement::run(ops,good));
    }
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
        if(h.slot<View::enemyBase)return world_->playerDamageAlarmPending(h,alarm,pending);
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
        bool alreadySorted=false,unsigned* retainedMask=nullptr,GameplayWorld::BoxAlarmPrefix boxes={}
#ifndef DOWNWELL_REFERENCE_ENEMY_ALARM_ROSTER
        ,EnemyAlarmRoster::Phase enemies={}
#endif
        ){
        DW_PROFILE_SCOPE(profileAlarmScan);
        if(!alreadySorted){DW_PROFILE_SCOPE(profileAlarmSort);snapshot.sort();}
        const unsigned count=snapshot.size();
        const auto bindingRevision=this->bindingRevision();
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
          bool boxesPending=alarm==0&&boxes.end;
#ifndef DOWNWELL_REFERENCE_ENEMY_ALARM_ROSTER
          bool enemiesPending=(enemies.alarms&(1u<<alarm))!=0;
          const auto dispatchEnemies=[&]{
              for(unsigned e=0;e<enemies.count;++e){
                  const auto& entry=enemies.entries[e];
                  if(!(entry.alarms&(1u<<alarm)))continue;
                  std::uint32_t flags;
                  if(!table_.flags(entry.handle,flags)||(flags&0x100003u))continue;
                  if(!check(world_->advanceEnemyRoomAlarm(
                      {entry.handle.slot-View::enemyBase,entry.handle.generation},alarm))||!valid())return false;
              }
              return true;
          };
          if(!boxesPending&&!enemiesPending&&!(capturedAlarms&(1u<<alarm))&&bindingsUnchanged(bindingRevision))continue;
#else
          if(!boxesPending&&!(capturedAlarms&(1u<<alarm))&&bindingsUnchanged(bindingRevision))continue;
#endif
          int cachedObject=-1;unsigned alarmMask=0;
          PackedObjectEvent event;bool eventCached=false,eventFound=false;
          unsigned groupEnd=0;
          for(unsigned i=0;i<count;++i){
            // Resource order, not layer or compact-array order. The native
            // box family is the sole object-92 owner in this combat phase.
            if(boxesPending&&snapshot.object(i)>92){
                if(!check(world_->advanceBoxAlarmPrefix(boxes)))return false;
                boxesPending=false;
            }
#ifndef DOWNWELL_REFERENCE_ENEMY_ALARM_ROSTER
            if(enemiesPending&&snapshot.object(i)>210){
                if(!dispatchEnemies())return false;
                enemiesPending=false;
            }
#endif
            if constexpr(SkipInactive)if(i>=groupEnd){
                // Captured boxes are live boxes. When none has this alarm set
                // at group entry, no member is pending and no callback runs.
                // The reference owner keeps the per-member walk as the oracle.
                groupEnd=snapshot.objectGroupEnd(i);
                const unsigned slot=snapshot.handle(i).slot;
                if(native_&&slot>=View::breakableBase&&slot<View::terrainBase&&!world_->boxAlarmMayDispatch(alarm)){
                    i=groupEnd-1;continue;
                }
            }
            const auto h=snapshot.handle(i);
            bool haveFlags=false;std::uint32_t flags;
            int object=snapshot.object(i);
            // A captured object's absent event needs no instance lookup while
            // bindings are unchanged. Any birth, retire, rebind or clear makes
            // the rest of this pass use live lookup, including later alarms.
            if(!bindingsUnchanged(bindingRevision)){
                haveFlags=eligibilityFlags(h,flags);
                if(!haveFlags||(flags&0x100003u))continue;
                object=objectOf(h);
            }
            if(object!=cachedObject){
                if(!check(alarmMetadata_.mask(object,alarmMask)))return false;
                cachedObject=object;eventCached=false;
            }
            if(!(alarmMask&(1u<<alarm))){
                // No callbacks occur in an absent event group. While bindings
                // match capture, every remaining member has the same resource
                // and can be skipped together. After mutation retain live reads.
                if(bindingsUnchanged(bindingRevision))i=snapshot.objectGroupEnd(i)-1;
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
          if(boxesPending&&!check(world_->advanceBoxAlarmPrefix(boxes)))return false;
#ifndef DOWNWELL_REFERENCE_ENEMY_ALARM_ROSTER
          if(enemiesPending&&!dispatchEnemies())return false;
#endif
        }
        return true;
    }
public:
    GameplayScriptSpawnInstances& scriptEffects(){return scriptEffects_;}
    GameplayDebrisInstances& debris(){return debris_;}
    NativeTimeChangeRegistration& timeChanges(){return timeChanges_;}
    GameplayWorld::RegisteredExpiry& registeredExpiry(){return expiry_;}
    GameplayWorld::RegisteredBlasts& registeredBlasts(){return blasts_;}
    bool createBlast(unsigned id,MotionBinary64 x,MotionBinary64 y,InstanceHandle& key){
        return check(valid()&&world_&&world_->createBlastWithSourceId(id,x,y,[&]{return requestMomentDelay();},key));
    }
    template<class Allocate,class Events,class Audio>
    bool stompSkeleton(SkeletonActor& actor,Allocate allocate,Events& events,
        SoundPlayEnvironment& environment,Audio& audio,bool& good){
        if(!valid()||!world_){good=false;return check(false);}
        return check(world_->stompSkeletonWithBlast(actor,allocate,[&]{return requestMomentDelay();},events,environment,audio,good));
    }
    GameplayWorld::RegisteredSmoke& registeredSmoke(){return smoke_;}
    GameplayWorld::RegisteredCorpse& registeredCorpses(){return corpses_;}
    GameplayWorld::ConsumptionParticles& consumptionParticles(){return consumption_;}
    GameplayWorld::RunControllers& runControllers(){return controllers_;}
    GameplayWorld::LevelNumbers& levelNumbers(){return levelNumbers_;}
    GameplayWorld::LimboShards& limboShards(){return limbo_;}
    GameplayWorld::RegisteredFlash& registeredFlash(){return flash_;}
    template<class Views> bool stepLimboShard(InstanceHandle local,bool stopped,int width,
        const LimboShard::Player& player,const SpriteMaskPack& masks,Views& views){
        const auto* r=limbo_.find(local);InstanceHandle shared;
        if(!valid()||!world_||!r||!masks.valid()||!View::encode(View::Kind::limboShard,local,shared))return check(false);
        if(!r->active||r->destroyRequested)return true;
        struct Queries {
            GameplayCollisionOwner& owner;InstanceHandle shared;const SpriteMaskPack& masks;Views& views;
            bool meeting(InstanceHandle,MotionBinary64 x,MotionBinary64 y,int object,bool& hit){
                return owner.registeredPlaceMeeting(shared,object,x,y,masks,hit);
            }
            bool population(unsigned& count){
                count=0;return owner.visitRegistered(View::limboShardBase,View::flashBallBase,
                    [&](InstanceHandle,int object,std::uint32_t flags){if(object==379&&!(flags&0x140003u))++count;return true;});
            }
            bool outside(RunnerImageValue x,RunnerImageValue y,bool& result){return views.outside(x,y,result);}
            bool viewTop(MotionBinary64& y){return views.viewTop(y);}
        } queries{*this,shared,masks,views};
        bool allowed=false;
        if(!TimeStopBound::evaluate(stopped,r->x.promoted(),r->y.promoted(),width,
            [&](auto x,auto y,bool& hit){return queries.meeting(local,x,y,82,hit);},allowed))return check(false);
        return check(world_->stepLimboShard(local,allowed,player,queries));
    }
    template<unsigned TimeCapacity,class Allocate,class Audio>
    bool recordSkeletonDeath(SkeletonActor& actor,bool cutGunshot,const TimeStopEffects<TimeCapacity>& stop,
        Allocate allocate,const LevelNumberTextContext& text,SoundPlayEnvironment& environment,Audio& audio){
        if(!valid()||!world_)return check(false);
        return check(world_->recordSkeletonDeath(actor,cutGunshot,stop,allocate,
            [&]{return requestMomentDelay();},text,environment,audio));
    }
    bool requestMomentDelay(){
        if(!valid()||!world_)return check(false);
        return check(world_->requestMomentDelay([&](auto apply){
            return visitEligibility([&](InstanceHandle h,int object,std::uint32_t flags){
                if(!families_.isA(object,37)||(flags&0x140003u))return true;
                View::Kind kind;InstanceHandle local;
                return View::decode(h,kind,local)&&kind==View::Kind::runController&&apply(local);
            });
        }));
    }
    bool configureCorpseMasks(const SpriteMaskPack& masks){
        if(!valid()||!masks.valid())return check(false);
        corpseMasks_=&masks;return true;
    }
    // Borrow the enclosing source-ID stream; no family-local numeric IDs.
    void configureCorpseChildIds(bool (*allocate)(void*,std::uint32_t&),void* context){
        corpseChildId_=allocate;corpseChildIdContext_=context;
    }
    bool advanceCorpseAlarm(InstanceHandle local,unsigned alarm){
        if(!valid()||!world_)return check(false);
        return check(corpses_.advanceAlarm(local,alarm,
            [&](int object,MotionBinary64 x,MotionBinary64 y,std::uint32_t& child){
                InstanceHandle key;std::uint32_t id=0;
                if(object!=242||!corpseChildId_||!corpseChildId_(corpseChildIdContext_,id)||
                   !expiry_.create(id,x,y,key))return false;
                child=id;return true;
            },[&](std::uint32_t child,int sprite){
                std::uint32_t frames=0;
                return sprite>=0&&sprites_.frameCount(unsigned(sprite),frames)&&
                    expiry_.sprite(expiry_.sourceHandle(child),sprite,frames);
            }));
    }
    NativeTimeShardRegistration& timeShards(){return timeShards_;}
    // Borrow the enclosing source stream for synchronous transition-script
    // births. One allocator serves both families, so nested Create and other
    // callers may consume the same stream between children. No local fallback.
    template<class Allocate,class Execute>
    bool withTimeEffectSourceIds(Allocate allocate,Execute execute){
        if(!valid()||!world_||timeEffectIdAllocate_||pendingTimeChangeId_||pendingTimeShardId_)return check(false);
        timeEffectIdContext_=&allocate;
        timeEffectIdAllocate_=[](void* context,std::uint32_t& id){
            return (*static_cast<Allocate*>(context))(id);
        };
        const bool ok=execute();
        timeEffectIdAllocate_=nullptr;timeEffectIdContext_=nullptr;
        return check(ok&&valid());
    }
    GameplayPlayerControllers& playerControllers(){return playerControllers_;}
    bool createTimeShardWithSourceId(TimeShardEffects<64>& effects,std::uint32_t id,
        MotionBinary64 x,MotionBinary64 y,bool lowSpec){
        if(!valid()||!world_||pendingTimeShardId_||!id||id>0x7fffffffu)return check(false);
        if(!effects.phasesBoundTo(timeShards_)&&!effects.bindPhases(timeShards_))return check(false);
        pendingTimeShardId_=id;
        const bool ok=world_->createTimeShard(effects,x,y,lowSpec);
        pendingTimeShardId_=0;return check(ok);
    }
    bool birth(InstanceHandle key,const NativeTimeShardRegistration::Record& record,std::uint32_t& id){
        if(!pendingTimeShardId_){
            if(!timeEffectIdAllocate_||!timeEffectIdAllocate_(timeEffectIdContext_,id)||!id||id>0x7fffffffu)
                return check(false);
            pendingTimeShardId_=id;
        }
        id=pendingTimeShardId_;
        const bool ok=timeShardBinding_.birth(key,record);
        pendingTimeShardId_=0;return ok;
    }
    bool markDestroyed(InstanceHandle key,const NativeTimeShardRegistration::Record& record){
        return timeShardBinding_.markDestroyed(key,record);
    }
    bool activation(InstanceHandle key,const NativeTimeShardRegistration::Record& record,bool active){
        return timeShardBinding_.activation(key,record,active);
    }
    bool retire(InstanceHandle key,const NativeTimeShardRegistration::Record& record){
        return timeShardBinding_.retire(key,record);
    }
    bool imageIndex(InstanceHandle key,const NativeTimeShardRegistration::Record& record,RunnerImageValue value){
        return timeShardBinding_.imageIndex(key,record,value);
    }
    bool position(InstanceHandle key,const NativeTimeShardRegistration::Record& record,RunnerImageValue x,RunnerImageValue y){
        return timeShardBinding_.move(key,record,x,y);
    }
    bool meeting(InstanceHandle key,const NativeTimeShardRegistration::Record& record,
        MotionBinary64 x,MotionBinary64 y,bool& hit){
        InstanceHandle shared;
        if(!timeShardStepMasks_||!timeShardBinding_.key(key,record,shared))return check(false);
        return registeredPlaceMeeting(shared,56,x,y,*timeShardStepMasks_,hit);
    }
    template<class Outside> bool stepTimeShard(InstanceHandle shared,bool stopped,
        const SpriteMaskPack& masks,Outside outside){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||timeShardStepMasks_||!masks.valid()||!resolves(shared)||
            !View::decode(shared,kind,local)||kind!=View::Kind::timeShard)return check(false);
        timeShardStepMasks_=&masks;
        const bool ok=world_->stepTimeShardRoomEffect(local,stopped,outside);
        timeShardStepMasks_=nullptr;
        return check(ok);
    }
    bool stepTimeShard(InstanceHandle shared,bool stopped,const SpriteMaskPack& masks){
        return stepTimeShard(shared,stopped,masks,[&](const auto& effect,bool& retire){
            return world_->timeShardOutsideView(effect.y,retire);
        });
    }
    bool animationMember(InstanceHandle key,const NativeTimeShardRegistration::Record& record,bool& eligible){
        InstanceHandle shared;
        if(!timeShardBinding_.key(key,record,shared))return check(false);
        eligible=room_.active(shared.slot);return true;
    }
    bool population(unsigned& count){
        if(!valid()||!world_)return check(false);
        count=0;
        return check(visitRegistered(View::timeShardBase,View::playerControllerBase,
            [&](InstanceHandle,int object,std::uint32_t flags){
                if(object==238&&!(flags&0x140003u))++count;
                return true;
            }));
    }
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
        if(!pendingTimeChangeId_){
            if(!timeEffectIdAllocate_||!timeEffectIdAllocate_(timeEffectIdContext_,id)||!id||id>0x7fffffffu)
                return check(false);
            pendingTimeChangeId_=id;
        }
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
    Owner::StateRead state(InstanceHandle key)const{return valid()?owner_.resolve(key):Owner::StateRead{};}
    typename PropertyStore::Read properties(InstanceHandle key)const{return state(key)?properties_.read(key.slot):typename PropertyStore::Read{};}
    // These take selected shared collision handles, not box-array indices.
    // Game fields can still be written after a destruction mark; only actual
    // room cleanup invalidates the handle. No activation/retirement happens.
    bool projectileWallParent(InstanceHandle h,int& parent)const{
        PackedObject object;
        if(!valid()||!resolves(h)||!objects_.findObject(objectOf(h),object))return false;
        parent=object.parentId;return true;
    }
    bool projectileWallMaterial(InstanceHandle h,const char*& material)const{
        if(!valid()||!resolves(h))return false;
        const int object=objectOf(h);
        if(object==60)material="foliage";
        else if(object==93)material="thin";
        else if(object==92){View::Record record;
            if(!View(*world_).resolve(h,record)||!record.breakable)return false;
            material=record.breakable->state.materialBreakable?"breakable":"wood";
        }else return false;
        return true;
    }
    bool projectileBoxHealth(InstanceHandle h,MotionBinary64& health)const{
        View::Record record;
        if(!valid()||!resolves(h)||!View(*world_).resolve(h,record)||!record.breakable)return false;
        health=record.breakable->state.wallHp;return true;
    }
    bool readBoxDestruction(InstanceHandle h,BoxDestructionScript::Caller& out)const{
        if(native_){
            View::Record record;
            if(!valid()||!resolves(h)||!View(*world_).resolve(h,record)||!record.breakable)return false;
            const auto* box=record.breakable;
            auto x=MotionBinary64::fromInteger(box->x),y=MotionBinary64::fromInteger(box->y);
            RunnerImageValue px,py;
            if(!x.add(MotionBinary64::fromInteger(240))||!y.add(MotionBinary64::fromInteger(176))||
                !px.assign(x)||!py.assign(y))return false;
            out={box->state,px.promoted(),py.promoted()};return true;
        }
        View::Record record;const auto position=state(h);
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
    bool projectileShootableFields(InstanceHandle h,MotionBinary64& health,bool& hitEffect){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!resolves(h)||!View::decode(h,kind,local))return check(false);
        const ShootableThing* value=nullptr;
        if(kind==View::Kind::bone){const auto* r=bones_.find(local);if(r)value=&r->state;}
        else if(kind==View::Kind::corpse){const auto* r=corpses_.find(local);if(r)value=&r->state;}
        if(!value)return check(false); // Never fabricate fields for an omitted descendant.
        health=value->health;hitEffect=value->hitEffect;return true;
    }
    bool projectileShootableHealthWrite(InstanceHandle h,MotionBinary64 health){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!resolves(h)||!View::decode(h,kind,local))return check(false);
        if(kind==View::Kind::bone)return check(bones_.writeHealth(local,health));
        return check(kind==View::Kind::corpse&&corpses_.writeShootableHealth(local,health));
    }
    bool projectileShootableHit(InstanceHandle h){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!resolves(h)||!View::decode(h,kind,local))return check(false);
        if(kind==View::Kind::bone)return check(bones_.writeHit(local));
        return check(kind==View::Kind::corpse&&corpses_.writeShootableHit(local));
    }
    bool boundTo(const GameplayWorld& world)const{return valid()&&world_==&world;}
    bool population(int object,unsigned& count)const{return valid()&&selectorAvailable(object)&&owner_.instanceNumber(object,families_,count);}
    bool terrainPositionMeeting(int object,MotionBinary64 x,MotionBinary64 y,const SpriteMaskPack& masks,bool& hit){
        if(native_)return nativeTerrainPositionMeeting(object,x,y,masks,hit);
        return terrainQuery(object)?registeredPositionMeeting(object,x,y,masks,hit):check(false);
    }
private:
    template<class Visit> bool nativeTerrainMembers(int object,Visit visit){
            bool stopped=false;
            return visitNativeRegistered(View::terrainBase,View::timeFieldBase,
                [&](InstanceHandle h,int child,std::uint32_t){
                    if(!stopped&&families_.isA(child,object))stopped=!visit(h.slot);
                    return true;
                });
    }
    template<class Visit> bool nativeQueryMembers(int object,Visit visit){
        if(object==56||object==57||object==59||object==83||object==90){
            // The solid parent spans native boxes, retained walls/platforms
            // and any other registered descendants. Merge by registration
            // order before filtering; separate family sweeps change the tree.
            // parentShootableThing likewise spans separate bone/corpse owners;
            // enumerate its entire registered family, not just bone slots.
            bool stopped=false;
            return visitNativeRegistered(0,View::capacity,
                [&](InstanceHandle h,int child,std::uint32_t){
                    if(!stopped&&families_.isA(child,object))stopped=!visit(h.slot);
                    return true;
                });
        }
        if(object!=82)return nativeTerrainMembers(object,visit);
        bool stopped=false;
        return visitNativeRegistered(View::timeFieldBase,View::exitControllerBase,
            [&](InstanceHandle h,int child,std::uint32_t){
                if(!stopped&&families_.isA(child,object))stopped=!visit(h.slot);
                return true;
            });
    }
    bool prepareNativeObjectQuery(int object,RunnerSpatialObjectPreparation::Path& path){
        if(!valid()||!world_||!native_)return check(false);
        if(object==82){if(!timeFieldFamily_)return check(false);}
        else if(object==90||object==83){/* No terrain descendants; use every registered subtype. */}
        else if(object==56||object==57||object==59){if(!world_->nativeTerrainInstances())return check(false);}
        else if(!world_->nativeTerrainInstances()||(object!=60&&object!=93)||
            !terrainFamilies_[object==93])return check(false);
        auto members=[&](auto visit){return nativeQueryMembers(object,visit);};
        unsigned count=0,height=0;
        if(!members([&](unsigned){++count;return true;})||!owner_.externalSpatialPublication(
            [&](auto& index){height=index.height();return true;}))return check(false);
        using Prep=RunnerSpatialObjectPreparation;
        bool prepared=true;
        path=Prep::prepare(count,height,*families_.flags(unsigned(object)),false,
            [&]{return families_.markSpatialFamily(unsigned(object));},
            [&](auto emit){
                prepared=members([&](unsigned slot){
                    Owner::State state;Owner::Sprite sprite;
                    if(!nativeCollisionVisual(handleAt(slot),state,sprite)){prepared=false;return false;}
                    return emit(slot,state.flags,state.bounds);
                })&&prepared;
            },[&](unsigned slot,std::uint32_t& flags,Owner::Box& bounds){
                Owner::State state;Owner::Sprite sprite;
                if(!(nativePosition(slot)?refreshNativeActor(handleAt(slot),false):refreshNativeTerrain(handleAt(slot),false))||
                    !nativeCollisionVisual(handleAt(slot),state,sprite))return false;
                flags=state.flags;bounds=state.bounds;return true;
            },[&](Owner::Box box,unsigned slot){
                const auto h=handleAt(slot);
                if(!owner_.externalSpatialPublication([&](auto& index){return index.insert(box,slot);}))return false;
                if(auto* state=nativePosition(slot)){
                    state->appearanceFlags|=0x8000u;return true;}
                return world_->publishNativeTerrainIndexed({slot-View::terrainBase,h.generation});
            });
        return check(prepared&&path!=Prep::Path::failed);
    }
    bool nativeTerrainPositionMeeting(int object,MotionBinary64 x,MotionBinary64 y,const SpriteMaskPack& masks,bool& hit){
        RunnerImageValue px,py;RunnerSpatialObjectPreparation::Path path;
        if((object!=60&&object!=93)||!masks.valid()||!px.assign(x)||!py.assign(y)||!prepareNativeObjectQuery(object,path))return check(false);
        return check(RunnerPointTraversal::run(path,px,py,true,[&]{return flushNativeSpatial();},
            [&](unsigned slot,bool indexed,bool& admitted){
                const auto h=handleAt(slot);std::uint32_t flags;
                if(!resolves(h)||!eligibilityFlags(h,flags))return false;
                admitted=!(flags&0x100003u)&&(!indexed||families_.isA(objectOf(h),object));return true;
            },[&](unsigned slot,RunnerImageValue qx,RunnerImageValue qy,bool& result){
                return collisionPointCandidate(handleAt(slot),qx,qy,true,true,masks,result);
            },[&](Owner::Box box,auto visit){return owner_.externalSpatialPublication([&](auto& index){return index.query(box,visit);});},
            [&](auto visit){return nativeTerrainMembers(object,visit);},hit));
    }
    bool registeredPositionMeeting(int object,MotionBinary64 x,MotionBinary64 y,const SpriteMaskPack& masks,bool& hit){
        RunnerImageValue px,py;bool result=false;
        if(!selectorAvailable(object)||!masks.valid()||!px.assign(x)||!py.assign(y))return check(false);
        auto update=[&](InstanceHandle h,bool publish){return resolves(h)&&refresh(h,publish);};
        if(!owner_.point(object,px,py,true,false,families_,update,
            [&](InstanceHandle h,RunnerImageValue rawX,RunnerImageValue rawY,bool& found){
                const auto p=properties_.read(h.slot);
                return resolves(h)&&owner_.pointCandidate(h,p->sprite,rawX,rawY,
                    true,true,sprites_,masks,update,inverseTrig,roundPixel,found);
            },result))return check(false);
        hit=result;return true;
    }
public:
    // Query all registered sParentSolid descendants. This does not claim that
    // the scene generator has admitted every original solid actor family.
    bool actorSolidPlaceMeeting(InstanceHandle self,MotionBinary64 x,MotionBinary64 y,
        const SpriteMaskPack& masks,bool& hit){
        return registeredPlaceMeeting(self,56,x,y,masks,hit);
    }
    // Explicit query over published actor state. Does not admit approximate
    // projectile trajectories or schedule any new actor collision events.
    bool actorTerrainPlaceMeeting(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        const SpriteMaskPack& masks,bool& hit){
        if(!valid()||!resolves(self)||!masks.valid()||(object!=60&&object!=93))return check(false);
        if(!native_)return registeredPlaceMeeting(self,object,x,y,masks,hit);
        Owner::Result result;
        if(!nativeActorPlaceSelection(self,object,x,y,masks,result))return false;
        hit=result.found;return true;
    }
private:
    bool nativeActorPlaceSelection(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        const SpriteMaskPack& masks,Owner::Result& selected){
        if(!valid()||!resolves(self)||!masks.valid())return check(false);
        auto* state=nativePosition(self.slot);RunnerImageValue px,py;std::uint32_t flags;
        if(!state||!px.assign(x)||!py.assign(y)||!eligibilityFlags(self,flags))return check(false);
        const auto* objectFlags=families_.flags(unsigned(objectOf(self)));if(!objectFlags)return check(false);
        auto sync=[&]{return eligibilityFlags(self,flags);};
        auto eligible=[&](unsigned slot,bool indexed){
            const auto h=handleAt(slot);std::uint32_t current;
            return slot!=self.slot&&resolves(h)&&eligibilityFlags(h,current)&&!(current&0x100003u)&&
                (!indexed||families_.isA(objectOf(h),object));
        };
        RunnerPlaceQuery::Result result;
        const bool okay=RunnerPlaceQuery::run(state->x,state->y,flags,state->bounds,px,py,
            [&]{RunnerSpatialObjectPreparation::Path path;
                return prepareNativeObjectQuery(object,path)?path:RunnerSpatialObjectPreparation::Path::failed;},
            [&]{return flushNativeSpatial()&&sync();},
            [&](bool publish){return refreshNativeActor(self,publish)&&sync();},
            [&](RunnerImageValue nx,RunnerImageValue ny){
                if(!sync()||!RunnerSpatialPosition::assign(self.slot,state->x,state->y,flags,nx,ny,
                    false,true,*objectFlags,nativeSpatialDirty_,nativeCollisionDirty_))return false;
                state->appearanceFlags=(state->appearanceFlags&~0xc208u)|(flags&0xc208u);return true;
            },[&](auto visit){return nativeQueryMembers(object,[&](unsigned slot){return !eligible(slot,false)||visit(slot);});},
            [&](Owner::Box box,auto visit){return owner_.externalSpatialPublication([&](auto& index){
                return index.query(box,[&](unsigned slot){return !eligible(slot,true)||visit(slot);});});},
            [&](unsigned slot,bool publish,bool& found){return collisionOverlap(self,handleAt(slot),masks,publish,found)&&sync();},result);
        if(!check(okay))return false;
        selected={result.found,result.found?handleAt(result.identity):InstanceHandle{}};return true;
    }
public:
    bool terrainPlaceMeeting(InstanceHandle local,int object,MotionBinary64 x,MotionBinary64 y,
        const SpriteMaskPack& masks,bool& hit){
        if(native_)return nativeWallPlaceMeeting(local,object,x,y,masks,hit);
        InstanceHandle self;
        if(!terrainQuery(object)||!masks.valid()||!View::encode(View::Kind::terrain,local,self)||
            !resolves(self))return check(false);
        return registeredPlaceMeeting(self,object,x,y,masks,hit);
    }
private:
    bool nativeWallPlaceMeeting(InstanceHandle local,int object,MotionBinary64 x,MotionBinary64 y,
        const SpriteMaskPack& masks,bool& hit){
        NativeWallQuery q;RunnerImageValue px,py;
        if((object!=60&&object!=93)||wallQuery_||!valid()||!world_||!masks.valid()||!View::encode(View::Kind::terrain,local,q.handle)||
            !resolves(q.handle)||objectOf(q.handle)!=60||!px.assign(x)||!py.assign(y)||
            !nativeCollisionVisual(q.handle,q.state,q.sprite)||
            !world_->nativeTerrainInstances()->cachedOffset(local,q.cacheX,q.cacheY))return check(false);
        const RunnerImageValue originX=q.state.x,originY=q.state.y;
        const int dx[]={0,0,16,0,-16},dy[]={0,-16,0,16,0};
        auto offset=[&](RunnerImageValue nx,RunnerImageValue ny,int& ox,int& oy){
            for(unsigned i=0;i<5;++i){RunnerImageValue ax,ay,tx,ty;
                if(!ax.assignInteger(-dx[i])||!ay.assignInteger(-dy[i])||
                    !RunnerSpatialSplit::sub(originX,ax,tx)||!RunnerSpatialSplit::sub(originY,ay,ty))return false;
                if(RunnerSpatialSplit::equal(nx,tx)&&RunnerSpatialSplit::equal(ny,ty)){ox=dx[i];oy=dy[i];return true;}
            }return false;
        };
        int targetX,targetY;if(!offset(px,py,targetX,targetY))return check(false);
        const auto* objectFlags=families_.flags(60);if(!objectFlags)return check(false);
        wallQuery_=&q;
        struct Release {NativeWallQuery*& slot;~Release(){slot=nullptr;}} release{wallQuery_};
        RunnerSpatialObjectPreparation::Path path=RunnerSpatialObjectPreparation::Path::failed;
        int savedX=q.cacheX,savedY=q.cacheY;bool moved=false;
        auto sync=[&]{return eligibilityFlags(q.handle,q.state.flags);};
        auto eligible=[&](unsigned slot,bool indexed){
            const auto h=handleAt(slot);std::uint32_t flags;
            return slot!=q.handle.slot&&resolves(h)&&eligibilityFlags(h,flags)&&!(flags&0x100003u)&&
                (!indexed||families_.isA(objectOf(h),object));
        };
        RunnerPlaceQuery::Result result;
        const bool okay=RunnerPlaceQuery::run(q.state.x,q.state.y,q.state.flags,q.state.bounds,px,py,
            [&]{return prepareNativeObjectQuery(object,path)&&sync()?path:RunnerSpatialObjectPreparation::Path::failed;},
            [&]{return flushNativeSpatial()&&sync();},
            [&](bool publish){return refreshNativeTerrain(q.handle,publish)&&sync();},
            [&](RunnerImageValue nx,RunnerImageValue ny){
                if(!moved){savedX=q.cacheX;savedY=q.cacheY;moved=true;}
                if(!offset(nx,ny,q.dx,q.dy)||!sync()||!RunnerSpatialPosition::assign(q.handle.slot,q.state.x,q.state.y,
                    q.state.flags,nx,ny,false,true,*objectFlags,nativeSpatialDirty_,nativeCollisionDirty_))return false;
                return world_->publishNativeTerrainQueryCache(local,q.cacheX,q.cacheY,(q.state.flags&8u)!=0);
            },[&](auto visit){return nativeTerrainMembers(object,[&](unsigned slot){return !eligible(slot,false)||visit(slot);});},
            [&](Owner::Box box,auto visit){return owner_.externalSpatialPublication([&](auto& index){
                return index.query(box,[&](unsigned slot){return !eligible(slot,true)||visit(slot);});});},
            [&](unsigned slot,bool publish,bool& found){return collisionOverlap(q.handle,handleAt(slot),masks,publish,found)&&sync();},result);
        const bool restored=!moved||path!=RunnerSpatialObjectPreparation::Path::spatial||
            world_->publishNativeTerrainQueryCache(local,savedX,savedY,(q.state.flags&8u)!=0);
        if(!check(okay&&restored))return false;
        hit=result.found;return true;
    }
public:
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
    // Boolean result, but preserve the source query's traversal and lazy
    // bounds publication on both backends, including rotated blood masks.
    bool bloodTimeAllowed(InstanceHandle local,bool pTimeStop,int roomWidth,const SpriteMaskPack& masks,bool& allowed){
        InstanceHandle self;const auto* child=blood_.find(local);
        if(!valid()||!world_||!child||child->spriteId!=84||
           !View::encode(View::Kind::blood,local,self)||!resolves(self))return check(false);
        return check(TimeStopBound::evaluate(pTimeStop,child->x.promoted(),child->y.promoted(),roomWidth,
            [&](auto x,auto y,bool& hit){
                return masks.valid()&&registeredPlaceMeeting(self,82,x,y,masks,hit);
            },allowed));
    }
    bool stepBlood(InstanceHandle shared,bool pTimeStop,int roomWidth,const SpriteMaskPack& masks){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||!View::decode(shared,kind,local)||kind!=View::Kind::blood||!resolves(shared))return check(false);
        return check(world_->stepBloodRoomEffect(local,[&](const auto&,bool& allowed){
            return bloodTimeAllowed(local,pTimeStop,roomWidth,masks,allowed);
        }));
    }
    bool stepBlast(InstanceHandle shared){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||!View::decode(shared,kind,local)||kind!=View::Kind::blast||!resolves(shared))return check(false);
        return check(blasts_.step(local));
    }
    bool stepSmoke(InstanceHandle shared){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||!View::decode(shared,kind,local)||kind!=View::Kind::smoke||!resolves(shared))return check(false);
        return check(world_->stepSmokeRoomEffect(local));
    }
    template<class Globals> bool stepConsumptionBlood(InstanceHandle shared,Globals& globals){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||!View::decode(shared,kind,local)||kind!=View::Kind::consumptionBlood||!resolves(shared))return check(false);
        return check(consumption_.step(local,globals));
    }
    // Entry from the captured mixed Step phase, never a corpse-family sweep.
    // The caller supplies live player time-stop state and synchronous RIP
    // child creation; missing child behavior must fail, not discard births.
    template<class Environment> bool stepCorpse(InstanceHandle shared,bool playerStopped,int roomWidth,
        Environment& environment){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||!corpseMasks_||!View::decode(shared,kind,local)||
           kind!=View::Kind::corpse||!resolves(shared))return check(false);
        return check(corpses_.dispatchPublished(local,[&](auto& state,auto& publish,bool& okay){
            return state.stepInWorld(playerStopped,roomWidth,publish,environment,okay);
        }));
    }
    bool currencyTimeAllowed(InstanceHandle local,bool stopped,int width,const SpriteMaskPack& masks,bool& allowed){
        const auto* child=currency_.find(local);
        if(!valid()||!world_||!child)return check(false);
        return check(TimeStopBound::evaluate(stopped,child->x.promoted(),child->y.promoted(),width,
            [&](auto x,auto y,bool& hit){return currencyFieldMeeting(local,x,y,masks,hit);},allowed));
    }
    bool currencyFieldMeeting(InstanceHandle local,MotionBinary64 x,MotionBinary64 y,const SpriteMaskPack& masks,bool& hit){
        const auto* child=currency_.find(local);InstanceHandle self;
        RunnerImageValue px,py;
        const int mask=child?(child->maskId<0?child->spriteId:child->maskId):-1;
        if(!valid()||!world_||!child||(mask!=133&&mask!=134)||
            !View::encode(View::Kind::currency,local,self)||!resolves(self)||!px.assign(x)||!py.assign(y))return check(false);
        return masks.valid()&&registeredPlaceMeeting(self,82,x,y,masks,hit);
    }
    bool currencyWallMeeting(InstanceHandle local,MotionBinary64 x,MotionBinary64 y,const SpriteMaskPack& masks,bool& hit){
        InstanceHandle self;
        if(!valid()||!world_||!View::encode(View::Kind::currency,local,self)||!resolves(self))return check(false);
        if(native_)return check(world_->currencyWallMeeting(local,x,y,hit));
        return masks.valid()&&registeredPlaceMeeting(self,57,x,y,masks,hit);
    }
    // Generated thin platforms only. Parent 87 also contains other objects;
    // callers must not use this as a complete parent-family selector.
    bool currencyPlatformMeeting(InstanceHandle local,MotionBinary64 x,MotionBinary64 y,const SpriteMaskPack& masks,bool& hit){
        InstanceHandle self;
        if(!valid()||!world_||!View::encode(View::Kind::currency,local,self)||!resolves(self))return check(false);
        if(native_)return check(world_->currencyPlatformMeeting(local,x,y,hit));
        return masks.valid()&&registeredPlaceMeeting(self,93,x,y,masks,hit);
    }
    // Native admission requires both terrain resolvers and a generation stream
    // with no unsupported thin walls/water. No partial Step/RNG on rejection.
    template<class Math,class ViewTop,class Audio>
    bool stepCurrency(InstanceHandle shared,bool stopped,int width,MotionBinary64 playerX,MotionBinary64 playerY,
        const SpriteMaskPack& masks,Math& math,ViewTop view,Audio& audio,MotionBinary64 volume,bool inWater){
        View::Kind kind;InstanceHandle local;
        if(!valid()||!world_||(native_&&!world_->nativeCurrencyQueriesReady())||!masks.valid()||
            !View::decode(shared,kind,local)||kind!=View::Kind::currency||!resolves(shared))return check(false);
        struct Queries {
            GameplayCollisionOwner& owner;InstanceHandle shared;const SpriteMaskPack& masks;
            bool meeting(InstanceHandle local,int object,MotionBinary64 x,MotionBinary64 y,bool& hit){
                if(owner.native_){
                    if(object==57)return owner.currencyWallMeeting(local,x,y,masks,hit);
                    if(object==87)return owner.currencyPlatformMeeting(local,x,y,masks,hit);
                    if(object==82)return owner.currencyFieldMeeting(local,x,y,masks,hit);
                    return false;
                }
                return owner.registeredPlaceMeeting(shared,object,x,y,masks,hit);
            }
            bool water(InstanceHandle,MotionBinary64 x,MotionBinary64 y,bool& hit){
                if(owner.native_){
                    RunnerImageValue px,py;
                    if(!owner.world_->currencyWaterAbsent()||!px.assign(x)||!py.assign(y))return false;
                    hit=false;return true;
                }
                return owner.registeredPositionMeeting(88,x,y,masks,hit);
            }
        } queries{*this,shared,masks};
        return check(world_->stepCurrencyRoomEffect(local,stopped,width,playerX,playerY,queries,math,view,audio,volume,inWater));
    }
    // Box Step's final alarmStop(0), including after a destroy request. Use
    // the retained published shape, not a point or a global blanket freeze.
    bool boxTimeAllowed(InstanceHandle local,bool pTimeStop,int roomWidth,const SpriteMaskPack& masks,bool& allowed){
        InstanceHandle self;
        if(!valid()||!world_||!View::encode(View::Kind::breakable,local,self)||!resolves(self))return check(false);
        BoxDestructionScript::Caller caller;if(!readBoxDestruction(self,caller))return check(false);
        return check(TimeStopBound::evaluate(pTimeStop,caller.x,caller.y,roomWidth,
                [&](auto x,auto y,bool& hit){
                    return masks.valid()&&registeredPlaceMeeting(self,82,x,y,masks,hit);
                },allowed));
    }
    bool stopBoxAlarm0(InstanceHandle local,bool pTimeStop,int roomWidth,const SpriteMaskPack& masks){
        if(!world_)return check(false);
        return check(world_->stopBoxAlarm0(local,[&](bool& allowed){
            return boxTimeAllowed(local,pTimeStop,roomWidth,masks,allowed);
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
            if(native_){Owner::State state;Owner::Sprite sprite;
                if(!nativeCollisionVisual(selected.handle,state,sprite))return check(false);
                next={true,selected.handle,state.x,state.y};
            }else{
                const auto state=owner_.resolve(selected.handle);
                if(!state||!resolves(selected.handle))return check(false);
                next={true,selected.handle,state->x,state->y};
            }
        }
        result=next;return true;
    }
    // scrTimeShard needs a Windows numeric identity and the selected field's
    // published coordinates. Do not replace an unnumbered legacy birth with
    // its internal key; the global source creation stream must supply the ID.
    bool playerTimeFieldInstance(RunnerImageValue x,RunnerImageValue y,const SpriteMaskPack& masks,
                                InstancePlaceResult& result){
        RegisteredFieldContact selected;
        if(!playerTimeFieldSelection(x.promoted(),y.promoted(),masks,selected))return false;
        InstancePlaceResult next;
        if(selected.found){
            View::Kind kind;InstanceHandle local;unsigned source=0;
            if(!View::decode(selected.handle,kind,local))return check(false);
            if(kind==View::Kind::timeField){
                const auto* field=world_->timeFields().find(local.generation);
                if(!field||world_->timeFields().identityAt(local.slot)!=local.generation)return check(false);
                source=field->sourceId;
            }else if(kind==View::Kind::goalField){
                const auto* field=world_->goalFieldInstances().find(local);
                if(!field)return check(false);source=field->sourceId;
            }else return check(false);
            if(!source||source>0x7fffffffu)return check(false);
            next.id=static_cast<std::int32_t>(source);next.handle=selected.handle;
            next.x=selected.x.promoted();next.y=selected.y.promoted();
        }
        result=next;return true;
    }
    // Compose the shared transition into the native player's existing Begin
    // Step boundary. Explicit room metadata and source allocation are required;
    // this does not enable partially numbered production scenes automatically.
    template<class Allocate,class Audio,class Emit,class Tick>
    bool withPlayerTimeStop(TimeStopEffects<64>& state,const PackedRoom& room,
        const SpriteMaskPack& masks,bool lowSpec,Allocate allocate,Audio audio,Emit emit,Tick tick){
        if(!valid()||!world_||!masks.valid()||room.width<0)return check(false);
        if(!state.shards.phasesBoundTo(timeShards_)&&!state.shards.bindPhases(timeShards_))return check(false);
        if(!state.changes.boundTo(timeChanges_)&&!state.changes.bind(timeChanges_))return check(false);
        return withTimeEffectSourceIds(allocate,[&]{
            return world_->withPlayerTimeTransition([&](const auto&,const auto& motion,int ox,int oy){
                InstanceHandle player;RunnerImageValue x,y;
                if(!View::encode(View::Kind::player,world_->playerHandle(),player))return check(false);
                if(native_){if(!nativePublishedPosition(player,x,y))return check(false);}
                else {const auto p=owner_.resolve(player);if(!p)return check(false);x=p->x;y=p->y;}
                typename TimeStopEffects<64>::PlayerPose pose;
                if(!pose.fromPortableMotion(x,y,motion,ox,oy))return check(false);
                return world_->beginTimeStop(state,pose,room.roomId==11||room.roomId==14,lowSpec,
                    [&](bool& outside){return TimeStopBound::outsideSides(pose.xx,room.width,outside);},
                    [&](bool& hit){return playerTimeFieldMeeting(pose.xx,pose.yy,masks,hit);},
                    [&](auto px,auto py,auto& id,auto& fx,auto& fy){
                        InstancePlaceResult found;
                        if(!playerTimeFieldInstance(px,py,masks,found))return false;
                        id=found.id;fx=found.x;fy=found.y;return true;
                    },audio,emit);
            },tick);
        });
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
        if(native_&&(object==82||object==56||object==57||object==90))return nativeActorPlaceSelection(self,object,x,y,masks,result);
        if(!selectorAvailable(object))return check(false);
        auto update=[&](InstanceHandle h,bool publish){return resolves(h)&&refresh(h,publish);};
        if(!owner_.placeVm(self,object,x,y,false,families_,update,
            [&](InstanceHandle a,InstanceHandle b,bool publish,bool& found){
                return resolves(a)&&resolves(b)&&owner_.collideCompatibility(a,b,
                    properties_.read(a.slot)->sprite,properties_.read(b.slot)->sprite,sprites_,masks,
                    RunnerMaskDispatch{},publish,update,inverseTrig,roundPixel,found);
            },result))return check(false);
        return true;
    }
public:
    // scrBulCheckSolid makes two distinct shape queries at the current x/y,
    // not a line query at the endpoint. Each invocation observes current
    // registered lifetimes/order; a preceding meeting result is not cached.
    // This does not certify that live scene generation owns every parent-90
    // descendant (gun modules, scenery, etc.) or enable the source gun Step.
    bool projectileShootableSelection(InstanceHandle self,MotionBinary64 x,MotionBinary64 y,
        const SpriteMaskPack& masks,Owner::Result& result){
        if(!valid()||!world_||!resolves(self)||!masks.valid())return check(false);
        return registeredPlaceSelection(self,90,x,y,masks,result);
    }
    bool projectileShootableMeeting(InstanceHandle self,MotionBinary64 x,MotionBinary64 y,
        const SpriteMaskPack& masks,bool& hit){
        Owner::Result result;
        if(!projectileShootableSelection(self,x,y,masks,result))return false;
        hit=result.found;return true;
    }
    // self is a shared collision handle, not a terrain-local storage slot.
    // Keep the completed-family gate: projectile parent families must not be
    // queried until all of their lifetimes and setters are owned here.
    bool terrainCollisionPoint(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        bool precise,bool notMe,const SpriteMaskPack& masks,Owner::Result& result){
        if(native_){
            RunnerImageValue px,py;
            if(!valid()||!world_||!world_->nativeTerrainInstances()||(object!=60&&object!=93)||
                !terrainFamilies_[object==93]||!masks.valid()||(notMe&&!resolves(self))||
                !px.assign(x)||!py.assign(y))return check(false);
            RunnerCollisionPoint::Result next;
            const bool ok=RunnerCollisionPoint::run(self,notMe,px,py,
                [&](auto visit){return nativeTerrainMembers(object,[&](unsigned slot){return visit(handleAt(slot));});},
                [&](InstanceHandle h,std::uint32_t& flags){return resolves(h)&&eligibilityFlags(h,flags);},
                [&](InstanceHandle h,RunnerImageValue qx,RunnerImageValue qy,bool& hit){
                    return collisionPointCandidate(h,qx,qy,true,precise,masks,hit);
                },next);
            if(!check(ok))return false;
            result={next.found,next.handle};return true;
        }
        if(!terrainQuery(object))return check(false);
        return registeredCollisionPoint(self,object,x,y,precise,notMe,masks,result);
    }
    bool projectileSolidPoint(InstanceHandle self,MotionBinary64 x,MotionBinary64 y,
        bool notMe,const SpriteMaskPack& masks,Owner::Result& result){
        if(!valid()||!world_)return check(false);
        if(native_){
            RunnerImageValue px,py;
            if(!world_->nativeTerrainInstances()||!masks.valid()||(notMe&&!resolves(self))||
                !px.assign(x)||!py.assign(y))return check(false);
            RunnerCollisionPoint::Result next;
            const bool ok=RunnerCollisionPoint::run(self,notMe,px,py,
                [&](auto visit){return nativeQueryMembers(59,[&](unsigned slot){return visit(handleAt(slot));});},
                [&](InstanceHandle h,std::uint32_t& flags){return resolves(h)&&eligibilityFlags(h,flags);},
                [&](InstanceHandle h,RunnerImageValue qx,RunnerImageValue qy,bool& hit){
                    return collisionPointCandidate(h,qx,qy,true,false,masks,hit);
                },next);
            if(!check(ok))return false;
            result={next.found,next.handle};return true;
        }
        return registeredCollisionPoint(self,59,x,y,false,notMe,masks,result);
    }
private:
    bool registeredCollisionPoint(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        bool precise,bool notMe,const SpriteMaskPack& masks,Owner::Result& result){
        if(!masks.valid()||!selectorAvailable(object))return check(false);
        auto update=[&](InstanceHandle h,bool publish){return resolves(h)&&refresh(h,publish);};
        return check(owner_.collisionPointVm(self,object,x,y,notMe,families_,
            [&](InstanceHandle h,RunnerImageValue px,RunnerImageValue py,bool& hit){
                const auto p=properties_.read(h.slot);
                return resolves(h)&&owner_.pointCandidate(h,p->sprite,px,py,
                    true,precise,sprites_,masks,update,inverseTrig,roundPixel,hit);
            },result));
    }
public:
    bool terrainNonPreciseLine(InstanceHandle self,int object,MotionBinary64 x1,MotionBinary64 y1,
        MotionBinary64 x2,MotionBinary64 y2,bool notMe,Owner::Result& result){
        if(native_){
            if(object!=60&&object!=93)return check(false);
            return nativeRegisteredLine(self,object,x1,y1,x2,y2,notMe,result);
        }
        if(!terrainQuery(object))return check(false);
        return registeredNonPreciseLine(self,object,x1,y1,x2,y2,notMe,result);
    }
    // scrBulCheckSolid uses parent solid 59 and parent enemy 83, not nearest
    // distance or an enemy-first sweep. Query registered wall, box and enemy
    // lifetimes through the original family/list/tree dispatcher. This does
    // not run impact events; parentShootableThing uses the separate shape path.
    bool projectileNonPreciseLine(InstanceHandle self,int object,MotionBinary64 x1,MotionBinary64 y1,
        MotionBinary64 x2,MotionBinary64 y2,bool notMe,Owner::Result& result){
        if(!valid()||!world_||(object!=59&&object!=83))return check(false);
        if(native_)return nativeRegisteredLine(self,object,x1,y1,x2,y2,notMe,result);
        return registeredNonPreciseLine(self,object,x1,y1,x2,y2,notMe,result);
    }
private:
    bool nativeRegisteredLine(InstanceHandle self,int object,MotionBinary64 x1,MotionBinary64 y1,
        MotionBinary64 x2,MotionBinary64 y2,bool notMe,Owner::Result& result){
        RunnerLineArguments args;RunnerSpatialObjectPreparation::Path path;
        if(!valid()||(notMe&&!resolves(self))||!args.assign(x1,y1,x2,y2)||
            !prepareNativeObjectQuery(object,path))return check(false);
        RunnerLineTraversal::Result next;
        const bool ok=RunnerLineTraversal::run(path,args,[&]{return flushNativeSpatial();},
            [&](unsigned slot,bool indexed,bool& admitted){
                const auto h=handleAt(slot);std::uint32_t flags;
                if(!resolves(h)||!eligibilityFlags(h,flags))return false;
                admitted=!(flags&0x100003u)&&!(notMe&&h.slot==self.slot&&h.generation==self.generation)&&
                    (!indexed||families_.isA(objectOf(h),object));return true;
            },[&](unsigned slot,const auto& line,bool& hit){
                const auto h=handleAt(slot);Owner::State state;Owner::Sprite sprite;
                if(!nativeCollisionVisual(h,state,sprite))return false;
                if(state.flags&8u){
                    if(!(nativePosition(h.slot)?refreshNativeActor(h,true):refreshNativeTerrain(h))||
                        !nativeCollisionVisual(h,state,sprite))return false;
                }
                bool inside=false;
                if(!RunnerLineBounds::test(line,state.bounds,true,(state.flags&0x100001u)!=0,inside))return false;
                if(!inside){hit=false;return true;}
                PackedSpriteCollision resource;
                hit=sprites_.collisionSprite(sprite.spriteId,sprite.maskId,resource)&&resource.frameCount;return true;
            },[&](const auto& line,auto visit){return owner_.externalSpatialPublication(
                [&](auto& index){return index.queryLine(line,visit);});},
            [&](auto visit){return nativeQueryMembers(object,visit);},next);
        if(!check(ok))return false;
        result={next.found,next.found?handleAt(next.slot):InstanceHandle{}};return true;
    }
    bool registeredNonPreciseLine(InstanceHandle self,int object,MotionBinary64 x1,MotionBinary64 y1,
        MotionBinary64 x2,MotionBinary64 y2,bool notMe,Owner::Result& result){
        RunnerLineArguments args;
        if(!selectorAvailable(object)||!args.assign(x1,y1,x2,y2))return check(false);
        auto update=[&](InstanceHandle h,bool publish){return resolves(h)&&refresh(h,publish);};
        return check(owner_.collisionLine(self,object,args,notMe,false,families_,update,
            [&](InstanceHandle h,const RunnerLineArguments& line,bool& hit){
                const auto p=properties_.read(h.slot);
                return resolves(h)&&owner_.nonPreciseLineCandidate(h,p->sprite,
                    line,true,sprites_,update,hit);
            },result));
    }
public:
    bool reconcileActivation(){
        if(!valid()||!world_)return check(false);
        if(!activationPending_)return true;
        const bool result=room_.reconcileChecked([&](unsigned slot){
            std::uint32_t flags;
            if(!eligibilityFlags(handleAt(slot),flags)){good_=false;return true;}
            return (flags&2u)!=0;
        },[&](unsigned slot,bool registered){
            if(native_&&slot>=View::terrainBase&&slot<View::timeFieldBase){
                // The identity lives in the terrain run owner, not table_.
                // Borrow registration links, never a generic instance Entry.
                const auto h=handleAt(slot);if(!resolves(h))return false;
                if(registered){
                    std::uint32_t order=0;
                    if(!table_.externalBirth(slot,order)||
                        !world_->rebindNativeTerrainOrder({slot-View::terrainBase,h.generation},order))return false;
                }
                return table_.registerExternalObject(h,objectOf(h),registered)&&
                    layers_.reconcile(h)&&publishPreparedDraw(h);
            }
            // Writes while inactive may have been consumed by an intervening
            // active-room pass. Promotion must refresh that retained snapshot.
            if(registered&&slot>=View::terrainBase&&slot<View::timeFieldBase)terrainSnapshotDirty_=true;
            const auto h=handleAt(slot);
            return good_&&(native_?table_.registerObject(h,registered):owner_.registerObject(h,registered))&&
                (!(slot>=View::enemyBase&&slot<View::breakableBase)||updateEnemyAlarmRoster(h))&&
                (!nativeBoxes_||!registered||table_.roomReactivated(h))&&layers_.reconcile(h)&&publishPreparedDraw(h);
        });
        if(!check(result&&good_))return false;
        activationPending_=false;return true;
    }
    // Distinguish immediate flag 2 from deferred room membership. Read-only;
    // this does not reconcile or derive order from object registrations.
    bool roomMembershipActive(InstanceHandle h,bool& active)const{
        if(!valid()||!resolves(h)||!room_.contains(h.slot))return false;
        active=room_.active(h.slot);return true;
    }
    // Dispatch a scheduler-selected wall, not a terrain-only phase. Effects
    // owns tile publication, synchronous limboShard Create and area-five lifetime
    // policy; none may physically retire this record during the callback.
    template<class Effects> bool wallEndStepEvent(InstanceHandle local,int area,GameMakerRandom& random,
        const SpriteMaskPack& masks,Effects& effects){
        if(!valid()||!world_||(!native_&&(!terrainRegistered_||!terrain_.find(local)))||
            (native_&&!world_->nativeTerrainInstances()))return check(false);
        struct Ops {
            GameplayCollisionOwner& owner;InstanceHandle local;const SpriteMaskPack& masks;Effects& effects;
            bool coordinates(int dx,int dy,MotionBinary64& x,MotionBinary64& y){
                if(owner.native_){
                    GameplayWorld::NativeTerrain::Identity id;bool active,destroyed;
                    if(!owner.world_->nativeTerrainInstances()->read(local,id,active,destroyed)||id.object!=60)return false;
                    x=MotionBinary64::fromInteger(id.x);y=MotionBinary64::fromInteger(id.y);
                }else{
                    const auto r=owner.terrain_.find(local);if(!r)return false;
                    x=MotionBinary64::fromInteger(r->x);y=MotionBinary64::fromInteger(r->y);
                }
                return x.add(MotionBinary64::fromInteger(dx))&&y.add(MotionBinary64::fromInteger(dy));
            }
            bool place(int dx,int dy,int object,bool& hit){MotionBinary64 x,y;
                return coordinates(dx,dy,x,y)&&owner.terrainPlaceMeeting(local,object,x,y,masks,hit);}
            bool point(int dx,int dy,int object,bool& hit){MotionBinary64 x,y;
                return coordinates(dx,dy,x,y)&&owner.terrainPositionMeeting(object,x,y,masks,hit);}
            // Native wallEvent lends its retained state to the shared script,
            // which stores imageIndex immediately after this callback.
            bool image(int frame){return owner.native_||owner.terrain_.imageIndex(local,frame);}
            bool tile(int resource,int sx,int sy,int w,int h,int dx,int dy,int depth){MotionBinary64 x,y;
                return resource==739&&sx==48&&sy==48&&w==16&&h==16&&depth==100&&
                    coordinates(dx,dy,x,y)&&effects.tile(x,y);}
            bool destroy(){return owner.native_?owner.world_->destroyNativeTerrain(local):owner.terrain_.requestDestroy(local);}
            bool deactivate(){return owner.native_?owner.world_->activateNativeTerrain(local,false):owner.terrain_.activation(local,false);}
            bool outOfView(){return effects.outOfView(local);}
            bool create(int object,int dx,int dy){MotionBinary64 x,y;
                return coordinates(dx,dy,x,y)&&effects.create(object,x,y);}
        } ops{*this,local,masks,effects};
        return check(native_?world_->stepNativeTerrainWall(local,area,random,ops):terrain_.wallEvent(local,area,random,ops));
    }
    // Source room cleanup visits active entries before inactive entries.
    // Retire may remove only the supplied lifetime, never create/rebind; it
    // must dispatch to the corresponding typed store before this call returns.
    template<class Retire> bool cleanupDestroyed(Retire retire){
        if(!valid()||!world_)return check(false);
        if(!destructionPending_)return true;
        DW_PROFILE_SCOPE(profileDrawCleanup);
        auto cleanup=[&](unsigned slot){
            const auto h=handleAt(slot);std::uint32_t flags;
            if(!eligibilityFlags(h,flags)||!(flags&1u))return false;
            View::Record record;
            if(!View(*world_).resolve(h,record))return false;
            if(!retire(h,record)||!valid()||(native_?table_.contains(h):owner_.contains(h)))return false;
            return !View(*world_).contains(h);
        };
        bool cleaned;
        if(nativeBoxes_){
            unsigned nextBox=0;
            auto before=[&](RunnerImageValue key,std::uint32_t order,bool active){
                const RunnerImageValue boxDepth{}; // objBox_n creation depth 0.
                while(nextBox<markedBoxCount_){
                    const auto local=markedBoxes_[nextBox];
                    const unsigned slot=View::breakableBase+local.slot;
                    const InstanceHandle h{slot,local.generation};const auto* b=nativeBox(h);
                    if(!b){++nextBox;continue;} // Explicit retirement already consumed it.
                    if(active&&(RunnerSpatialSplit::less(key,boxDepth)||
                        (RunnerSpatialSplit::equal(key,boxDepth)&&b->nativeRegistration>order)))break;
                    ++nextBox;if(!cleanup(slot))return false;
                }
                return true;
            };
            cleaned=room_.visitMarkedCleanupOrdered([&](unsigned slot,RunnerImageValue key,bool active){
                const auto h=handleAt(slot);
                const auto order=slot>=View::terrainBase&&slot<View::timeFieldBase&&world_->nativeTerrainInstances()?
                    world_->nativeTerrainInstances()->registrationOrder({slot-View::terrainBase,h.generation}):table_.roomOrder(h);
                return before(key,order,active)&&cleanup(slot);
            })&&before(RunnerImageValue{},0,false);
            if(cleaned)markedBoxCount_=0;
        }else
            cleaned=room_.visitMarkedCleanupChecked(cleanup);
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
                case View::Kind::terrain:return record.nativeTerrain?world_->retireNativeTerrain(local):
                    record.terrain==terrain_.find(local)&&terrain_.retireMarked(local);
                case View::Kind::enemy:return record.enemy==world_->resolveEnemyLifetime(local)&&world_->retireMarkedEnemy(local);
                case View::Kind::breakable:return record.breakable==world_->findBreakable(local)&&world_->retireMarkedBreakable(local);
                case View::Kind::goalField:return record.goalField==world_->goalFieldInstances().find(local)&&world_->retireMarkedGoalField(local);
                case View::Kind::bullet:return record.bullet==world_->resolveBulletLifetime(local)&&world_->retireMarkedBullet(local);
                case View::Kind::scriptEffect:return retireEffect(local,record);
                case View::Kind::debris:return record.debris&&world_->retireDebrisRoomEffect(local);
                case View::Kind::timeChange:return record.timeChange&&world_->retireTimeChangeRoomEffect(local);
                case View::Kind::timeShard:return record.timeShard&&world_->retireTimeShardRoomEffect(local);
                case View::Kind::entryDither:return record.entryDither&&entries_.retire(local);
                case View::Kind::ditherAbove:return record.ditherAbove&&above_.retireMarked(local);
                case View::Kind::drone:return record.drone&&drones_.retire(local);
                case View::Kind::bone:return record.bone&&bones_.retire(local);
                case View::Kind::skeleton:return record.skeleton&&skeletons_.retire(local);
                case View::Kind::blood:return record.blood&&blood_.retire(local);
                case View::Kind::corpseExpiry:return record.corpseExpiry&&expiry_.retire(local);
                case View::Kind::levelNumber:return record.levelNumber&&levelNumbers_.retire(local);
                case View::Kind::limboShard:return record.limboShard&&limbo_.retire(local);
                case View::Kind::flashBall:return record.flashBall&&flash_.retire(local);
                case View::Kind::smoke:return record.smoke&&smoke_.retire(local);
                case View::Kind::blast:return record.blast&&blasts_.retire(local);
                case View::Kind::corpse:return record.corpse&&corpses_.retire(local);
                case View::Kind::consumptionBlood:return record.consumptionBlood&&consumption_.retire(local);
                case View::Kind::currency:return record.currency&&currency_.retire(local);
                case View::Kind::playerController:return record.playerController&&playerControllers_.retire(local);
                default:return false;
            }
        });
    }
};
}
#endif

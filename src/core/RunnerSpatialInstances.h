#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_INSTANCES_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_INSTANCES_H
#include "InstanceHandle.h"
#include "RunnerSpatialLifetimes.h"
#include "RunnerCollisionPairs.h"
#include "RunnerCollisionCollect.h"
#include "RunnerCollisionMotion.h"
#include "RunnerTransformField.h"
#include "RunnerObjectMembership.h"
#include "RunnerRunMembership.h"
#include "RunnerSpatialIndex.h"
#include "RunnerSpatialJournal.h"
#include "RunnerSpatialBoundsUpdate.h"
#include "RunnerSpatialPosition.h"
#include "RunnerPlaceQuery.h"
#include "RunnerSpriteBounds.h"
#include "SpriteCollisionPack.h"
#include "RunnerMaskPair.h"
#include "RunnerSpriteOverlap.h"
#include "RunnerSpritePoint.h"
#include "RunnerPointTraversal.h"
#include "RunnerMaskPoint.h"
#include "RunnerLineBounds.h"
#include "RunnerLineTraversal.h"
#include "RunnerCollisionPoint.h"
#include "RunnerSpriteChange.h"
#include "RunnerSpriteSetters.h"
#include "ObjectPack.h"
#include "PreparedObjectDefaults.h"
#include "PreparedTerrainCreate.h"
#include "Profile.h"
#include "RunnerSpatialStates.h"
#include "RunnerObservedDirtyQueue.h"
namespace downwell {
template<unsigned Capacity,unsigned First,unsigned Last,bool Runs> struct RunnerRegistrationStorage {
    using Type=RunnerObjectMembership<Capacity,First,Last>;
};
template<unsigned Capacity,unsigned First,unsigned Last> struct RunnerRegistrationStorage<Capacity,First,Last,true> {
    using Type=RunnerRunMembership<Capacity,First,Last>;
};
template<unsigned Capacity,unsigned Nodes,bool Scheduled,bool PreparedGeometry=false> struct RunnerSpatialIndexStorage {
    using Type=RunnerSpatialIndex<Nodes>;
};
template<unsigned Capacity,unsigned Nodes,bool PreparedGeometry> struct RunnerSpatialIndexStorage<Capacity,Nodes,true,PreparedGeometry> {
    // Hold a level's terrain births plus actor churn without forcing the old
    // 512-operation drain during ordinary descent. Pressure still drains in
    // order; no finite log replaces testing the first observing query.
    using Type=RunnerSpatialJournal<Nodes,Capacity,4096,PreparedGeometry>;
};
// Shared collision ownership for externally allocated instance handles. The
// scheduler must attach every relevant birth before Create, publish its bounds,
// and retire it before reusing its slot. This is not an activation scheduler.
// No platform types or asset residency decisions belong here.
template<unsigned Capacity,unsigned Nodes,bool ScheduledIndex=false,unsigned RetainedFirst=0,unsigned RetainedLast=0,bool RunRegistration=ScheduledIndex,class Lifetimes=RunnerSpatialLifetimes<Capacity>,
    class Registration=typename RunnerRegistrationStorage<Capacity,RetainedFirst,RetainedLast,
        RunRegistration&&(RetainedFirst<RetainedLast)>::Type,bool PreparedGeometry=false,class TerrainStates=void,
    class StateStorage=RunnerSpatialStates<RunnerSpatialState,Capacity,RetainedFirst,RetainedLast,TerrainStates>> class RunnerSpatialInstances {
    static_assert(Capacity>0,"Spatial instances need stable slots");
    using Queue=typename RunnerDirtyQueueStorage<Capacity,RetainedFirst,RetainedLast,TerrainStates,0>::Type;
    using CollisionQueue=typename RunnerDirtyQueueStorage<Capacity,RetainedFirst,RetainedLast,TerrainStates,1>::Type;
    using Math=RunnerSpatialSplit;
    using Membership=Registration;
public:
    using Real=RunnerImageValue;
    template<unsigned First,unsigned Last> using RegistrationProjection=
        typename Membership::template RegistrationProjection<First,Last>;
    using Box=Math::Box;
    using State=RunnerSpatialState;
    using States=StateStorage;
    bool bindStateStorage(State* records){return states_.bindStorage(records);}
    void observeStateFailure(bool& owner){states_.observeFailure(owner);}
    using StateRead=typename States::Read;
    struct Result {bool found=false;InstanceHandle handle{};};
    struct Sprite {
        std::int32_t spriteId=-1,maskId=-1;
        Real scaleX{0x3f800000},scaleY{0x3f800000},angle{},imageIndex{};
    };
    struct Properties {
        Sprite sprite;
        Real depth{},timeline{},cachedFrame{0xbf800000u};
    };
    using TransformField=RunnerTransformField;
    // 0x432170 stores only the narrowed image_index. Sequence reconciliation
    // belongs to animation; do not invent bounds dirtiness or reset timeline.
    bool writeImageIndex(InstanceHandle h,Sprite& sprite,Real value){
        if(!valid()||!live(h))return false;
        if(!checked(Math::valid(value)))return false;
        sprite.imageIndex=value;return true;
    }
    // Already-narrowed fields from 0x46b6d0/0x46b780/0x46b560. Appearance
    // inputs are explicit because source fast-draw flag 0x4000 also depends on
    // alpha and blend. No angle wrapping. Equal signed zeros are a no-op.
    bool changeTransform(InstanceHandle h,Sprite& sprite,TransformField field,Real value,
                         Real alpha,std::uint32_t blend,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        auto flags=states_.flags(h.slot);bool changed;
        if(!checked(RunnerSpriteSetters::transform(sprite,field,value,alpha,blend,flags,changed)))return false;
        if(!changed)return true;
        {auto edit=states_.edit(h.slot);edit.get().flags=flags;}
        markSpatialChange(h.slot,objectFlags);return true;
    }
    // Integer setter 0x46bfd0. VM-number narrowing/rounding is a separate entry
    // point; an invalid resource ID remains stored and means a missing mask.
    bool changeMask(InstanceHandle h,Sprite& sprite,int mask,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        if(sprite.maskId==mask)return true;
        sprite.maskId=mask;{auto edit=states_.edit(h.slot);edit.get().flags|=8u;}
        markSpatialChange(h.slot,objectFlags);return true;
    }
    // Known ordinary-object binding: source membership -> mask/depth defaults
    // -> flags/sprite setter. Dynamic creation may override depth afterward.
    // Graph must include recovered property bits (bindInstanceFlags), not only
    // spatial flags. Missing object definitions are unsupported, not object 0.
    template<class Graph>
    bool bindObjectFromPacks(InstanceHandle h,int object,bool registered,Properties& properties,
                            const ObjectPack& objects,const SpriteCollisionPack& sprites,
                            const Graph& graph,bool allPrepared,std::int32_t* creationDepth=nullptr){
        if(!valid()||!live(h))return false;
        PackedObject definition;
        {
            DW_PROFILE_SCOPE(profileBindingDefinition);
            if(!objects.valid()||!sprites.valid()||!graph.valid()||object<0||
               !objects.findObject(object,definition)||!graph.flags(unsigned(object)))return checked(false);
        }
        const auto objectFlags=*graph.flags(unsigned(object));
        const unsigned expected=(definition.solid()?1u:0u)|(definition.visible()?2u:0u)|
            (definition.persistent()?4u:0u)|(definition.managed()?0x80u:0u);
        if((objectFlags&0x87u)!=expected)return checked(false);
        Real depth;
        if(!depth.assignInteger(definition.depth))return checked(false);
        {
            DW_PROFILE_SCOPE(profileBindingMembership);
            if(!rebindObjectMembership(h,object,registered))return checked(false);
        }
        properties.sprite.maskId=definition.textureMaskId;
        properties.depth=depth;
        const bool applied=applyBindingProperties(h,objectFlags,allPrepared,[&](InstanceHandle live){
            return changeSpriteFromPack(live,properties.sprite,properties.timeline,properties.cachedFrame,
                                        definition.spriteId,sprites,objectFlags);
        });
        // Compatibility Create uses another depth from the same immutable
        // record. Return it on success instead of decoding the record again.
        if(applied&&creationDepth)*creationDepth=definition.creationDepth;
        return applied;
    }
    // Prepared counterpart of bindObjectFromPacks. Only immutable resource
    // work moves earlier; membership, current graph flags and setter actions
    // remain synchronous, in the same order as the packed reference above.
    template<unsigned Resources,class Graph>
    bool bindObjectPrepared(InstanceHandle h,int object,bool registered,Properties& properties,
        const PreparedObjectDefaults<Resources>& defaults,const Graph& graph,
        bool allPrepared,Real* creationDepth=nullptr){
        if(!valid()||!live(h))return false;
        const auto* definition=defaults.get(object);
        if(!definition||!graph.valid()||!graph.flags(unsigned(object)))return checked(false);
        const auto objectFlags=*graph.flags(unsigned(object));
        if((objectFlags&0x87u)!=definition->propertyFlags)return checked(false);
        {
            DW_PROFILE_SCOPE(profileBindingMembership);
            if(!rebindObjectMembership(h,object,registered))return checked(false);
        }
        properties.sprite.maskId=definition->maskId;
        properties.depth=definition->depth;
        const bool applied=applyBindingProperties(h,objectFlags,allPrepared,[&](InstanceHandle live){
            return changeSprite(live,properties.sprite,properties.timeline,properties.cachedFrame,
                definition->spriteId,definition->frames,objectFlags);
        });
        if(applied&&creationDepth)*creationDepth=definition->creationDepth;
        return applied;
    }
    // Fresh construction only: attach guarantees this slot has no lifetime.
    // Properties are initialized here, so setter intermediate states cannot
    // be observed. Keep attach/rebind order and revision changes; coalesce
    // repeated head-of-queue marks only within this callback-free interval.
    template<unsigned Resources,class Graph>
    bool attachObjectPrepared(InstanceHandle h,int object,const State& state,bool registered,
        Properties& properties,const PreparedObjectDefaults<Resources>& defaults,
        const Graph& graph,Real* creationDepth=nullptr){
        if constexpr(RunRegistration){
            const auto* definition=defaults.get(object);
            const auto* flags=graph.flags(unsigned(object));
            // No callbacks occur between attach and application of immutable
            // defaults. Unsupported inputs retain the exact original failure
            // prefix, including attachment before resource validation.
            if(registered&&definition&&graph.valid()&&flags&&
                (*flags&0x87u)==definition->propertyFlags&&
                (state.flags&~2u)==8u&&!state.special&&
                (definition->spriteId==-1||definition->frames<=65535u)&&
                valid()&&lifetimes_.canAttach(h,object)&&
                Math::valid(state.x)&&Math::valid(state.y)){
                auto edit=states_.edit(h.slot);auto& s=edit.get();
                // Store semantic fields directly; do not copy padding or the
                // caller's previous-position fields which attach overwrites.
                s.x=state.x;s.y=state.y;
                s.bounds.left=state.bounds.left;s.bounds.top=state.bounds.top;
                s.bounds.right=state.bounds.right;s.bounds.bottom=state.bounds.bottom;
                s.flags=state.flags;s.kind=state.kind;s.special=false;
                s.previous={state.x,state.y};lifetimes_.publish(h,object);
                if(!membership_.bind(h.slot,object,h.generation)){
                    changedBinding(h.slot);return false;
                }
                // The old attach/rebind pair advances both revisions twice.
                // Nothing can observe their intermediate values here.
                changedFreshBinding(h.slot);membership_.repeatFreshTailBinding();
                properties.sprite.spriteId=definition->spriteId;
                properties.sprite.maskId=definition->maskId;
                properties.sprite.scaleX.bits=properties.sprite.scaleY.bits=0x3f800000u;
                properties.sprite.angle.bits=properties.sprite.imageIndex.bits=0;
                properties.depth=definition->depth;properties.timeline.bits=0;
                properties.cachedFrame.bits=0xbf800000u;
                s.flags=definition->initialFlags|(state.flags&2u);
                markSpatialChange(h.slot,*flags);
                if(creationDepth)*creationDepth=definition->creationDepth;
                return true;
            }
        }
        return attachObjectPreparedReference(h,object,state,registered,properties,defaults,graph,creationDepth);
    }
    template<unsigned Resources,class Graph>
    bool attachObjectPreparedReference(InstanceHandle h,int object,const State& state,bool registered,
        Properties& properties,const PreparedObjectDefaults<Resources>& defaults,
        const Graph& graph,Real* creationDepth=nullptr){
        {
            DW_PROFILE_SCOPE(profileBirthRegistration);
            if(!attach(h,object,state))return false;
        }
        properties=Properties{};
        const auto* definition=defaults.get(object);
        // Unusual caller states and unsupported frame counts retain the
        // general path, including its failure/mutation ordering.
        if(!definition||(state.flags&~2u)!=8u||state.special||
            (definition->spriteId!=-1&&definition->frames>65535u))
            return bindObjectPrepared(h,object,registered,properties,defaults,graph,false,creationDepth);
        if(!graph.valid()||!graph.flags(unsigned(object)))return checked(false);
        const auto objectFlags=*graph.flags(unsigned(object));
        if((objectFlags&0x87u)!=definition->propertyFlags)return checked(false);
        {
            DW_PROFILE_SCOPE(profileBindingMembership);
            if constexpr(RunRegistration){
                if(registered){
                    // attach() just appended this exact lifetime/object. Only
                    // immutable defaults/graph reads occurred since then.
                    // Keep both owner and membership revision effects without
                    // resolving and traversing the unchanged registration.
                    changedBinding(h.slot);
                    membership_.repeatFreshTailBinding();
                }else if(!rebindObjectMembership(h,object,false))return checked(false);
            }else if(!rebindObjectMembership(h,object,registered))return checked(false);
        }
        properties.sprite.maskId=definition->maskId;
        properties.sprite.spriteId=definition->spriteId;
        properties.depth=definition->depth;
        {auto edit=states_.edit(h.slot);edit.get().flags=definition->initialFlags|(state.flags&2u);}
        markSpatialChange(h.slot,objectFlags);
        if(creationDepth)*creationDepth=definition->creationDepth;
        return true;
    }
    template<class Terrain> bool bindTerrainLifetimes(Terrain& terrain){
        if(!lifetimes_.bind(terrain)||!membership_.bindTerrain(terrain)||!states_.bind(terrain))return false;
        if constexpr(States::runOwned)return spatial_.bindTerrain(terrain)&&collision_.bindTerrain(terrain);
        return true;
    }
    bool bindTerrainGeometry(const PreparedTerrainBirthBounds& geometry){
        if constexpr(ScheduledIndex&&PreparedGeometry){states_.geometry(geometry);return index_.bindGeometry(geometry);}
        else return false;
    }
    template<unsigned N> void prepareTerrainStates(const PreparedObjectDefaults<N>& defaults){states_.prepare(defaults);}
    // One owner-controlled birth transaction. Between observes only the
    // creation depth and may publish room ownership; it must not inspect or
    // mutate this spatial owner. On failure publish the same partial state as
    // the separate attach/default/room/bounds operations. On success write
    // final bounds once, instead of constructing a zero cache to overwrite.
    template<bool AllocatedTerrain=false,unsigned Resources,class Graph,class Between>
    bool attachPreparedTerrainBirth(InstanceHandle h,int object,const State& state,
        Properties& p,const PreparedObjectDefaults<Resources>& defaults,Graph& graph,
        const Box& bounds,Math::PixelBox pixels,bool precise,Between between,
        PreparedTerrainBirthBounds::Key geometry={}){
        const auto* d=defaults.get(object);const auto* objectFlags=graph.flags(unsigned(object));
        if constexpr(RunRegistration&&ScheduledIndex){
            if(d&&graph.valid()&&objectFlags&&(*objectFlags&0x87u)==d->propertyFlags&&
                (state.flags&~2u)==8u&&!state.special&&
                !(state.bounds.left.bits|state.bounds.top.bits|state.bounds.right.bits|state.bounds.bottom.bits)&&
                (d->spriteId==-1||d->frames<=65535u)&&
                (AllocatedTerrain||(valid()&&lifetimes_.canAttach(h,object)))&&
                Math::valid(state.x)&&Math::valid(state.y)){
                auto edit=states_.edit(h.slot);auto& s=edit.get();
                s.x=state.x;s.y=state.y;s.previous={state.x,state.y};
                s.kind=state.kind;s.special=false;s.flags=state.flags;
                lifetimes_.publish(h,object);
                if constexpr(AllocatedTerrain){
                    static_assert(PreparedGeometry,"Admitted native terrain owner only");
                    if(!membership_.bindAllocatedTerrain(h.slot,object)){
                        s.bounds={};changedBinding(h.slot);return false;
                    }
                }else if(!membership_.bind(h.slot,object,h.generation)){
                    s.bounds={};changedBinding(h.slot);return false;
                }
                changedFreshBinding(h.slot);membership_.repeatFreshTailBinding();
                p.sprite.spriteId=d->spriteId;p.sprite.maskId=d->maskId;
                p.sprite.scaleX.bits=p.sprite.scaleY.bits=0x3f800000u;
                p.sprite.angle.bits=p.sprite.imageIndex.bits=0;
                p.depth=d->creationDepth;p.timeline.bits=0;p.cachedFrame.bits=0xbf800000u;
                s.flags=d->initialFlags|(state.flags&2u);
                markSpatialChange(h.slot,*objectFlags);
                if(!between(p.depth)){s.bounds={};return false;}
                if(!checked(graph.prepareBirth(unsigned(object)))){s.bounds={};return false;}
                // prepareBirth may change the family's live preparation bits.
                const auto flags=*graph.flags(unsigned(object));
                if(!finishFreshPreparedTerrain(h.slot,flags,bounds,pixels,precise,true,geometry))return false;
                if(flags&8u)collision_.mark(h.slot);
                return true;
            }
        }
        return attachPreparedTerrainBirthReference(h,object,state,p,defaults,graph,bounds,pixels,precise,between);
    }
    template<unsigned Resources,class Graph,class Between>
    bool attachPreparedTerrainBirthReference(InstanceHandle h,int object,const State& state,
        Properties& p,const PreparedObjectDefaults<Resources>& defaults,Graph& graph,
        const Box& bounds,Math::PixelBox pixels,bool precise,Between between){
        Real depth;
        if(!attachObjectPrepared(h,object,state,true,p,defaults,graph,&depth))return false;
        p.depth=depth;
        if(!between(depth))return false;
        return publishBirth(h,graph,[&](InstanceHandle live,bool publish){
            return refreshPreparedTerrain(live,*graph.flags(unsigned(object)),bounds,pixels,precise,publish);
        });
    }
    // The builder supplies a homogeneous run, and its owner admits immutable
    // defaults once. No user callbacks, queries, setters, or other births may
    // intervene between cells. Do not prepublish cells: room, graph, journal
    // pressure, and failure prefixes still occur synchronously for each cell.
    template<class Graph,class Between>
    bool attachTerrainRunCell(InstanceHandle h,int object,const State& state,Properties& p,
        const PreparedObjectDefault& d,Graph& graph,const std::uint32_t* objectFlags,
        const Box& bounds,Math::PixelBox pixels,bool precise,Between between,
        PreparedTerrainBirthBounds::Key geometry,bool& prepared,bool inherited=false){
        return attachTerrainRunCellPublication(h,object,state,d,graph,objectFlags,bounds,pixels,precise,
            [&]{
                p.sprite.spriteId=d.spriteId;p.sprite.maskId=d.maskId;
                p.sprite.scaleX.bits=p.sprite.scaleY.bits=0x3f800000u;
                p.sprite.angle.bits=p.sprite.imageIndex.bits=0;
                p.depth=d.creationDepth;p.timeline.bits=0;p.cachedFrame.bits=0xbf800000u;
            },between,geometry,prepared,inherited);
    }
    template<class Graph,class PropertiesBirth,class Between>
    bool attachTerrainRunCellPublication(InstanceHandle h,int object,const State& state,
        const PreparedObjectDefault& d,Graph& graph,const std::uint32_t* objectFlags,
        const Box& bounds,Math::PixelBox pixels,bool precise,PropertiesBirth properties,Between between,
        PreparedTerrainBirthBounds::Key geometry,bool& prepared,bool inherited=false){
        static_assert(PreparedGeometry&&RunRegistration&&ScheduledIndex,"Owned terrain run only");
        DW_PROFILE_SCOPE(profileInstanceBirth);
        if constexpr(States::runOwned){
            return attachGeneratedTerrainCell(h,object,(state.flags&2u)!=0,d,graph,objectFlags,
                pixels,properties,between,geometry,prepared,inherited);
        }
        auto edit=states_.edit(h.slot);auto& s=edit.get();
        s.x=state.x;s.y=state.y;s.previous={state.x,state.y};
        s.kind=1;s.special=false;s.flags=state.flags;
        lifetimes_.publish(h,object);
        if(!membership_.bindAllocatedTerrain(h.slot,object,inherited)){
            s.bounds={};changedBinding(h.slot);return false;
        }
        changedFreshBinding(h.slot);membership_.repeatFreshTailBinding();
        properties();
        s.flags=d.initialFlags|(state.flags&2u);
        markSpatialChange(h.slot,*objectFlags);
        if(!between(d.creationDepth)){s.bounds={};return false;}
        if(!prepared){
            if(!checked(graph.prepareBirth(unsigned(object)))){s.bounds={};return false;}
            prepared=true;
        }
        if(!finishFreshPreparedTerrain(h.slot,*objectFlags,bounds,pixels,precise,true,geometry))return false;
        if(*objectFlags&8u)collision_.mark(h.slot);
        return true;
    }
    // Admitted generated identity owns its coordinates and immutable geometry.
    // Publish only observable stages; do not construct unused runner records.
    struct GeneratedTerrainContinuation {
        bool queue=false,publish=false,collision=false;
    };
    struct InheritedTerrainBatch {
        unsigned first=0,count=0,spatialCount=0,collisionCount=0;
        unsigned lastSpatial=0,lastCollision=0;
    };
    bool attachInheritedTerrainCell(InstanceHandle h,bool inactive,
        const GeneratedTerrainContinuation& plan,Math::PixelBox pixels,
        PreparedTerrainBirthBounds::Key geometry,InheritedTerrainBatch& batch){
        static_assert(States::runOwned,"Run-owned continuation only");
        if(!batch.count)batch.first=h.slot;
        ++batch.count;
        states_.inheritedConstructor(h.slot,inactive,plan.queue,plan.queue&&plan.collision);
        if(plan.queue){++batch.spatialCount;batch.lastSpatial=h.slot;
            if(plan.collision){++batch.collisionCount;batch.lastCollision=h.slot;}}
        if(plan.publish){
            if(pixels.right<pixels.left){const auto v=pixels.left;pixels.left=pixels.right;pixels.right=v;}
            if(pixels.bottom<pixels.top){const auto v=pixels.top;pixels.top=pixels.bottom;pixels.bottom=v;}
            DW_PROFILE_COUNT(counterSpatialFirstPublication);
            if(!checked(index_.insertGeometry(geometry,h.slot,pixels)))return false;
            states_.stageGenerated(h.slot,4);
        }
        if(plan.collision&&!plan.queue){
            // A failed journal insert precedes this collision publication.
            collision_.publishInheritedMembership(h.slot);
            ++batch.collisionCount;batch.lastCollision=h.slot;
        }
        return true;
    }
    void completeInheritedTerrain(int object,const InheritedTerrainBatch& batch){
        if(!batch.count)return;
        membership_.completeInheritedTerrain(object,batch.count);
        spatial_.completeInheritedTerrain(batch.lastSpatial,batch.spatialCount);
        collision_.completeInheritedTerrain(batch.lastCollision,batch.collisionCount);
        const unsigned delta=batch.count*2u;
        bindingRevision_=bindingRevision_>0xffffffffu-delta?0xffffffffu:bindingRevision_+delta;
        captureRevision_=captureRevision_>0xffffffffu-delta?0xffffffffu:captureRevision_+delta;
        const unsigned end=batch.first+batch.count;
        if(captureDirty_&&((batch.first<captureLast_&&end>captureFirst_)||
            (batch.first<captureEnd_&&end>captureSecond_))) *captureDirty_=true;
    }
    static GeneratedTerrainContinuation generatedTerrainContinuation(
        const PreparedObjectDefault& d,std::uint32_t preparedFlags,bool inactive){
        // Bounds refresh changes 0x208, neither of which affects queue
        // eligibility or first publication. Capture after graph preparation.
        const auto flags=(d.initialFlags&~0x208u)|(inactive?2u:0u);
        return {Queue::shouldQueue(true,preparedFlags,flags),
            Queue::shouldPublishBounds(preparedFlags,flags,false),(preparedFlags&8u)!=0};
    }
    template<class Graph,class PropertiesBirth,class Between>
    bool attachGeneratedTerrainCell(InstanceHandle h,int object,bool inactive,
        const PreparedObjectDefault& d,Graph& graph,const std::uint32_t* objectFlags,
        Math::PixelBox pixels,PropertiesBirth properties,Between between,
        PreparedTerrainBirthBounds::Key geometry,bool& prepared,bool inherited=false,
        const GeneratedTerrainContinuation* continuation=nullptr){
            static_assert(States::runOwned&&PreparedGeometry&&RunRegistration&&ScheduledIndex,
                "Admitted run-owned terrain only");
            const bool queue=continuation?continuation->queue:
                Queue::shouldQueue(true,*objectFlags,d.initialFlags|(inactive?2u:0u));
            const auto order=membership_.nextGeneratedOrder();
            if(!inherited&&order.present()){
                RunnerRegistrationOrder spatial,collision;
                if(queue){spatial=spatial_.generatedOrder(h.slot);
                    if(*objectFlags&8u)collision=collision_.generatedOrder(h.slot);}
                // The admitted callbacks publish properties/room only. They
                // cannot inspect this owner or issue births/queries. Commit
                // precisely the completed prefix at its first observation:
                // return to the caller, including every terminal-failure exit.
                unsigned stage=2;
                const auto finish=[&](bool success){
                    const bool published=membership_.publishGeneratedConstructor(h.slot,object,inactive,
                        order,spatial,collision,stage);
                    changedFreshBinding(h.slot);return published&&success;
                };
                properties();
                if(!between(d.creationDepth))return finish(false);
                if(!prepared){if(!checked(graph.prepareBirth(unsigned(object))))return finish(false);prepared=true;}
                stage=3;
                const auto boundedFlags=(d.initialFlags&~0x208u)|(inactive?2u:0u);
                if(continuation?continuation->publish:Queue::shouldPublishBounds(*objectFlags,boundedFlags,false)){
                    if(pixels.right<pixels.left){const auto v=pixels.left;pixels.left=pixels.right;pixels.right=v;}
                    if(pixels.bottom<pixels.top){const auto v=pixels.top;pixels.top=pixels.bottom;pixels.bottom=v;}
                    DW_PROFILE_COUNT(counterSpatialFirstPublication);
                    if(!checked(index_.insertGeometry(geometry,h.slot,pixels)))return finish(false);
                    stage=4;
                }
                if((continuation?continuation->collision:bool(*objectFlags&8u))&&!collision.present())
                    collision=collision_.generatedOrder(h.slot);
                return finish(true);
            }else{
                // Preserve the legacy failed-registration prefix at clock
                // exhaustion, and the independent inherited reference path.
                states_.freshGenerated(h.slot,inactive);lifetimes_.publish(h,object);
                if(!membership_.bindAllocatedTerrain(h.slot,object,inherited)){
                    changedBinding(h.slot);return false;
                }
                changedFreshBinding(h.slot);membership_.repeatFreshTailBinding();
                properties();states_.stageGenerated(h.slot,2);
                if(queue){spatial_.markAllocatedTerrain(h.slot,inherited);
                    if(*objectFlags&8u)collision_.markAllocatedTerrain(h.slot,inherited);}
            }
            if(!between(d.creationDepth))return false;
            if(!prepared){if(!checked(graph.prepareBirth(unsigned(object))))return false;prepared=true;}
            states_.stageGenerated(h.slot,3);
            if(continuation?continuation->publish:Queue::shouldPublishBounds(*objectFlags,states_.flags(h.slot),false)){
                if(pixels.right<pixels.left){const auto v=pixels.left;pixels.left=pixels.right;pixels.right=v;}
                if(pixels.bottom<pixels.top){const auto v=pixels.top;pixels.top=pixels.bottom;pixels.bottom=v;}
                DW_PROFILE_COUNT(counterSpatialFirstPublication);
                if(!checked(index_.insertGeometry(geometry,h.slot,pixels)))return false;
                states_.stageGenerated(h.slot,4);
            }
            if(continuation?continuation->collision:bool(*objectFlags&8u)){
                // A continuation has already put this cell at the collision
                // head when queue-eligible. No observer occurs in between.
                if(!continuation||!continuation->queue)collision_.markAllocatedTerrain(h.slot,inherited);
            }
            return true;
    }
    void finishInheritedTerrainCreate(InstanceHandle h,const PreparedTerrainCreate::Plan& plan){
        // The admitted run has prepared family flags. Create changes only
        // bounds-dirty (not queue eligibility), so its repeated queue marks
        // would target the heads already published by this same birth.
        if(plan.changedSprite)states_.dirtyGenerated(h.slot);
    }
    bool valid()const{return good_ && states_.valid() && index_.valid();}
    // No logical instance work happens here. Reads force completion themselves.
    bool serviceSpatialIndex(unsigned budget){
        if constexpr(ScheduledIndex)return checked(index_.service(budget));
        return valid();
    }
    unsigned pendingSpatialIndex()const{
        if constexpr(ScheduledIndex)return index_.pending();
        return 0;
    }
    // Exclusive native ownership may reuse this allocation for its derived
    // tree without attaching duplicate generic lifetime/property records.
    // The caller owns lifetime validation, cached bounds and dirty flags.
    // Never mix this route with registered reference instances.
    template<class Operation> bool externalSpatialPublication(Operation operation){
        if(!valid()||membership_.size())return false;
        return checked(operation(index_));
    }
    std::uint32_t bindingRevision()const{return bindingRevision_;}
    std::uint32_t captureRevision()const{return captureRevision_;}
    bool captureUnchanged(std::uint32_t revision)const{
        return revision!=0xffffffffu&&revision==captureRevision_;
    }
    bool bindingsUnchanged(std::uint32_t revision)const{
        return revision!=0xffffffffu&&revision==bindingRevision_;
    }
    // A retained phase may capture only two disjoint slot ranges. Mutations
    // elsewhere must not invalidate it. The caller owns the flag and resets it
    // after capture, BEFORE callbacks; a same-phase mutation dirties it again.
    bool watchCaptureRanges(unsigned first,unsigned last,unsigned second,unsigned end,bool& dirty){
        if(first>last||last>second||second>end||end>Capacity)return false;
        captureFirst_=first;captureLast_=last;captureSecond_=second;captureEnd_=end;
        captureDirty_=&dirty;dirty=true;return true;
    }
    // Clear retires memberships but preserves generation high-water marks.
    // Replacing the external handle domain requires replacing this owner too.
    void clear(){
        changedBinding();
        membership_.clear();spatial_.clear();collision_.clear();index_.clear();
        lifetimes_.clear();states_.clear();
        good_=true;
    }
    StateRead resolve(InstanceHandle h)const{
        return live(h)?states_.read(h.slot):StateRead{};
    }
    // Lifetime/eligibility observations do not require geometry or previous
    // coordinates. Like resolve(), these remain readable after terminal
    // failure for partial-publication inspection; they do not imply valid().
    bool contains(InstanceHandle h)const{return live(h);}
    bool instanceFlags(InstanceHandle h,std::uint32_t& out)const{
        if(!live(h))return false;
        out=states_.flags(h.slot);return true;
    }
    // For a synchronous owner-controlled slot walk only. Deferred consumers
    // must retain the returned generation, never look it up again after reuse.
    InstanceHandle currentHandle(unsigned slot)const{
        return slot<Capacity&&lifetimes_.object(slot)>=0?handle(slot):InstanceHandle{};
    }
    int objectId(InstanceHandle h)const{return live(h)?lifetimes_.object(h.slot):-1;}
    // Borrow until the registry takes its own snapshot. Birth wrappers must
    // not copy the complete state through each ownership boundary.
    bool attach(InstanceHandle h,int object,const State& state){
        if(!valid() || !lifetimes_.canAttach(h,object) ||
            (state.flags&0x8000u) || !Math::valid(state.x)||!Math::valid(state.y))return false;
        auto edit=states_.edit(h.slot);auto& s=edit.get();s=state;s.previous={state.x,state.y};
        lifetimes_.publish(h,object);
        changedBinding(h.slot);
        return membership_.bind(h.slot,object,h.generation);
    }
    // 0x50dbf8..0x50dc0a, before this instance's animation in active-room
    // traversal. Do not filter flags here or dirty either spatial queue.
    bool capturePrevious(InstanceHandle h){
        if(!valid()||!live(h))return false;
        return checked(states_.capture(h.slot));
    }
    // Optional room-animation optimization. The caller owns the flag for this
    // owner's lifetime. Attach initializes previous=x/y; every later coordinate
    // publication goes through move(), including place-query restoration.
    bool watchSnapshotRange(unsigned first,unsigned last,bool& dirty){
        if(first>last||last>Capacity)return false;
        snapshotFirst_=first;snapshotLast_=last;snapshotDirty_=&dirty;dirty=true;return true;
    }
    // At source reconciliation boundaries, not when the active flag is written.
    bool registerObject(InstanceHandle h,bool registered){
        if(!valid()||!live(h))return false;
        if(membership_.contains(h.slot)!=registered)changedCapture(h.slot);
        if(!registered)membership_.unbind(h.slot);
        else if(!membership_.contains(h.slot))membership_.bind(h.slot,lifetimes_.object(h.slot),h.generation);
        auto edit=states_.edit(h.slot);auto& flags=edit.get().flags;
        if(registered)flags&=~0x80000u;else flags|=0x80000u;
        return true;
    }
    // Membership portion of 0x46b830 only, before new object properties are
    // applied. Even same-object binding appends anew. Registration is an
    // explicit argument, not inferred from activation flags. The caller must
    // separately apply the resource/flag/queue effects at 0x46b8b2..0x46b968.
    bool rebindObjectMembership(InstanceHandle h,int object,bool registered){
        if(!valid()||!live(h)||object<0)return false;
        if(!lifetimes_.rebind(h.slot,object))return false;
        changedBinding(h.slot);
        if(registered)membership_.bind(h.slot,object,h.generation);
        else membership_.unbind(h.slot);
        return true;
    }
    // Collision/flag portion of 0x46b8b2..0x46b968, after membership rebinding
    // and mask/depth defaults. objectFlags is the RUNNER word, not DWOB flags.
    // The sprite setter runs between visible/persistent and managed/dirty writes;
    // it owns resource/index changes and any additional source queue effects.
    // Failure is terminal: preceding removals/flag writes cannot be rolled back.
    template<class SetSprite>
    bool applyBindingProperties(InstanceHandle h,std::uint32_t objectFlags,
                                bool allPrepared,SetSprite setSprite){
        DW_PROFILE_SCOPE(profileBindingProperties);
        if(!valid()||!live(h))return false;
        auto edit=states_.edit(h.slot);auto& flags=edit.get().flags;
        if(objectFlags&1u){
            flags|=0x28u;
            markSpatialChange(h.slot,objectFlags);
        }else{
            flags&=~0x20u;
            if(!allPrepared && !(objectFlags&0x28u) && !removeSpatialMembership(h))return false;
        }
        flags=(flags&~0x50u)|((objectFlags&2u)?0x10u:0u)|((objectFlags&4u)?0x40u:0u);
        if(!checked(setSprite(h)))return false;
        flags=(flags&~0x200000u)|((objectFlags&0x80u)?0x200000u:0u)|8u;
        markSpatialChange(h.slot,objectFlags);
        return true;
    }
    // Collision side of 0x4627d0/0x467640. Room pending requests and traversal
    // cache invalidation belong to the scheduler. Do NOT rebind object lists
    // here, or synthesize bounds-dirty bit 8: reactivation queues a refresh even
    // when that bit is clear. 0x501e50 also queues collision processing when
    // eligible; repeated activation without an intervening deactivation is idle.
    bool setActive(InstanceHandle h,bool active,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        auto edit=states_.edit(h.slot);auto& flags=edit.get().flags;
        if(bool(flags&2u)==active)changedCapture(h.slot);
        if(active){
            if((flags&2u)&&Queue::shouldQueue(true,objectFlags,flags)){
                spatial_.mark(h.slot);
                if(objectFlags&8u)collision_.mark(h.slot);
            }
            flags&=~2u;
        }else flags|=2u;
        return true;
    }
    // 0x4879ca then 0x501dc0, before PreCreate/Create: inherit ancestor
    // preparation, publish bounds immediately, then queue collision processing.
    // This does not itself queue a spatial refresh; earlier setters may have.
    template<class Graph,class Refresh>
    bool publishBirth(InstanceHandle h,Graph& graph,Refresh refreshBounds){
        if(!valid()||!live(h)||!graph.valid())return false;
        const auto object=unsigned(lifetimes_.object(h.slot));
        if(!graph.flags(object))return false;
        if(!checked(graph.prepareBirth(object)&&refreshBounds(h,true)))return false;
        if(*graph.flags(object)&8u)collision_.mark(h.slot);
        return true;
    }
    // Immediate exclusion flags can precede deferred object-list reconciliation.
    bool setExclusions(InstanceHandle h,std::uint32_t exclusions){
        if(!valid()||!live(h)||(exclusions&~0x100003u))return false;
        auto edit=states_.edit(h.slot);auto& f=edit.get().flags;
        if((f&0x100003u)!=exclusions)changedCapture(h.slot);
        f=(f&~0x100003u)|exclusions;return true;
    }
    bool move(InstanceHandle h,Real x,Real y,std::uint32_t objectFlags){
        return moveDomain<true>(h,x,y,objectFlags);
    }
    template<bool MayBeTerrain> bool moveDomain(InstanceHandle h,Real x,Real y,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        if(snapshotDirty_&&h.slot>=snapshotFirst_&&h.slot<snapshotLast_)*snapshotDirty_=true;
        auto edit=states_.template editDomain<MayBeTerrain>(h.slot);auto& s=edit.get();
#ifdef DOWNWELL_PROBES
        if(snapshotDirty_&&h.slot>=snapshotFirst_&&h.slot<snapshotLast_){
            if(!s.special&&Math::equal(s.x,x)&&Math::equal(s.y,y)){
                DW_PROFILE_COUNT(counterSnapshotEqualMoves);
            }else{DW_PROFILE_COUNT(counterSnapshotChangedMoves);}
        }
#endif
        return checked(RunnerSpatialPosition::assign(h.slot,s.x,s.y,s.flags,x,y,
            s.special,true,objectFlags,spatial_,collision_));
    }
    // Caller resolves the verified ordinary/unit-track frame count (zero for
    // missing/empty resources). Cached frame is runner offset 0x180; timeline
    // is 0x88, distinct from image_index. Special sprite ownership is unsupported.
    bool changeSprite(InstanceHandle h,Sprite& sprite,Real& timeline,Real& cachedFrame,
                      int next,unsigned frames,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        if(sprite.spriteId==next)return true;
        if(states_.special(h.slot))return checked(false);
        RunnerSpriteChange::Result change;
        if(!checked(RunnerSpriteChange::plan(sprite.spriteId,next,sprite.imageIndex,frames,change)))return false;
        sprite.spriteId=next;cachedFrame={0xbf800000u};
        if(change.resetIndex)sprite.imageIndex={};
        if(change.resetTimeline)timeline={};
        {auto edit=states_.edit(h.slot);edit.get().flags|=8u;}
        markSpatialChange(h.slot,objectFlags);
        return true;
    }
    // Resource-backed entry point for this pinned build's verified ordinary
    // unit tracks. Missing IDs differ from an unavailable/corrupt resource pack.
    bool changeSpriteFromPack(InstanceHandle h,Sprite& sprite,Real& timeline,Real& cachedFrame,
                             int next,const SpriteCollisionPack& pack,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        if(sprite.spriteId==next)return true; // Source no-op performs no lookup.
        if(!pack.valid())return checked(false);
        std::uint32_t frames=0;
        if(next>=0)pack.frameCount(unsigned(next),frames);
        return changeSprite(h,sprite,timeline,cachedFrame,next,frames,objectFlags);
    }
    // Only immediately after successful owner-controlled terrain birth: the
    // lifetime is admitted, index/timeline are zero and cachedFrame is -1.
    // No external callback may intervene. The prepared script cannot fail or
    // draw RNG. Preserve default cached bounds and mark changed appearance for
    // later refresh, using LIVE family flags rather than preparation-time ones.
    void finishAdmittedTerrainCreate(InstanceHandle h,Properties& p,
        const PreparedTerrainCreate::Plan& plan,std::uint32_t objectFlags){
        if(plan.changedSprite)p.sprite.spriteId=plan.sprite;
        finishAdmittedTerrainCreate(h,plan,objectFlags);
        if(plan.writeIndex)p.sprite.imageIndex=plan.imageIndex;
    }
    void finishAdmittedTerrainCreate(InstanceHandle h,
        const PreparedTerrainCreate::Plan& plan,std::uint32_t objectFlags){
        if(plan.changedSprite){
            if constexpr(States::runOwned)if(states_.dirtyGenerated(h.slot)){
                markSpatialChange(h.slot,objectFlags);return;
            }
            {auto edit=states_.edit(h.slot);edit.get().flags|=8u;}
            markSpatialChange(h.slot,objectFlags);
        }
    }
    bool refresh(InstanceHandle h,std::uint32_t objectFlags,bool allPrepared,
        const Box* bounds,bool precise,bool publish){
        DW_PROFILE_SCOPE(profileBoundsPublication);
        if(!valid()||!live(h))return false;
        auto edit=states_.edit(h.slot);auto& s=edit.get();
        return checked(RunnerSpatialBoundsUpdate::finishRefresh(index_,h.slot,
            objectFlags,s.flags,allPrepared,s.bounds,bounds,s.x,s.y,precise,publish));
    }
    // The terrain preparation owns both representations of these bounds.
    // Fresh zero-cache births need no old-rectangle normalization/removal.
    // Later refreshes retain the ordinary wrapper. Integer endpoints are the
    // exact decoded geometry supplied by the immutable terrain preparer.
    bool refreshPreparedTerrain(InstanceHandle h,std::uint32_t objectFlags,
        const Box& bounds,Math::PixelBox pixels,bool precise,bool publish){
        if constexpr(!ScheduledIndex)return refresh(h,objectFlags,false,&bounds,precise,publish);
        else{
            DW_PROFILE_SCOPE(profileBoundsPublication);
            if(!valid()||!live(h))return false;
            auto edit=states_.edit(h.slot);auto& s=edit.get();
            if(!(s.flags&0x8000u)&&!(s.bounds.left.bits|s.bounds.top.bits|s.bounds.right.bits|s.bounds.bottom.bits)){
                return finishFreshPreparedTerrain(h.slot,objectFlags,bounds,pixels,precise,publish);
            }
            if(pixels.right<pixels.left){auto a=pixels.left;pixels.left=pixels.right;pixels.right=a;}
            if(pixels.bottom<pixels.top){auto a=pixels.top;pixels.top=pixels.bottom;pixels.bottom=a;}
            struct PreparedIndex {
                typename RunnerSpatialIndexStorage<Capacity,Nodes,ScheduledIndex,PreparedGeometry>::Type& index;
                const Math::PixelBox& pixels;
                bool insert(const Box& box,std::uint32_t id){return index.insertPrepared(box,id,&pixels);}
                bool remove(const Box& box,std::uint32_t id,bool retry,bool& removed){
                    return index.remove(box,id,retry,removed);
                }
            } prepared{index_,pixels};
            return checked(RunnerSpatialBoundsUpdate::finishRefresh(prepared,h.slot,
                objectFlags,s.flags,false,s.bounds,&bounds,s.x,s.y,precise,publish));
        }
    }
    // Ordinary resource adapter for 0x466010. A corrupt/unavailable pack is a
    // port failure, not the source's missing sprite case. An invalid explicit
    // mask ID does NOT fall back to artwork. Trig receives binary32 degrees and
    // must implement the source angle conversion and narrowed cosine/sine.
    // Rounding policy is explicit until the runtime FP environment is verified.
    template<class Trig,class Round>
    bool refreshSprite(InstanceHandle h,std::uint32_t objectFlags,bool allPrepared,
        const SpriteCollisionPack& pack,const Sprite& sprite,bool compatibility,
        Trig trig,Round round,bool publish){
        if(!valid()||!live(h))return false;
        if(!pack.valid())return checked(false);
        PackedSpriteCollision resource;
        if(!pack.collisionSprite(sprite.spriteId,sprite.maskId,resource))
            return refresh(h,objectFlags,allPrepared,nullptr,false,publish);
        auto edit=states_.edit(h.slot);auto& s=edit.get();
        if((s.special&&sprite.maskId<0)||!Math::valid(sprite.angle))return checked(false);
        const bool rotated=(sprite.angle.bits&0x7fffffffu)!=0;
        Real cosine{},sine{};Box bounds;
        if(rotated&&!trig(sprite.angle,cosine,sine))return checked(false);
        {
            DW_PROFILE_SCOPE(profileBoundsGeometry);
            if(!RunnerSpriteBounds::fromSprite(resource,s.x,s.y,sprite.scaleX,sprite.scaleY,
                rotated,cosine,sine,compatibility,round,bounds))return checked(false);
        }
        return refresh(h,objectFlags,allPrepared,&bounds,resource.collisionMode==1,publish);
    }
    // Ordinary compatibility-mode branch of 0x464d30. refreshBounds(handle,
    // publish) must resolve that handle's current sprite properties. The other
    // instance refreshes FIRST, and update=false suppresses both lazy refreshes.
    // Non-compatibility and special-resource dispatch are not approximated here.
    template<class Refresh,class Trig,class Round>
    bool collideCompatibility(InstanceHandle self,InstanceHandle other,
        const Sprite& a,const Sprite& b,const SpriteCollisionPack& sprites,
        const SpriteMaskPack& masks,const RunnerMaskDispatch& dispatch,bool publish,
        Refresh refreshBounds,Trig inverseTrig,Round round,bool& hit){
        if(!valid()||!live(self)||!live(other))return false;
        auto firstEdit=states_.edit(self.slot);auto secondEdit=states_.edit(other.slot);
        auto& first=firstEdit.get();auto& second=secondEdit.get();
        if(self.slot==other.slot || ((first.flags|second.flags)&0x100001u)){hit=false;return true;}
        if((first.special&&a.maskId<0)||(second.special&&b.maskId<0))return checked(false);
        if(publish){
            if((second.flags&8u)&&!checked(refreshBounds(other,true)))return false;
            if((first.flags&8u)&&!checked(refreshBounds(self,true)))return false;
        }
        return checked(RunnerSpriteOverlap::test(first,second,a,b,sprites,masks,dispatch,inverseTrig,round,hit));
    }
    // collision_line precise=false, ordinary sprite candidate (0x4650b0).
    // Bounds refresh precedes exclusions and missing-resource checks. Pack
    // admission excludes rotated-box/skeletal resource paths; those must not
    // silently fall through to this non-precise rectangle gate.
    template<class Refresh>
    bool nonPreciseLineCandidate(InstanceHandle h,const Sprite& sprite,const RunnerLineArguments& args,
        bool compatibility,const SpriteCollisionPack& sprites,Refresh refreshBounds,bool& hit){
        if(!valid()||!live(h))return false;
        auto edit=states_.edit(h.slot);auto& state=edit.get();
        if(state.special&&sprite.maskId<0)return checked(false);
        if((state.flags&8u)&&!checked(refreshBounds(h,true)))return false;
        bool inside=false;
        if(!checked(RunnerLineBounds::test(args,state.bounds,compatibility,
            (state.flags&0x100001u)!=0,inside)))return false;
        if(!inside){hit=false;return true;}
        if(!sprites.valid())return checked(false);
        PackedSpriteCollision resource;
        hit=sprites.collisionSprite(sprite.spriteId,sprite.maskId,resource)&&resource.frameCount;
        return true;
    }
    // Ordinary candidate portion of position_meeting, 0x465520. The outer
    // query owns suppression, inactivity, family filtering and traversal.
    // Dirty bounds refresh precedes even destroyed/bounds rejection here.
    template<class Refresh,class Trig,class Round>
    bool pointCandidate(InstanceHandle h,const Sprite& sprite,Real x,Real y,
        bool compatibility,bool precise,const SpriteCollisionPack& sprites,
        const SpriteMaskPack& masks,Refresh refreshBounds,Trig inverseTrig,
        Round round,bool& hit){
        if(!valid()||!live(h))return false;
        auto edit=states_.edit(h.slot);auto& state=edit.get();
        if(!Math::valid(x)||!Math::valid(y)||(state.special&&sprite.maskId<0))return checked(false);
        if((state.flags&8u)&&!checked(refreshBounds(h,true)))return false;
        return checked(RunnerSpritePoint::test(state,sprite,x,y,compatibility,precise,
            sprites,masks,inverseTrig,round,hit));
    }
    // 0x501f70: remove old bounds (retry at root), clear indexed flag, then
    // unlink both dirty memberships. Missing resources may have changed the
    // cached bounds without moving the old index entry, so retry is essential.
    // Object rebinding also calls this on a LIVE instance: object membership,
    // generation and all flags other than indexed must survive.
    bool removeSpatialMembership(InstanceHandle h){
        if(!valid()||!live(h))return false;
        auto edit=states_.edit(h.slot);auto& state=edit.get();
        if(state.flags&0x8000u){
            Box box=state.bounds;
            if(!normalize(box))return checked(false);
            bool removed=false;
            if(!checked(index_.remove(box,h.slot,true,removed)))return false;
            state.flags&=~0x8000u;
        }
        spatial_.remove(h.slot);collision_.remove(h.slot);
        return true;
    }
    bool retire(InstanceHandle h){
        if(!removeSpatialMembership(h))return false;
        membership_.unbind(h.slot);
        lifetimes_.retire(h.slot);changedBinding(h.slot);return true;
    }
    unsigned pendingSpatial()const{return spatial_.size();}
    unsigned pendingCollision()const{return collision_.size();}
    // Tail of engine-component refresh 0x466a44: unconditional bounds dirtiness
    // and 0x501e50 requeue, even for an unchanged direction or zero speed.
    bool engineMotionChanged(InstanceHandle h,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        {auto edit=states_.edit(h.slot);edit.get().flags|=8u;}
        markSpatialChange(h.slot,objectFlags);return true;
    }
    // 0x501ef0, called for OTHER after a precise collision and inactivity
    // checks. Queue only collision processing, without dirtying spatial bounds.
    bool queueCollision(InstanceHandle h,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        return !(objectFlags&8u)||checked(collision_.mark(h.slot));
    }
    template<class Refresh> bool flush(Refresh refreshBounds){
        if(!valid())return false;
        return checked(spatial_.flush([&](unsigned slot){return states_.flags(slot);},
            [&](unsigned slot){return refreshBounds(handle(slot),true);}));
    }
    // 0x5022c0: flush bounds, query collision-dirty instances in intrusive-list
    // order, then retain animated masks for the next pass. indexAvailable is
    // the runner's global index-pointer state, NOT whether the tree is empty.
    // hasEvent resolves inherited collision events; maskFrames resolves mask
    // or fallback sprite (zero for missing). Neither may mutate the world.
    // This collects only: precise tests and paired handlers run after sealing.
    template<unsigned Pairs,class Refresh,class HasEvent,class MaskFrames>
    bool collectCollisionPairs(RunnerCollisionPairs<Capacity,Pairs>& pairs,
        bool indexAvailable,Refresh refreshBounds,HasEvent hasEvent,MaskFrames maskFrames){
        if(!valid())return false;
        unsigned retained[Capacity];
        return checked(RunnerCollisionCollect::run(pairs,collision_,retained,indexAvailable,
            [&]{return flush(refreshBounds);},[&](unsigned slot){return lifetimes_.object(slot)>=0?handle(slot):InstanceHandle{};},
            [&](InstanceHandle h,std::uint32_t& flags){if(!live(h))return false;flags=states_.flags(h.slot);return true;},
            [&](InstanceHandle h,Box& box){if(!live(h))return false;box=states_.read(h.slot)->bounds;return true;},
            [&](Box box,auto visit){return index_.query(box,visit);},hasEvent,maskFrames));
    }
    // VM instance_place wrapper 0x529e50 narrows absolute room coordinates
    // to binary32 before 0x4fe560. Do not round to pixels or narrow a relative
    // offset before adding the room origin. Unsupported values are terminal,
    // before preparation can mutate family membership or the spatial index.
    template<class Graph,class Refresh,class Precise>
    bool placeVm(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        bool allPrepared,Graph& graph,Refresh refreshBounds,Precise precise,Result& result){
        if(!valid())return false;
        Real nx,ny;
        if(!checked(nx.assign(x)&&ny.assign(y)))return false;
        return place(self,object,nx,ny,allPrepared,graph,refreshBounds,precise,result);
    }
    // Object-resource selector path of position_meeting (0x52b780). Unlike
    // instance_place, this query does not move or exclude a calling instance.
    // candidate(h,rawX,rawY,hit) runs pointCandidate with the current resources.
    template<class Graph,class Refresh,class Candidate>
    bool point(int object,Real x,Real y,bool compatibility,bool allPrepared,
        Graph& graph,Refresh refreshBounds,Candidate candidate,bool& hit){
        if(!valid()||object<0||object>=100000||!graph.valid()||!graph.flags(unsigned(object)))return false;
        if(!Math::valid(x)||!Math::valid(y))return checked(false);
        auto isA=[&](int child,int parent){return graph.isA(child,parent);};
        using Prep=RunnerSpatialObjectPreparation;
        const auto path=Prep::prepare(membership_.count(object,isA),index_.height(),
            *graph.flags(unsigned(object)),allPrepared,
            [&]{return graph.markSpatialFamily(unsigned(object));},
            [&](auto emit){membership_.visit(object,isA,[&](unsigned slot,std::uint32_t){
                auto edit=states_.edit(slot);auto& state=edit.get();return emit(slot,state.flags,state.bounds);
            });},
            [&](unsigned slot,auto&,auto&){return refreshBounds(handle(slot),false);},
            [&](Box box,unsigned slot){return index_.insert(box,slot);});
        return checked(RunnerPointTraversal::run(path,x,y,compatibility,[&]{return flush(refreshBounds);},
            [&](unsigned slot,bool indexed,bool& admitted){
                if(slot>=Capacity||lifetimes_.object(slot)<0)return false;
                admitted=!(states_.flags(slot)&0x100003u)&&
                    (!indexed||(states_.kind(slot)==1&&isA(lifetimes_.object(slot),object)));return true;
            },[&](unsigned slot,Real px,Real py,bool& result){return candidate(handle(slot),px,py,result);},
            [&](Box box,auto visit){return index_.query(box,visit);},
            [&](auto visit){membership_.visit(object,isA,[&](unsigned slot,std::uint32_t){return visit(slot);});return true;},hit));
    }
    // collision_line object-resource selector, 0x4fd370. Args have passed the
    // VM float wrapper. Family preparation selects list/tree; tree queries
    // flush dirty bounds and filter kind/family before the candidate call.
    template<class Graph,class Refresh,class Candidate>
    bool collisionLine(InstanceHandle self,int object,const RunnerLineArguments& args,bool notMe,
        bool allPrepared,Graph& graph,Refresh refreshBounds,Candidate candidate,Result& result){
        if(!valid()||object<0||object>=100000||!graph.valid()||!graph.flags(unsigned(object)))return false;
        if(!checked(Math::valid(args.x1)&&Math::valid(args.y1)&&Math::valid(args.x2)&&Math::valid(args.y2))||
           (notMe&&!live(self)))return false;
        auto isA=[&](int child,int parent){return graph.isA(child,parent);};
        using Prep=RunnerSpatialObjectPreparation;
        const auto path=Prep::prepare(membership_.count(object,isA),index_.height(),
            *graph.flags(unsigned(object)),allPrepared,[&]{return graph.markSpatialFamily(unsigned(object));},
            [&](auto emit){membership_.visit(object,isA,[&](unsigned slot,std::uint32_t){
                auto edit=states_.edit(slot);auto& state=edit.get();return emit(slot,state.flags,state.bounds);
            });},[&](unsigned slot,auto&,auto&){return refreshBounds(handle(slot),false);},
            [&](Box box,unsigned slot){return index_.insert(box,slot);});
        RunnerLineTraversal::Result next;
        const bool ok=RunnerLineTraversal::run(path,args,[&]{return flush(refreshBounds);},
            [&](unsigned slot,bool indexed,bool& admitted){
            if(slot>=Capacity||lifetimes_.object(slot)<0)return false;
            const auto h=handle(slot);
            admitted=!((states_.flags(slot)&0x100003u)||(notMe&&h.slot==self.slot&&h.generation==self.generation)||
               (indexed&&(states_.kind(slot)!=1||!isA(lifetimes_.object(slot),object))));return true;
        },[&](unsigned slot,const auto& line,bool& hit){return candidate(handle(slot),line,hit);},
        [&](const auto& line,auto visit){return index_.queryLine(line,visit);},
        [&](auto visit){membership_.visit(object,isA,[&](unsigned slot,std::uint32_t){return visit(slot);});return true;},next);
        if(!checked(ok))return false;
        result={next.found,next.found?handle(next.slot):InstanceHandle{}};return true;
    }
    // collision_point object-resource path, 0x4fd79c. This is deliberately NOT
    // point()/position_meeting: no family preparation, no index query and no
    // global dirty flush. Each candidate owns its lazy bounds refresh. Return
    // the first eligible registration, not a closest hit or storage-slot order.
    // Special all/instance selectors and collect-all are separate entry points.
    template<class Graph,class Candidate>
    bool collisionPointVm(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        bool notMe,const Graph& graph,Candidate candidate,Result& result){
        return collisionPointVmMode<false>(self,object,x,y,notMe,graph,candidate,result);
    }
    template<class Graph,class Candidate>
    bool collisionPointVmReference(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        bool notMe,const Graph& graph,Candidate candidate,Result& result){
        return collisionPointVmMode<true>(self,object,x,y,notMe,graph,candidate,result);
    }
private:
    template<bool FullState,class Graph,class Candidate>
    bool collisionPointVmMode(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        bool notMe,const Graph& graph,Candidate candidate,Result& result){
        if(!valid()||object<0||object>=100000||!graph.valid()||!graph.flags(unsigned(object)))return false;
        Real nx,ny;
        if(!checked(nx.assign(x)&&ny.assign(y))||(notMe&&!live(self)))return false;
        RunnerCollisionPoint::Result next;
        const bool ok=RunnerCollisionPoint::run(self,notMe,nx,ny,[&](auto visit){
            membership_.visit(object,[&](int child,int parent){return graph.isA(child,parent);},
                [&](unsigned slot,std::uint32_t generation){return visit(InstanceHandle{slot,generation});});
            return true;
        },[&](InstanceHandle h,std::uint32_t& flags){return readEligibility<FullState>(h,flags);},candidate,next);
        if(!checked(ok))return false;
        result={next.found,next.handle};return true;
    }
    template<bool FullState> bool readEligibility(InstanceHandle h,std::uint32_t& flags)const{
        if constexpr(FullState){
            const auto state=resolve(h);if(!state)return false;
            flags=state->flags;return true;
        }else return instanceFlags(h,flags);
    }
public:
    // Borrow exact registrations for phase snapshots; callbacks cannot mutate
    // membership. The scheduler supplies phase-specific exclusion flags.
    template<class Visit> bool visitRegistrations(Visit visit)const{
        return visitRegistrationRange(0,Capacity,visit);
    }
    template<class Visit> bool visitRegistrationFlags(Visit visit)const{
        return valid()&&membership_.visitRegistrationRange(0,Capacity,
            [&](unsigned slot,std::uint32_t generation,int object){
                const InstanceHandle h{slot,generation};std::uint32_t flags;
                return instanceFlags(h,flags)&&visit(h,object,flags);
            });
    }
    // Narrow a phase snapshot to a disjoint owner domain before resolving
    // records. Selected lifetimes remain checked; no slot-order iteration.
    template<class Visit> bool visitRegistrationRange(unsigned first,unsigned last,Visit visit)const{
        return valid()&&membership_.visitRegistrationRange(first,last,[&](unsigned slot,std::uint32_t generation,int object){
            const InstanceHandle h{slot,generation};const auto state=resolve(h);
            return state&&visit(h,object,*state);
        });
    }
    template<unsigned First,unsigned Last,class Visit>
    bool visitRegistrationRange(RegistrationProjection<First,Last>& projection,
        unsigned first,unsigned last,Visit visit)const{
        return valid()&&membership_.visitRegistrationRange(projection,first,last,
            [&](unsigned slot,std::uint32_t generation,int object){
                const InstanceHandle h{slot,generation};const auto state=resolve(h);
                return state&&visit(h,object,*state);
            });
    }
    template<class Visit> bool visitRetainedRegistrationRange(unsigned first,unsigned last,Visit visit)const{
        return valid()&&membership_.visitRetainedRegistrationRange(first,last,
            [&](unsigned slot,std::uint32_t generation,int object){
                const InstanceHandle h{slot,generation};const auto state=resolve(h);
                return state&&visit(h,object,*state);
            });
    }
    // Object-resource branch of instance_number (0x4fe49b). This is NOT the
    // raw registration count used by collision preparation below. Filtering
    // flags does not unlink membership or alter the selected search strategy.
    // Special negative/instance-ID selectors are outside this entry point.
    template<class Graph> bool instanceNumber(int object,const Graph& graph,unsigned& result)const{
        return instanceNumberMode<false>(object,graph,result);
    }
    template<class Graph> bool instanceNumberReference(int object,const Graph& graph,unsigned& result)const{
        return instanceNumberMode<true>(object,graph,result);
    }
private:
    template<bool FullState,class Graph> bool instanceNumberMode(int object,const Graph& graph,unsigned& result)const{
        if(!valid()||object<0||!graph.valid()||!graph.flags(unsigned(object)))return false;
        unsigned count=0;bool good=true;
        membership_.visit(object,[&](int child,int parent){return graph.isA(child,parent);},
            [&](unsigned slot,std::uint32_t generation){
                std::uint32_t flags;
                if(!readEligibility<FullState>({slot,generation},flags)){good=false;return false;}
                if(!(flags&0x140003u))++count;
                return true;
            });
        if(!good)return false;
        result=count;return true;
    }
public:
    // Graph supplies validated flags/isA/markSpatialFamily. refreshBounds(h,
    // publish) resolves assets and calls refresh above (or handles/rejects a
    // special resource). precise(a,b,publish,hit) owns the exact pair test and
    // any lazy refresh. Neither callback may create/retire/rebind instances.
    template<class Graph,class Refresh,class Precise>
    bool place(InstanceHandle self,int object,Real x,Real y,bool allPrepared,
        Graph& graph,Refresh refreshBounds,Precise precise,Result& result){
        if(!valid()||!live(self)||object<0||!graph.valid()||!graph.flags(unsigned(object))||
            !graph.flags(unsigned(lifetimes_.object(self.slot))))return false;
        auto isA=[&](int child,int parent){return graph.isA(child,parent);};
        auto prepare=[&]{
            using Prep=RunnerSpatialObjectPreparation;
            return Prep::prepare(membership_.count(object,isA),index_.height(),
                *graph.flags(unsigned(object)),allPrepared,
                [&]{return graph.markSpatialFamily(unsigned(object));},
                [&](auto emit){membership_.visit(object,isA,[&](unsigned slot,std::uint32_t){
                    auto edit=states_.edit(slot);auto& s=edit.get();return emit(slot,s.flags,s.bounds);
                });},
                [&](unsigned slot,auto&,auto&){return refreshBounds(handle(slot),false);},
                [&](Box box,unsigned slot){return index_.insert(box,slot);});
        };
        auto eligible=[&](unsigned slot,bool spatial){
            return slot<Capacity && slot!=self.slot && lifetimes_.object(slot)>=0 &&
                !(states_.flags(slot)&0x100003u) &&
                (!spatial || (states_.kind(slot)==1 && isA(lifetimes_.object(slot),object)));
        };
        auto edit=states_.edit(self.slot);auto& s=edit.get();
        RunnerPlaceQuery::Result hit;
        const bool success=RunnerPlaceQuery::run(s.x,s.y,s.flags,s.bounds,x,y,prepare,
            [&]{return flush(refreshBounds);},[&](bool publish){return refreshBounds(self,publish);},
            [&](Real nx,Real ny){return move(self,nx,ny,*graph.flags(unsigned(lifetimes_.object(self.slot))));},
            [&](auto visit){membership_.visit(object,isA,[&](unsigned slot,std::uint32_t){
                return !eligible(slot,false)||visit(slot);
            });return true;},
            [&](Box box,auto visit){return index_.query(box,[&](unsigned slot){
                return !eligible(slot,true)||visit(slot);
            });},
            [&](unsigned slot,bool publish,bool& found){return precise(self,handle(slot),publish,found);},hit);
        if(!checked(success))return false;
        result={hit.found,hit.found?handle(hit.identity):InstanceHandle{}};return true;
    }
private:
    bool finishFreshPreparedTerrain(unsigned slot,std::uint32_t objectFlags,const Box& bounds,
        Math::PixelBox pixels,bool precise,bool publish,PreparedTerrainBirthBounds::Key geometry={}){
        auto edit=states_.edit(slot);auto& s=edit.get();
        s.bounds=bounds;s.flags=(s.flags&~0x208u)|(precise?0x200u:0u);
        if(!publish||!Queue::shouldPublishBounds(objectFlags,s.flags,false))return true;
        if constexpr(ScheduledIndex&&PreparedGeometry)if(geometry.value){
            // The immutable geometry producer supplies exact finite bounds.
            // Keep the instance cache unnormalized; only journal replay sees
            // normalized endpoints. No mutable terrain record is borrowed.
            if(pixels.right<pixels.left){const auto v=pixels.left;pixels.left=pixels.right;pixels.right=v;}
            if(pixels.bottom<pixels.top){const auto v=pixels.top;pixels.top=pixels.bottom;pixels.bottom=v;}
            DW_PROFILE_COUNT(counterSpatialFirstPublication);
            if(!checked(index_.insertGeometry(geometry,slot,pixels)))return false;
            s.flags|=0x8000u;return true;
        }
        if(!Math::valid(bounds.left)||!Math::valid(bounds.top)||
           !Math::valid(bounds.right)||!Math::valid(bounds.bottom))return checked(false);
        Box normalized=bounds;
        if(pixels.right<pixels.left){
            auto p=pixels.left;pixels.left=pixels.right;pixels.right=p;
            auto b=normalized.left;normalized.left=normalized.right;normalized.right=b;
        }
        if(pixels.bottom<pixels.top){
            auto p=pixels.top;pixels.top=pixels.bottom;pixels.bottom=p;
            auto b=normalized.top;normalized.top=normalized.bottom;normalized.bottom=b;
        }
        DW_PROFILE_COUNT(counterSpatialFirstPublication);
        if(!checked(index_.insertPrepared(normalized,slot,&pixels)))return false;
        s.flags|=0x8000u;return true;
    }
    Lifetimes lifetimes_;
    States states_;
    Membership membership_;
    Queue spatial_;
    CollisionQueue collision_;
    typename RunnerSpatialIndexStorage<Capacity,Nodes,ScheduledIndex,PreparedGeometry>::Type index_;
    bool good_=true;
    std::uint32_t bindingRevision_=0;
    std::uint32_t captureRevision_=0;
    bool* captureDirty_=nullptr;
    unsigned captureFirst_=0,captureLast_=0,captureSecond_=0,captureEnd_=0;
    void changedCapture(unsigned slot=Capacity){
        if(captureRevision_!=0xffffffffu)++captureRevision_;
        if(captureDirty_&&(slot==Capacity||(slot>=captureFirst_&&slot<captureLast_)||
            (slot>=captureSecond_&&slot<captureEnd_))) *captureDirty_=true;
    }
    void changedBinding(unsigned slot=Capacity){
        if(bindingRevision_!=0xffffffffu)++bindingRevision_;
        changedCapture(slot);
    }
    void changedFreshBinding(unsigned slot){
        bindingRevision_=bindingRevision_>=0xfffffffeu?0xffffffffu:bindingRevision_+2u;
        captureRevision_=captureRevision_>=0xfffffffeu?0xffffffffu:captureRevision_+2u;
        if(captureDirty_&&((slot>=captureFirst_&&slot<captureLast_)||
            (slot>=captureSecond_&&slot<captureEnd_))) *captureDirty_=true;
    }
    bool* snapshotDirty_=nullptr;
    unsigned snapshotFirst_=0,snapshotLast_=0;
    bool checked(bool ok){if(!ok)good_=false;return ok;}
    void markSpatialChange(unsigned slot,std::uint32_t objectFlags){
        if(Queue::shouldQueue(true,objectFlags,states_.flags(slot))){
            spatial_.mark(slot);
            if(objectFlags&8u)collision_.mark(slot);
        }
    }
    bool live(InstanceHandle h)const{return lifetimes_.contains(h);}
    InstanceHandle handle(unsigned slot)const{return lifetimes_.handle(slot);}
    static bool normalize(Box& b){
        if(!Math::valid(b.left)||!Math::valid(b.top)||!Math::valid(b.right)||!Math::valid(b.bottom))return false;
        if(Math::less(b.right,b.left)){auto n=b.left;b.left=b.right;b.right=n;}
        if(Math::less(b.bottom,b.top)){auto n=b.top;b.top=b.bottom;b.bottom=n;}
        return true;
    }
};
}
#endif

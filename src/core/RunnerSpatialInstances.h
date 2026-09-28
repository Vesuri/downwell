#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_INSTANCES_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_INSTANCES_H
#include "InstanceHandle.h"
#include "RunnerCollisionPairs.h"
#include "RunnerCollisionMotion.h"
#include "RunnerTransformField.h"
#include "RunnerObjectMembership.h"
#include "RunnerSpatialIndex.h"
#include "RunnerSpatialBoundsUpdate.h"
#include "RunnerSpatialPosition.h"
#include "RunnerPlaceQuery.h"
#include "RunnerSpriteBounds.h"
#include "SpriteCollisionPack.h"
#include "RunnerMaskPair.h"
#include "RunnerMaskPoint.h"
#include "RunnerLineBounds.h"
#include "RunnerSpriteChange.h"
#include "ObjectPack.h"
#include "Profile.h"
namespace downwell {
// Shared collision ownership for externally allocated instance handles. The
// scheduler must attach every relevant birth before Create, publish its bounds,
// and retire it before reusing its slot. This is not an activation scheduler.
// No platform types or asset residency decisions belong here.
template<unsigned Capacity,unsigned Nodes> class RunnerSpatialInstances {
    static_assert(Capacity>0,"Spatial instances need stable slots");
    using Queue=RunnerSpatialDirtyQueue<Capacity>;
    using Math=RunnerSpatialSplit;
public:
    using Real=RunnerImageValue;
    template<unsigned First,unsigned Last> using RegistrationProjection=
        typename RunnerObjectMembership<Capacity>::template RegistrationProjection<First,Last>;
    using Box=Math::Box;
    struct State {
        Real x{},y{};
        Box bounds{};
        std::uint32_t flags=8;
        unsigned kind=1;
        bool special=false;
        RunnerCollisionMotion::Previous previous{};
    };
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
        Real* target=nullptr;
        switch(field){
            case TransformField::scaleX:target=&sprite.scaleX;break;
            case TransformField::scaleY:target=&sprite.scaleY;break;
            case TransformField::angle:target=&sprite.angle;break;
            default:return checked(false);
        }
        if(!Math::valid(*target)||!Math::valid(value))return checked(false);
        if(Math::equal(*target,value))return true;
        if(!Math::valid(sprite.scaleX)||!Math::valid(sprite.scaleY)||
           !Math::valid(sprite.angle)||!Math::valid(alpha))return checked(false);
        *target=value;
        auto& flags=entries_[h.slot].state.flags;
        const Real one{0x3f800000u};
        const bool ordinary=Math::equal(sprite.scaleX,one)&&Math::equal(sprite.scaleY,one)&&
            Math::equal(sprite.angle,Real{})&&Math::equal(alpha,one)&&blend==0xffffffu;
        flags=(flags&~0x4000u)|(ordinary?0x4000u:0u)|8u;
        markSpatialChange(h.slot,objectFlags);return true;
    }
    // Integer setter 0x46bfd0. VM-number narrowing/rounding is a separate entry
    // point; an invalid resource ID remains stored and means a missing mask.
    bool changeMask(InstanceHandle h,Sprite& sprite,int mask,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        if(sprite.maskId==mask)return true;
        sprite.maskId=mask;entries_[h.slot].state.flags|=8u;
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
        if(!objects.valid()||!sprites.valid()||!graph.valid()||object<0||
           !objects.findObject(object,definition)||!graph.flags(unsigned(object)))return checked(false);
        const auto objectFlags=*graph.flags(unsigned(object));
        const unsigned expected=(definition.solid()?1u:0u)|(definition.visible()?2u:0u)|
            (definition.persistent()?4u:0u)|(definition.managed()?0x80u:0u);
        if((objectFlags&0x87u)!=expected)return checked(false);
        Real depth;
        if(!depth.assignInteger(definition.depth))return checked(false);
        if(!rebindObjectMembership(h,object,registered))return checked(false);
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
    bool valid()const{return good_ && index_.valid();}
    std::uint32_t bindingRevision()const{return bindingRevision_;}
    std::uint32_t captureRevision()const{return captureRevision_;}
    bool captureUnchanged(std::uint32_t revision)const{
        return revision!=0xffffffffu&&revision==captureRevision_;
    }
    bool bindingsUnchanged(std::uint32_t revision)const{
        return revision!=0xffffffffu&&revision==bindingRevision_;
    }
    // Clear retires memberships but preserves generation high-water marks.
    // Replacing the external handle domain requires replacing this owner too.
    void clear(){
        changedBinding();
        membership_.clear();spatial_.clear();collision_.clear();index_.clear();
        for(auto& entry:entries_)entry.object=-1;
        good_=true;
    }
    const State* resolve(InstanceHandle h)const{
        return live(h)?&entries_[h.slot].state:nullptr;
    }
    int objectId(InstanceHandle h)const{return live(h)?entries_[h.slot].object:-1;}
    bool attach(InstanceHandle h,int object,State state){
        if(!valid() || h.slot>=Capacity || !h.generation || object<0 ||
            entries_[h.slot].object>=0 || h.generation<=entries_[h.slot].generation ||
            (state.flags&0x8000u) || !Math::valid(state.x)||!Math::valid(state.y))return false;
        auto& e=entries_[h.slot];e.state=state;e.state.previous={state.x,state.y};
        e.object=object;e.generation=h.generation;
        changedBinding();
        return membership_.bind(h.slot,object,h.generation);
    }
    // 0x50dbf8..0x50dc0a, before this instance's animation in active-room
    // traversal. Do not filter flags here or dirty either spatial queue.
    bool capturePrevious(InstanceHandle h){
        if(!valid()||!live(h))return false;
        auto& s=entries_[h.slot].state;s.previous={s.x,s.y};return true;
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
        if(membership_.contains(h.slot)!=registered)changedCapture();
        if(!registered)membership_.unbind(h.slot);
        else if(!membership_.contains(h.slot))membership_.bind(h.slot,entries_[h.slot].object,h.generation);
        auto& flags=entries_[h.slot].state.flags;
        if(registered)flags&=~0x80000u;else flags|=0x80000u;
        return true;
    }
    // Membership portion of 0x46b830 only, before new object properties are
    // applied. Even same-object binding appends anew. Registration is an
    // explicit argument, not inferred from activation flags. The caller must
    // separately apply the resource/flag/queue effects at 0x46b8b2..0x46b968.
    bool rebindObjectMembership(InstanceHandle h,int object,bool registered){
        if(!valid()||!live(h)||object<0)return false;
        entries_[h.slot].object=object;
        changedBinding();
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
        if(!valid()||!live(h))return false;
        auto& flags=entries_[h.slot].state.flags;
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
        auto& flags=entries_[h.slot].state.flags;
        if(bool(flags&2u)==active)changedCapture();
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
        const auto object=unsigned(entries_[h.slot].object);
        if(!graph.flags(object))return false;
        if(!checked(graph.prepareBirth(object)&&refreshBounds(h,true)))return false;
        if(*graph.flags(object)&8u)collision_.mark(h.slot);
        return true;
    }
    // Immediate exclusion flags can precede deferred object-list reconciliation.
    bool setExclusions(InstanceHandle h,std::uint32_t exclusions){
        if(!valid()||!live(h)||(exclusions&~0x100003u))return false;
        auto& f=entries_[h.slot].state.flags;
        if((f&0x100003u)!=exclusions)changedCapture();
        f=(f&~0x100003u)|exclusions;return true;
    }
    bool move(InstanceHandle h,Real x,Real y,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        if(snapshotDirty_&&h.slot>=snapshotFirst_&&h.slot<snapshotLast_)*snapshotDirty_=true;
        auto& s=entries_[h.slot].state;
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
        if(entries_[h.slot].state.special)return checked(false);
        RunnerSpriteChange::Result change;
        if(!checked(RunnerSpriteChange::plan(sprite.spriteId,next,sprite.imageIndex,frames,change)))return false;
        sprite.spriteId=next;cachedFrame={0xbf800000u};
        if(change.resetIndex)sprite.imageIndex={};
        if(change.resetTimeline)timeline={};
        entries_[h.slot].state.flags|=8u;
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
        PackedSpriteCollision resource;
        const unsigned frames=next>=0&&pack.sprite(unsigned(next),resource)?resource.frameCount:0;
        return changeSprite(h,sprite,timeline,cachedFrame,next,frames,objectFlags);
    }
    bool refresh(InstanceHandle h,std::uint32_t objectFlags,bool allPrepared,
        const Box* bounds,bool precise,bool publish){
        DW_PROFILE_SCOPE(profileBoundsPublication);
        if(!valid()||!live(h))return false;
        auto& s=entries_[h.slot].state;
        return checked(RunnerSpatialBoundsUpdate::finishRefresh(index_,h.slot,
            objectFlags,s.flags,allPrepared,s.bounds,bounds,s.x,s.y,precise,publish));
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
        const auto& s=entries_[h.slot].state;
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
        auto& first=entries_[self.slot].state;auto& second=entries_[other.slot].state;
        if(self.slot==other.slot || ((first.flags|second.flags)&0x100001u)){hit=false;return true;}
        if((first.special&&a.maskId<0)||(second.special&&b.maskId<0))return checked(false);
        if(publish){
            if((second.flags&8u)&&!checked(refreshBounds(other,true)))return false;
            if((first.flags&8u)&&!checked(refreshBounds(self,true)))return false;
        }
        // Compatibility mode adds binary32 1 to each inclusive far edge and
        // uses a strict comparison. Do not replace this with integer overlap.
        const Real near[]={second.bounds.left,first.bounds.left,second.bounds.top,first.bounds.top};
        const Real far[]={first.bounds.right,second.bounds.right,first.bounds.bottom,second.bounds.bottom};
        for(unsigned i=0;i<4;++i){
            Real upper;
            if(!Math::valid(near[i])||!Math::valid(far[i])||
                !Math::sub(far[i],Real{0xbf800000},upper))return checked(false);
            if(!Math::less(near[i],upper)){hit=false;return true;}
        }
        if(!sprites.valid())return checked(false);
        PackedSpriteCollision resources[2];
        if(!sprites.collisionSprite(a.spriteId,a.maskId,resources[0])||!resources[0].frameCount||
           !sprites.collisionSprite(b.spriteId,b.maskId,resources[1])||!resources[1].frameCount){hit=false;return true;}
        if(!resources[0].collisionMode&&!resources[1].collisionMode){hit=true;return true;}
        if(!masks.valid())return checked(false);
        const Sprite* visuals[]={&a,&b};const State* states[]={&first,&second};
        RunnerMaskPair::Shape shapes[2];
        for(unsigned i=0;i<2;++i){
            const auto& v=*visuals[i];const auto& s=*states[i];const auto& r=resources[i];
            unsigned frame;
            if(!RunnerMaskPair::storedFrame(v.imageIndex,r.collisionMaskCount,frame)||
                !masks.mask(unsigned(v.maskId<0?v.spriteId:v.maskId),frame,shapes[i].mask))return checked(false);
            shapes[i].x=s.x;shapes[i].y=s.y;shapes[i].bounds=s.bounds;
            shapes[i].transform={v.scaleX,v.scaleY,v.angle};
            shapes[i].originX=r.originX;shapes[i].originY=r.originY;shapes[i].frameCount=r.frameCount;
        }
        RunnerMaskPair::Result result;
        if(!checked(RunnerMaskPair::test(shapes[0],shapes[1],dispatch,inverseTrig,round,result)))return false;
        hit=result.hit;return true;
    }
    // collision_line precise=false, ordinary sprite candidate (0x4650b0).
    // Bounds refresh precedes exclusions and missing-resource checks. Pack
    // admission excludes rotated-box/skeletal resource paths; those must not
    // silently fall through to this non-precise rectangle gate.
    template<class Refresh>
    bool nonPreciseLineCandidate(InstanceHandle h,const Sprite& sprite,const RunnerLineArguments& args,
        bool compatibility,const SpriteCollisionPack& sprites,Refresh refreshBounds,bool& hit){
        if(!valid()||!live(h))return false;
        auto& state=entries_[h.slot].state;
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
        auto& state=entries_[h.slot].state;
        if(!Math::valid(x)||!Math::valid(y)||(state.special&&sprite.maskId<0))return checked(false);
        if((state.flags&8u)&&!checked(refreshBounds(h,true)))return false;
        const auto& b=state.bounds;
        // Fractional mode shrinks the far edge by float 0.00001; it is not
        // the half-open cached box used by the broad spatial index.
        const Real extension=compatibility?Real{0x3f800000}:Real{0xb727c5ac};
        Real right,bottom;
        if(!Math::valid(b.left)||!Math::valid(b.top)||!Math::valid(b.right)||!Math::valid(b.bottom)||
           !Math::sub(b.right,Math::negate(extension),right)||
           !Math::sub(b.bottom,Math::negate(extension),bottom))return checked(false);
        if(!Math::less(x,right)||Math::less(x,b.left)||!Math::less(y,bottom)||Math::less(y,b.top)||
           (state.flags&0x100001u)){hit=false;return true;}
        if(!sprites.valid())return checked(false);
        PackedSpriteCollision resource;
        if(!sprites.collisionSprite(sprite.spriteId,sprite.maskId,resource)||!resource.frameCount){hit=false;return true;}
        // Admitted pack modes are axis-aligned box (0) and precise (1), not
        // rotated-box or skeletal resources with their extra point gates.
        if(!precise||!resource.collisionMode){hit=true;return true;}
        RunnerMaskPoint::Shape shape;
        std::int32_t px,py;
        // The Windows fistp sequence is query Y, query X, instance Y, X.
        if(!checked(round(y,py)&&round(x,px)&&round(state.y,shape.y)&&round(state.x,shape.x)))return false;
        shape.originX=resource.originX;shape.originY=resource.originY;
        shape.scaleX=sprite.scaleX;shape.scaleY=sprite.scaleY;shape.angle=sprite.angle;
        // No serialized masks: 0x4bbbb0 cannot expand any, so 0x4ba85f
        // returns true. Distinguish this from a missing/corrupt offline pack.
        if(!resource.collisionMaskCount){hit=true;return true;}
        unsigned frame;SpriteMaskPack::Mask mask;
        // Missing offline mask bytes are not evidence for the runner's
        // lazy-mask-unavailable fallback. Fail instead of inventing a hit.
        if(!masks.valid()||!RunnerMaskPair::storedFrame(sprite.imageIndex,resource.collisionMaskCount,frame)||
           !masks.mask(unsigned(sprite.maskId<0?sprite.spriteId:sprite.maskId),frame,mask))return checked(false);
        bool result;
        if(!checked(RunnerMaskPoint::contains(shape,mask,px,py,inverseTrig,result)))return false;
        hit=result;return true;
    }
    // 0x501f70: remove old bounds (retry at root), clear indexed flag, then
    // unlink both dirty memberships. Missing resources may have changed the
    // cached bounds without moving the old index entry, so retry is essential.
    // Object rebinding also calls this on a LIVE instance: object membership,
    // generation and all flags other than indexed must survive.
    bool removeSpatialMembership(InstanceHandle h){
        if(!valid()||!live(h))return false;
        auto& e=entries_[h.slot];
        if(e.state.flags&0x8000u){
            Box box=e.state.bounds;
            if(!normalize(box))return checked(false);
            bool removed=false;
            if(!checked(index_.remove(box,h.slot,true,removed)))return false;
            e.state.flags&=~0x8000u;
        }
        spatial_.remove(h.slot);collision_.remove(h.slot);
        return true;
    }
    bool retire(InstanceHandle h){
        if(!removeSpatialMembership(h))return false;
        membership_.unbind(h.slot);
        entries_[h.slot].object=-1;changedBinding();return true;
    }
    unsigned pendingSpatial()const{return spatial_.size();}
    unsigned pendingCollision()const{return collision_.size();}
    // 0x501ef0, called for OTHER after a precise collision and inactivity
    // checks. Queue only collision processing, without dirtying spatial bounds.
    bool queueCollision(InstanceHandle h,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        return !(objectFlags&8u)||checked(collision_.mark(h.slot));
    }
    template<class Refresh> bool flush(Refresh refreshBounds){
        if(!valid())return false;
        return checked(spatial_.flush([&](unsigned slot){return entries_[slot].state.flags;},
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
        if(!valid()||!checked(pairs.beginPass()))return false;
        unsigned retained[Capacity],count=0;
        if(indexAvailable){
            if(!flush(refreshBounds))return false;
            unsigned slot=collision_.first();
            while(slot!=Queue::End){
                const auto self=handle(slot);
                if(!checked(live(self)))return false;
                const bool eligible=Queue::shouldRefreshQueued(entries_[slot].state.flags);
                if(eligible){
                    Box box=entries_[slot].state.bounds;
                    if(!checked(normalize(box)))return false;
                    bool callbacks=true;
                    const bool queried=index_.query(box,[&](unsigned candidate){
                        if(candidate>=Capacity||entries_[candidate].object<0){callbacks=false;return false;}
                        callbacks=pairs.candidate(self,handle(candidate),hasEvent);
                        return callbacks;
                    });
                    // query's early callback stop is not itself an index error.
                    if(!checked(queried&&callbacks))return false;
                }
                if(!checked(collision_.contains(slot)))return false;
                const unsigned successor=collision_.next(slot);
                collision_.remove(slot);
                if(eligible){
                    if(entries_[slot].state.flags&0x200u){
                        unsigned frames=0;
                        if(!checked(maskFrames(self,frames)))return false;
                        if(frames>1)retained[count++]=slot;
                    }
                    if(!checked(pairs.finishInstance(self)))return false;
                }
                slot=successor;
            }
        }
        collision_.clear();
        // Source prepends to a temporary list, then prepends back: preserve
        // query order, rather than reversing animated instances each frame.
        while(count)collision_.mark(retained[--count]);
        return checked(pairs.seal());
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
                auto& state=entries_[slot].state;return emit(slot,state.flags,state.bounds);
            });},
            [&](unsigned slot,auto&,auto&){return refreshBounds(handle(slot),false);},
            [&](Box box,unsigned slot){return index_.insert(box,slot);});
        if(path==Prep::Path::failed)return checked(false);
        if(path==Prep::Path::none){hit=false;return true;}
        const bool indexed=path==Prep::Path::spatial;
        if(indexed&&!checked(flush(refreshBounds)))return false;
        bool good=true,result=false;
        auto visit=[&](unsigned slot){
            if(slot>=Capacity||entries_[slot].object<0){good=false;return false;}
            const auto& entry=entries_[slot];
            // 0x501e40 is an unconditional false stub in the pinned runner.
            if((entry.state.flags&0x100003u)||
               (indexed&&(entry.state.kind!=1||!isA(entry.object,object))))return true;
            if(!candidate(handle(slot),x,y,result)){good=false;return false;}
            return !result;
        };
        if(indexed){
            // cvttss2si + cvtdq2ps affects only broad-phase coordinates;
            // candidate tests still receive the original narrowed fractions.
            const auto truncate=[](Real value){
                const int exponent=int((value.bits>>23)&255)-127;
                if(exponent<0)return Real{};
                if(exponent>=31)return Real{0xcf000000u}; // SSE integer indefinite.
                if(exponent<23)value.bits&=~((1u<<unsigned(23-exponent))-1u);
                return value;
            };
            const Real bx=compatibility?truncate(x):x,by=compatibility?truncate(y):y;
            if(!index_.query({bx,by,bx,by},visit))good=false;
        }else membership_.visit(object,isA,[&](unsigned slot,std::uint32_t){return visit(slot);});
        if(!checked(good))return false;
        hit=result;return true;
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
                auto& state=entries_[slot].state;return emit(slot,state.flags,state.bounds);
            });},[&](unsigned slot,auto&,auto&){return refreshBounds(handle(slot),false);},
            [&](Box box,unsigned slot){return index_.insert(box,slot);});
        if(path==Prep::Path::failed)return checked(false);
        if(path==Prep::Path::none){result={};return true;}
        const bool indexed=path==Prep::Path::spatial;
        if(indexed&&!checked(flush(refreshBounds)))return false;
        Result next;bool good=true;
        auto visit=[&](unsigned slot){
            if(slot>=Capacity||entries_[slot].object<0){good=false;return false;}
            const auto h=handle(slot);const auto& entry=entries_[slot];
            if((entry.state.flags&0x100003u)||(notMe&&h.slot==self.slot&&h.generation==self.generation)||
               (indexed&&(entry.state.kind!=1||!isA(entry.object,object))))return true;
            bool hit=false;if(!candidate(h,args,hit)){good=false;return false;}
            if(hit){next={true,h};return false;}return true;
        };
        if(indexed){if(!index_.queryLine(args,visit))good=false;}
        else membership_.visit(object,isA,[&](unsigned slot,std::uint32_t){return visit(slot);});
        if(!checked(good))return false;
        result=next;return true;
    }
    // collision_point object-resource path, 0x4fd79c. This is deliberately NOT
    // point()/position_meeting: no family preparation, no index query and no
    // global dirty flush. Each candidate owns its lazy bounds refresh. Return
    // the first eligible registration, not a closest hit or storage-slot order.
    // Special all/instance selectors and collect-all are separate entry points.
    template<class Graph,class Candidate>
    bool collisionPointVm(InstanceHandle self,int object,MotionBinary64 x,MotionBinary64 y,
        bool notMe,const Graph& graph,Candidate candidate,Result& result){
        if(!valid()||object<0||object>=100000||!graph.valid()||!graph.flags(unsigned(object)))return false;
        Real nx,ny;
        if(!checked(nx.assign(x)&&ny.assign(y))||(notMe&&!live(self)))return false;
        Result next;bool good=true;
        membership_.visit(object,[&](int child,int parent){return graph.isA(child,parent);},
            [&](unsigned slot,std::uint32_t generation){
                const InstanceHandle h{slot,generation};
                const auto* state=resolve(h);
                if(!state){good=false;return false;}
                if((state->flags&0x100003u)||(notMe&&h.slot==self.slot&&h.generation==self.generation))return true;
                bool hit=false;
                if(!candidate(h,nx,ny,hit)){good=false;return false;}
                if(hit){next={true,h};return false;}
                return true;
            });
        if(!checked(good))return false;
        result=next;return true;
    }
    // Borrow exact registrations for phase snapshots; callbacks cannot mutate
    // membership. The scheduler supplies phase-specific exclusion flags.
    template<class Visit> bool visitRegistrations(Visit visit)const{
        return visitRegistrationRange(0,Capacity,visit);
    }
    // Narrow a phase snapshot to a disjoint owner domain before resolving
    // records. Selected lifetimes remain checked; no slot-order iteration.
    template<class Visit> bool visitRegistrationRange(unsigned first,unsigned last,Visit visit)const{
        return valid()&&membership_.visitRegistrationRange(first,last,[&](unsigned slot,std::uint32_t generation,int object){
            const InstanceHandle h{slot,generation};const auto* state=resolve(h);
            return state&&visit(h,object,*state);
        });
    }
    template<unsigned First,unsigned Last,class Visit>
    bool visitRegistrationRange(RegistrationProjection<First,Last>& projection,
        unsigned first,unsigned last,Visit visit)const{
        return valid()&&membership_.visitRegistrationRange(projection,first,last,
            [&](unsigned slot,std::uint32_t generation,int object){
                const InstanceHandle h{slot,generation};const auto* state=resolve(h);
                return state&&visit(h,object,*state);
            });
    }
    // Object-resource branch of instance_number (0x4fe49b). This is NOT the
    // raw registration count used by collision preparation below. Filtering
    // flags does not unlink membership or alter the selected search strategy.
    // Special negative/instance-ID selectors are outside this entry point.
    template<class Graph> bool instanceNumber(int object,const Graph& graph,unsigned& result)const{
        if(!valid()||object<0||!graph.valid()||!graph.flags(unsigned(object)))return false;
        unsigned count=0;bool good=true;
        membership_.visit(object,[&](int child,int parent){return graph.isA(child,parent);},
            [&](unsigned slot,std::uint32_t generation){
                const auto* state=resolve({slot,generation});
                if(!state){good=false;return false;}
                if(!(state->flags&0x140003u))++count;
                return true;
            });
        if(!good)return false;
        result=count;return true;
    }
    // Graph supplies validated flags/isA/markSpatialFamily. refreshBounds(h,
    // publish) resolves assets and calls refresh above (or handles/rejects a
    // special resource). precise(a,b,publish,hit) owns the exact pair test and
    // any lazy refresh. Neither callback may create/retire/rebind instances.
    template<class Graph,class Refresh,class Precise>
    bool place(InstanceHandle self,int object,Real x,Real y,bool allPrepared,
        Graph& graph,Refresh refreshBounds,Precise precise,Result& result){
        if(!valid()||!live(self)||object<0||!graph.valid()||!graph.flags(unsigned(object))||
            !graph.flags(unsigned(entries_[self.slot].object)))return false;
        auto isA=[&](int child,int parent){return graph.isA(child,parent);};
        auto prepare=[&]{
            using Prep=RunnerSpatialObjectPreparation;
            return Prep::prepare(membership_.count(object,isA),index_.height(),
                *graph.flags(unsigned(object)),allPrepared,
                [&]{return graph.markSpatialFamily(unsigned(object));},
                [&](auto emit){membership_.visit(object,isA,[&](unsigned slot,std::uint32_t){
                    auto& s=entries_[slot].state;return emit(slot,s.flags,s.bounds);
                });},
                [&](unsigned slot,auto&,auto&){return refreshBounds(handle(slot),false);},
                [&](Box box,unsigned slot){return index_.insert(box,slot);});
        };
        auto eligible=[&](unsigned slot,bool spatial){
            return slot<Capacity && slot!=self.slot && entries_[slot].object>=0 &&
                !(entries_[slot].state.flags&0x100003u) &&
                (!spatial || (entries_[slot].state.kind==1 && isA(entries_[slot].object,object)));
        };
        auto& s=entries_[self.slot].state;
        RunnerPlaceQuery::Result hit;
        const bool success=RunnerPlaceQuery::run(s.x,s.y,s.flags,s.bounds,x,y,prepare,
            [&]{return flush(refreshBounds);},[&](bool publish){return refreshBounds(self,publish);},
            [&](Real nx,Real ny){return move(self,nx,ny,*graph.flags(unsigned(entries_[self.slot].object)));},
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
    struct Entry {State state{};int object=-1;std::uint32_t generation=0;};
    Entry entries_[Capacity]{};
    RunnerObjectMembership<Capacity> membership_;
    Queue spatial_,collision_;
    RunnerSpatialIndex<Nodes> index_;
    bool good_=true;
    std::uint32_t bindingRevision_=0;
    std::uint32_t captureRevision_=0;
    void changedCapture(){if(captureRevision_!=0xffffffffu)++captureRevision_;}
    void changedBinding(){if(bindingRevision_!=0xffffffffu)++bindingRevision_;changedCapture();}
    bool* snapshotDirty_=nullptr;
    unsigned snapshotFirst_=0,snapshotLast_=0;
    bool checked(bool ok){if(!ok)good_=false;return ok;}
    void markSpatialChange(unsigned slot,std::uint32_t objectFlags){
        if(Queue::shouldQueue(true,objectFlags,entries_[slot].state.flags)){
            spatial_.mark(slot);
            if(objectFlags&8u)collision_.mark(slot);
        }
    }
    bool live(InstanceHandle h)const{return h.slot<Capacity && h.generation &&
        entries_[h.slot].object>=0 && entries_[h.slot].generation==h.generation;}
    InstanceHandle handle(unsigned slot)const{return {slot,entries_[slot].generation};}
    static bool normalize(Box& b){
        if(!Math::valid(b.left)||!Math::valid(b.top)||!Math::valid(b.right)||!Math::valid(b.bottom))return false;
        if(Math::less(b.right,b.left)){auto n=b.left;b.left=b.right;b.right=n;}
        if(Math::less(b.bottom,b.top)){auto n=b.top;b.top=b.bottom;b.bottom=n;}
        return true;
    }
};
}
#endif

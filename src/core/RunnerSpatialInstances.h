#ifndef DOWNWELL_CORE_RUNNER_SPATIAL_INSTANCES_H
#define DOWNWELL_CORE_RUNNER_SPATIAL_INSTANCES_H
#include "InstanceHandle.h"
#include "RunnerTransformField.h"
#include "RunnerObjectMembership.h"
#include "RunnerSpatialIndex.h"
#include "RunnerSpatialBoundsUpdate.h"
#include "RunnerSpatialPosition.h"
#include "RunnerPlaceQuery.h"
#include "RunnerSpriteBounds.h"
#include "SpriteCollisionPack.h"
#include "RunnerMaskPair.h"
#include "RunnerSpriteChange.h"
#include "ObjectPack.h"
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
    using Box=Math::Box;
    struct State {
        Real x{},y{};
        Box bounds{};
        std::uint32_t flags=8;
        unsigned kind=1;
        bool special=false;
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
                            const Graph& graph,bool allPrepared){
        if(!valid()||!live(h))return false;
        PackedObject definition;
        if(!objects.valid()||!sprites.valid()||!graph.valid()||object<0||
           !objects.findObject(object,definition)||!graph.flags(unsigned(object)))return checked(false);
        const auto objectFlags=*graph.flags(unsigned(object));
        const unsigned expected=(definition.solid()?1u:0u)|(definition.visible()?2u:0u)|
            (definition.persistent()?4u:0u)|(definition.managed()?0x80u:0u);
        if((objectFlags&0x87u)!=expected)return checked(false);
        Real depth;
        if(!depth.assign(MotionBinary64::fromInteger(definition.depth)))return checked(false);
        if(!rebindObjectMembership(h,object,registered))return checked(false);
        properties.sprite.maskId=definition.textureMaskId;
        properties.depth=depth;
        return applyBindingProperties(h,objectFlags,allPrepared,[&](InstanceHandle live){
            return changeSpriteFromPack(live,properties.sprite,properties.timeline,properties.cachedFrame,
                                        definition.spriteId,sprites,objectFlags);
        });
    }
    bool valid()const{return good_ && index_.valid();}
    // Clear retires memberships but preserves generation high-water marks.
    // Replacing the external handle domain requires replacing this owner too.
    void clear(){
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
        auto& e=entries_[h.slot];e.state=state;e.object=object;e.generation=h.generation;
        return membership_.bind(h.slot,object,h.generation);
    }
    // At source reconciliation boundaries, not when the active flag is written.
    bool registerObject(InstanceHandle h,bool registered){
        if(!valid()||!live(h))return false;
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
        membership_.unbind(h.slot);
        entries_[h.slot].object=object;
        if(registered)membership_.bind(h.slot,object,h.generation);
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
        auto& f=entries_[h.slot].state.flags;f=(f&~0x100003u)|exclusions;return true;
    }
    bool move(InstanceHandle h,Real x,Real y,std::uint32_t objectFlags){
        if(!valid()||!live(h))return false;
        auto& s=entries_[h.slot].state;
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
        if(!RunnerSpriteBounds::fromSprite(resource,s.x,s.y,sprite.scaleX,sprite.scaleY,
            rotated,cosine,sine,compatibility,round,bounds))return checked(false);
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
        entries_[h.slot].object=-1;return true;
    }
    unsigned pendingSpatial()const{return spatial_.size();}
    unsigned pendingCollision()const{return collision_.size();}
    template<class Refresh> bool flush(Refresh refreshBounds){
        if(!valid())return false;
        return checked(spatial_.flush([&](unsigned slot){return entries_[slot].state.flags;},
            [&](unsigned slot){return refreshBounds(handle(slot),true);}));
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

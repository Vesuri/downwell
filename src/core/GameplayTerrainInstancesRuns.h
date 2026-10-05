#ifndef DOWNWELL_GAMEPLAY_TERRAIN_INSTANCES_RUNS_H
#define DOWNWELL_GAMEPLAY_TERRAIN_INSTANCES_RUNS_H
#include "GameplayTerrainInstancesScalar.h"
#include "FailureSignal.h"
#include "TerrainOwnershipRuns.h"
#include "TerrainRunnerPublication.h"
#include "RunnerRegistrationOrder.h"
#include "RunnerRoomOrderState.h"
#include "RunnerLogicalLayerOrder.h"
#include "AreaOneBuilder.h"
#include <cstddef>
namespace downwell {
class GameplayTerrainInstancesRuns {
public:
    static constexpr unsigned capacity=2048;
    using Record=GameplayTerrainInstancesScalar::Record;
    using Binding=RecordLifecycleBinding<Record>;
    using Presentation=GameplayTerrainInstancesScalar::Presentation;
    using WallCollision=GameplayTerrainInstancesScalar::WallCollision;
    struct CreateValues {int spriteId=-1,imageIndex=0,imageSpeed=1;};
    class Read {
        const GameplayTerrainInstancesRuns* owner_=nullptr;
        InstanceHandle handle_{};Record value_{};
    public:
        Read()=default;
        Read(decltype(nullptr)){}
        Read(const GameplayTerrainInstancesRuns* owner,InstanceHandle h,const Record& value):owner_(owner),handle_(h),value_(value){}
        explicit operator bool()const{return owner_!=nullptr;}
        const Record* operator->()const{return &value_;}
        const Record& operator*()const{return value_;}
        bool operator==(const Read& b)const{return owner_==b.owner_&&handle_.slot==b.handle_.slot&&handle_.generation==b.handle_.generation;}
        bool operator!=(const Read& b)const{return !(*this==b);}
    };
private:
    struct Mutable:CreateValues {
        WallEndStepState wall{};
        bool active=true,destroyRequested=false,wallNeedsReset=false;
        // Replaces the old ever-used byte: pristine, allocated, published,
        // spatially retired (low two bits). Bits 2, 3 and 4 independently
        // expose room, registration and layer publication. Slot reuse clears
        // all three before inherited run metadata can become observable.
        std::uint8_t allocation=0;
        std::uint8_t spatialStage=0;
        std::uint8_t dirtyMembership=0;
    };
    using Ids=TerrainOwnershipRuns<capacity,TerrainRunnerPublication>;
    using Identity=typename Ids::Identity;
    Binding binding_;Presentation presentation_;WallCollision collision_;
    void* initializedContext_=nullptr;
    bool (*initializedBirth_)(void*,GameplayTerrainInstancesRuns&,const Identity&,int,GameMakerRandom&,CreateValues&,bool&)=nullptr;
    bool (*initializedRun_)(void*,const AreaOneBuilder::TerrainRun&,int,GameMakerRandom&,
        AreaOneBuilder::EmitFunction,void*)=nullptr;
    Ids identities_;
    TerrainRunnerPublication* birthPublication_=nullptr;
    unsigned birthSlot_=capacity;
    // One admitted constructor owns this writer across all publication phases.
    // Nested allocation or retirement invalidates it, so an outer transaction
    // resumes through the checked path rather than retaining a stale borrow.
    struct BirthPublication {
        GameplayTerrainInstancesRuns& owner;
        BirthPublication(GameplayTerrainInstancesRuns& o,InstanceHandle h):owner(o){
            o.birthPublication_=o.identities_.singletonPublication(h);o.birthSlot_=h.slot;
        }
        ~BirthPublication(){owner.birthPublication_=nullptr;}
    };
    template<class Part> bool publishPart(unsigned slot,Part TerrainRunnerPublication::* member,const Part& value){
        if(birthPublication_&&slot==birthSlot_){birthPublication_->*member=value;return true;}
        return identities_.changePublicationPart(identities_.currentHandle(slot),member,value);
    }
    Mutable mutable_[capacity]{};
    unsigned freeHint_=0;std::uint32_t next_=1;
    unsigned registered_[3]{},registrationRevision_=0;
    unsigned roomRevision_=0;
    static unsigned family(int object){return object==60?0:object==93?1:2;}
    FailureSignal<> good_=true;bool dispatching_=false,unboundedWallArt_=false;
    Record record(const Identity& id)const{
        const auto& m=mutable_[id.handle.slot];
        Record r{id.object,id.x,id.y,m.active,id.handle.generation,id.source,id.creationCode};
        r.spriteId=m.spriteId;r.imageIndex=m.imageIndex;r.imageSpeed=m.imageSpeed;
        r.wall=m.wall;r.destroyRequested=m.destroyRequested;r.wallNeedsReset=m.wallNeedsReset;return r;
    }
    static bool visible(const Identity& id,const Mutable& m){return id.object==60&&m.active&&!m.destroyRequested;}
    static int drawSprite(const Identity& id,const Mutable& m){return visible(id,m)?(m.wall.checked?m.spriteId:51):-1;}
    bool note(const Identity& id,int before,int after,bool changed){
        if(dispatching_)return true;
        if(changed&&!presentation_.notify(id.x,id.y,before,after))return good_=false;
        return true;
    }
    bool admitted(InstanceHandle h,Identity& id){return good_&&identities_.read(h,id)?true:(good_=false);}
    static std::uint32_t bits(float value){std::uint32_t result;std::memcpy(&result,&value,sizeof(result));return result;}
public:
    explicit GameplayTerrainInstancesRuns(Binding b={},Presentation p={},WallCollision c={}):binding_(b),presentation_(p),collision_(c){}
    GameplayTerrainInstancesRuns(const GameplayTerrainInstancesRuns&)=delete;
    GameplayTerrainInstancesRuns& operator=(const GameplayTerrainInstancesRuns&)=delete;
    bool valid()const{return good_;}
    void observeFailure(bool& owner){good_.observe(owner);}
    // The built-in collision owner can own the entire generated birth/Create
    // transaction. Arbitrary lifecycle observers retain the separate callbacks.
    template<class Owner> bool bindInitializedBirth(Owner& owner){
        if(initializedBirth_||size())return false;
        initializedContext_=&owner;
        initializedBirth_=[](void* p,GameplayTerrainInstancesRuns& terrain,const Identity& id,int tile,GameMakerRandom& random,
            CreateValues& values,bool& unbounded){
            return static_cast<Owner*>(p)->initializedTerrainBirth(id.handle,terrain.record(id),tile,random,values,unbounded);
        };
        return true;
    }
    // The native transaction consumes identity plus its live Create outputs.
    // Expand the legacy actor-shaped record only for a rejected preparation.
    template<class Owner> bool bindInitializedIdentity(Owner& owner){
        if(initializedBirth_||size())return false;
        initializedContext_=&owner;
        initializedBirth_=[](void* p,GameplayTerrainInstancesRuns& terrain,const Identity& id,int tile,GameMakerRandom& random,
            CreateValues& values,bool& unbounded){
            bool admitted=false;
            const bool result=static_cast<Owner*>(p)->initializedTerrainIdentity(id.handle,id.object,id.x,id.y,
                terrain.mutable_[id.handle.slot].active,tile,values,unbounded,admitted);
            if(admitted)return result;
            return static_cast<Owner*>(p)->initializedTerrainBirth(id.handle,terrain.record(id),tile,random,values,unbounded);
        };
        return true;
    }
    unsigned size()const{return identities_.size();}
    unsigned ownershipRunCount()const{return identities_.runCount();}
    template<unsigned Which> bool dirtyContains(unsigned slot)const{
        static_assert(Which<2,"Spatial or collision dirty membership");
        return slot<capacity&&(mutable_[slot].dirtyMembership&(1u<<Which));
    }
    template<unsigned Which> void dirtyRemove(unsigned slot){mutable_[slot].dirtyMembership&=~(1u<<Which);}
    template<unsigned Which> void dirtyInherit(unsigned slot){mutable_[slot].dirtyMembership|=1u<<Which;}
    template<unsigned Which> RunnerRegistrationOrder dirtyOrder(unsigned slot)const{
        RunnerRegistrationOrder order;
        identities_.publicationPart(identities_.currentHandle(slot),
            Which?&TerrainRunnerPublication::collisionDirty:&TerrainRunnerPublication::spatialDirty,order);
        return order;
    }
    template<unsigned Which> bool dirtyMark(unsigned slot,RunnerRegistrationOrder order){
        if(!publishPart(slot,
            Which?&TerrainRunnerPublication::collisionDirty:&TerrainRunnerPublication::spatialDirty,order))return good_=false;
        dirtyInherit<Which>(slot);return true;
    }
    template<unsigned Which> void dirtyClear(){for(auto& m:mutable_)m.dirtyMembership&=~(1u<<Which);}
    template<unsigned Which,class Visit> bool visitDirty(Visit visit)const{
        return identities_.visitPublicationRuns([&](const auto& run){
            const auto& basis=Which?run.publication().collisionDirty:run.publication().spatialDirty;
            return run.visitSlots([&](unsigned slot,unsigned offset){
                return !dirtyContains<Which>(slot)||visit(slot,basis.shifted(offset));
            });
        });
    }
    unsigned spatialStage(unsigned slot)const{return mutable_[slot].spatialStage;}
    void spatialStage(unsigned slot,unsigned stage){mutable_[slot].spatialStage=static_cast<std::uint8_t>(stage);}
    bool spatialPosition(unsigned slot,int& object,int& x,int& y)const{
        Identity id;if(!identities_.read(identities_.currentHandle(slot),id))return false;
        object=id.object;x=id.x;y=id.y;return true;
    }
    RunnerSpatialState spatialLiteral(unsigned slot)const{
        RunnerSpatialState value;
        identities_.publicationPart(identities_.currentHandle(slot),&TerrainRunnerPublication::spatial,value);
        return value;
    }
    void spatialLiteral(unsigned slot,const RunnerSpatialState& value){
        if(!publishPart(slot,&TerrainRunnerPublication::spatial,value)){
            good_=false;return;
        }
        mutable_[slot].spatialStage=7;
    }
    template<class Owner> bool bindInitializedRun(Owner& owner){
        if(!initializedBirth_||initializedContext_!=&owner||initializedRun_||size())return false;
        initializedRun_=[](void* p,const AreaOneBuilder::TerrainRun& run,int tile,GameMakerRandom& random,
            AreaOneBuilder::EmitFunction backend,void* context){
            return static_cast<Owner*>(p)->initializedTerrainRun(run,tile,random,backend,context);
        };return true;
    }
    bool appendInitializedRun(const AreaOneBuilder::TerrainRun& run,int tile,GameMakerRandom& random,
        AreaOneBuilder::EmitFunction backend,void* context){
        if(initializedRun_)return initializedRun_(initializedContext_,run,tile,random,backend,context);
        return run.executeEach([&](const DeferredCreateInstance& command){
            return backend(context,command)&&appendInitialized(command.objectId,command.x,command.y,
                command.active,tile,random);
        });
    }
    // This callback belongs to the same owner as initializedBirth_. It sees
    // only the newly allocated identity and live Create outputs, not a copied
    // actor-sized Record. Allocation and failure still happen one cell at a time.
    template<class Birth> bool appendInitializedCell(int object,int x,int y,bool active,Birth birth){
        DW_PROFILE_SCOPE(profileTerrainRecord);
        Identity id;bool inherited;
        if(!allocate(object,x,y,active,0,-1,id,&inherited))return false;
        BirthPublication publication(*this,id.handle);
        return birth(id.handle,mutable_[id.handle.slot],unboundedWallArt_,inherited)?true:(good_=false);
    }
    // Admit a callback-free homogeneous continuation only after its first
    // complete birth. Clock wrap and nonstandard room state retain the full
    // publication path. This reads state; it reserves no future memberships.
    bool canContinuePublication(InstanceHandle h,unsigned remaining,bool requireSpatial=false,bool requireCollision=false)const{
        if(!identities_.contains(h)||(mutable_[h.slot].allocation&12u)!=12u)return false;
        TerrainRunnerPublication p;if(!identities_.publication(h,p))return false;
        const auto safe=[&](RunnerRegistrationOrder order){
            return order.present()&&(order.high!=0xffffffffu||remaining<=0xffffffffu-order.low);
        };
        return (!requireSpatial||dirtyContains<0>(h.slot))&&(!requireCollision||dirtyContains<1>(h.slot))&&
            safe(p.registration)&&p.room.list==0&&!p.room.marked&&safe(p.room.order)&&
            (!(mutable_[h.slot].allocation&16u)||safe(p.layer.order.serial))&&
            (!dirtyContains<0>(h.slot)||safe(p.spatialDirty))&&
            (!dirtyContains<1>(h.slot)||safe(p.collisionDirty));
    }
    void publishInheritedRoom(unsigned slot){mutable_[slot].allocation|=4u;}
    // Only a callback-free generated continuation. The birth transaction
    // flushes owner counters before leaving the interval, even on failure.
    void inheritedConstructor(unsigned slot,bool inactive,bool spatial,bool collision){
        auto& m=mutable_[slot];m.allocation=2u|4u|8u|32u;
        m.spatialStage=static_cast<std::uint8_t>(3u|(inactive?128u:0u));
        m.dirtyMembership=static_cast<std::uint8_t>((spatial?1u:0u)|(collision?2u:0u));
    }
    // One authoritative constructor publication at the admitted transaction's
    // return boundary, including its exact failed prefix. Room publication
    // may already have succeeded. Order owners supply clocks, not list nodes.
    bool generatedConstructor(unsigned slot,int object,bool inactive,
        RunnerRegistrationOrder registration,RunnerRegistrationOrder spatial,
        RunnerRegistrationOrder collision,unsigned stage=2){
        if(birthPublication_&&birthSlot_==slot){
            birthPublication_->registration=registration;
            birthPublication_->spatialDirty=spatial;
            birthPublication_->collisionDirty=collision;
        }else if(!publishPart(slot,&TerrainRunnerPublication::registration,registration)||
            (spatial.present()&&!publishPart(slot,&TerrainRunnerPublication::spatialDirty,spatial))||
            (collision.present()&&!publishPart(slot,&TerrainRunnerPublication::collisionDirty,collision)))return good_=false;
        auto& m=mutable_[slot];m.allocation=(m.allocation&4u)|2u|8u|32u;
        m.spatialStage=static_cast<std::uint8_t>(stage|(inactive?128u:0u));
        m.dirtyMembership=static_cast<std::uint8_t>((spatial.present()?1u:0u)|(collision.present()?2u:0u));
        ++registered_[family(object)];++registrationRevision_;return true;
    }
    void registerInheritedCount(int object,unsigned count){
        registered_[family(object)]+=count;registrationRevision_+=count;
    }
    void publishInheritedLayer(unsigned slot){mutable_[slot].allocation|=16u;}
    void registerInherited(unsigned slot,int object){
        ++registered_[family(object)];mutable_[slot].allocation|=8u;++registrationRevision_;
    }
    void setAllocatedRoomState(unsigned slot,const RunnerRoomOrderState& state){setRoomState(slot,state);}
    TerrainAppearance appearance(unsigned slot)const{
        TerrainAppearance result;
        if(!identities_.occupied(slot))return result;
        if(mutable_[slot].allocation&64u)
            identities_.publicationPart(identities_.currentHandle(slot),&TerrainRunnerPublication::appearance,result);
        else if(mutable_[slot].allocation&32u)
            result.model=static_cast<unsigned char>(8+family(identities_.object(slot)));
        return result;
    }
    void constructorAppearance(unsigned slot){
        // Shared completed-run appearance remains hidden during constructor
        // publication. Failure before Create exposes immutable defaults.
        mutable_[slot].allocation=(mutable_[slot].allocation&~64u)|32u;
    }
    // Same admitted run and prepared Create plan as the completed first cell.
    // Constructor publication hid the shared result; Create only reveals it.
    void publishInheritedAppearance(unsigned slot){mutable_[slot].allocation|=96u;}
    void setAppearance(unsigned slot,const TerrainAppearance& appearance){
        if(!publishPart(slot,&TerrainRunnerPublication::appearance,appearance)){
            good_=false;return;
        }
        mutable_[slot].allocation|=96u;
    }
    RunnerRetainedLayerRead layerState(unsigned slot)const{
        if(!identities_.occupied(slot))return nullptr;
        RunnerRetainedLayerState p;
        if(mutable_[slot].allocation&16u)
            identities_.publicationPart(identities_.currentHandle(slot),&TerrainRunnerPublication::layer,p);
        return p;
    }
    RunnerRetainedLayerRead layerState(InstanceHandle h)const{
        return identities_.contains(h)?layerState(h.slot):RunnerRetainedLayerRead{};
    }
    void setLayerState(unsigned slot,const RunnerRetainedLayerState& state){
        if(!state.bound()){mutable_[slot].allocation&=~16u;return;}
        if(!publishPart(slot,&TerrainRunnerPublication::layer,state)){
            good_=false;return;
        }
        mutable_[slot].allocation|=16u;
    }
    template<class Visit> bool visitLayerStates(Visit visit)const{
        return identities_.visit([&](const Identity& id){return visit(id.handle,*layerState(id.handle.slot));});
    }
    template<class Visit> bool visitLayerCutoffs(unsigned layer,const RunnerLayerOrderKey* after,Visit visit)const{
        return identities_.visitPublicationRuns([&](const auto& run){
            const auto& basis=run.publication().layer;
            if(!basis.bound()||basis.layer!=layer||!basis.inactive)return true;
            bool found=false;unsigned first=0;
            run.visitSlots([&](unsigned slot,unsigned offset){
                if(!(mutable_[slot].allocation&16u))return true;
                if(after&&!after->before(basis.shifted(offset).order))return true;
                if(!found||(basis.order.tail?offset<first:offset>first)){found=true;first=offset;}
                return true;
            });
            return !found||visit(basis.shifted(first).order);
        });
    }
    unsigned roomRevision()const{return roomRevision_;}
    RunnerRoomOrderState roomState(unsigned slot)const{
        RunnerRoomOrderState state;
        if(identities_.occupied(slot)&&(mutable_[slot].allocation&4u))
            identities_.publicationPart(identities_.currentHandle(slot),&TerrainRunnerPublication::room,state);
        return state;
    }
    void setRoomState(unsigned slot,const RunnerRoomOrderState& state){
        if(state.list<0){mutable_[slot].allocation&=~4u;return;}
        if(!publishPart(slot,&TerrainRunnerPublication::room,state)){
            good_=false;return;
        }
        mutable_[slot].allocation|=4u;
    }
    void roomChanged(){++roomRevision_;}
    void roomChanged(unsigned count){roomRevision_+=count;}
    template<class Visit> bool visitRoomSlots(Visit visit)const{
        return identities_.visit([&](const Identity& id){
            return !(mutable_[id.handle.slot].allocation&4u)||visit(id.handle.slot);
        });
    }
    template<class Visit> bool visitRoomRuns(Visit visit)const{
        return identities_.visitPublicationRuns([&](const auto& run){
            return visit(run.publication().room,[&](auto cell){
                return run.visitSlots([&](unsigned slot,unsigned offset){
                    return !(mutable_[slot].allocation&4u)||cell(slot,offset);
                });
            });
        });
    }
    template<class Visit> bool visitRoomRunReverse(unsigned representative,Visit visit)const{
        return identities_.visitPublicationRun(representative,[&](const auto& run){
            return run.template visitSlots<true>([&](unsigned slot,unsigned){
                return !(mutable_[slot].allocation&4u)||visit(slot);
            });
        });
    }
    unsigned registrationRevision()const{return registrationRevision_;}
    unsigned registeredCount()const{return registered_[0]+registered_[1]+registered_[2];}
    bool registered(unsigned slot)const{
        return identities_.occupied(slot)&&(mutable_[slot].allocation&8u);
    }
    RunnerRegistrationOrder registrationOrder(unsigned slot)const{
        RunnerRegistrationOrder p;
        if(registered(slot))
            identities_.publicationPart(identities_.currentHandle(slot),&TerrainRunnerPublication::registration,p);
        return p;
    }
    InstanceHandle registrationHandle(unsigned slot)const{return identities_.currentHandle(slot);}
    int registrationObject(unsigned slot)const{return identities_.object(slot);}
    bool registerInstance(InstanceHandle h,int object,RunnerRegistrationOrder order){
        if(!order.present()||!identities_.contains(h)||identities_.object(h.slot)!=object)return false;
        if(!registered(h.slot))++registered_[family(object)];
        return publishRegistration(h,order);
    }
    // Only inside appendInitialized's admitted fresh-birth transaction. The
    // allocator owns slot, family and empty membership; no external observer
    // or setter may intervene. General registration retains its checks above.
    void registerAllocated(unsigned slot,int object,RunnerRegistrationOrder order){
        ++registered_[family(object)];publishRegistration(identities_.currentHandle(slot),order);
    }
    bool unregisterInstance(unsigned slot){
        if(!registered(slot))return false;
        --registered_[family(identities_.object(slot))];mutable_[slot].allocation&=~8u;
        ++registrationRevision_;return true;
    }
    void clearRegistrations(){
        if(!registeredCount())return;
        for(auto& m:mutable_)m.allocation&=~8u;
        registered_[0]=registered_[1]=registered_[2]=0;++registrationRevision_;
    }
    template<class IsA> unsigned countRegistered(int object,IsA isA)const{
        const int objects[]={60,93,99};unsigned count=0;
        for(unsigned i=0;i<3;++i)if(registered_[i]&&(objects[i]==object||isA(objects[i],object)))count+=registered_[i];
        return count;
    }
    template<class Visit> bool visitRegistrations(Visit visit)const{
        return identities_.visit([&](const Identity& id){
            return !(mutable_[id.handle.slot].allocation&8u)||visit(id.handle.slot,id.handle.generation,id.object);
        });
    }
    bool spatialCanAttach(InstanceHandle h,int object)const{
        return identities_.contains(h)&&(mutable_[h.slot].allocation&3u)==1&&identities_.object(h.slot)==object;
    }
    void spatialPublish(unsigned slot){mutable_[slot].allocation=(mutable_[slot].allocation&~3u)|2u;}
    void spatialRetire(unsigned slot){mutable_[slot].allocation=(mutable_[slot].allocation&~3u)|3u;}
    void spatialClear(){for(auto& m:mutable_)if((m.allocation&3u)==2)m.allocation=(m.allocation&~3u)|3u;}
    int spatialObject(unsigned slot)const{
        return slot<capacity&&(mutable_[slot].allocation&3u)==2?identities_.object(slot):-1;
    }
    InstanceHandle spatialHandle(unsigned slot)const{
        return spatialObject(slot)>=0?identities_.currentHandle(slot):InstanceHandle{};
    }
    InstanceHandle handle(unsigned ordinal)const{return identities_.handle(ordinal);}
    bool read(InstanceHandle h,Record& out)const{
        Identity id;if(!identities_.read(h,id))return false;out=record(id);return true;
    }
    Read find(InstanceHandle h)const{Identity id;if(!identities_.read(h,id))return nullptr;return {this,h,record(id)};}
    bool ownsIdentity(InstanceHandle h,const Record& r)const{
        Identity id;return identities_.read(h,id)&&r.lifetimeId==h.generation&&
            r.objectId==id.object&&r.x==id.x&&r.y==id.y&&r.sourceInstanceId==id.source&&r.creationCodeId==id.creationCode;
    }
    template<class Visit> bool visitWallRows(int first,int end,Visit visit)const{
        if(first>=end)return true;
        const auto expand=[&](const Identity& id){return visit(record(id));};
        return unboundedWallArt_?identities_.visit(expand):identities_.visitRows(first,end,expand);
    }
private:
    bool publishRegistration(InstanceHandle h,RunnerRegistrationOrder order){
        if(!publishPart(h.slot,&TerrainRunnerPublication::registration,order))return good_=false;
        mutable_[h.slot].allocation|=8u;++registrationRevision_;return true;
    }
    bool allocate(int object,int x,int y,bool active,std::uint32_t source,int code,Identity& id,bool* inherited=nullptr){
        birthPublication_=nullptr;
        if(!good_)return false;
        if((object!=60&&object!=93&&object!=99)||size()==capacity||!next_)return good_=false;
        unsigned slot=freeHint_;while(slot<capacity&&identities_.occupied(slot))++slot;
        if(slot==capacity)return good_=false;
        const InstanceHandle h{slot,next_};
        // Room publication is invisible until its allocation bit is set.
        // A new adjacent cell may inherit the run's order basis; its actual
        // room boundary validates/replaces that prediction synchronously.
        if(!identities_.appendInherited(h,object,x,y,source,code,inherited))return good_=false;
        ++next_;freeHint_=slot+1;
        auto& m=mutable_[slot];if(m.allocation)m=Mutable{};
        m.allocation=1;m.active=active;if(object==99)m.spriteId=46;
        id={h,object,x,y,code,source};
        return true;
    }
public:
    bool append(int object,int x,int y,bool active,std::uint32_t source=0,int code=-1){
        DW_PROFILE_SCOPE(profileTerrainRecord);
        Identity id;
        if(!allocate(object,x,y,active,source,code,id))return false;
        return binding_.birth(id.handle,record(id))?true:(good_=false);
    }
    bool appendInitialized(int object,int x,int y,bool active,int tile,GameMakerRandom& random){
        if(!initializedBirth_)return append(object,x,y,active)&&createProperties(handle(size()-1),tile,false,random);
        DW_PROFILE_SCOPE(profileTerrainRecord);
        Identity id;
        if(!allocate(object,x,y,active,0,-1,id))return false;
        BirthPublication publication(*this,id.handle);
        return initializedBirth_(initializedContext_,*this,id,tile,random,
            mutable_[id.handle.slot],unboundedWallArt_)?true:(good_=false);
    }
    bool appendRoom(const PackedRoomInstance& r){
        if(!r.instanceId||bits(r.scaleX)!=0x3f800000u||bits(r.scaleY)!=0x3f800000u||
            (bits(r.rotation)&0x7fffffffu)||bits(r.imageSpeed)!=0x3f800000u||
            (bits(r.imageIndex)&0x7fffffffu)||r.preCreateCodeId!=-1||r.flags)return good_=false;
        return append(r.objectId,r.x,r.y,true,r.instanceId,r.creationCodeId);
    }
    bool createProperties(InstanceHandle h,int tile,bool glitch,GameMakerRandom& random){
        DW_PROFILE_SCOPE(profileTerrainCreate);
        Identity id;if(!admitted(h,id))return false;auto& m=mutable_[h.slot];
        if(id.object==99)return true;
        if(!terrainCreateProperties(id.object,tile,glitch,random,[&](int sprite){
            if(!binding_.sprite(h,record(id),sprite))return false;
            m.spriteId=sprite;
            if(id.object==60&&(wallRadius(sprite)<0||wallRadius(sprite)>16))unboundedWallArt_=true;
            return true;
        },[&](int index){
            if(!binding_.phaseIndex(h,record(id),unsigned(index)*256u))return false;
            m.imageIndex=index;return true;
        },[&](int speed){m.imageSpeed=speed;return true;}))return good_=false;
        if(id.object==60&&m.wallNeedsReset){m.wall={};m.wallNeedsReset=false;}return true;
    }
    bool roomCreation(InstanceHandle h){
        Identity id;if(!admitted(h,id))return false;auto& m=mutable_[h.slot];
        if(id.creationCode==-1)return true;m.wallNeedsReset=true;
        if(id.object!=60||!wallRoomCreation(id.creationCode,m.wall))return good_=false;return true;
    }
    bool requestDestroy(InstanceHandle h){
        Identity id;if(!admitted(h,id))return false;auto& m=mutable_[h.slot];
        if(m.destroyRequested)return true;
        if(!binding_.markDestroyed(h,record(id)))return good_=false;
        const int before=drawSprite(id,m);const bool wasVisible=visible(id,m);m.destroyRequested=true;
        if(wasVisible&&!collision_.notify(id.x,id.y,false))return good_=false;
        return note(id,before,-1,before>=0);
    }
    bool imageIndex(InstanceHandle h,int frame){
        Identity id;if(!admitted(h,id)||frame<0||frame>15)return good_=false;auto& m=mutable_[h.slot];
        const bool changed=m.imageIndex!=frame;
        if(!binding_.phaseIndex(h,record(id),unsigned(frame)*256u))return good_=false;
        m.imageIndex=frame;return note(id,drawSprite(id,m),drawSprite(id,m),changed&&m.wall.checked&&visible(id,m));
    }
    template<class Ops> bool wallEvent(InstanceHandle h,int area,GameMakerRandom& random,Ops& ops){
        Identity id;if(dispatching_||!admitted(h,id))return good_=false;auto& m=mutable_[h.slot];
        if(!m.active||m.destroyRequested||id.object==99)return true;
        if(id.object!=60)return good_=false;
        const bool selected=m.wall.checked!=0,wasVisible=visible(id,m);
        const int sprite=m.spriteId,frame=m.imageIndex,before=drawSprite(id,m);
        dispatching_=true;m.wallNeedsReset=true;
        const bool succeeded=wallEndStep(m.wall,area,random,ops);dispatching_=false;
        if(!succeeded||!good_)return good_=false;
        return note(id,before,drawSprite(id,m),wasVisible!=visible(id,m)||(visible(id,m)&&
            (selected!=(m.wall.checked!=0)||sprite!=m.spriteId||frame!=m.imageIndex)));
    }
    bool activation(InstanceHandle h,bool active){
        Identity id;if(!admitted(h,id))return false;auto& m=mutable_[h.slot];
        const int before=drawSprite(id,m);const bool wasVisible=visible(id,m);
        if(!binding_.activation(h,record(id),active))return good_=false;
        m.active=active;
        if(wasVisible!=visible(id,m)&&!collision_.notify(id.x,id.y,visible(id,m)))return good_=false;
        return note(id,before,drawSprite(id,m),before!=drawSprite(id,m));
    }
    bool retireMarked(InstanceHandle h){
        birthPublication_=nullptr;
        Identity id;if(!admitted(h,id)||!mutable_[h.slot].destroyRequested)return good_=false;
        if(!binding_.retire(h,record(id)))return good_=false;
        unregisterInstance(h.slot);
        ++roomRevision_;
        if(!identities_.retire(h))return good_=false;
        if(h.slot<freeHint_)freeHint_=h.slot;return true;
    }
    bool clear(){
        birthPublication_=nullptr;
        if(!good_)return false;
        while(size()){
            const auto h=handle(0);Identity id;identities_.read(h,id);
            if(!binding_.retire(h,record(id)))return good_=false;
            unregisterInstance(h.slot);
            ++roomRevision_;
            if(!identities_.retire(h))return good_=false;
            if(h.slot<freeHint_)freeHint_=h.slot;
        }
        unboundedWallArt_=false;return true;
    }
};
}
#endif

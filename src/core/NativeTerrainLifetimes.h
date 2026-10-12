#ifndef DOWNWELL_NATIVE_TERRAIN_LIFETIMES_H
#define DOWNWELL_NATIVE_TERRAIN_LIFETIMES_H
#include "TerrainOwnershipRuns.h"
#include "PlacementNew.h"
#include "WallEndStep.h"
#include "PackedWallState.h"
#include "WallFootprint.h"
#include "RunnerLogicalLayerOrder.h"
namespace downwell {
// Identity and birth chronology: no duplicate spatial tree, room list, animation state or
// raster cache. Generated source IDs remain unknown until the shared global
// allocation stream is connected; family-local generations are never VM IDs.
template<unsigned Capacity> class NativeTerrainLifetimes {
    struct Order {
        std::uint32_t stamp=0;
        RunnerRetainedLayerState layer;
        Order shifted(unsigned n)const{return {stamp?stamp+n:0,layer.shifted(n)};}
        bool same(const Order& other)const{return stamp==other.stamp&&layer.same(other.layer);}
    };
    using Runs=TerrainOwnershipRuns<Capacity,Order>;
    Runs runs_;
    unsigned next_=1,freeHint_=0;
    unsigned char flags_[Capacity]{};
    std::int16_t createSprite_[Capacity]{};
    PackedWallState walls_[Capacity]{};
    WallEndStepState* stepping_=nullptr;
    InstanceHandle steppingHandle_{};
    bool good_=true;
    bool wallPresentationState_=false,unboundedWallArt_=false;
    Order* pendingPublication_=nullptr;
    InstanceHandle pendingHandle_{};
    bool publicationRun_=false;
    RunnerRetainedLayerState inheritedLayer_;
    unsigned inheritedLayerCount_=0;
    // Membership only: retained layer/order state stays authoritative in runs_.
    // Active scenery must not participate in Draw's inactive cutoff query.
    unsigned char layerCutoffSlots_[(Capacity+7)/8]{};
    unsigned layerCutoffCount_=0;
    void selectLayerCutoff(unsigned slot,bool selected){
        auto& byte=layerCutoffSlots_[slot/8];const unsigned bit=1u<<(slot&7);
        if(bool(byte&bit)==selected)return;
        if(selected){byte|=bit;++layerCutoffCount_;}
        else{byte&=~bit;--layerCutoffCount_;}
    }
public:
    static constexpr unsigned capacity=Capacity;
    using Identity=typename Runs::Identity;
    enum class SpanResult { unsupported, success, failure };
    // Borrowed AreaOneBuilder prepared-cell stream: two bytes per cell, with
    // object kind in bit 7 and a signed-grid column biased by 32 in bits 0–6.
    struct PreparedCells {
        const unsigned char* bytes=nullptr;unsigned count=0;int y=0;
        // Optional lifetime promise, never inferred from a raw pointer: bytes
        // remain immutable until these terrain lifetimes have all been cleared.
        bool retained=false;
        unsigned kind(unsigned i)const{return bytes[i*2]>>7;}
        int object(unsigned i)const{return kind(i)?93:60;}
        int x(unsigned i)const{return (int(bytes[i*2]&127)-32)*16;}
    };
    struct Binding {
        void* context=nullptr;
        bool (*admit)(void*,unsigned)=nullptr;
        bool (*birth)(void*,const Identity&,bool,std::uint32_t&,std::uint32_t&)=nullptr;
        bool (*retire)(void*,unsigned)=nullptr;
        bool (*mark)(void*,unsigned)=nullptr;
        bool (*create)(void*,const Identity&,int,std::uint32_t&)=nullptr;
        bool (*collision)(void*,const Identity&,bool)=nullptr;
        bool (*presentation)(void*,const Identity&,int,int)=nullptr;
        bool (*activation)(void*,const Identity&,bool,std::uint32_t)=nullptr;
        bool (*beginRun)(void*)=nullptr;
        bool (*endRun)(void*)=nullptr;
        SpanResult (*constructSpan)(void*,const Identity&,PreparedCells,int,unsigned&,
                                   std::uint32_t&,std::uint32_t*,RunnerRetainedLayerState&)=nullptr;
        // Complete callback-free admission/publication, with no per-cell run
        // session. Unsupported must leave every owner untouched.
        decltype(constructSpan) constructDirectSpan=nullptr;
    };
    bool spatialFlags(InstanceHandle h,std::uint32_t& out)const{
        if(!contains(h))return false;
        const unsigned value=flags_[h.slot];
        out=(value&3u)|((value&4u)?8u:0u)|((value&8u)?0x200u:0u)|((value&16u)?0x8000u:0u);
        return true;
    }
    bool bind(Binding binding){
        if(!good_)return false;
        if(!binding.context&&(binding.admit||binding.birth||binding.retire||binding.mark||binding.create||binding.collision||binding.presentation||binding.activation||binding.beginRun||binding.endRun||binding.constructSpan||binding.constructDirectSpan))return good_=false;
        if(binding_.context)return binding_.context==binding.context&&binding_.admit==binding.admit&&
            binding_.birth==binding.birth&&binding_.retire==binding.retire&&binding_.mark==binding.mark&&binding_.create==binding.create&&
            binding_.collision==binding.collision&&binding_.presentation==binding.presentation&&binding_.activation==binding.activation&&
            binding_.beginRun==binding.beginRun&&binding_.endRun==binding.endRun&&binding_.constructSpan==binding.constructSpan&&
            binding_.constructDirectSpan==binding.constructDirectSpan;
        if(binding.context&&(!binding.admit||!binding.birth||!binding.retire||!binding.mark||size()))return good_=false;
        binding_=binding;return true;
    }
    unsigned registrationOrder(InstanceHandle h)const{
        Order value;return good_&&runs_.publication(h,value)?value.stamp:0;
    }
    bool rebindOrder(InstanceHandle h,std::uint32_t order){
        Order value;
        if(!good_||!order||!runs_.publication(h,value))return good_=false;
        value.stamp=order;return good_=runs_.changePublication(h,value);
    }
    RunnerRetainedLayerRead layerState(InstanceHandle h)const{
        if(pendingPublication_&&h.slot==pendingHandle_.slot&&h.generation==pendingHandle_.generation)
            return pendingPublication_->layer;
        Order value;return runs_.publication(h,value)?RunnerRetainedLayerRead(value.layer):nullptr;
    }
    RunnerRetainedLayerRead layerState(unsigned slot)const{
        return layerState(pendingPublication_&&slot==pendingHandle_.slot?pendingHandle_:runs_.currentHandle(slot));
    }
    void setLayerState(unsigned slot,const RunnerRetainedLayerState& layer){
        if(pendingPublication_&&slot==pendingHandle_.slot){
            pendingPublication_->layer=layer;
            if(publicationRun_){inheritedLayer_=layer;inheritedLayerCount_=1;}
            return;
        }
        const auto h=runs_.currentHandle(slot);Order value;
        if(!runs_.publication(h,value)){good_=false;return;}
        value.layer=layer;if(!runs_.changePublication(h,value))good_=false;
        else selectLayerCutoff(slot,layer.bound()&&layer.inactive);
    }
    void publishInheritedLayer(unsigned slot){
        if(!publicationRun_||!pendingPublication_||slot!=pendingHandle_.slot||!inheritedLayer_.bound()){
            good_=false;return;
        }
        pendingPublication_->layer=inheritedLayer_.shifted(inheritedLayerCount_++);
    }
    // The caller admits a callback-free terrain segment, not an event Create.
    // Completed prefixes must be charged even when a later cell fails.
    template<class Emit> bool publishRun(Emit emit){
        if(!good_||publicationRun_||pendingPublication_)return good_=false;
        publicationRun_=true;inheritedLayer_={};inheritedLayerCount_=0;
        const bool began=!binding_.beginRun||binding_.beginRun(binding_.context);
        const bool emitted=began&&emit();
        const bool ended=!began||!binding_.endRun||binding_.endRun(binding_.context);
        publicationRun_=false;
        return good_=good_&&began&&emitted&&ended;
    }
    template<class Visit> bool visitLayerStates(Visit visit)const{
        return runs_.visit([&](const Identity& id){return visit(id.handle,*layerState(id.handle));});
    }
    template<class Visit> bool visitLayerCutoffs(unsigned layer,const RunnerLayerOrderKey* after,Visit visit)const{
        if(!layerCutoffCount_)return true;
        // Candidates are unordered: the caller reduces to the earliest key
        // or tests existence in an interval. Read keys live after layer moves.
        for(unsigned byte=0;byte<(Capacity+7)/8;++byte){
            const unsigned bits=layerCutoffSlots_[byte];if(!bits)continue;
            for(unsigned bit=0;bit<8;++bit)if(bits&(1u<<bit)){
                const auto state=layerState(byte*8+bit);
                if(state&&state->bound()&&state->layer==layer&&state->inactive&&
                   (!after||after->before(state->order))&&!visit(state->order))return false;
            }
        }
        return true;
    }
    template<class Visit> bool visitLayerCutoffsReference(unsigned layer,const RunnerLayerOrderKey* after,Visit visit)const{
        return runs_.visitPublicationRuns([&](const auto& run){
            const auto& basis=run.publication().layer;
            if(!basis.bound()||basis.layer!=layer||!basis.inactive)return true;
            bool found=false;unsigned first=0;
            run.visitSlots([&](unsigned,unsigned offset){
                if(after&&!after->before(basis.shifted(offset).order))return true;
                if(!found||(basis.order.tail?offset<first:offset>first)){found=true;first=offset;}
                return true;
            });
            return !found||visit(basis.shifted(first).order);
        });
    }
    bool valid()const{return good_;}
    std::uint32_t lastLifetime()const{return next_-1;}
    // -2 means constructor defaults, not the runner's missing-sprite value.
    bool appearance(InstanceHandle h,int& sprite,unsigned& index,bool& created)const{
        if(!contains(h))return false;
        created=(flags_[h.slot]&32u)!=0;sprite=createSprite_[h.slot];
        index=created&&runs_.object(h.slot)==93?23u:0u;
        if(created&&runs_.object(h.slot)==60){WallEndStepState state;if(!wallState(h,state))return false;index=state.imageIndex;}
        return true;
    }
    bool cachedSprite(InstanceHandle h,int& sprite)const{
        if(!contains(h))return false;
        sprite=(flags_[h.slot]&64u)?createSprite_[h.slot]:-2;return true;
    }
    bool cachedOffset(InstanceHandle h,int& x,int& y)const{
        return contains(h)&&walls_[h.slot].cachedOffset(x,y);
    }
    // Store the cache's origin, not the instance position. The source list
    // branch restores position but leaves these temporary bounds dirty.
    bool publishCachedOffset(InstanceHandle h,int x,int y){
        return good_=(good_&&contains(h)&&walls_[h.slot].storeCachedOffset(x,y));
    }
    bool publishQueryDirty(InstanceHandle h){
        if(!contains(h))return good_=false;
        flags_[h.slot]|=4u;return true;
    }
    bool publishIndexed(InstanceHandle h){
        if(!contains(h))return good_=false;
        flags_[h.slot]|=16u;return true;
    }
    // One flags bit selects constructor versus post-Create cached resource.
    // A normal refresh also returns the cached origin to the actual position.
    bool publishRefreshedBounds(InstanceHandle h,std::uint32_t spatial){
        if(!contains(h))return good_=false;
        if(!walls_[h.slot].storeCachedOffset(0,0))return good_=false;
        const unsigned previous=flags_[h.slot];
        flags_[h.slot]=(previous&163u)|((previous&32u)?64u:0u)|((spatial&8u)?4u:0u)|
            ((spatial&0x200u)?8u:0u)|((spatial&0x8000u)?16u:0u);
        return true;
    }
    bool createProperties(InstanceHandle h,int tile){
        Identity id;
        if(!readIdentity(h,id))return good_=false;
        return createKnownIdentity(id,tile);
    }
private:
    bool createKnownIdentity(const Identity& id,int tile){
        const auto h=id.handle;std::uint32_t spatial;
        if(!spatialFlags(h,spatial)||(flags_[h.slot]&32u))return good_=false;
        if(id.object!=99){
            if(tile<0||tile>32767||(binding_.context&&
                (!binding_.create||!binding_.create(binding_.context,id,tile,spatial))))return good_=false;
            createSprite_[h.slot]=static_cast<std::int16_t>(tile);
            if(id.object==60&&(wallRadius(tile)<0||wallRadius(tile)>16))unboundedWallArt_=true;
        }
        flags_[h.slot]=(flags_[h.slot]&3u)|32u|((spatial&8u)?4u:0u)|
            ((spatial&0x200u)?8u:0u)|((spatial&0x8000u)?16u:0u);
        walls_[h.slot].store(WallEndStepState{});
        return true;
    }
public:
    // Serialized creation code runs after object Create, before the next
    // room birth. Keep its application stage in the spare flags bit and its
    // immutable operand in the existing run identity, not a per-cell record.
    bool roomCreation(InstanceHandle h){
        Identity id;
        if(stepping_||!readIdentity(h,id)||!(flags_[h.slot]&32u))return good_=false;
        WallEndStepState state=walls_[h.slot].load();
        if(id.creationCode!=-1&&(id.object!=60||!wallRoomCreation(id.creationCode,state)))return good_=false;
        if(!walls_[h.slot].store(state))return good_=false;
        flags_[h.slot]|=128u;return true;
    }
    // Initial event state only; wallState exposes retained End Step mutations.
    // Never reconstruct a previously stepped wall with this initial snapshot.
    bool initialWallState(InstanceHandle h,WallEndStepState& state)const{
        Identity id;
        if(!readIdentity(h,id)||id.object!=60||!(flags_[h.slot]&32u))return false;
        state={};
        return !(flags_[h.slot]&128u)||wallRoomCreation(id.creationCode,state);
    }
    // Read-only inspection also exposes the retained failure prefix. Execution
    // remains terminal after failure; stale handles never read a reused slot.
    bool wallState(InstanceHandle h,WallEndStepState& state)const{
        Identity id;if(!runs_.read(h,id)||id.object!=60||!(flags_[h.slot]&32u))return false;
        state=stepping_&&h.slot==steppingHandle_.slot&&h.generation==steppingHandle_.generation?
            *stepping_:walls_[h.slot].load();return true;
    }
    bool setWallDeactivation(InstanceHandle h,bool value){
        WallEndStepState state;if(!good_||!wallState(h,state))return good_=false;
        state.deactivate=value;
        if(stepping_&&h.slot==steppingHandle_.slot&&h.generation==steppingHandle_.generation)*stepping_=state;
        return walls_[h.slot].store(state)||(good_=false);
    }
    template<class Ops> bool wallEvent(InstanceHandle h,int area,GameMakerRandom& random,Ops& ops){
        Identity id;WallEndStepState state;
        if(stepping_||!readIdentity(h,id))return good_=false;
        // Wall (99) owns an empty End Step override; it must never inherit
        // code 986, consume its RNG or require object-60 wall state.
        if((flags_[h.slot]&3u)||id.object==99)return true;
        if(id.object!=60||!wallState(h,state))return good_=false;
        const int before=drawSprite(h);const bool checked=state.checked!=0;const int image=state.imageIndex;
        stepping_=&state;steppingHandle_=h;
        const bool ok=wallEndStep(state,area,random,ops);
        stepping_=nullptr;steppingHandle_={};
        const bool retained=walls_[h.slot].store(state);
        if(!ok||!retained||!good_)return good_=false;
        wallPresentationState_=true;
        const int after=drawSprite(h);
        return notifyPresentation(id,before,after,before!=after||
            (after>=0&&(checked!=(state.checked!=0)||image!=state.imageIndex)));
    }
    unsigned size()const{return runs_.size();}
    bool hasWallPresentationState()const{return wallPresentationState_;}
    struct WallDrawRecord {
        int objectId,x,y;bool active,destroyRequested;int spriteId,imageIndex;WallEndStepState wall;
    };
    template<class Visit> bool visitWallRows(int first,int end,Visit visit)const{
        if(!good_)return false;
        const auto emit=[&](const Identity& id){
            if(id.object!=60)return true;
            WallEndStepState state;
            if((flags_[id.handle.slot]&32u)&&!wallState(id.handle,state))return false;
            const int sprite=state.checked?createSprite_[id.handle.slot]:51;
            if(!wallIntersectsRows(sprite,id.y,first,end))return true;
            return visit(WallDrawRecord{id.object,id.x,id.y,!(flags_[id.handle.slot]&2u),
                (flags_[id.handle.slot]&1u)!=0,createSprite_[id.handle.slot],state.imageIndex,state});
        };
        return unboundedWallArt_?runs_.visit(emit):runs_.visitRows(first,end,emit);
    }
    unsigned runCount()const{return runs_.runCount();}
    unsigned available()const{return Capacity-size();}
    bool contains(InstanceHandle h)const{return good_&&runs_.contains(h);}
    bool read(InstanceHandle h,Identity& id,bool& active,bool& destroyed)const{
        if(!good_||!runs_.read(h,id))return false;
        active=(flags_[h.slot]&2)==0;destroyed=(flags_[h.slot]&1)!=0;return true;
    }
    InstanceHandle handle(unsigned ordinal)const{return runs_.handle(ordinal);}
    InstanceHandle currentHandle(unsigned slot)const{return good_?runs_.currentHandle(slot):InstanceHandle{};}
    template<class Visit> bool visit(Visit visit)const{
        return good_&&runs_.visit([&](const Identity& id){return visit(id,(flags_[id.handle.slot]&2)==0,
            (flags_[id.handle.slot]&1)!=0);});
    }
    bool append(int object,int x,int y,bool active=true,unsigned source=0,int creation=-1){
        return appendConstructed(object,x,y,active,source,creation,nullptr,0);
    }
    // Only a callback-free generated segment may borrow this constructor.
    // Each cell still completes birth and then Create before the next cell.
    // The born identity is live on the stack: do not recover it by ordinal
    // and reconstruct it from the run just to execute that same Create.
    template<class Producer> bool constructRun(int tile,bool& effectsSucceeded,Producer producer){
        return publishRun([&]{
            return producer([&](int object,int x,int y){
                if(object!=60&&object!=93)return good_=false;
                return appendConstructed(object,x,y,true,0,-1,&effectsSucceeded,tile);
            });
        });
    }
    // Admit the complete immutable segment once. If it needs a new dynamic
    // layer or a journal service boundary, keep the scalar birth sequence.
    bool constructPrepared(int tile,bool& effectsSucceeded,PreparedCells cells){
        if(cells.count&&!cells.bytes)return good_=false;
        if(!good_||publicationRun_||pendingPublication_)return good_=false;
        if(cells.retained&&cells.count&&binding_.constructDirectSpan){
            bool admitted=false;
            publicationRun_=true;
            const bool okay=appendPreparedCells(cells,tile,effectsSucceeded,true,&admitted);
            publicationRun_=false;
            if(admitted||!okay)return okay;
        }
        return publishRun([&]{
            if(!cells.count)return true;
            return appendPreparedCells(cells,tile,effectsSucceeded);
        });
    }
private:
    bool appendPreparedCells(PreparedCells cells,int tile,bool& effectsSucceeded,
                             bool direct=false,bool* admitted=nullptr){
        const unsigned count=cells.count;
        unsigned slot=freeHint_;while(slot<Capacity&&runs_.occupied(slot))++slot;
        bool contiguous=count<=Capacity-slot;
        if(contiguous)for(unsigned i=0;i<count;++i)if(runs_.occupied(slot+i)){contiguous=false;break;}
        const auto construct=direct?binding_.constructDirectSpan:binding_.constructSpan;
        if(construct&&contiguous&&canAppend(count)){
            Order temporary;Order* order=&temporary;
            if(cells.retained){
                order=runs_.reserveRetainedCells({slot,next_},cells.bytes,cells.y,count);
                if(!order)return good_=false;
            }
            unsigned completed=0;std::uint32_t spatial[2]{};
            const Identity first{{slot,next_},cells.object(0),cells.x(0),cells.y,-1,0};
            const auto result=construct(binding_.context,first,cells,tile,
                                                    completed,order->stamp,spatial,order->layer);
            if(result!=SpanResult::unsupported){
                if(admitted)*admitted=true;
                if(completed>count){if(cells.retained)runs_.commitRetainedCells(0);return good_=false;}
                if(cells.retained&&!runs_.commitRetainedCells(completed))return good_=false;
                unsigned i=0;
                if(cells.retained&&completed){
                    for(;i<completed;++i){
                        const auto kind=cells.kind(i),value=spatial[kind];
                        if(order->layer.bound()&&order->layer.inactive)selectLayerCutoff(slot+i,true);
                        flags_[slot+i]=32u|((value&8u)?4u:0u)|((value&0x200u)?8u:0u)|((value&0x8000u)?16u:0u);
                        createSprite_[slot+i]=static_cast<std::int16_t>(tile);walls_[slot+i]=PackedWallState{};
                        if(!kind&&(wallRadius(tile)<0||wallRadius(tile)>16))unboundedWallArt_=true;
                    }
                }
                while(i<completed){
                    const unsigned kind=cells.kind(i);const int x=cells.x(i),object=cells.object(i);
                    int step=16;if(i+1<completed&&cells.x(i+1)==x-16)step=-16;
                    unsigned n=1;
                    while(i+n<completed&&cells.kind(i+n)==kind&&cells.x(i+n)==x+int(n)*step)++n;
                    if(!runs_.appendSpan({slot+i,next_+i},object,x,cells.y,step,n,order->shifted(i)))return good_=false;
                    if(order->layer.bound()&&order->layer.inactive)
                        for(unsigned j=0;j<n;++j)selectLayerCutoff(slot+i+j,true);
                    const auto value=spatial[kind];
                    const unsigned char flags=32u|((value&8u)?4u:0u)|((value&0x200u)?8u:0u)|((value&0x8000u)?16u:0u);
                    for(unsigned j=0;j<n;++j){
                        flags_[slot+i+j]=flags;createSprite_[slot+i+j]=static_cast<std::int16_t>(tile);
                        walls_[slot+i+j]=PackedWallState{};
                    }
                    if(!kind&&(wallRadius(tile)<0||wallRadius(tile)>16))unboundedWallArt_=true;
                    i+=n;
                }
                next_+=completed;freeHint_=slot+completed;
                // This callback can fail only in birth publication. Like the
                // scalar birth path, do not report it as a Create failure by
                // overwriting the caller's separate effects flag.
                return good_=good_&&result==SpanResult::success&&completed==count;
            }
            if(cells.retained&&!runs_.commitRetainedCells(0))return good_=false;
        }
        if(direct)return true;
        for(unsigned i=0;i<count;++i){
            if(!appendConstructed(cells.object(i),cells.x(i),cells.y,true,0,-1,&effectsSucceeded,tile))return false;
        }
        return true;
    }
    bool appendConstructed(int object,int x,int y,bool active,unsigned source,int creation,
        bool* effectsSucceeded,int tile){
        if(!canAppend(1)||(object!=60&&object!=93&&object!=99))return good_=false;
        unsigned slot=freeHint_;while(slot<Capacity&&runs_.occupied(slot))++slot;
        Order order;
        const Identity identity{{slot,next_},object,x,y,creation,source};
        std::uint32_t spatial=8u;
        if(slot==Capacity||pendingPublication_)return good_=false;
        pendingPublication_=&order;pendingHandle_=identity.handle;
        const bool published=!binding_.birth||binding_.birth(binding_.context,identity,active,order.stamp,spatial);
        pendingPublication_=nullptr;pendingHandle_={};
        if(!published||!runs_.append({slot,next_},object,x,y,source,creation,order))return good_=false;
        if(order.layer.bound()&&order.layer.inactive)selectLayerCutoff(slot,true);
        ++next_;freeHint_=slot+1;
        flags_[slot]=(active?0:2)|((spatial&8u)?4u:0u)|((spatial&0x200u)?8u:0u)|((spatial&0x8000u)?16u:0u);
        createSprite_[slot]=-2;
        walls_[slot]=PackedWallState{}; // Slot reuse must not inherit a query cache.
        walls_[slot].store(WallEndStepState{});
        if(object==60&&!active)wallPresentationState_=true;
        if(effectsSucceeded)
            return *effectsSucceeded=*effectsSucceeded&&createKnownIdentity(identity,tile);
        return true;
    }
public:
    bool canAppend(unsigned count)const{
        return good_&&count<=available()&&next_&&(!count||count-1<=0xffffffffu-next_)&&
            (!binding_.admit||binding_.admit(binding_.context,count));
    }
    bool markDestroyed(InstanceHandle h){
        if(!good_||!runs_.contains(h))return good_=false;
        if(flags_[h.slot]&1)return true;
        if(binding_.mark&&!binding_.mark(binding_.context,h.slot))return good_=false;
        Identity id;if(!runs_.read(h,id))return good_=false;
        const int before=drawSprite(h);flags_[h.slot]|=1;
        if(id.object==60)wallPresentationState_=true;
        if(before>=0&&binding_.collision&&!binding_.collision(binding_.context,id,false))return good_=false;
        return stepping_?true:notifyPresentation(id,before,-1,before>=0);
    }
    bool activation(InstanceHandle h,bool active){
        Identity id;std::uint32_t spatial;
        if(!readIdentity(h,id)||!spatialFlags(h,spatial))return good_=false;
        const int before=drawSprite(h);
        if(binding_.activation&&!binding_.activation(binding_.context,id,active,spatial))return good_=false;
        flags_[h.slot]=active?(flags_[h.slot]&~2u):(flags_[h.slot]|2u);
        const int after=drawSprite(h);
        if(before!=after){
            wallPresentationState_=true;
            if(binding_.collision&&!binding_.collision(binding_.context,id,after>=0))return good_=false;
        }
        return stepping_?true:notifyPresentation(id,before,after,before!=after);
    }
    bool retire(InstanceHandle h){
        if(stepping_||!good_||!runs_.contains(h)||!(flags_[h.slot]&1)||
            (binding_.retire&&!binding_.retire(binding_.context,h.slot))||!runs_.retire(h))return good_=false;
        selectLayerCutoff(h.slot,false);
        if(h.slot<freeHint_)freeHint_=h.slot;
        return true;
    }
    // Room teardown invalidates every old key without resetting generations.
    // No bulk temporary on the Amiga task stack.
    bool clear(){
        if(stepping_||runs_.reservationPending())return good_=false;
        if(!good_)return false;
        // Releasing retained layers can split publication runs. Reacquire by
        // ordinal rather than mutating a borrowed descriptor traversal.
        if(binding_.retire)for(unsigned i=0;i<runs_.size();++i)
            if(!binding_.retire(binding_.context,runs_.handle(i).slot))return good_=false;
        runs_.~Runs();new(&runs_) Runs;freeHint_=0;wallPresentationState_=unboundedWallArt_=false;
        for(auto& byte:layerCutoffSlots_)byte=0;layerCutoffCount_=0;return true;
    }
private:
    int drawSprite(InstanceHandle h)const{
        if(runs_.object(h.slot)!=60||(flags_[h.slot]&3u))return -1;
        WallEndStepState state;
        return wallState(h,state)&&state.checked?createSprite_[h.slot]:51;
    }
    bool notifyPresentation(const Identity& id,int before,int after,bool changed){
        if(changed&&binding_.presentation&&!binding_.presentation(binding_.context,id,before,after))return good_=false;
        return true;
    }
    bool readIdentity(InstanceHandle h,Identity& id)const{return good_&&runs_.read(h,id);}
    Binding binding_{};
};
}
#endif

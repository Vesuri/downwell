#include "core/SpritePack.h"
#include "core/EnemyPresentation.h"
#include "core/RunnerSpriteBounds.h"
#include "core/SpriteCollisionPack.h"
#include "core/RunnerSpatialInstances.h"
#include "core/RunnerObjectFamilies.h"
#include "core/StoredCollisionLifecycle.h"
#include "core/RoomLoader.h"
#include "core/DeferredInstanceQueue.h"
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void testBasicValidation() {
    const downwell::SpritePack empty(nullptr, 0);
    require(empty.error() == downwell::SpritePackError::tooShort,
            "empty sprite pack is rejected");
    const std::uint8_t bad[] = {'N', 'O', 'P', 'E', 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0};
    require(downwell::SpritePack(bad, sizeof(bad)).error() ==
                downwell::SpritePackError::badMagic,
            "bad sprite-pack magic is rejected");
}

void testGeneratedPack(const char* path,const char* maskPath,const char* objectPath) {
    std::ifstream input(path, std::ios::binary);
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                          std::istreambuf_iterator<char>());
    const downwell::SpritePack pack(bytes.data(), bytes.size());
    require(pack.valid(), "generated sprite pack passes native validation");
    std::string collisionPath(path);collisionPath.replace(collisionPath.find_last_of('.'),std::string::npos,".dwsc");
    std::ifstream collisionInput(collisionPath,std::ios::binary);
    const std::vector<std::uint8_t> compactBytes((std::istreambuf_iterator<char>(collisionInput)),
                                                std::istreambuf_iterator<char>());
    const downwell::SpriteCollisionPack compact(compactBytes.data(),compactBytes.size());
    require(compact.valid()&&compact.spriteCount()==pack.spriteCount(),"compact collision table validates");
    require(compactBytes.size()==26700,"collision-only table excludes atlas payload and frame mappings");
    std::ifstream maskInput(maskPath,std::ios::binary);
    const std::vector<std::uint8_t> maskBytes((std::istreambuf_iterator<char>(maskInput)),{});
    const downwell::SpriteMaskPack masks(maskBytes.data(),maskBytes.size());
    require(masks.valid(),"all-partner mask pack resolves");
    {
        std::ifstream objectInput(objectPath,std::ios::binary);
        const std::vector<std::uint8_t> objectBytes((std::istreambuf_iterator<char>(objectInput)),{});
        const downwell::ObjectPack objects(objectBytes.data(),objectBytes.size());
        downwell::RunnerObjectFamilies<386> families;
        require(families.bindInstanceFlags(objects),"load complete object binding flags");
        using Owner=downwell::RunnerSpatialInstances<2,8>;
        downwell::PreparedObjectDefaults<386> preparedDefaults;
        require(preparedDefaults.prepare(objects,compact),"prepare immutable object defaults before births");
        require(!preparedDefaults.get(-1)&&!preparedDefaults.get(int(objects.objectCount())),
            "prepared defaults reject absent object IDs");
        downwell::PreparedObjectDefaults<1> tooSmall;
        require(!tooSmall.prepare(objects,compact)&&!tooSmall.get(0),"insufficient default capacity fails closed");
        for(unsigned id=0;id<objects.objectCount();++id)for(bool registered:{false,true}){
            Owner owner;Owner::Properties properties;
            properties.sprite.spriteId=-999;properties.sprite.maskId=597;
            properties.sprite.imageIndex={0x41000000u};properties.timeline={0x41000000u};
            Owner preparedOwner;auto preparedProperties=properties;
            require(preparedOwner.attach({0,1},0,{}),"attach prepared binding lifetime");
            require(owner.attach({0,1},0,{}),"attach object-binding lifetime");
            std::int32_t creationDepth=0x12345678;
            require(owner.bindObjectFromPacks({0,1},int(id),registered,properties,objects,compact,families,false,&creationDepth),
                    "bind complete original object through both resource packs");
            Owner::Real preparedCreation,expectedCreation;
            require(preparedOwner.bindObjectPrepared({0,1},int(id),registered,preparedProperties,
                preparedDefaults,families,false,&preparedCreation)&&expectedCreation.assignInteger(creationDepth),
                "bind each original object through prepared defaults");
            require(preparedCreation.bits==expectedCreation.bits&&
                preparedProperties.depth.bits==properties.depth.bits&&
                preparedProperties.sprite.spriteId==properties.sprite.spriteId&&
                preparedProperties.sprite.maskId==properties.sprite.maskId&&
                preparedProperties.sprite.imageIndex.bits==properties.sprite.imageIndex.bits&&
                preparedProperties.timeline.bits==properties.timeline.bits&&
                preparedProperties.cachedFrame.bits==properties.cachedFrame.bits&&
                preparedOwner.resolve({0,1})->flags==owner.resolve({0,1})->flags&&
                preparedOwner.pendingSpatial()==owner.pendingSpatial()&&
                preparedOwner.pendingCollision()==owner.pendingCollision()&&
                preparedOwner.bindingRevision()==owner.bindingRevision()&&
                preparedOwner.captureRevision()==owner.captureRevision(),
                "prepared and immediate binding preserve properties, flags, queues and revisions");
            downwell::PackedObject original;require(objects.object(id,original),"read independent object record");
            require(creationDepth==original.creationDepth,"binding returns the independently decoded compatibility depth");
            require(properties.sprite.spriteId==original.spriteId&&properties.sprite.maskId==original.textureMaskId,
                    "binding restores object artwork and explicit mask defaults");
            Owner::Real depth;require(depth.assign(downwell::MotionBinary64::fromInteger(original.depth)),"convert original depth");
            require(properties.depth.bits==depth.bits,"binding uses serialized depth, not compatibility creation override");
            downwell::PackedSprite artwork;
            const unsigned frames=original.spriteId>=0&&pack.sprite(unsigned(original.spriteId),artwork)?artwork.frameCount:0;
            const bool reset=8>=frames;
            require(properties.sprite.imageIndex.bits==(reset?0u:0x41000000u)&&
                    properties.timeline.bits==(reset&&frames?0u:0x41000000u),"binding composes original sprite resets");
            const unsigned flags=8u|(original.solid()?0x20u:0u)|(original.visible()?0x10u:0u)|
                (original.persistent()?0x40u:0u)|(original.managed()?0x200000u:0u);
            require(owner.resolve({0,1})->flags==flags,"binding installs source instance flags without changing lifetime");
            const bool queued=original.solid()||(*families.flags(id)&0x28u);
            require(owner.pendingSpatial()==unsigned(queued)&&
                    owner.pendingCollision()==unsigned(queued&&(*families.flags(id)&8u)),"binding queues only eligible families");
            // Same-sprite rebinding must not reset animation even when its
            // current index is beyond the immutable resource's frame count.
            properties.sprite.imageIndex=preparedProperties.sprite.imageIndex={0x47800000u};
            properties.timeline=preparedProperties.timeline={0x41000000u};
            properties.cachedFrame=preparedProperties.cachedFrame={0x40000000u};
            require(owner.bindObjectFromPacks({0,1},int(id),registered,properties,objects,compact,families,true)&&
                preparedOwner.bindObjectPrepared({0,1},int(id),registered,preparedProperties,preparedDefaults,families,true)&&
                preparedProperties.sprite.imageIndex.bits==properties.sprite.imageIndex.bits&&
                preparedProperties.timeline.bits==properties.timeline.bits&&
                preparedProperties.cachedFrame.bits==properties.cachedFrame.bits&&
                preparedOwner.resolve({0,1})->flags==owner.resolve({0,1})->flags&&
                preparedOwner.pendingSpatial()==owner.pendingSpatial()&&
                preparedOwner.pendingCollision()==owner.pendingCollision(),
                "prepared rebinding retains source same-sprite no-op and all-prepared queue behavior");
        }
        require(!preparedDefaults.prepare(objects,downwell::SpriteCollisionPack(nullptr,0))&&
            !preparedDefaults.get(0)&&preparedDefaults.prepare(objects,compact),
            "invalid preparation clears stale defaults and valid preparation restores them");
        for(unsigned id=0;id<objects.objectCount();++id)
        for(unsigned flags:{8u,10u,0u,0x200078u})for(bool registered:{false,true}){
            Owner reference,materialized;Owner::Properties a,b;Owner::State initial;initial.flags=flags;
            initial.x={0x43800000u};initial.y={0x44000000u};
            std::int32_t depth;Owner::Real preparedDepth,expectedDepth;
            require(reference.attach({0,1},int(id),initial)&&
                reference.bindObjectFromPacks({0,1},int(id),registered,a,objects,compact,families,false,&depth)&&
                materialized.attachObjectPrepared({0,1},int(id),initial,registered,b,preparedDefaults,families,&preparedDepth)&&
                expectedDepth.assignInteger(depth),"materialize each original newborn and unusual-state fallback");
            require(a.sprite.spriteId==b.sprite.spriteId&&a.sprite.maskId==b.sprite.maskId&&
                a.sprite.imageIndex.bits==b.sprite.imageIndex.bits&&a.sprite.scaleX.bits==b.sprite.scaleX.bits&&
                a.sprite.scaleY.bits==b.sprite.scaleY.bits&&a.sprite.angle.bits==b.sprite.angle.bits&&
                a.depth.bits==b.depth.bits&&a.timeline.bits==b.timeline.bits&&a.cachedFrame.bits==b.cachedFrame.bits&&
                preparedDepth.bits==expectedDepth.bits&&
                reference.resolve({0,1})->flags==materialized.resolve({0,1})->flags&&
                reference.pendingSpatial()==materialized.pendingSpatial()&&
                reference.pendingCollision()==materialized.pendingCollision()&&
                reference.bindingRevision()==materialized.bindingRevision()&&
                reference.captureRevision()==materialized.captureRevision(),
                "prepared construction preserves fields, flags, queues and membership revisions");
            if(flags==8u||flags==10u){
                Owner positioned;Owner::Properties p;Owner::Real positionedDepth;
                for(unsigned generation=1;generation<=2;++generation){
                    require(positioned.attachObjectPrepared({0,generation},int(id),initial,
                        registered,p,preparedDefaults,families,&positionedDepth),
                        "prepared birth on fresh and recycled storage");
                    const auto& s=*positioned.resolve({0,generation});
                    require(s.x.bits==initial.x.bits&&s.y.bits==initial.y.bits&&
                        s.previous.x.bits==initial.x.bits&&s.previous.y.bits==initial.y.bits&&
                        !s.bounds.left.bits&&!s.bounds.top.bits&&!s.bounds.right.bits&&!s.bounds.bottom.bits&&
                        s.kind==1&&!s.special&&s.flags==materialized.resolve({0,1})->flags&&
                        p.sprite.spriteId==b.sprite.spriteId&&p.sprite.maskId==b.sprite.maskId&&
                        p.sprite.scaleX.bits==b.sprite.scaleX.bits&&p.sprite.scaleY.bits==b.sprite.scaleY.bits&&
                        p.sprite.angle.bits==b.sprite.angle.bits&&p.sprite.imageIndex.bits==b.sprite.imageIndex.bits&&
                        p.depth.bits==b.depth.bits&&p.timeline.bits==b.timeline.bits&&
                        p.cachedFrame.bits==b.cachedFrame.bits&&positionedDepth.bits==preparedDepth.bits,
                        "recycled birth equals fresh prepared state and properties");
                    Owner::Box changed{{0x3f800000u},{0x40000000u},{0x40400000u},{0x40800000u}};
                    require(positioned.refresh({0,generation},*families.flags(id),false,&changed,true,false)&&
                        positioned.retire({0,generation}),"recycled birth discards previous cached bounds");
                    // Reuse must rebuild every property, not just position and
                    // resource IDs. None of these stores is a live setter.
                    p.sprite.spriteId=-999;p.sprite.maskId=597;
                    p.sprite.scaleX={0x40000000u};p.sprite.scaleY={0x3f000000u};
                    p.sprite.angle={0x42b40000u};p.sprite.imageIndex={0x41000000u};
                    p.depth={0x42f60000u};p.timeline={0x41000000u};p.cachedFrame={0x40800000u};
                }
            }
        }
        Owner incomplete;Owner::Properties unchanged;
        require(incomplete.attach({0,1},0,{}),"attach incomplete-graph fixture");
        downwell::RunnerObjectFamilies<386> spatialOnly;require(spatialOnly.bindSpatialFlags(objects),"load deliberately incomplete flags");
        std::int32_t failedDepth=0x12345678;
        require(!incomplete.bindObjectFromPacks({0,1},0,true,unchanged,objects,compact,spatialOnly,false,&failedDepth)&&
                !incomplete.valid()&&unchanged.sprite.spriteId==-1,"reject missing property flags before mutation");
        require(failedDepth==0x12345678,"failed binding does not publish compatibility depth");
        Owner rejectedBirth;Owner::Properties rejectedProperties;
        rejectedProperties.sprite.spriteId=51;rejectedProperties.sprite.maskId=585;
        rejectedProperties.sprite.scaleX={0x40000000u};rejectedProperties.timeline={0x41000000u};
        Owner::Real unpublishedDepth{0x12345678u};
        require(!rejectedBirth.attachObjectPrepared({0,1},60,{},true,rejectedProperties,
                preparedDefaults,spatialOnly,&unpublishedDepth)&&!rejectedBirth.valid()&&
            rejectedBirth.resolve({0,1})&&rejectedBirth.resolve({0,1})->flags==8&&
            rejectedProperties.sprite.spriteId==-1&&rejectedProperties.sprite.maskId==-1&&
            rejectedProperties.sprite.scaleX.bits==0x3f800000u&&!rejectedProperties.timeline.bits&&
            unpublishedDepth.bits==0x12345678u&&!rejectedBirth.pendingSpatial()&&!rejectedBirth.pendingCollision(),
            "failed prepared construction retains attached lifetime and reset-property prefix without publishing depth or queues");
        using StoredOwner=downwell::RunnerSpatialInstances<4,8>;
        StoredOwner storedOwner;StoredOwner::Properties properties[4];
        downwell::InstanceStore<4> records;
        auto trig=[](auto,auto&,auto&){return false;}; // Zero angle must bypass trigonometry.
        auto round=[](auto value,int& out){return value.promoted().roundPixel(out);};
        downwell::StoredCollisionLifecycle<4,8,decltype(families),decltype(trig),decltype(round)>
            lifecycle{storedOwner,properties,objects,compact,families,trig,round};
        downwell::PackedObject field;require(objects.findObject(82,field),"read placed field defaults");
        downwell::PackedObject bat;require(objects.findObject(116,bat)&&bat.spriteId==153&&bat.textureMaskId==-1,
                "bat serialized default follows its sprite until Step sets an explicit mask");
        downwell::RoomInstanceDefinition definition{123,82,false};
        definition.spriteId=field.spriteId;definition.maskId=field.textureMaskId;
        definition.x=32;definition.y=64;definition.scaleX=0.5f;definition.depth=-123;
        unsigned callbacks=0;
        auto event=[&](downwell::InstanceRecord&,downwell::RoomInstancePhase phase){
            if(phase!=downwell::RoomInstancePhase::preCreate)return;
            ++callbacks;const auto h=records.handleAt(0);const auto* state=storedOwner.resolve(h);
            require(state&&!(state->flags&8u),"resource-backed bounds published before PreCreate");
            int depth;
            require(properties[h.slot].sprite.scaleX.bits==0x3f000000u&&properties[h.slot].depth.promoted().roundPixel(depth)&&depth==-123,
                    "placed scale and layer depth override object defaults before callbacks");
        };
        auto room=[](int){};auto deferred=[](auto&){};
        require(downwell::RoomLoader<4>::enter(records,7,&definition,1,true,event,room,deferred,lifecycle)==downwell::LoadRoomResult::success&&callbacks==1,
                "real-resource lifecycle composes with room loading");
        const auto old=records.handleAt(0);
        require(downwell::RoomLoader<4>::enter(records,8,nullptr,0,false,event,room,deferred,lifecycle)==downwell::LoadRoomResult::success&&
                !storedOwner.resolve(old)&&records.empty(),"room transition retires the resource-backed owner");
        downwell::DeferredInstanceQueue<2> births;
        downwell::DeferredCreateInstance command(82,30,60);command.active=false;
        command.scaleX=0.5f;command.imageIndex=2.5f;
        require(births.enqueueCreate(command),"queue inactive resource-backed birth");
        std::uint32_t next=400;unsigned creates=0;
        require(births.apply(records,objects,8,next,[&](downwell::InstanceRecord& record){
            ++creates;const auto h=records.handleAt(0);int depth;
            require(storedOwner.resolve(h)&&!(storedOwner.resolve(h)->flags&8u)&&
                    (storedOwner.resolve(h)->flags&2u),"inactive dynamic birth refreshes bounds before Create");
            require(properties[h.slot].depth.promoted().roundPixel(depth)&&depth==field.creationDepth&&record.depth==depth,
                    "dynamic compatibility depth supersedes serialized binding depth");
            require(properties[h.slot].sprite.imageIndex.bits==0x40200000u,"command index is installed after default sprite binding");
        },lifecycle)==downwell::ApplyInstanceCommandsResult::success&&creates==1,"dynamic resource lifecycle reaches Create once");
        const auto born=records.handleAt(0);
        require(records.reconcileObjectMembership([&](auto h,const auto& record,bool member){return lifecycle.membership(h,record,member);}),
                "dormant dynamic birth leaves active object list");
        require(records.requestActivation(born,true,[&](auto h,const auto& record,bool active){return lifecycle.activation(h,record,active);}),
                "activate dynamic birth through collision adapter");
        const auto movement=downwell::InstanceLifecycleBinding::bindMasks(lifecycle);
        require(movement.integerPosition(born,records.at(0),-32,64)&&
                storedOwner.resolve(born)->x.bits==0xc2000000u&&
                storedOwner.resolve(born)->y.bits==0x42800000u&&records.at(0).x==30&&
                storedOwner.pendingSpatial()!=0,
                "movement binding publishes signed runner coordinates and queues bounds before record mutation");
        require(!movement.integerPosition(old,records.at(0),0,0)&&
                storedOwner.resolve(born)->x.bits==0xc2000000u,
                "movement binding rejects an expired lifetime without moving its replacement");
        require(lifecycle.move(born,records.at(0),{0x41f40000u},{0x42700000u}),"move using explicit fractional actor position");
        require(storedOwner.resolve(born)->x.bits==0x41f40000u&&records.at(0).x==30,
                "collision owner retains 30.5 rather than rounded record position");
        require(storedOwner.flush([&](auto h,bool publish){return lifecycle.refresh(h,*records.resolve(h),publish);}),
                "dirty pass refreshes through stored resources");
        auto& animated=properties[born.slot];
        animated.sprite.imageIndex.bits=0x41000000u; // 8, beyond the one-frame skeleton sprite.
        animated.timeline.bits=0x41000000u;animated.cachedFrame.bits=0x40000000u;
        require(movement.sprite(born,records.at(0),205)&&animated.sprite.spriteId==205&&
                animated.sprite.imageIndex.bits==0&&animated.timeline.bits==0&&
                animated.cachedFrame.bits==0xbf800000u&&(storedOwner.resolve(born)->flags&8u),
                "stored sprite mutation applies resource-backed sequence resets and invalidation");
        require(storedOwner.flush([&](auto h,bool publish){return lifecycle.refresh(h,*records.resolve(h),publish);}),
                "sprite mutation refreshes collision bounds through the same owner");
        animated.cachedFrame.bits=0x40000000u;
        require(movement.sprite(born,records.at(0),205)&&animated.cachedFrame.bits==0x40000000u&&
                !(storedOwner.resolve(born)->flags&8u)&&storedOwner.pendingSpatial()==0,
                "same sprite preserves cached frame and clean bounds");
        const auto flagsBeforeIndex=storedOwner.resolve(born)->flags;
        const auto timelineBeforeIndex=animated.timeline.bits;
        for(unsigned phase : {0u,1u,127u,128u,255u,256u,639u,640u,65535u,
                0xffffffu,0x1000000u,0x1000001u,0x1000002u,0x1000003u,
                0x7fffffbfu,0x7fffffc0u,0x7fffffffu}) {
            const float expected=static_cast<float>(phase)/256.0f;
            std::uint32_t bits;std::memcpy(&bits,&expected,sizeof(bits));
            require(movement.phaseIndex(born,records.at(0),phase)&&animated.sprite.imageIndex.bits==bits&&
                    storedOwner.resolve(born)->flags==flagsBeforeIndex&&storedOwner.pendingSpatial()==0&&
                    animated.timeline.bits==timelineBeforeIndex&&animated.cachedFrame.bits==0x40000000u,
                    "phase publication narrows index without dirtying bounds or resetting sequence state");
        }
        const auto savedIndex=animated.sprite.imageIndex.bits;
        require(!movement.phaseIndex(born,records.at(0),0x80000000u)&&
                !movement.phaseIndex(old,records.at(0),0)&&animated.sprite.imageIndex.bits==savedIndex,
                "unsupported phase and stale lifetime cannot overwrite index");
        using Transform=downwell::RunnerTransformField;
        const downwell::RunnerImageValue opaque{0x3f800000u};
        require(movement.integerTransform(born,records.at(0),Transform::scaleX,-1,opaque,0xffffffu)&&
                animated.sprite.scaleX.bits==0xbf800000u&&(storedOwner.resolve(born)->flags&8u)&&
                !(storedOwner.resolve(born)->flags&0x4000u)&&storedOwner.pendingSpatial()!=0,
                "mirroring publishes narrowed scale and invalidates resource-backed bounds");
        require(movement.integerTransform(born,records.at(0),Transform::scaleY,-1,opaque,0xffffffu)&&
                movement.integerTransform(born,records.at(0),Transform::angle,270,opaque,0xffffffu)&&
                animated.sprite.scaleY.bits==0xbf800000u&&animated.sprite.angle.bits==0x43870000u,
                "binding forwards vertical mirror and crawler quarter-turn angle");
        require(movement.integerTransform(born,records.at(0),Transform::scaleX,1,opaque,0xffffffu)&&
                movement.integerTransform(born,records.at(0),Transform::scaleY,1,opaque,0xffffffu)&&
                movement.integerTransform(born,records.at(0),Transform::angle,0,opaque,0xffffffu)&&
                (storedOwner.resolve(born)->flags&0x4000u),"ordinary transforms restore source fast-draw flag");
        require(movement.transform(born,records.at(0),Transform::scaleX,{0x3f000000u},opaque,0xffffffu)&&
                !(storedOwner.resolve(born)->flags&0x4000u)&&
                storedOwner.flush([&](auto h,bool publish){return lifecycle.refresh(h,*records.resolve(h),publish);}),
                "fractional scale retains original placed transform and refreshes zero-angle bounds");
        require(movement.transform(born,records.at(0),Transform::scaleX,{0x3f000000u},opaque,0xffffffu)&&
                !(storedOwner.resolve(born)->flags&8u)&&storedOwner.pendingSpatial()==0&&
                !movement.integerTransform(old,records.at(0),Transform::scaleX,-1,opaque,0xffffffu)&&
                animated.sprite.scaleX.bits==0x3f000000u,
                "equal transforms stay clean and retired handles cannot change their replacement");
        auto wrong=records.at(0);wrong.objectId=35;
        const auto originalMask=animated.sprite.maskId;
        require(movement.mask(born,records.at(0),153)&&animated.sprite.maskId==153&&
                (storedOwner.resolve(born)->flags&8u)&&storedOwner.pendingSpatial()!=0&&
                storedOwner.flush([&](auto h,bool publish){return lifecycle.refresh(h,*records.resolve(h),publish);}),
                "mask binding updates resource bounds and dirty queues");
        require(movement.mask(born,records.at(0),153)&&!(storedOwner.resolve(born)->flags&8u)&&
                !movement.mask(old,records.at(0),154)&&!movement.mask(born,wrong,154)&&animated.sprite.maskId==153,
                "same mask is clean and stale or mismatched records cannot change it");
        require(movement.mask(born,records.at(0),originalMask),"restore original field mask");
        require(!lifecycle.move(born,wrong,{}, {})&&!lifecycle.refresh(old,records.at(0),true)&&
                !lifecycle.sprite(born,wrong,206)&&!lifecycle.sprite(old,records.at(0),206)&&
                animated.sprite.spriteId==205,
                "wrong-object snapshots and expired handles cannot mutate collision ownership");
        require(!lifecycle.rebind(born,wrong,35,true)&&!lifecycle.rebind(old,records.at(0),35,true)&&
                storedOwner.objectId(born)==82,
                "rebind rejects stale handles and wrong old-object snapshots before changing identity");
        const auto rebind=[&](auto h,const auto& record,int object,bool registered){
            require(record.objectId==storedOwner.objectId(h),"rebind callback sees matching old identity");
            return lifecycle.rebind(h,record,object,registered);
        };
        const auto oldPosition=*storedOwner.resolve(born);
        downwell::PackedObject playerDefaults;require(objects.findObject(35,playerDefaults),"read player binding defaults");
        require(records.bindObject(born,35,true,rebind)&&storedOwner.objectId(born)==35&&records.at(0).objectId==35,
                "store and collision owner publish new object identity at the checked boundary");
        const auto exactOnly=[](int,int){return false;};
        require(records.registeredObjectCount(82,exactOnly)==0&&records.registeredObjectCount(35,exactOnly)==1,
                "checked rebind removes the old object membership and registers the new one");
        int reboundDepth;
        require(animated.sprite.maskId==playerDefaults.textureMaskId&&animated.sprite.spriteId==playerDefaults.spriteId&&
                animated.depth.promoted().roundPixel(reboundDepth)&&reboundDepth==playerDefaults.depth&&
                animated.sprite.scaleX.bits==0x3f000000u&&storedOwner.resolve(born)->x.bits==oldPosition.x.bits&&
                storedOwner.resolve(born)->y.bits==oldPosition.y.bits,
                "rebind installs original resource defaults without discarding fractional position or placed scale");
        require(records.bindObject(born,82,false,rebind)&&storedOwner.objectId(born)==82&&records.at(0).objectId==82,
                "unregistered object rebind returns to the field identity without replacing its lifetime");
        require(records.registeredObjectCount(82,exactOnly)==0&&records.registeredObjectCount(35,exactOnly)==0,
                "unregistered rebind leaves neither object list populated");
        require(records.clear([&](auto h,const auto& record){return lifecycle.retire(h,record);}),"retire dynamic birth before next room");
        definition.rotation=30.0f;
        require(downwell::RoomLoader<4>::enter(records,9,&definition,1,false,event,room,deferred,lifecycle)==downwell::LoadRoomResult::lifecycleFailure&&
                callbacks==1&&!storedOwner.valid(),"unsupported bounds calculation stops before PreCreate and invalidates collision owner");
    }
    {
        using Owner=downwell::RunnerSpatialInstances<2,8>;
        unsigned empty=0;
        for(unsigned id=0;id<pack.spriteCount();++id){
            downwell::PackedSprite original;require(pack.sprite(id,original),"read original frame count");
            for(int eighth:{-9,-1,0,7,int(original.frameCount*8),int(original.frameCount*8+1)}){
                Owner owner;Owner::State clean;clean.flags=0;
                require(owner.attach({0,1},1,clean),"attach sprite resource test");
                Owner::Sprite sprite;sprite.spriteId=-999;sprite.maskId=597;
                const float index=float(eighth)/8.0f;
                std::memcpy(&sprite.imageIndex.bits,&index,4);
                const auto before=sprite.imageIndex;
                Owner::Real timeline{0x40000000u},cached{0x3f800000u};
                require(owner.changeSpriteFromPack({0,1},sprite,timeline,cached,int(id),compact,0x38),"resource-backed sprite change succeeds");
                const bool reset=eighth/8>=int(original.frameCount);
                require(sprite.spriteId==int(id)&&sprite.maskId==597&&sprite.imageIndex.bits==(reset?0u:before.bits),
                    "resource count drives reset without replacing explicit collision mask");
                require(timeline.bits==(reset&&original.frameCount?0u:0x40000000u)&&cached.bits==0xbf800000u,
                    "empty resources preserve sequence position while invalidating cached frame");
                require(owner.pendingSpatial()==1&&owner.pendingCollision()==1,"resource change queues both eligible updates");
            }
            empty+=original.frameCount==0;
        }
        require(empty==3,"all three original empty sprites covered");
        Owner owner;require(owner.attach({0,1},1,{}),"attach missing-resource fixture");
        Owner::Sprite sprite;sprite.spriteId=208;Owner::Real timeline{0x40000000u},cached{};
        const downwell::SpriteCollisionPack invalid(nullptr,0);
        require(owner.changeSpriteFromPack({0,1},sprite,timeline,cached,208,invalid,0x38)&&owner.valid(),
            "same-ID setter does not consult unavailable metadata");
        require(owner.changeSpriteFromPack({0,1},sprite,timeline,cached,999999,compact,0x38)&&timeline.bits==0x40000000u,
            "absent sprite ID is a zero-frame resource, not a pack error");
        const auto before=sprite.spriteId;
        require(!owner.changeSpriteFromPack({0,1},sprite,timeline,cached,208,invalid,0x38)&&!owner.valid()&&sprite.spriteId==before,
            "unavailable pack fails before sprite mutation");
    }
    {
        std::ifstream objectInput(objectPath,std::ios::binary);
        const std::vector<std::uint8_t> objectBytes((std::istreambuf_iterator<char>(objectInput)),{});
        const downwell::ObjectPack objects(objectBytes.data(),objectBytes.size());
        using Owner=downwell::RunnerSpatialInstances<4,16>;
        Owner owner;Owner::Properties properties[4];downwell::InstanceStore<4> records;
        downwell::RunnerObjectFamilies<386> families;
        require(families.bindInstanceFlags(objects),"query fixture source families");
        require(families.isA(255,82),"exit field belongs to the recovered parentTimeField family");
        auto trig=[](auto,auto&,auto&){return false;};
        auto round=[](auto value,int& out){return value.promoted().roundPixel(out);};
        downwell::StoredCollisionLifecycle<4,16,decltype(families),decltype(trig),decltype(round)>
            lifecycle{owner,properties,objects,compact,families,trig,round};
        downwell::RoomInstanceDefinition placed[]={{100,35,false},{101,82,false},{102,255,false}};
        for(auto& item:placed){
            downwell::PackedObject object;require(objects.findObject(item.objectId,object),"query object metadata");
            item.spriteId=object.spriteId;item.maskId=item.objectId==35?31:object.textureMaskId;
            if(item.objectId!=35){item.x=240;item.y=100;}
            if(item.objectId==255)item.scaleX=item.scaleY=2.0f;
        }
        require(downwell::RoomLoader<4>::enterChecked(records,7,placed,3,false,
            [](auto&,auto){return true;},[](int){return true;},[](auto&){return true;},lifecycle)==
            downwell::LoadRoomResult::success,"query fixture publishes all resource-backed births");
        const auto player=records.handleAt(0);downwell::RunnerMaskDispatch dispatch;
        const auto x=downwell::RunnerImageValue{0x43704000u}.promoted(); // 240.25
        const auto y=downwell::RunnerImageValue{0x42c84000u}.promoted(); // 100.125
        Owner::Result hit;
        auto query=[&](){return lifecycle.placeVm(records,player,82,x,y,masks,dispatch,trig,hit);};
        require(query()&&hit.found&&hit.handle.slot!=player.slot,"fractional player query resolves a precise field");
        const auto selected=hit.handle;
        require(records.resolve(selected)&&families.isA(records.resolve(selected)->objectId,82),
            "selected collision handle resolves the correct live family");
        require(owner.resolve(player)->x.bits==0&&owner.resolve(player)->y.bits==0&&records.at(0).x==0,
            "temporary query movement restores published position without editing stored coordinates");
        require(records.erase(records.resolve(selected)->instanceId,
            [&](auto h,const auto& record){return lifecycle.retire(h,record);}),"retire selected field before store invalidation");
        require(!records.resolve(selected)&&!owner.resolve(selected)&&query()&&hit.found&&
            !(hit.handle.slot==selected.slot&&hit.handle.generation==selected.generation),
            "next query resolves surviving field after compaction, never a stale handle");
        auto surviving=hit.handle;
        require(records.requestActivation(surviving,false,[&](auto h,const auto& record,bool active){
            return lifecycle.activation(h,record,active);}),"deactivate selected field");
        require(query()&&!hit.found,"inactive precise field is excluded before membership reconciliation");
        require(records.requestActivation(surviving,true,[&](auto h,const auto& record,bool active){
            return lifecycle.activation(h,record,active);})&&query()&&hit.found,"reactivated field rejoins query eligibility");
        downwell::DeferredInstanceQueue<1> replacement;
        require(replacement.enqueueCreate(35,1000,1000),"queue unrelated slot replacement");
        std::uint32_t nextId=200;
        require(replacement.applyChecked(records,objects,7,nextId,[](auto&){return true;},lifecycle)==
            downwell::ApplyInstanceCommandsResult::success,"replacement publishes through checked lifecycle");
        const auto reused=records.handleAt(records.size()-1);
        require(reused.slot==selected.slot&&reused.generation!=selected.generation&&
            !records.resolve(selected)&&!owner.resolve(selected)&&query()&&hit.found&&hit.handle.slot==surviving.slot,
            "slot reuse cannot resurrect a selected field or change its object family");
        // Deliberately violate the adapter contract; never quietly skip this
        // live store/owner mismatch or publish a false-negative result.
        records.resolve(surviving)->objectId=35;
        hit={true,{3,99}};
        require(!query()&&!owner.valid()&&hit.found&&hit.handle.generation==99,
            "candidate identity mismatch is terminal and leaves result unpublished");
    }
    {
        using Collisions=downwell::RunnerSpatialInstances<4,8>;
        Collisions collisions;Collisions::Sprite visuals[2];
        visuals[0].spriteId=84;visuals[1].spriteId=597;
        visuals[1].imageIndex.bits=0xc0f80000; // -7.75 truncates then wraps onto the one mask.
        require(collisions.attach({0,1},3,{})&&collisions.attach({1,1},1,{}),"attach collision partners");
        std::vector<unsigned> refreshOrder;
        auto round=[](auto v,std::int32_t& pixel){return v.promoted().roundPixel(pixel);};
        auto noTrig=[](auto,auto&,auto&){require(false,"unrotated pair must not request trig");return false;};
        auto refresh=[&](downwell::InstanceHandle h,bool publish){
            refreshOrder.push_back(h.slot);
            return collisions.refreshSprite(h,0x30,false,compact,visuals[h.slot],true,noTrig,round,publish);
        };
        bool hit=false;
        auto collide=[&](bool update,const downwell::SpriteMaskPack& pixels){
            return collisions.collideCompatibility({0,1},{1,1},visuals[0],visuals[1],compact,pixels,{},
                update,refresh,noTrig,round,hit);
        };
        require(collide(true,masks)&&hit&&refreshOrder==std::vector<unsigned>({1,0}),
            "source collision dispatch refreshes candidate before self then samples original masks");
        struct Graph {
            std::uint32_t value=0x30;
            bool valid()const{return true;}
            const std::uint32_t* flags(unsigned)const{return &value;}
            bool isA(int child,int parent)const{return child==parent;}
            bool markSpatialFamily(unsigned){return true;}
        } graph;
        Collisions::Result selected;
        require(collisions.place({0,1},1,{}, {},false,graph,refresh,
            [&](auto a,auto b,bool update,bool& found){
                require(a.slot==0&&b.slot==1,"query resolves lifetime-safe candidate handles");
                return collisions.collideCompatibility(a,b,visuals[a.slot],visuals[b.slot],compact,masks,{},
                    update,refresh,noTrig,round,found);
            },selected)&&selected.found&&selected.handle.slot==1&&selected.handle.generation==1,
            "instance-place owner reaches resource-backed pair dispatch and returns field identity");
        require(collisions.move({0,1},{0x42200000},{},0x30),"move blood away from the field");
        refreshOrder.clear();
        require(collide(false,masks)&&!hit&&refreshOrder.empty()&&(collisions.resolve({0,1})->flags&8),
            "update-disabled collision leaves dirty caches untouched and uses current sampling position");
        require(collide(true,masks)&&!hit&&refreshOrder==std::vector<unsigned>({0}),
            "updated separated pair fails broad phase");
        require(collisions.move({0,1},{},{},0x30),"restore blood position");
        visuals[1].spriteId=84;visuals[1].imageIndex.bits=0x7f800000;
        require(refresh({1,1},true),"refresh second rectangular resource");
        downwell::SpriteMaskPack unavailable(nullptr,0);
        require(collide(true,unavailable)&&hit,"rectangular pair requires neither mask pixels nor image-index conversion");
        visuals[1].maskId=597;visuals[1].imageIndex={};
        require(refresh({1,1},true)&&collide(true,masks)&&hit,"explicit precise mask selects stored pixels despite rectangular art");
        require(collisions.setExclusions({1,1},2),"set source flag 2");
        require(collide(true,masks)&&hit,"direct pair gate excludes 0x100001, unlike query candidate gate 0x100003");
        require(collisions.setExclusions({1,1},1),"exclude candidate");
        refreshOrder.clear();
        require(collide(true,unavailable)&&!hit&&refreshOrder.empty(),"excluded pair returns before refresh/resource reads");
        require(collisions.setExclusions({1,1},0),"restore eligibility");
        visuals[1].maskId=9999;
        require(collide(true,masks)&&!hit,"invalid explicit mask does not fall back in pair dispatch");
        visuals[1].maskId=597;hit=true;
        require(!collide(true,unavailable)&&!collisions.valid()&&hit,"missing mask payload fails owner without returning a false miss");
    }
    using Owner=downwell::RunnerSpatialInstances<2,4>;
    Owner owner;
    require(owner.attach({0,1},0,{}),"attach metadata-backed collision instance");
    auto round=[](downwell::RunnerImageValue v,std::int32_t& pixel){return v.promoted().roundPixel(pixel);};
    auto noTrig=[](auto,auto&,auto&){require(false,"zero angle must not invoke trigonometry");return false;};
    for(unsigned id=0;id<pack.spriteCount();++id){
        downwell::PackedSprite full;downwell::PackedSpriteCollision small;
        require(pack.sprite(id,full)&&compact.sprite(id,small),"both metadata views resolve every ID");
        require(full.originX==small.originX&&full.originY==small.originY&&
            full.marginLeft==small.marginLeft&&full.marginTop==small.marginTop&&
            full.marginRight==small.marginRight&&full.marginBottom==small.marginBottom&&
            full.collisionMode==small.collisionMode&&full.collisionMaskCount==small.collisionMaskCount&&
            full.frameCount==small.frameCount,"all source collision fields agree across views");
        Owner::Sprite visual;visual.spriteId=id;
        downwell::RunnerSpriteBounds::Box expected;
        require(downwell::RunnerSpriteBounds::fromSprite(full,{}, {},visual.scaleX,visual.scaleY,
            false,{}, {},true,round,expected),"full metadata bounds resolve");
        require(owner.refreshSprite({0,1},0x30,false,compact,visual,true,noTrig,round,true),
            "compact metadata drives owner publication");
        const auto* actual=owner.resolve({0,1});
        require(actual->bounds.left.bits==expected.left.bits&&actual->bounds.top.bits==expected.top.bits&&
            actual->bounds.right.bits==expected.right.bits&&actual->bounds.bottom.bits==expected.bottom.bits&&
            actual->flags==(0x8000u|(full.collisionMode==1?0x200u:0u)),
            "all sprite resources update cached bounds, precise and indexed flags");
    }
    Owner::Sprite visual;visual.spriteId=84;visual.maskId=597;
    require(owner.refreshSprite({0,1},0x30,false,compact,visual,true,noTrig,round,true)&&
        (owner.resolve({0,1})->flags&0x200),"explicit precise mask overrides rectangular artwork in owner");
    visual.maskId=9999;
    require(owner.refreshSprite({0,1},0x30,false,compact,visual,true,noTrig,round,true)&&
        owner.resolve({0,1})->flags==0x8000&&owner.resolve({0,1})->bounds.left.bits==0,
        "missing explicit mask collapses cache without discarding existing index membership");
    require(owner.retire({0,1}),"resource-backed stale bounds retire with root retry");
    require(owner.attach({0,2},0,{}),"reuse retired slot for rotated metadata adapter");
    visual.maskId=-1;visual.angle.bits=0x42b40000; // 90 degrees, explicit narrowed trig input.
    auto quarterTurn=[](auto angle,auto& cosine,auto& sine){
        require(angle.bits==0x42b40000,"trig receives the original binary32 degrees");
        cosine={};sine={0x3f800000};return true;
    };
    require(owner.refreshSprite({0,2},0x30,false,compact,visual,true,quarterTurn,round,false),
        "rotated refresh can update cache without publishing");
    const auto* rotated=owner.resolve({0,2});
    require(rotated->flags==0&&rotated->bounds.left.bits==0xbf800000&&
        rotated->bounds.right.bits==0x3f800000&&rotated->bounds.top.bits==0xc0000000&&
        rotated->bounds.bottom.bits==0x3f800000,"rotated adapter uses collision margins and update flag");
    require(!owner.refreshSprite({0,2},0x30,false,downwell::SpriteCollisionPack(nullptr,0),
        visual,true,quarterTurn,round,true)&&!owner.valid(),"invalid asset pack fails loudly instead of resolving no sprite");
    owner.clear();Owner::State special;special.special=true;
    require(owner.attach({0,3},0,special),"attach unsupported special state");
    visual.maskId=9999;visual.angle.bits=0x7f800000;
    require(owner.refreshSprite({0,3},0x30,false,compact,visual,true,noTrig,round,true),
        "missing resource resolves before angle or special-state processing");
    visual.maskId=84;visual.angle={};
    require(owner.refreshSprite({0,3},0x30,false,compact,visual,true,noTrig,round,true),
        "explicit ordinary mask can override special artwork state");
    visual.maskId=-1;
    require(!owner.refreshSprite({0,3},0x30,false,compact,visual,true,noTrig,round,true)&&!owner.valid(),
        "resolved special state cannot silently use ordinary sprite bounds");
    owner.clear();require(owner.attach({0,4},0,{}),"attach fractional bounds case");
    visual.angle.bits=0x80000000;visual.scaleX.bits=0xbf000000; // Negative zero angle, mirrored half scale.
    unsigned roundCalls=0;
    auto rejectRound=[&](auto,std::int32_t&){++roundCalls;return false;};
    require(owner.refreshSprite({0,4},0x30,false,compact,visual,false,noTrig,rejectRound,false)&&!roundCalls,
        "fractional mode and negative-zero angle bypass rounding and trigonometry");
    const auto fractional=owner.resolve({0,4})->bounds;
    require(!owner.refreshSprite({0,4},0x30,false,compact,visual,true,noTrig,rejectRound,true)&&roundCalls==1,
        "rounding failure propagates without publication");
    require(!owner.valid()&&owner.resolve({0,4})->bounds.left.bits==fractional.left.bits&&
        owner.resolve({0,4})->bounds.right.bits==fractional.right.bits,
        "failed bounds calculation does not replace the last cached rectangle");
    require(!downwell::SpriteCollisionPack(nullptr,0).valid(),"null compact table rejected");
    for(auto length:{std::size_t(0),std::size_t(23),compactBytes.size()-1})
        require(!downwell::SpriteCollisionPack(compactBytes.data(),length).valid(),"truncated compact table rejected");
    for(unsigned offset:{0u,4u,8u,16u,20u,48u}){
        auto bad=compactBytes;bad[offset]=255;
        require(!downwell::SpriteCollisionPack(bad.data(),bad.size()).valid(),"corrupt compact header/mode rejected");
    }
    downwell::PackedSpriteCollision selected;
    require(compact.collisionSprite(84,597,selected)&&selected.collisionMode==1,"compact mask override resolves");
    require(!compact.collisionSprite(84,9999,selected)&&selected.collisionMode==1,"compact invalid mask does not fall back");
    require(compact.collisionSprite(84,-1,selected)&&selected.marginLeft==10,"compact negative mask falls back");
    downwell::RunnerSpriteBounds::Box compactBounds;
    require(downwell::RunnerSpriteBounds::fromSprite(selected,{}, {},{0x3f800000},{0x3f800000},false,{}, {},true,
        [](downwell::RunnerImageValue v,std::int32_t& pixel){return v.promoted().roundPixel(pixel);},compactBounds)&&
        compactBounds.left.bits==0xc0000000&&compactBounds.right.bits==0x3f800000,
        "compact metadata feeds the same geometry adapter");
    require(pack.spriteCount() == 741 && pack.frameCount() == 3152 &&
                pack.pageItemCount() == 3161 && pack.texturePageCount() == 2,
            "generated sprite pack has audited totals");
    downwell::PackedSprite sprite;
    require(pack.sprite(84,sprite) && sprite.marginLeft==10 && sprite.marginTop==11 &&
        sprite.marginRight==13 && sprite.marginBottom==13 && sprite.collisionMode==0 && sprite.collisionMaskCount==1,
        "blood sprite retains original collision margins, not full artwork bounds");
    require(pack.sprite(597,sprite) && sprite.marginLeft==0 && sprite.marginTop==0 &&
        sprite.marginRight==47 && sprite.marginBottom==47 && sprite.collisionMode==1 && sprite.collisionMaskCount==1,
        "time-field precise collision metadata is retained");
    unsigned precise=0;
    for(unsigned id=0;id<pack.spriteCount();++id){
        require(pack.sprite(id,sprite),"every source sprite resolves");precise+=sprite.collisionMode==1;
    }
    require(precise==4,"all four precise-mask resources are represented");
    require(pack.collisionSprite(84,-1,sprite)&&sprite.spriteId==84,"negative mask selects artwork");
    downwell::RunnerSpriteBounds::Box bounds;
    require(downwell::RunnerSpriteBounds::fromSprite(sprite,{}, {},{0x3f800000},{0x3f800000},false,{}, {},true,
        [](downwell::RunnerImageValue value,std::int32_t& pixel){return value.promoted().roundPixel(pixel);},bounds),
        "decoded margins feed shared bounds arithmetic");
    require(bounds.left.bits==0xc0000000 && bounds.top.bits==0xbf800000 &&
        bounds.right.bits==0x3f800000 && bounds.bottom.bits==0x3f800000,"blood collision bounds use decoded origin and margins");
    require(pack.collisionSprite(84,597,sprite)&&sprite.spriteId==597,"explicit mask overrides artwork");
    require(!pack.collisionSprite(84,9999,sprite)&&sprite.spriteId==597,"missing explicit mask does not fall back or modify output");
    require(!pack.collisionSprite(-1,-1,sprite),"missing artwork and mask resolve no resource");
    auto corrupt=bytes;corrupt[4]=1;
    require(downwell::SpritePack(corrupt.data(),corrupt.size()).error()==downwell::SpritePackError::unsupportedVersion,
        "old record layout is rejected");
    corrupt=bytes;corrupt[downwell::SpritePack::headerSize+48]=2;
    require(downwell::SpritePack(corrupt.data(),corrupt.size()).error()==downwell::SpritePackError::badSprite,
        "unknown collision mode is rejected");
    // Include the Step-selected hit/hang/wake poses, not only each enemy's
    // default art. Both runtimes wrap animation with this shared table.
    for (int id : {153, 154, 157, 162, 163, 164, 166, 168, 170, 173,
                   177, 178, 186, 187, 188, 278})
        require(pack.sprite(id, sprite) &&
                    sprite.frameCount == downwell::enemySpriteFrameCount(id),
                "shared enemy animation lengths match serialized source resources");
    require(pack.sprite(733, sprite) && sprite.width == 8 && sprite.height == 8 &&
                sprite.frameCount == 12,
            "sprDitherFade metadata is decoded");
    downwell::PackedTexturePageItem frame;
    require(pack.spriteFrame(sprite, 0, frame) && frame.itemId == 2469 &&
                frame.texturePageId == 1 && frame.sourceX == 382 &&
                frame.sourceY == 1640 && frame.sourceWidth == 8 &&
                frame.sourceHeight == 8,
            "sprDitherFade frame zero resolves to its recovered atlas rectangle");
}
} // namespace

int main(int argc, char** argv) {
    testBasicValidation();
    require(argc==4,"sprite, collision-mask and object pack paths required");
    testGeneratedPack(argv[1],argv[2],argv[3]);
    std::cout << "sprite pack tests passed\n";
    return 0;
}

#include "core/RoomPack.h"
#include "core/PackedRoomLoader.h"
#include "core/PackedRoomLayerLoader.h"
#include "core/RunnerLayerBackground.h"
#include "core/RunnerLayerBackgrounds.h"
#include "core/RunnerLayerBackgroundDraw.h"
#include "core/RunnerLayerDraw.h"
#include "core/RunnerLayerLifetimes.h"
#include "core/StoredLayerLifecycle.h"
#include "core/StoredRoomLayers.h"
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

void appendU16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value));
    bytes.push_back(static_cast<std::uint8_t>(value >> 8));
}

void appendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8)
        bytes.push_back(static_cast<std::uint8_t>(value >> shift));
}

void appendI32(std::vector<std::uint8_t>& bytes, std::int32_t value) {
    appendU32(bytes, static_cast<std::uint32_t>(value));
}

void appendU64(std::vector<std::uint8_t>& bytes, std::uint64_t value) {
    appendU32(bytes, static_cast<std::uint32_t>(value));
    appendU32(bytes, static_cast<std::uint32_t>(value >> 32));
}

void appendF32(std::vector<std::uint8_t>& bytes, float value) {
    std::uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    appendU32(bytes, bits);
}

void appendInstance(std::vector<std::uint8_t>& bytes,
                    std::uint32_t instanceId,
                    std::int32_t objectId,
                    bool persistent) {
    appendU32(bytes, instanceId);
    appendI32(bytes, objectId);
    appendI32(bytes, -64);
    appendI32(bytes, 80);
    appendF32(bytes, 1.0F);
    appendF32(bytes, 1.0F);
    appendF32(bytes, 0.0F);
    appendF32(bytes, 1.0F);
    appendF32(bytes, 0.0F);
    appendI32(bytes, -1);
    appendI32(bytes, -1);
    appendU32(bytes, persistent ? 1 : 0);
}

std::vector<std::uint8_t> makePack(bool persistentController=true) {
    std::vector<std::uint8_t> bytes;
    bytes.insert(bytes.end(), {'D', 'W', 'R', 'M'});
    appendU16(bytes, downwell::RoomPack::formatVersion);
    appendU16(bytes, downwell::RoomPack::headerSize);
    appendU32(bytes, downwell::RoomPack::manifestSchema);
    appendU32(bytes, 1); // rooms
    appendU32(bytes, 2); // instances
    appendU32(bytes, 1); // layers
    appendU32(bytes, 2); // memberships
    appendU64(bytes, downwell::RoomPack::sourceHashPrefix);
    appendU32(bytes, 105186); // GEN8.LastObj

    appendI32(bytes, 7);
    appendI32(bytes, 1000);
    appendI32(bytes, 3000);
    appendI32(bytes, 0);
    appendI32(bytes, 648);
    appendU32(bytes, 0);
    appendU32(bytes, 0);
    appendU32(bytes, 2);
    appendU32(bytes, 0);
    appendU32(bytes, 1);

    appendInstance(bytes, 100065, 37, persistentController);
    appendInstance(bytes, 100066, 36, false);

    appendU32(bytes, 13);
    appendI32(bytes, -300000);
    appendU32(bytes, 1);
    appendU32(bytes, 0);
    appendU32(bytes, 2);

    appendU32(bytes, 2); // instance layer
    appendI32(bytes, -1); // no background sprite
    appendU32(bytes, 0);
    appendU32(bytes, 0);
    appendF32(bytes, 0);
    appendF32(bytes, 0);
    appendU32(bytes, 0);
    appendU32(bytes, 100065);
    appendU32(bytes, 100066);
    return bytes;
}

void testDecode() {
    const std::vector<std::uint8_t> bytes = makePack();
    const downwell::RoomPack pack(bytes.data(), bytes.size());
    require(pack.valid(), "valid pack is accepted");
    require(pack.roomCount() == 1 && pack.instanceCount() == 2,
            "header counts are decoded");
    require(pack.nextRuntimeInstanceId() == 105186,
            "runtime instance allocator boundary is decoded");

    downwell::PackedRoom room;
    require(pack.findRoom(7, room), "room is found by resource ID");
    require(room.width == 1000 && room.height == 3000 && room.creationCodeId == 648,
            "room fields are decoded");

    downwell::PackedRoomInstance instance;
    require(pack.roomInstance(room, 0, instance), "first room instance is decoded");
    require(instance.instanceId == 100065 && instance.objectId == 37 &&
                instance.x == -64 && instance.y == 80 && instance.persistent(),
            "instance identity, transform and flags are decoded");
    require(instance.scaleX == 1.0F && instance.rotation == 0.0F,
            "little-endian floats are decoded");

    downwell::PackedRoomLayer layer;
    require(pack.roomLayer(room, 0, layer), "room layer is decoded");
    require(layer.layerId == 13 && layer.depth == -300000 && layer.visible()&&
                layer.type==2&&layer.backgroundSpriteId==-1,
            "layer fields are decoded");
    std::uint32_t member = 0;
    require(pack.membership(1, member) && member == 100066,
            "layer membership is decoded");
}

void testBackgroundDecode(){
    auto bytes=makePack();
    const auto layerOffset=downwell::RoomPack::headerSize+downwell::RoomPack::roomRecordSize+
        2*downwell::RoomPack::instanceRecordSize;
    auto word=[&](unsigned at,unsigned value){for(unsigned i=0;i<4;++i)bytes[layerOffset+at+i]=std::uint8_t(value>>(i*8));};
    word(16,0);word(20,1);word(24,740);word(28,0xff123456u);word(32,31);
    word(36,0x3fc00000u);word(40,0x41700000u);word(44,1);
    const downwell::RoomPack pack(bytes.data(),bytes.size());downwell::PackedRoomLayer layer;
    require(pack.valid()&&pack.layer(0,layer)&&layer.type==1&&layer.backgroundSpriteId==740&&
        layer.backgroundColor==0xff123456u&&layer.backgroundFlags==31&&layer.backgroundFirstFrame==1.5f&&
        layer.backgroundAnimationSpeed==15.f&&layer.backgroundAnimationSpeedType==1,"background payload round trip");
    word(24,0xffffffffu);
    require(downwell::RoomPack(bytes.data(),bytes.size()).layer(0,layer)&&layer.backgroundSpriteId==-1,"sprite-less background sentinel");
    word(16,1);
    require(downwell::RoomPack(bytes.data(),bytes.size()).error()==downwell::RoomPackError::badLayerType,"background cannot own instance membership");
    word(16,0);word(20,99);
    require(downwell::RoomPack(bytes.data(),bytes.size()).error()==downwell::RoomPackError::badLayerType,"unknown serialized layer type fails");
    bytes=makePack();bytes[4]=2;
    require(downwell::RoomPack(bytes.data(),bytes.size()).error()==downwell::RoomPackError::unsupportedVersion,"old layer payload format is rejected");
}

void testBackgroundConstruction(){
    for(unsigned alpha=0;alpha<256;++alpha)for(unsigned flags=0;flags<32;++flags){
        downwell::PackedRoomLayer source{};source.type=1;
        source.backgroundSpriteId=alpha&1?740:-1;
        source.backgroundFlags=flags;source.backgroundColor=(alpha<<24)|0x123456u;
        source.backgroundFirstFrame=1.5f;source.backgroundAnimationSpeed=15.f;
        source.backgroundAnimationSpeedType=1;
        downwell::RunnerLayerBackground background;
        require(downwell::RunnerLayerBackground::create(source,background),"background construction");
        const float expected=float(alpha)/255.f;unsigned bits;std::memcpy(&bits,&expected,4);
        require(background.alpha.bits==bits&&background.color==0x123456u&&background.flags==flags&&
            background.spriteId==source.backgroundSpriteId&&background.firstFrame.bits==0x3fc00000u&&
            background.speed.bits==0x41700000u&&background.speedType==1&&
            background.scaleX.bits==0x3f800000u&&background.scaleY.bits==0x3f800000u&&!background.initialized,
            "source color split, float alpha, flags, animation and unit scale");
        background.initialize();background.initialize();
        require(background.initialized&&background.spriteId==source.backgroundSpriteId&&background.alpha.bits==bits,
            "background initialization is idempotent and retains sprite-less payload");
        unsigned color=0;
        require(downwell::RunnerLayerBackgroundDraw::solidColor(background,color)&&color==source.backgroundColor,
            "all serialized alpha bytes survive rounded quad color reconstruction");
        source.type=2;
        require(!downwell::RunnerLayerBackground::create(source,background)&&background.initialized,"wrong type leaves output unchanged");
        source.type=1;source.backgroundFlags=32;
        require(!downwell::RunnerLayerBackground::create(source,background)&&background.initialized,"unknown flags leave output unchanged");
    }
}

void testBackgroundDraw(){
    downwell::RunnerLayerBackground background;background.flags=1;background.color=0x123456;
    for(float alpha:{-1.f,-0.5f,0.f,0.001f,0.5f,0.999f,1.f,1.5f}){
        std::memcpy(&background.alpha.bits,&alpha,4);
        unsigned color=0;int expected=int(alpha*255.f);if(expected<0)expected=0;if(expected>255)expected=255;
        require(downwell::RunnerLayerBackgroundDraw::solidColor(background,color)&&
            color==(0x123456u|(unsigned(expected)<<24)),"quad alpha truncates after float multiplication then clamps");
    }
    background.alpha.bits=0x3f800000u;
    unsigned lookups=0,fills=0,sprites=0;
    auto lookup=[&](int id,bool& exists){++lookups;exists=id==740;return true;};
    auto fill=[&](unsigned color){++fills;return color==0xff123456u;};
    auto sprite=[&](const auto& b){++sprites;return b.spriteId==740;};
    require(downwell::RunnerLayerBackgroundDraw::visit(background,lookup,fill,sprite)&&fills==1&&sprites==0,
        "sprite-less background draws a colored viewport quad");
    background.spriteId=999;
    require(downwell::RunnerLayerBackgroundDraw::visit(background,lookup,fill,sprite)&&fills==2,"missing positive sprite also uses solid path");
    background.spriteId=740;
    require(downwell::RunnerLayerBackgroundDraw::visit(background,lookup,fill,sprite)&&sprites==1,"valid sprite dispatch");
    background.flags=0;
    require(downwell::RunnerLayerBackgroundDraw::visit(background,lookup,fill,sprite)&&lookups==3,"hidden background skips lookup and draw");
    background.flags=1;
    require(!downwell::RunnerLayerBackgroundDraw::visit(background,[](int,bool&){return false;},fill,sprite)&&
        fills==2&&sprites==1,"lookup failure suppresses draw");
    background.spriteId=-1;background.alpha.bits=0;
    require(!downwell::RunnerLayerBackgroundDraw::visit(background,lookup,[&](unsigned c){++fills;return c!=0x123456u;},sprite)&&fills==3,
        "zero-alpha quad still dispatches and propagates renderer failure");
    background.alpha.bits=0x7fc00000u;unsigned color=42;
    require(!downwell::RunnerLayerBackgroundDraw::solidColor(background,color)&&color==42,"unsupported alpha preserves output");
}

void testBackgroundOwner(){
    using Registry=downwell::RunnerLayerInstances<2,4>;
    Registry registry;Registry::Layers layers;
    downwell::RunnerLayerBackgrounds<2,4,1> backgrounds;
    unsigned layer=2;
    require(layers.insertAfterEquals(17,0,false,layer)&&registry.reserve(layers,layer,100)&&
        registry.reserve(layers,layer,101),"mixed layer seed");
    downwell::RunnerDepthService::Instance a,b;
    require(registry.attach(layers,layer,100,{0,1},a,false)&&
        registry.attach(layers,layer,101,{1,1},b,true),"mixed instance bindings");
    downwell::PackedRoomLayer source{};source.type=1;source.layerId=17;source.backgroundSpriteId=-1;
    source.backgroundColor=0xff000000u;source.backgroundFlags=1;
    auto active=[&](unsigned e){const auto* r=registry.get(e);return r&&r->sourceId==100;};
    require(backgrounds.create(layers,layer,source,active),"background inserts after active instance prefix");
    std::vector<unsigned> order;
    require(layers.visitElements(layer,[&](unsigned e){order.push_back(e);return true;}),"mixed element traversal");
    require(order.size()==3&&order[0]==registry.find(100)&&order[2]==registry.find(101)&&
        backgrounds.get(order[1])&&!registry.get(order[1]),"background and instance slots never alias");
    const unsigned element=order[1];
    auto initialize=[&](auto other){return registry.initializeRoom(layers,
        [](unsigned,downwell::InstanceHandle&,downwell::RunnerDepthService::Instance*&){return false;},
        [](auto){return true;},[&](unsigned e){const auto* r=registry.get(e);return r&&r->sourceId==101;},
        [](int,unsigned&){return false;},other);};
    require(initialize([&](unsigned e){return backgrounds.initialize(e);})&&backgrounds.get(element)->background.initialized,
        "mixed initialization dispatches background owner");
    require(!backgrounds.create(layers,layer,source,active)&&layers.elementCount(layer)==3,"background capacity failure preserves links");
    require(!backgrounds.retire(layers,registry.find(100))&&registry.get(registry.find(100)),"background owner cannot retire an instance");
    require(backgrounds.retire(layers,element)&&!backgrounds.get(element)&&layers.elementCount(layer)==2,"background retirement frees only its slot");
    require(backgrounds.create(layers,layer,source,active)&&backgrounds.get(element)&&
        !backgrounds.get(element)->background.initialized,"reused background slot resets initialization state");
    downwell::RunnerLayerBackgrounds<2,4,0> empty;
    require(!empty.create(layers,layer,source,active),"zero background capacity fails");
    Registry failedRegistry;Registry::Layers failedLayers;
    downwell::RunnerLayerBackgrounds<2,4,1> failedBackgrounds;
    unsigned failedLayer=2;
    require(failedLayers.insertAfterEquals(17,0,false,failedLayer)&&
        failedBackgrounds.create(failedLayers,failedLayer,source,[](unsigned){return false;}),"unknown initialization seed");
    require(!failedRegistry.initializeRoom(failedLayers,
        [](unsigned,downwell::InstanceHandle&,downwell::RunnerDepthService::Instance*&){return false;},
        [](auto){std::abort();return false;},[](unsigned){std::abort();return false;},
        [](int,unsigned&){return false;})&&!failedBackgrounds.get(0)->background.initialized,
        "missing non-instance initializer stops before global pass and normalization");
}

void testValidation() {
    std::vector<std::uint8_t> bytes = makePack();
    bytes[0] = 'X';
    require(downwell::RoomPack(bytes.data(), bytes.size()).error() ==
                downwell::RoomPackError::badMagic,
            "bad magic is rejected");

    bytes = makePack();
    bytes.pop_back();
    require(downwell::RoomPack(bytes.data(), bytes.size()).error() ==
                downwell::RoomPackError::sizeOverflow,
            "truncated pack is rejected");

    bytes = makePack();
    // Room first-instance index at header + 24.
    bytes[downwell::RoomPack::headerSize + 24] = 3;
    require(downwell::RoomPack(bytes.data(), bytes.size()).error() ==
                downwell::RoomPackError::badRoomRange,
            "out-of-range room slice is rejected");

    bytes = makePack();
    // First instance ID is 100065; move the runtime boundary down to it.
    const std::size_t boundaryOffset = 36;
    bytes[boundaryOffset + 0] = 0xe1;
    bytes[boundaryOffset + 1] = 0x86;
    bytes[boundaryOffset + 2] = 0x01;
    bytes[boundaryOffset + 3] = 0x00;
    require(downwell::RoomPack(bytes.data(), bytes.size()).error() ==
                downwell::RoomPackError::badInstanceIdBoundary,
            "serialized IDs crossing the runtime boundary are rejected");
}

void testPackedRoomLoader() {
    const std::vector<std::uint8_t> bytes = makePack();
    const downwell::RoomPack pack(bytes.data(), bytes.size());
    downwell::InstanceStore<3> instances;
    for(unsigned failAt=1;failAt<=12;++failAt){
        downwell::InstanceStore<3> checked;
        unsigned calls=0;
        const auto result=downwell::PackedRoomLoader<3>::enterChecked(checked,pack,7,true,
            [&](auto&,auto){return ++calls!=failAt;},
            [&](auto){return ++calls!=failAt;},
            [&](auto&){return ++calls!=failAt;});
        require(result==downwell::PackedRoomLoadResult::callbackFailure&&calls==failAt,
            "packed loader preserves checked callback failure without later dispatch");
    }
    const auto noInstanceEvents = [](downwell::InstanceRecord&,
                                     downwell::RoomInstancePhase) {};
    const auto noRoomCode = [](std::int32_t) {};
    require(downwell::PackedRoomLoader<3>::enter(
                instances, pack, 7, true, noInstanceEvents, noRoomCode) ==
                downwell::PackedRoomLoadResult::success,
            "packed room loads into native instance storage");
    require(instances.size() == 2, "all packed instances are loaded");
    require(instances.at(0).instanceId == 100065 &&
                instances.at(0).objectId == 37 && instances.at(0).persistent,
            "packed creation order and persistence are retained");
    require(instances.at(0).x == -64 && instances.at(0).y == 80 &&
                instances.at(0).scaleX == 1.0F,
            "packed transform is retained");
    require(instances.at(0).layerId == 13 && instances.at(0).depth == -300000,
            "layer membership resolves into instance state");

    std::vector<std::uint8_t> objectBytes={'D','W','O','B'};
    appendU16(objectBytes,downwell::ObjectPack::formatVersion);
    appendU16(objectBytes,downwell::ObjectPack::headerSize);
    appendU32(objectBytes,downwell::ObjectPack::manifestSchema);
    appendU32(objectBytes,38);appendU32(objectBytes,0);
    appendU64(objectBytes,downwell::ObjectPack::sourceHashPrefix);
    appendU32(objectBytes,0);appendU64(objectBytes,0);
    for(int id=0;id<38;++id){
        appendI32(objectBytes,id);appendI32(objectBytes,12+id);appendI32(objectBytes,-1);
        appendI32(objectBytes,90+id);appendI32(objectBytes,0);appendU32(objectBytes,0);
        appendU32(objectBytes,0);appendU32(objectBytes,0);appendI32(objectBytes,0);
    }
    const downwell::ObjectPack objects(objectBytes.data(),objectBytes.size());
    require(objects.valid(),"synthetic object defaults validate");
    downwell::InstanceStore<3> withDefaults;
    unsigned initialized=0;
    auto phases=[&](downwell::InstanceRecord& instance,downwell::RoomInstancePhase phase){
        if(phase==downwell::RoomInstancePhase::preCreate){
            require(instance.spriteId==12+instance.objectId&&instance.maskId==90+instance.objectId,
                "room object sprite and mask are bound before PreCreate");
            instance.maskId=777;++initialized;
        }else if(phase==downwell::RoomInstancePhase::create)
            require(instance.maskId==777,"Create retains mask mutations from PreCreate");
    };
    auto noDeferred=[](auto&){};
    require(downwell::PackedRoomLoader<3>::enter(withDefaults,pack,7,true,phases,noRoomCode,
        noDeferred,&objects)==downwell::PackedRoomLoadResult::success&&initialized==2,
        "packed loader publishes object defaults once per birth");
    const downwell::ObjectPack invalidObjects(nullptr,0);
    require(downwell::PackedRoomLoader<3>::enter(withDefaults,pack,7,true,phases,noRoomCode,
        noDeferred,&invalidObjects)==downwell::PackedRoomLoadResult::invalidObject&&
        initialized==2&&withDefaults.size()==2&&withDefaults.at(0).maskId==777,
        "invalid object metadata is rejected before any room transition or callback");
    struct RejectBirth : downwell::NoInstanceLifecycle {
        bool birth(downwell::InstanceHandle,const downwell::InstanceRecord&)const{return false;}
    } reject;
    downwell::InstanceStore<3> failedBirth;
    initialized=0;
    require(downwell::PackedRoomLoader<3>::enter(failedBirth,pack,7,true,phases,noRoomCode,
        noDeferred,&objects,reject)==downwell::PackedRoomLoadResult::lifecycleFailure&&
        initialized==0&&failedBirth.size()==1,
        "packed loader propagates registration failure before any PreCreate callback");
    struct RejectRetirement : downwell::NoInstanceLifecycle {
        bool retire(downwell::InstanceHandle,const downwell::InstanceRecord&)const{return false;}
    } rejectRetirement;
    downwell::InstanceStore<3> failedRetirement;
    require(failedRetirement.create(999,1,6,false)==downwell::CreateInstanceResult::success,"seed retirement failure");
    require(downwell::PackedRoomLoader<3>::enter(failedRetirement,pack,7,false,phases,noRoomCode,
        noDeferred,&objects,rejectRetirement)==downwell::PackedRoomLoadResult::lifecycleFailure&&
        initialized==0&&failedRetirement.size()==1&&failedRetirement.find(999)->roomId==6,
        "failed retirement propagates without creating new room instances");

    std::vector<std::uint8_t> damaged = bytes;
    // Final membership is changed from instance 100066 to 100065, making one
    // duplicate membership and leaving the second instance without a layer.
    const std::size_t finalMembership = damaged.size() - 4;
    damaged[finalMembership + 0] = 0xe1;
    damaged[finalMembership + 1] = 0x86;
    damaged[finalMembership + 2] = 0x01;
    damaged[finalMembership + 3] = 0x00;
    const downwell::RoomPack damagedPack(damaged.data(), damaged.size());
    downwell::InstanceStore<3> empty;
    require(downwell::PackedRoomLoader<3>::enter(
                empty, damagedPack, 7, true, noInstanceEvents, noRoomCode) ==
                downwell::PackedRoomLoadResult::invalidLayerMembership,
            "duplicate or missing layer membership is rejected");
    require(empty.empty(), "invalid packed room leaves instance store untouched");
}

void testGeneratedPack(const char* path) {
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "generated room pack opens");
    const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                          std::istreambuf_iterator<char>());
    const downwell::RoomPack pack(bytes.data(), bytes.size());
    require(pack.valid(), "generated room pack passes native validation");
    require(pack.roomCount() == 25 && pack.instanceCount() == 5186 &&
                pack.layerCount() == 132 && pack.membershipCount() == 5186,
            "generated room pack has audited totals");
    require(pack.nextRuntimeInstanceId() == 105186,
            "generated pack carries authoritative GEN8.LastObj");
    unsigned instanceLayers=0,backgroundLayers=0,spriteBackgrounds=0;
    for(unsigned i=0;i<pack.layerCount();++i){
        downwell::PackedRoomLayer layer;require(pack.layer(i,layer),"generated layer decode");
        if(layer.type==2){++instanceLayers;continue;}
        ++backgroundLayers;
        downwell::RunnerLayerBackground background;
        require(downwell::RunnerLayerBackground::create(layer,background)&&background.alpha.bits==0x3f800000u,
            "every recovered background constructs with original opaque alpha");
        require(layer.backgroundFlags==1&&layer.backgroundFirstFrame==0&&layer.backgroundAnimationSpeedType==0,
            "original background flags and animation start");
        if(layer.backgroundSpriteId==740){
            ++spriteBackgrounds;
            require(layer.backgroundColor==0xffffffffu&&layer.backgroundAnimationSpeed==30,"Steam sprite background metadata");
        }else require(layer.backgroundSpriteId==-1&&layer.backgroundColor==0xff000000u&&
            layer.backgroundAnimationSpeed==15,"sprite-less black background metadata");
    }
    require(instanceLayers==108&&backgroundLayers==24&&spriteBackgrounds==6,"all original layer payloads retained");

    downwell::PackedRoom room;
    require(pack.room(0, room) && room.roomId == 7 && room.instanceCount == 2,
            "first packed room is authoritative rmInit");
    require(pack.room(13, room) && room.roomId == 20 &&
                room.instanceCount == 192 && room.layerCount == 10,
            "packed rmMenu has audited counts");
    downwell::PackedRoomInstance player;
    require(pack.roomInstance(room, 0, player) &&
                player.instanceId == 104218 && player.objectId == 35,
            "rmMenu begins with recovered player instance");
    downwell::RunnerLayerIds sourceIds;unsigned expectedMaximum=0;
    downwell::RunnerLayerIds catalogIds;
    require(downwell::PackedRoomLayerLoader::loadCatalogIds(pack,catalogIds)&&catalogIds.loadedMaximum==131&&
        catalogIds.dynamicMaximum==0,"catalog seed matches original all-room load");
    std::int32_t nextId=-1;
    require(catalogIds.allocate(nextId)&&nextId==132&&
        downwell::PackedRoomLayerLoader::loadCatalogIds(pack,catalogIds)&&catalogIds.allocate(nextId)&&nextId==133,
        "catalog refresh cannot rewind dynamic allocation");
    for(unsigned roomIndex=0;roomIndex<pack.roomCount();++roomIndex){
        require(pack.room(roomIndex,room),"generated layer room");
        downwell::RunnerLayers<16,1200> layers;
        downwell::RunnerLayerInstances<16,1200> registry;
        downwell::RunnerLayerBackgrounds<16,1200,2> backgrounds;
        unsigned published=0;
        require(downwell::PackedRoomLayerLoader::load(pack,room.roomId,layers,registry,
            [&](unsigned slot,const auto& layer){
                require(sourceIds.loadedMaximum==int(expectedMaximum),"ID floor updates after serialized layer completion");
                if(layer.layerId>expectedMaximum)expectedMaximum=layer.layerId;
                ++published;
                return layer.type!=1||backgrounds.create(layers,slot,layer,[](unsigned){return false;});
            },&sourceIds),"all original room memberships load");
        require(published==room.layerCount,"all layer metadata published");
        require(sourceIds.loadedMaximum==int(expectedMaximum),"process maximum preserved across loaded rooms");
        require(registry.initializeRoom(layers,
            [](unsigned,downwell::InstanceHandle&,downwell::RunnerDepthService::Instance*&){return true;},
            [](auto){return true;},[](unsigned){return false;},[](int,unsigned&){return false;},
            [&](unsigned e){return backgrounds.initialize(e);}),"all original mixed layer elements initialize");
        unsigned backgroundDraws=0;
        require(downwell::RunnerLayerDraw::visit(layers,[](unsigned,const auto&){return true;},
            [](unsigned,const auto&){return true;},[&](unsigned e,auto& state){
                state.type=backgrounds.get(e)?1:2;return true;
            },[&](unsigned,unsigned e,auto action){
                const auto* entry=backgrounds.get(e);
                if(!entry||action!=downwell::RunnerLayerDraw::Action::other)return false;
                ++backgroundDraws;
                return downwell::RunnerLayerBackgroundDraw::visit(entry->background,
                    [](int id,bool& exists){exists=id==740;return true;},
                    [](unsigned color){return color==0xff000000u;},
                    [](const auto& bg){return bg.spriteId==740;});
            },[](unsigned,const auto&){return true;}),"original backgrounds dispatch through mixed layer Draw");
        unsigned expectedBackgrounds=0;
        for(unsigned l=0;l<room.layerCount;++l){downwell::PackedRoomLayer p;require(pack.roomLayer(room,l,p),"count room backgrounds");expectedBackgrounds+=p.type==1;}
        require(backgroundDraws==expectedBackgrounds,"every original background appears in layer traversal");
        downwell::InstanceStore<1> emptyStore;
        downwell::RunnerLayerIds mixedIds;
        downwell::StoredRoomLayers<1,16,1200,2> mixed(mixedIds);
        require(mixed.prepare(pack,room.roomId,emptyStore),"prepare original room in production layer owner");
        unsigned mixedBackgrounds=0;
        require(mixed.visitDraw([](auto,unsigned,auto&){return false;},
            [](auto,unsigned,int,auto){return false;},[&](const auto& background,int){
                ++mixedBackgrounds;return background.initialized;
            })&&mixedBackgrounds==expectedBackgrounds,
            "production mixed Draw visits original backgrounds and skips unbound reservations");
        for(unsigned l=0;l<room.layerCount;++l){
            downwell::PackedRoomLayer packed;require(pack.roomLayer(room,l,packed),"generated layer");
            const auto slot=layers.find(int(packed.layerId));unsigned remaining=packed.membershipCount;
            require(layers.get(slot)&&!layers.get(slot)->dynamic&&layers.get(slot)->depth==packed.depth,"fixed layer identity/depth");
            require(layers.visitElements(slot,[&](unsigned e){
                if(packed.type==1){
                    const auto* background=backgrounds.get(e);
                    return background&&background->background.initialized&&!registry.get(e)&&
                        background->background.spriteId==packed.backgroundSpriteId&&layers.elementCount(slot)==1;
                }
                if(!remaining)return false;
                std::uint32_t id;
                return pack.membership(packed.firstMembership+--remaining,id)&&registry.get(e)&&
                    registry.get(e)->sourceId==id&&!registry.get(e)->instance.generation;
            })&&!remaining,"all serialized instance elements reversed, initially unbound");
        }
    }
}

void testLayerStartup(){
    const auto bytes=makePack();const downwell::RoomPack pack(bytes.data(),bytes.size());
    using Layers=downwell::RunnerLayers<2,3>;using Registry=downwell::RunnerLayerInstances<2,3>;
    Layers layers;Registry registry;
    unsigned metadataCalls=0;
    struct Binding {
        Layers& layers;Registry& registry;
        bool birth(downwell::InstanceHandle h,const downwell::InstanceRecord& r){
            downwell::RunnerDepthService::Instance state;
            return registry.attach(layers,layers.find(int(r.layerId)),r.instanceId,h,state,!r.active)&&state.layerId==int(r.layerId);
        }
        bool retire(downwell::InstanceHandle h,const downwell::InstanceRecord& r){return registry.retire(layers,r.instanceId,h);}
    } binding{layers,registry};
    downwell::InstanceStore<3> store;unsigned events=0;
    require(store.create(513,10,6,true)==downwell::CreateInstanceResult::success,"persistent source instance");
    store.find(513)->active=false;
    const auto persistentHandle=store.handleAt(0);
    downwell::RunnerDepthService::Instance persistentState;
    persistentState.layerId=13; // Saved fixed-layer identity wins over depth.
    persistentState.depth.bits=0x41280000u;
    downwell::RunnerStepSnapshot<3> globalOrder;
    std::vector<unsigned> initializationLookups;
    require(downwell::PackedRoomLoader<3>::enterChecked(store,pack,7,true,[&](auto& r,auto){
        ++events;const unsigned element=registry.find(r.instanceId);
        return element!=Registry::End&&store.resolve(registry.get(element)->instance)==&r;
    },[](int){return true;},[](auto&){return true;},nullptr,binding,[&](auto& survivors,int room){
        require(survivors.size()==1&&survivors.resolve(persistentHandle)&&events==0&&room==7,"layer preparation before first birth");
        if(!downwell::PackedRoomLayerLoader::load(pack,room,layers,registry,[&](unsigned,const auto& l){
            ++metadataCalls;return l.layerId==13&&l.visible();
        })||!survivors.captureGlobalInstanceOrder(globalOrder))return false;
        return registry.initializeRoom(layers,
            [&](unsigned id,downwell::InstanceHandle& h,downwell::RunnerDepthService::Instance*& state){
                initializationLookups.push_back(id);
                if(id==513){h=persistentHandle;state=&persistentState;}
                return true;
            },[&](auto visit){
                for(unsigned i=0;i<globalOrder.size();++i){
                    const auto h=globalOrder.handle(i);const auto* r=survivors.resolve(h);
                    if(!r||!visit(r->instanceId,h,persistentState,!r->active))return false;
                }
                return true;
            },[&](unsigned e){
                const auto* entry=registry.get(e);
                if(!entry||!entry->instance.generation)return false;
                const auto* r=survivors.resolve(entry->instance);
                require(r!=nullptr,"normalization lifetime resolves");return !r->active;
            },[](int,unsigned&){return false;});
    })==downwell::PackedRoomLoadResult::success&&events&&metadataCalls==1,
        "real packed room creation binds layers before first instance event");
    std::vector<unsigned> ids;
    layers.visitElements(layers.find(13),[&](unsigned e){ids.push_back(registry.get(e)->sourceId);return true;});
    require(initializationLookups==std::vector<unsigned>({100066,100065}),"serialized initialization precedes new births in element order");
    require(ids==std::vector<unsigned>({100066,100065,513})&&persistentState.attached&&persistentState.layerId==13,
        "packed startup binds survivor then normalizes it behind reserved births");
    require(!downwell::PackedRoomLayerLoader::load(pack,7,layers,registry,[](unsigned,const auto&){return true;}),"reject loading over live layer owner");
    Layers rejected;Registry rejectedRegistry;
    require(!downwell::PackedRoomLayerLoader::load(pack,7,rejected,rejectedRegistry,[](unsigned,const auto&){return false;})&&
        rejected.find(13)!=Layers::End&&rejectedRegistry.find(100065)==Registry::End,"metadata failure stops before reservations without rollback");
    auto corrupt=bytes;const auto offset=corrupt.size()-4;
    for(unsigned i=0;i<4;++i)corrupt[offset+i]=static_cast<unsigned char>(100065u>>(8*i));
    const downwell::RoomPack bad(corrupt.data(),corrupt.size());Layers badLayers;Registry badRegistry;
    require(!downwell::PackedRoomLayerLoader::load(bad,7,badLayers,badRegistry,[](unsigned,const auto&){return true;}),"duplicate membership rejected");
}

void testLayerRoomReentry(){
    using namespace downwell;
    const auto bytes=makePack(false);const RoomPack pack(bytes.data(),bytes.size());
    using Registry=RunnerLayerInstances<4,4>;using Layers=Registry::Layers;
    using Owner=RunnerLayerLifetimes<4,4,3>;
    Layers layers;Registry registry;RunnerLayerIds ids;
    require(PackedRoomLayerLoader::loadCatalogIds(pack,ids),"reentry catalog identities");
    Owner owner(layers,registry,ids);InstanceStore<3> store;
    require(store.create(513,10,6,true)==CreateInstanceResult::success,"reentry persistent instance");
    const auto survivor=store.handleAt(0);
    require(owner.adopt({survivor,513,{{0x41280000u},13,false,false},false}),"adopt preexisting survivor");
    StoredLayerLifecycle<4,4,3> layerLifecycle{owner};
    InstanceLifecycleBinding binding;
    require(binding.observeLayers(layerLifecycle)&&!binding.hasOwner(),
        "layer observation does not claim collision query ownership");
    RunnerStepSnapshot<3> globals;
    unsigned preparations=0,events=0;
    auto enter=[&](bool first){
        return PackedRoomLoader<3>::enterChecked(store,pack,7,first,[&](auto& r,auto){
            ++events;const auto e=registry.find(r.instanceId);
            return e!=Registry::End&&store.resolve(registry.get(e)->instance)==&r&&owner.resolve(registry.get(e)->instance);
        },[](int){return true;},[](auto&){return true;},nullptr,binding,[&](auto& survivors,int room){
            ++preparations;
            require(survivors.size()==1&&survivors.resolve(survivor)&&owner.resolve(survivor),"same survivor at preparation");
            if(preparations>1){layers=Layers{};registry=Registry{};}
            if(!PackedRoomLayerLoader::load(pack,room,layers,registry,[](unsigned,const auto&){return true;})||
                !survivors.captureGlobalInstanceOrder(globals))return false;
            return owner.initializeRoom([&](auto visit){
                for(unsigned i=0;i<globals.size();++i)if(!visit(globals.handle(i)))return false;
                return true;
            },[](unsigned){return false;});
        })==PackedRoomLoadResult::success;
    };
    require(enter(true)&&events&&preparations==1,"first packed room entry uses lifetime owner");
    const auto oldPlaced=registry.get(registry.find(100065))->instance;
    require(owner.writeDepth(survivor,MotionBinary64::fromInteger(-60000))&&
        owner.writeDepth(oldPlaced,MotionBinary64::fromInteger(5))&&owner.pendingDepths()==2,
        "pending writes before room exit");
    require(owner.uninitializeRoom([](unsigned){return false;})&&enter(false),"packed reentry retires unbound lifetimes");
    require(preparations==2&&owner.pendingDepths()==1&&!store.resolve(oldPlaced)&&!owner.resolve(oldPlaced)&&
        store.resolve(survivor)&&owner.resolve(survivor),"retirement cancels only departing lifetime request");
    require(owner.serviceDepths()&&owner.pendingDepths()==0,"surviving pending request completes after fallback");
    require(layers.get(layers.find(owner.resolve(survivor)->depth.layerId))->depth==-60000,
        "room fallback preserves surviving published depth across owner storage replacement");
    const auto newPlaced=registry.get(registry.find(100065))->instance;
    require(owner.resolve(newPlaced)&&!(newPlaced.slot==oldPlaced.slot&&newPlaced.generation==oldPlaced.generation),
        "serialized rebirth uses a new generation");
}

void testLayerLifecycleBinding(){
    using namespace downwell;
    using Registry=RunnerLayerInstances<2,3>;Registry registry;Registry::Layers layers;RunnerLayerIds ids;
    RunnerLayerLifetimes<2,3,2> owner(layers,registry,ids);
    StoredLayerLifecycle<2,3,2> observer{owner};
    struct Primary {
        RunnerLayerLifetimes<2,3,2>& owner;
        bool good=true;unsigned calls=0;
        bool birth(InstanceHandle h,const InstanceRecord&){++calls;require(!owner.resolve(h),"primary birth precedes layer observer");return good;}
        bool retire(InstanceHandle h,const InstanceRecord&){++calls;require(owner.resolve(h),"primary retirement precedes layer observer");return good;}
        bool activation(InstanceHandle h,const InstanceRecord&,bool active){
            ++calls;require(owner.resolve(h)->inactive==active,"primary activation sees prior layer flag");return good;
        }
        bool membership(InstanceHandle h,const InstanceRecord&,bool registered){
            ++calls;require(owner.resolve(h)->inactive==!registered,"membership sees already-published flag");return good;
        }
    } primary{owner};
    auto binding=InstanceLifecycleBinding::bind(primary);
    require(binding.observeLayers(observer)&&!binding.observeLayers(observer)&&binding.hasOwner(),
        "observer installation preserves primary ownership and rejects replacement");
    InstanceRecord record;record.instanceId=100;record.active=true;
    const InstanceHandle h{0,1};
    require(!binding.birthAt(h,record,{},{})&&!primary.calls&&!owner.resolve(h),
        "layer observer cannot supply missing positioned collision birth");
    primary.good=false;
    require(!binding.birth(h,record)&&!owner.resolve(h),"primary failure suppresses observer");
    primary.good=true;
    require(binding.birth(h,record)&&owner.resolve(h),"composed birth publishes layer lifetime");
    const auto copied=binding;
    const auto originalLayer=owner.resolve(h)->depth.layerId;
    require(copied.publishedDepth(h,record,RunnerImageValue{0x41280000u}.promoted())&&
        owner.resolve(h)->depth.depth.bits==0x41280000u&&owner.pendingDepths()==1&&
        owner.resolve(h)->depth.layerId==originalLayer,"borrowed depth setter publishes fractional value without immediate migration");
    InstanceRecord wrong=record;wrong.instanceId=101;
    require(!copied.publishedDepth(h,wrong,{})&&owner.pendingDepths()==1&&owner.succeeded(),
        "depth setter rejects mismatched source identity without publishing");
    require(owner.serviceDepths()&&owner.pendingDepths()==0&&
        layers.get(layers.find(owner.resolve(h)->depth.layerId))->depth==10&&
        owner.resolve(h)->depth.depth.bits==0x41280000u,"singleton layer moves but published fractional depth survives");
    require(copied.activation(h,record,false)&&owner.resolve(h)->inactive&&copied.membership(h,record,false),
        "copied borrowed binding retains separate activation and reconciliation");
    require(copied.activation(h,record,true)&&!owner.resolve(h)->inactive&&copied.membership(h,record,true),
        "reactivation reaches shared layer owner");
    primary.good=false;
    require(!binding.retire(h,record)&&owner.resolve(h),"primary retirement failure retains layer lifetime");
    primary.good=true;
    require(binding.retire(h,record)&&!owner.resolve(h),"successful retirement reaches both owners");
    require(!copied.publishedDepth(h,record,{}),"retired lifetime cannot receive a depth write");
    // Primary publication is intentionally not rolled back if the observer
    // rejects capacity. The calling world must stop after false.
    const unsigned before=primary.calls;
    require(!binding.birth({2,1},record)&&primary.calls==before+1&&!owner.succeeded(),
        "observer failure is returned after completed primary publication");
}

void testStoredLayerDraw(){
    using namespace downwell;
    for(bool visible:{false,true}){
        auto bytes=makePack(false);
        const unsigned layerStart=RoomPack::headerSize+RoomPack::roomRecordSize+2*RoomPack::instanceRecordSize;
        bytes[layerStart+8]=visible?1:0;
        RoomPack pack(bytes.data(),bytes.size());InstanceStore<3> store;RunnerLayerIds ids;
        StoredRoomLayers<3,4,4,1> room(ids);
        require(room.prepare(pack,7,store),"mixed Draw room preparation");
        for(unsigned id:{100065u,100066u,100100u}){
            require(store.create(id,35,7,false,-300000)==CreateInstanceResult::success,"mixed Draw instance storage");
            require(room.observer().birth(store.handleAt(store.size()-1),store.at(store.size()-1)),"mixed Draw layer birth");
        }
        unsigned lookups=0;
        auto lookup=[&](InstanceHandle h,unsigned id,RunnerLayerDraw::Element& out){
            ++lookups;const auto* r=store.resolve(h);
            if(!r||r->instanceId!=id)return false;
            out.flags=0x10u|(r->active?0:2);out.object=true;out.sprite=true;
            return true;
        };
        std::vector<unsigned> drawn;
        auto draw=[&](InstanceHandle h,unsigned id,int depth,RunnerLayerDraw::Action action){
            require(store.resolve(h)&&depth==-300000&&action==RunnerLayerDraw::Action::sprite,"mixed Draw identity/depth/action");
            drawn.push_back(id);return true;
        };
        auto background=[](const auto&,int){return false;};
        require(room.visitDraw(lookup,draw,background)&&drawn==(visible?
            std::vector<unsigned>{100100,100066,100065}:std::vector<unsigned>{100100})&&lookups==drawn.size(),
            "dynamic equal-depth layer draws first; hidden fixed layer does not even query instances");
        store.at(1).active=false;drawn.clear();lookups=0;
        require(room.visitDraw(lookup,draw,background)&&drawn==std::vector<unsigned>{100100}&&lookups==(visible?2u:1u),
            "inactive entry terminates only its layer before later active entries");
        unsigned calls=0;
        require(!room.visitDraw(lookup,[&](auto,unsigned,int,auto){++calls;return false;},background)&&calls==1,
            "renderer failure stops mixed Draw immediately");
    }
}

} // namespace

int main(int argc, char** argv) {
    testDecode();
    testBackgroundDecode();
    testBackgroundConstruction();
    testBackgroundDraw();
    testBackgroundOwner();
    testValidation();
    testPackedRoomLoader();
    testLayerStartup();
    testLayerRoomReentry();
    testLayerLifecycleBinding();
    testStoredLayerDraw();
    if (argc == 2)
        testGeneratedPack(argv[1]);
    std::cout << "room pack tests passed\n";
    return 0;
}

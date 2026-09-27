#include "core/RoomPack.h"
#include "core/PackedRoomLoader.h"
#include "core/PackedRoomLayerLoader.h"
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

std::vector<std::uint8_t> makePack() {
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

    appendInstance(bytes, 100065, 37, true);
    appendInstance(bytes, 100066, 36, false);

    appendU32(bytes, 13);
    appendI32(bytes, -300000);
    appendU32(bytes, 1);
    appendU32(bytes, 0);
    appendU32(bytes, 2);

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
    require(layer.layerId == 13 && layer.depth == -300000 && layer.visible(),
            "layer fields are decoded");
    std::uint32_t member = 0;
    require(pack.membership(1, member) && member == 100066,
            "layer membership is decoded");
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
        unsigned published=0;
        require(downwell::PackedRoomLayerLoader::load(pack,room.roomId,layers,registry,
            [&](unsigned,const auto& layer){
                require(sourceIds.loadedMaximum==int(expectedMaximum),"ID floor updates after serialized layer completion");
                if(layer.layerId>expectedMaximum)expectedMaximum=layer.layerId;
                ++published;return true;
            },&sourceIds),"all original room memberships load");
        require(published==room.layerCount,"all layer metadata published");
        require(sourceIds.loadedMaximum==int(expectedMaximum),"process maximum preserved across loaded rooms");
        for(unsigned l=0;l<room.layerCount;++l){
            downwell::PackedRoomLayer packed;require(pack.roomLayer(room,l,packed),"generated layer");
            const auto slot=layers.find(int(packed.layerId));unsigned remaining=packed.membershipCount;
            require(layers.get(slot)&&!layers.get(slot)->dynamic&&layers.get(slot)->depth==packed.depth,"fixed layer identity/depth");
            require(layers.visitElements(slot,[&](unsigned e){
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
    require(downwell::PackedRoomLoader<3>::enterChecked(store,pack,7,true,[&](auto& r,auto){
        ++events;const unsigned element=registry.find(r.instanceId);
        return element!=Registry::End&&store.resolve(registry.get(element)->instance)==&r;
    },[](int){return true;},[](auto&){return true;},nullptr,binding,[&](auto& survivors,int room){
        require(survivors.empty()&&events==0&&room==7,"layer preparation before first birth");
        return downwell::PackedRoomLayerLoader::load(pack,room,layers,registry,[&](unsigned,const auto& l){
            ++metadataCalls;return l.layerId==13&&l.visible();
        })&&layers.normalizeRoomInstances([](unsigned){return false;});
    })==downwell::PackedRoomLoadResult::success&&events&&metadataCalls==1,
        "real packed room creation binds layers before first instance event");
    std::vector<unsigned> ids;
    layers.visitElements(layers.find(13),[&](unsigned e){ids.push_back(registry.get(e)->sourceId);return true;});
    require(ids==std::vector<unsigned>({100066,100065}),"packed startup retains reserved element order");
    require(!downwell::PackedRoomLayerLoader::load(pack,7,layers,registry,[](unsigned,const auto&){return true;}),"reject loading over live layer owner");
    Layers rejected;Registry rejectedRegistry;
    require(!downwell::PackedRoomLayerLoader::load(pack,7,rejected,rejectedRegistry,[](unsigned,const auto&){return false;})&&
        rejected.find(13)!=Layers::End&&rejectedRegistry.find(100065)==Registry::End,"metadata failure stops before reservations without rollback");
    auto corrupt=bytes;const auto offset=corrupt.size()-4;
    for(unsigned i=0;i<4;++i)corrupt[offset+i]=static_cast<unsigned char>(100065u>>(8*i));
    const downwell::RoomPack bad(corrupt.data(),corrupt.size());Layers badLayers;Registry badRegistry;
    require(!downwell::PackedRoomLayerLoader::load(bad,7,badLayers,badRegistry,[](unsigned,const auto&){return true;}),"duplicate membership rejected");
}

} // namespace

int main(int argc, char** argv) {
    testDecode();
    testValidation();
    testPackedRoomLoader();
    testLayerStartup();
    if (argc == 2)
        testGeneratedPack(argv[1]);
    std::cout << "room pack tests passed\n";
    return 0;
}

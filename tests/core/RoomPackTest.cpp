#include "core/RoomPack.h"
#include "core/PackedRoomLoader.h"
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
    appendU32(bytes, 0);

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
}

void testPackedRoomLoader() {
    const std::vector<std::uint8_t> bytes = makePack();
    const downwell::RoomPack pack(bytes.data(), bytes.size());
    downwell::InstanceStore<3> instances;
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
}

} // namespace

int main(int argc, char** argv) {
    testDecode();
    testValidation();
    testPackedRoomLoader();
    if (argc == 2)
        testGeneratedPack(argv[1]);
    std::cout << "room pack tests passed\n";
    return 0;
}

#include "platform/amiga/AmigaMenu.h"
#include "core/ObjectPack.h"
#include "core/RoomPack.h"
#include "core/StartupRuntime.h"
#include <cstdlib>
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

std::vector<std::uint8_t> readFile(const char* path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}

} // namespace

int main(int argc, char** argv) {
    require(argc == 3 || argc == 4, "room and object pack paths are supplied");
    const std::vector<std::uint8_t> roomBytes = readFile(argv[1]);
    const std::vector<std::uint8_t> objectBytes = readFile(argv[2]);
    const downwell::RoomPack rooms(roomBytes.data(), roomBytes.size());
    const downwell::ObjectPack objects(objectBytes.data(), objectBytes.size());
    require(rooms.valid() && objects.valid(), "generated packs are valid");

    AmigaMenuDrawList list{};
    require(amigaBuildMenuDrawList(rooms, objects, list),
            "native menu draw list builds");

    if (argc == 4) {
        // The front scene bank keeps only the scenery frames this seed-2
        // reconstruction selects. Every drawn frame must be in that bank.
        const std::vector<std::uint8_t> front = readFile(argv[3]);
        auto word = [&](std::size_t at) { return unsigned(front[at]) << 8 | front[at + 1]; };
        require(front.size() >= 20 && word(0) == 0x4457 && word(2) == 0x4150,
                "front scene bank is a planar pack");
        const unsigned count = word(8) << 16 | word(10);
        require(20 + std::size_t(count) * 24 <= front.size(), "front bank directory fits");
        for (unsigned i = 0; i < list.count; ++i) {
            bool found = false;
            for (unsigned r = 0; r < count && !found; ++r)
                found = word(20 + r * 24) == unsigned(list.items[i].spriteId) &&
                        word(22 + r * 24) == list.items[i].frame;
            require(found, "front scene bank holds every garden draw-list frame");
        }
    }

    downwell::StartupRuntime reference(
        downwell::PlatformProfile::nonDesktop, &rooms, &objects);
    reference.step(false);
    require(reference.inMenu(), "reference enters the recovered menu");
    require(list.randomDrawCount == reference.randomDrawCount(),
            "native menu reconstruction consumes the recovered RNG stream");

    // Independent raw-word oracle for the VM's mixed choose/irandom calls.
    // Comparing only the two adapters could preserve the same mistake in both.
    downwell::GameMakerRandom oracle(2);
    downwell::PackedRoom menu;
    require(rooms.findRoom(20, menu), "decoded menu exists");
    for (unsigned index = 0; index < menu.instanceCount; ++index) {
        downwell::PackedRoomInstance placed;
        downwell::PackedObjectEvent event;
        require(rooms.roomInstance(menu, index, placed), "decoded placement exists");
        if (!objects.findEvent(placed.objectId, 0, 0, event)) continue;
        const auto* instance = reference.instance(placed.instanceId);
        switch (event.codeId) {
        case 836:
            oracle.irandom(5); // groundPlayerSet, not choose.
            break;
        case 2478: {
            const unsigned frame = oracle.nextU32() % 6 == 5 ? 1 : 0;
            require(instance && instance->imageIndex == frame, "tree choice matches VM operand order");
            break;
        }
        case 2480:
            require(instance && instance->imageIndex == oracle.irandom(3),
                    "ground-tree frame remains a genuine integer range");
            break;
        case 2486: {
            const int sprite = 635 + oracle.nextU32() % 4;
            const float speed = static_cast<float>(0.15 + oracle.random(0.1));
            const unsigned frame = oracle.irandom(3);
            const bool active = oracle.random(10) >= 1;
            require(instance && instance->spriteId == sprite && instance->imageIndex == frame &&
                    instance->imageSpeed == speed && instance->active == active,
                    "grass choice, range, frame and removal preserve VM order");
            break;
        }
        default: break;
        }
    }
    require(oracle.drawCount() == reference.randomDrawCount() &&
            oracle.stateIndex() == reference.randomState().stateIndex(),
            "startup RNG count/index match independent VM call sequence");
    for (unsigned word = 0; word < 16; ++word)
        require(oracle.stateWord(word) == reference.randomState().stateWord(word),
                "all startup RNG words match independent VM call sequence");
    std::cout << "Garden creation draws=" << oracle.drawCount() << '\n';

    unsigned grassCount = 0;
    unsigned alternateGrassCount = 0;
    for (unsigned index = 0; index < list.count; ++index) {
        const AmigaMenuDrawItem& item = list.items[index];
        const downwell::InstanceRecord* instance =
            reference.instance(item.instanceId);
        require(instance != nullptr && instance->active,
                "drawn native menu instance is active in the reference");
        require(item.frame == static_cast<unsigned>(instance->imageIndex),
                "native menu frame matches recovered Create logic");
        if (item.objectId == 313) {
            ++grassCount;
            require(instance->spriteId == item.spriteId,
                    "native grass sprite matches recovered Create logic");
            if (item.spriteId != 635)
                ++alternateGrassCount;
        }
        if (index > 0)
            require(list.items[index - 1].depth >= item.depth,
                    "native menu keeps GameMaker depth order");
    }
    require(grassCount > 0 && alternateGrassCount > 0,
            "seeded menu includes surviving alternate grass artwork");

    // Independent source membership walk, not a sort by OBJT depth or a call
    // into the native builder. Every scenery item must retain its exact ID.
    unsigned painted=0;
    for(unsigned i=menu.layerCount;i>0;--i){
        downwell::PackedRoomLayer layer;
        require(rooms.roomLayer(menu,i-1,layer),"source garden layer exists");
        if(layer.type!=2||!layer.visible())continue;
        for(unsigned j=layer.membershipCount;j>0;--j){
            std::uint32_t id;
            require(rooms.membership(layer.firstMembership+j-1,id),"source garden member exists");
            const auto* instance=reference.instance(id);
            downwell::PackedObject object;
            if(!instance||!instance->active||instance->objectId==35||
               !objects.findObject(instance->objectId,object)||!object.visible()||
               (instance->spriteId<0&&object.spriteId<0))continue;
            require(painted<list.count&&list.items[painted].instanceId==id&&
                list.items[painted].depth==layer.depth,
                "native static scenery follows source layer and reverse membership order");
            ++painted;
        }
    }
    require(painted==list.count,"all native static scenery is source-layer owned");

    unsigned removedGrassCount = 0;
    for (std::size_t index = 0; index < reference.activeInstanceCount(); ++index) {
        const downwell::InstanceRecord& instance = reference.instanceAt(index);
        if (instance.roomId == downwell::StartupRuntime::menuRoomId &&
            instance.objectId == 313 && !instance.active)
            ++removedGrassCount;
    }
    require(removedGrassCount > 0,
            "native reconstruction preserves seeded grass removal");

    if(std::getenv("DOWNWELL_DUMP_GARDEN_SCENERY"))
        for(unsigned i=0;i<list.count;++i){
            const auto& item=list.items[i];
            std::cout<<"SCENERY "<<item.instanceId<<' '<<item.objectId<<' '
                <<item.spriteId<<' '<<item.frame<<' '<<item.x<<' '<<item.y<<' '<<item.depth<<'\n';
        }

    std::cout << "Amiga menu reconstruction tests passed\n";
    return 0;
}

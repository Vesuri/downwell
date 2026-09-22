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
    require(argc == 3, "room and object pack paths are supplied");
    const std::vector<std::uint8_t> roomBytes = readFile(argv[1]);
    const std::vector<std::uint8_t> objectBytes = readFile(argv[2]);
    const downwell::RoomPack rooms(roomBytes.data(), roomBytes.size());
    const downwell::ObjectPack objects(objectBytes.data(), objectBytes.size());
    require(rooms.valid() && objects.valid(), "generated packs are valid");

    AmigaMenuDrawList list{};
    require(amigaBuildMenuDrawList(rooms, objects, list),
            "native menu draw list builds");

    downwell::StartupRuntime reference(
        downwell::PlatformProfile::nonDesktop, &rooms, &objects);
    reference.step(false);
    require(reference.inMenu(), "reference enters the recovered menu");
    require(list.randomDrawCount == reference.randomDrawCount(),
            "native menu reconstruction consumes the recovered RNG stream");

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

    unsigned removedGrassCount = 0;
    for (std::size_t index = 0; index < reference.activeInstanceCount(); ++index) {
        const downwell::InstanceRecord& instance = reference.instanceAt(index);
        if (instance.roomId == downwell::StartupRuntime::menuRoomId &&
            instance.objectId == 313 && !instance.active)
            ++removedGrassCount;
    }
    require(removedGrassCount > 0,
            "native reconstruction preserves seeded grass removal");

    std::cout << "Amiga menu reconstruction tests passed\n";
    return 0;
}

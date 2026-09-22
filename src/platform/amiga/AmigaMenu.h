#ifndef DOWNWELL_PLATFORM_AMIGA_MENU_H
#define DOWNWELL_PLATFORM_AMIGA_MENU_H

#include <cstdint>

namespace downwell {
class ObjectPack;
class RoomPack;
}

struct AmigaMenuDrawItem {
    std::uint32_t instanceId;
    int x;
    int y;
    int objectId;
    int spriteId;
    unsigned frame;
    int depth;
};

struct AmigaMenuDrawList {
    static constexpr unsigned capacity = 256;

    AmigaMenuDrawItem items[capacity];
    unsigned count;
    std::uint64_t randomDrawCount;
};

bool amigaBuildMenuDrawList(const downwell::RoomPack& rooms,
                            const downwell::ObjectPack& objects,
                            AmigaMenuDrawList& list);

#endif

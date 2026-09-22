#include "AmigaMenu.h"
#include "core/GameMakerRandom.h"
#include "core/ObjectPack.h"
#include "core/RoomPack.h"

namespace {

constexpr int menuRoomId = 20;
constexpr int playerObjectId = 35;

unsigned nonNegativeFrame(float value) {
    union FloatBits {
        float value;
        std::uint32_t bits;
    } converted = {value};
    if ((converted.bits & 0x80000000U) != 0)
        return 0;
    const unsigned exponent = (converted.bits >> 23U) & 0xffU;
    if (exponent < 127U)
        return 0;
    if (exponent > 158U)
        return 0xffffffffU;
    const std::uint32_t mantissa =
        (converted.bits & 0x007fffffU) | 0x00800000U;
    const unsigned shift = exponent - 127U;
    return shift >= 23U ? mantissa << (shift - 23U)
                        : mantissa >> (23U - shift);
}

void applyRecoveredCreate(int codeId,
                          downwell::GameMakerRandom& random,
                          int& spriteId,
                          unsigned& frame,
                          bool& active) {
    switch (codeId) {
    case 836: // objPlayer_n: choose one of six sleeping poses.
        random.irandomSmall(5);
        break;
    case 2478: // objTree: five chances of frame zero, one of frame one.
        frame = random.irandomSmall(5) == 5 ? 1U : 0U;
        break;
    case 2480: // groundTrees: static random frame.
        frame = random.irandomSmall(3);
        break;
    case 2486: // objGrass: sprite, animation phase, and 10% removal.
        spriteId = 635 + static_cast<int>(random.irandomSmall(3));
        random.nextU32(); // random(0.1), used only for image_speed.
        frame = random.irandomSmall(3);
        active = random.randomIntegerSmall(10) != 0;
        break;
    default:
        break;
    }
}

} // namespace

bool amigaBuildMenuDrawList(const downwell::RoomPack& rooms,
                            const downwell::ObjectPack& objects,
                            AmigaMenuDrawList& list) {
    downwell::PackedRoom room;
    downwell::GameMakerRandom random(2);
    list.count = 0;
    list.randomDrawCount = 0;
    if (!rooms.findRoom(menuRoomId, room))
        return false;

    for (unsigned index = 0; index < room.instanceCount; ++index) {
        downwell::PackedRoomInstance instance;
        downwell::PackedObject object;
        downwell::PackedObjectEvent createEvent;
        if (!rooms.roomInstance(room, index, instance) ||
            !objects.findObject(instance.objectId, object))
            return false;

        int spriteId = instance.objectId == 60 ? 76 : object.spriteId;
        unsigned frame = nonNegativeFrame(instance.imageIndex);
        bool active = true;
        if (objects.findEvent(instance.objectId, 0, 0, createEvent))
            applyRecoveredCreate(createEvent.codeId, random, spriteId,
                                 frame, active);

        if (!active || !object.visible() || spriteId < 0 ||
            instance.objectId == playerObjectId)
            continue;
        if (list.count == AmigaMenuDrawList::capacity)
            return false;

        AmigaMenuDrawItem item = {
            instance.instanceId, instance.x, instance.y, instance.objectId,
            spriteId, frame, object.depth
        };
        unsigned destination = list.count;
        while (destination > 0 &&
               list.items[destination - 1].depth < item.depth) {
            list.items[destination] = list.items[destination - 1];
            --destination;
        }
        list.items[destination] = item;
        ++list.count;
    }
    list.randomDrawCount = random.drawCount();
    return true;
}

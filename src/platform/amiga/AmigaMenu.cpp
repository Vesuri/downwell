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
    case 836: // groundPlayerSet: genuine irandom(5), not the choose builtin.
        random.irandomSmall(5);
        break;
    case 2478: // objTree: five chances of frame zero, one of frame one.
        frame = random.chooseIndexSmall(6) == 5 ? 1U : 0U;
        break;
    case 2480: // groundTrees: static random frame.
        frame = random.irandomSmall(3);
        break;
    case 2486: // objGrass: sprite, animation phase, and 10% removal.
        spriteId = 635 + static_cast<int>(random.chooseIndexSmall(4));
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
        list.items[list.count++] = item;
    }
    list.randomDrawCount = random.drawCount();

    // Creates above must consume RNG in placement order. Painting is a
    // separate order: the selected GMS2 runner traverses layers backwards,
    // and reserves serialized instance elements at the head of each layer.
    // Reorder in place, without another cache-sized allocation on the 68000.
    unsigned destination = 0;
    for (unsigned layerIndex = room.layerCount; layerIndex > 0; --layerIndex) {
        downwell::PackedRoomLayer layer;
        if (!rooms.roomLayer(room, layerIndex - 1, layer)) return false;
        if (layer.type == 1) continue; // Solid room background; cache is cleared.
        if (layer.type != 2 || !layer.visible()) return false;
        for (unsigned member = layer.membershipCount; member > 0; --member) {
            std::uint32_t id;
            if (!rooms.membership(layer.firstMembership + member - 1, id)) return false;
            unsigned source = destination;
            while (source < list.count && list.items[source].instanceId != id) ++source;
            if (source == list.count) continue; // Invisible, inactive or player sprite.
            const auto selected = list.items[source];
            list.items[source] = list.items[destination];
            list.items[destination] = selected;
            list.items[destination++].depth = layer.depth;
        }
    }
    // This static scene adapter only owns serialized scenery. Do not silently
    // append objects missing from its source layers using a legacy sort.
    if (destination != list.count) return false;
    return true;
}

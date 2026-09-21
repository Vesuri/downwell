#include "Game.h"
#include "ObjectPack.h"
#include <array>
#include <cmath>
namespace downwell {

Game::Game(const RoomPack* roomPack, const ObjectPack* objectPack)
    : startup_(PlatformProfile::desktop, roomPack, objectPack),
      objectPack_(objectPack) {}

void Game::update(const Input& input) {
    finished_ = finished_ || input.quit;
    startup_.step(input, input.fire && !fire_);
    fire_ = input.fire;
    ++tick_;
}

void Game::render(Renderer& renderer) const {
    renderer.clear({10, 10, 12, 255});
    if (startup_.inMenu()) {
        renderMenu(renderer);
    } else {
        const int stage = startup_.showSplash();
        if (stage == 2)
            renderer.drawSprite(696, 0, 80, 140); // sprDevolverLogo
        if (stage == 4 || stage == 5)
            renderer.drawSprite(695, 0, 80, 140); // sprControls
        if (stage == 5 || stage == 6)
            renderer.drawSpriteTiled(733, 11, -300, 0); // sprDitherFade
    }
}

void Game::renderMenu(Renderer& renderer) const {
    if (objectPack_ == nullptr)
        return;

    const float viewX = startup_.menuCameraViewX();
    const float viewY = startup_.menuCameraViewY();

    std::array<std::size_t, StartupRuntime::instanceCapacity> order{};
    std::size_t count = 0;
    for (std::size_t index = 0; index < startup_.activeInstanceCount(); ++index) {
        const InstanceRecord& instance = startup_.instanceAt(index);
        PackedObject object;
        if (instance.active &&
            instance.objectId != 382 && // ditherFade draws a captured surface
            objectPack_->findObject(instance.objectId, object) &&
            object.visible() && object.spriteId >= 0) {
            order[count++] = index;
        }
    }

    // GameMaker draws high depths first, with later creation-order instances
    // naturally appearing over earlier peers at the same depth.
    for (std::size_t index = 1; index < count; ++index) {
        const std::size_t value = order[index];
        std::size_t destination = index;
        while (destination > 0 &&
               startup_.instanceAt(order[destination - 1]).depth <
                   startup_.instanceAt(value).depth) {
            order[destination] = order[destination - 1];
            --destination;
        }
        order[destination] = value;
    }

    for (std::size_t index = 0; index < count; ++index) {
        const InstanceRecord& instance = startup_.instanceAt(order[index]);
        PackedObject object;
        if (!objectPack_->findObject(instance.objectId, object))
            continue;

        float roomX = static_cast<float>(instance.x);
        float roomY = static_cast<float>(instance.y);
        if (instance.objectId == 312) { // objBgNightsky Step: parallax view follow
            roomX = std::floor(120.0F + viewX / 1.2F);
            roomY = std::floor((viewY + logicalHeight / 2.0F) / 1.1F);
        }
        const bool isPlayer = instance.objectId == 35;
        const std::int32_t spriteId = isPlayer
                                          ? startup_.menuPlayerSpriteId()
                                          : object.spriteId;
        const float runtimeImageIndex = isPlayer
                                            ? startup_.menuPlayerImageIndex()
                                            : instance.imageIndex;
        const float imageIndex = runtimeImageIndex < 0.0F
                                     ? 0.0F : runtimeImageIndex;
        if (instance.objectId == 315) { // objWell Draw: black shaft behind sprite
            renderer.fillRect(static_cast<int>(roomX - 24.0F - viewX),
                              static_cast<int>(roomY + 64.0F - viewY),
                              160, 1000, {0, 0, 0, 255});
        }
        renderer.drawSpriteTransformed(
            spriteId, static_cast<std::uint32_t>(imageIndex),
            roomX - viewX, roomY - viewY,
            instance.scaleX * (isPlayer ? startup_.menuPlayerFacing() : 1),
            instance.scaleY, instance.rotation);
    }
}

} // namespace downwell

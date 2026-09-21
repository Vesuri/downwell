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
    } else if (startup_.inPlayMenu()) {
        renderPlayMenu(renderer);
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
            instance.objectId != 69 &&  // EnterSequence has a custom Draw event
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
        std::int32_t spriteId = isPlayer
                                    ? startup_.menuPlayerSpriteId()
                                    : object.spriteId;
        if (instance.objectId == 60) // objWall_n Create selects wallTile
            spriteId = 76; // global.levelTile[0] / sprTileSurface
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

void Game::renderPlayMenu(Renderer& renderer) const {
    renderer.clear({0, 0, 0, 255});
    const float yOffset = startup_.playMenuYOffset();
    const float centerY = logicalHeight / 2.0F + yOffset;
    const float smoothX = startup_.playMenuSmoothX();
    constexpr std::int32_t styleRuns[] = {27, 25, 26, 28, 29};

    if (!startup_.playMenuSelected()) {
        for (int style = 0; style < 5; ++style) {
            const float drawX = 80.0F + style * 48.0F - smoothX;
            const float distance = drawX - 80.0F;
            const float drawY = centerY - 14.0F - distance * distance / 200.0F;
            // Only the default style is unlocked in the no-save reference state.
            const std::int32_t sprite = style == 0 ? styleRuns[style] : 43;
            const std::uint32_t frame = style == startup_.playMenuCursor()
                                            ? static_cast<std::uint32_t>(
                                                  startup_.playMenuRunFrame())
                                            : 0U;
            renderer.drawSpriteTransformed(sprite, frame, drawX, drawY,
                                           1.0F, 1.0F, 0.0F);
        }

        const int tileY = static_cast<int>(startup_.playMenuTileY());
        for (int row = 0; row <= 36; ++row) {
            renderer.drawSprite(51, 13, 0, tileY + row * 16);
            renderer.drawSprite(51, 7, 160, tileY + row * 16);
        }
        if (startup_.playMenuCursor() < 4)
            renderer.drawSprite(44, 0, 96, static_cast<int>(centerY - 10.0F));
        if (startup_.playMenuCursor() > 0)
            renderer.drawSpriteTransformed(44, 0, 64.0F, centerY - 10.0F,
                                           -1.0F, 1.0F, 0.0F);

        constexpr const char* styleNames[] = {
            "USUAL STYLE", "ARM SPIN STYLE", "BOULDER STYLE",
            "LEVITATE STYLE", "HANDSTAND STYLE"
        };
        constexpr const char* styleDescriptions[] = {
            "standard", "only finds\ngun modules\n\nshops are rare",
            "starts with\nmore HP\n\nless upgrade\noptions", "floaty",
            "no upgrades\n\ncheaper\nshop items"
        };
        const int cursor = startup_.playMenuCursor();
        const float titleY = centerY - 70.0F;
        renderer.drawText(2, "STYLE SELECT", 80.0F, titleY + 1.0F,
                          {255, 0, 0, 255}, TextAlign::center);
        renderer.drawText(2, "STYLE SELECT", 80.0F, titleY,
                          {255, 255, 255, 255}, TextAlign::center);
        const float textY = centerY + 10.0F;
        const char* styleName = cursor == 0 ? styleNames[cursor] : "LOCKED!";
        renderer.drawText(2, styleName, 80.0F, textY + 1.0F,
                          {255, 0, 0, 255}, TextAlign::center);
        renderer.drawText(2, styleName, 80.0F, textY,
                          {255, 255, 255, 255}, TextAlign::center);
        if (cursor == 0)
            renderer.drawText(2, styleDescriptions[cursor], 80.0F,
                              textY + 22.0F, {255, 255, 255, 255},
                              TextAlign::center);
    } else {
        renderer.drawSprite(
            styleRuns[startup_.playStyle()],
            static_cast<std::uint32_t>(startup_.playMenuRunFrame()), 80,
            static_cast<int>(centerY - 14.0F +
                             startup_.playMenuPlayerDescent()));
    }
    renderer.drawSprite(129, 0, 80,
                        static_cast<int>(startup_.playMenuDitherY()));
    if (startup_.playMenuSelected())
        renderer.drawSprite(128, 0, 80,
                            static_cast<int>(
                                startup_.playMenuVerticalDitherY()));
}

} // namespace downwell

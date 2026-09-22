#include "Game.h"
#include "HudNumbers.h"
#include "ObjectPack.h"
#include <array>
#include <cmath>
#include <string>
namespace downwell {
namespace {
class OffsetRenderer final : public Renderer {
public:
    OffsetRenderer(Renderer& renderer, int x) : renderer_(renderer), x_(x) {}
    void clear(Color color) override { renderer_.clear(color); }
    void setClipRect(int x, int y, int width, int height) override {
        renderer_.setClipRect(x + x_, y, width, height);
    }
    void clearClipRect() override { renderer_.clearClipRect(); }
    void fillRect(int x, int y, int width, int height, Color color) override {
        renderer_.fillRect(x + x_, y, width, height, color);
    }
    void drawSpriteTiled(std::int32_t id, std::uint32_t frame,
                         int x, int y) override {
        renderer_.drawSpriteTiled(id, frame, x + x_, y);
    }
    void drawSprite(std::int32_t id, std::uint32_t frame,
                    int x, int y) override {
        renderer_.drawSprite(id, frame, x + x_, y);
    }
    void drawSpriteTransformed(std::int32_t id, std::uint32_t frame,
                               float x, float y, float sx, float sy,
                               float rotation) override {
        renderer_.drawSpriteTransformed(id, frame, x + x_, y, sx, sy, rotation);
    }
    void drawText(std::int32_t id, const char* text, float x, float y,
                  Color color, TextAlign align) override {
        renderer_.drawText(id, text, x + x_, y, color, align);
    }
private:
    Renderer& renderer_;
    int x_;
};
} // namespace

Game::Game(const RoomPack* roomPack, const ObjectPack* objectPack,
           const BuilderPatternPack* builderPatternPack)
    : startup_(PlatformProfile::desktop, roomPack, objectPack,
               StartupRuntime::defaultRandomSeed, builderPatternPack),
      objectPack_(objectPack) {}

void Game::update(const Input& input) {
    finished_ = finished_ || input.quit;
    startup_.step(input, input.fire && !fire_);
    fire_ = input.fire;
    ++tick_;
}

void Game::render(Renderer& renderer) const {
    // GameMaker renders the 160x284 room surface into the centre of the
    // 380x284 PC application surface.  Everything outside it is the HUD
    // layer, whose untouched pen is true black.
    renderer.clear({0, 0, 0, 255});
    renderer.setClipRect(playfieldX, 0, StartupRuntime::surfaceWidth,
                         logicalHeight);
    OffsetRenderer playfield(renderer, playfieldX);
    if (startup_.inMenu()) {
        renderMenu(playfield);
    } else if (startup_.inPlayMenu()) {
        renderPlayMenu(playfield);
    } else if (startup_.inMainRoom()) {
        renderMain(playfield);
    } else {
        const int stage = startup_.showSplash();
        if (stage == 2)
            playfield.drawSprite(696, 0, 80, 140); // sprDevolverLogo
        if (stage == 4 || stage == 5)
            playfield.drawSprite(695, 0, 80, 140); // sprControls
        if (stage == 5 || stage == 6)
            playfield.drawSpriteTiled(733, 11, -300, 0); // sprDitherFade
    }
    renderer.clearClipRect();
    if (startup_.inMainRoom())
        renderLandscapeHud(renderer);
}

void Game::renderMain(Renderer& renderer) const {
    const InstanceRecord* player = startup_.instance(100855);
    if (player == nullptr)
        return;

    const float viewX = startup_.mainCameraViewX();
    const float viewY = startup_.mainCameraViewY();
    const auto hasWallAt = [this](std::int32_t x, std::int32_t y) {
        for (std::size_t index = 0; index < startup_.activeInstanceCount(); ++index) {
            const InstanceRecord& candidate = startup_.instanceAt(index);
            if (candidate.active && candidate.objectId == 60 &&
                candidate.x == x && candidate.y == y)
                return true;
        }
        return false;
    };

    // objWall_n computes this four-neighbour bit mask on its first End Step.
    // Area one uses sprTile1 (resource 51) with its native 16-pixel mask.
    for (std::size_t index = 0; index < startup_.activeInstanceCount(); ++index) {
        const InstanceRecord& instance = startup_.instanceAt(index);
        if (!instance.active || instance.objectId != 60)
            continue;
        std::uint32_t frame = 0;
        frame |= hasWallAt(instance.x, instance.y - 16) ? 1U : 0U;
        frame |= hasWallAt(instance.x + 16, instance.y) ? 2U : 0U;
        frame |= hasWallAt(instance.x, instance.y + 16) ? 4U : 0U;
        frame |= hasWallAt(instance.x - 16, instance.y) ? 8U : 0U;
        renderer.drawSpriteTransformed(51, frame,
                                       instance.x - viewX,
                                       instance.y - viewY,
                                       1.0F, 1.0F, 0.0F);
    }

    // Generated objects carry their recovered Create-event presentation state
    // even though their movement and combat Step events are translated later.
    for (std::size_t index = 0; index < startup_.activeInstanceCount(); ++index) {
        const InstanceRecord& instance = startup_.instanceAt(index);
        if (!instance.active)
            continue;
        std::int32_t sprite = -1;
        if (instance.objectId == 93) sprite = 51;
        if (instance.objectId == 92) sprite = 51;
        if (instance.objectId == 111) sprite = 163;
        if (instance.objectId == 112) sprite = 186;
        if (instance.objectId == 113) sprite = 166;
        if (instance.objectId == 116) sprite = 153;
        if (instance.objectId == 117) sprite = 177;
        if (instance.objectId == 119) sprite = 278;
        if (instance.objectId == 289) sprite = 550;
        if (instance.objectId == 293) sprite = 569;
        if (instance.objectId == 296) sprite = 568;
        if (instance.objectId == 109) sprite = 139;
        if (instance.objectId == 306) sprite = 127;
        if (instance.objectId == 111 && !instance.grounded) sprite = 164;
        if (instance.objectId == 112 && !instance.grounded) sprite = 187;
        if (instance.objectId == 112 && instance.grounded &&
            instance.auxiliaryState != 0) sprite = 188;
        if (instance.objectId == 113 && !instance.grounded) sprite = 168;
        if (instance.objectId == 113 && instance.grounded &&
            instance.auxiliaryState != 0) sprite = 170;
        if (sprite >= 0)
            renderer.drawSpriteTransformed(
                sprite, static_cast<std::uint32_t>(instance.imageIndex),
                instance.x - viewX, instance.y - viewY,
                instance.scaleX, instance.scaleY, instance.rotation);
    }

    renderer.drawSpriteTransformed(
        startup_.mainPlayerSpriteId(),
        static_cast<std::uint32_t>(startup_.mainPlayerImageIndex()),
        startup_.mainPlayerX() - viewX, startup_.mainPlayerY() - viewY,
        static_cast<float>(startup_.mainPlayerFacing()), 1.0F, 0.0F);
    for (std::size_t index = 0; index < startup_.mainBulletCount(); ++index) {
        const StartupRuntime::MainBulletState& bullet =
            startup_.mainBulletAt(index);
        renderer.drawSprite(459, 0,
                            static_cast<int>(bullet.x - viewX),
                            static_cast<int>(bullet.y - viewY));
    }
    for (std::size_t index = 0; index < startup_.mainCurrencyCount(); ++index) {
        const StartupRuntime::MainCurrencyState& gem =
            startup_.mainCurrencyAt(index);
        renderer.drawSprite(gem.large ? 134 : 133,
                            static_cast<std::uint32_t>((gem.age / 2U) % 4U),
                            static_cast<int>(gem.x - viewX),
                            static_cast<int>(gem.y - viewY));
    }

}

void Game::renderLandscapeHud(Renderer& renderer) const {
    constexpr int origin = playfieldX;
    renderer.drawSprite(408, 0, origin - 2, 0);
    renderer.drawSpriteTransformed(408, 0, origin + 162, 0,
                                   -1.0F, 1.0F, 0.0F);

    // Recovered scrDrawHud4x3 coordinates use the playfield's left edge as
    // zero, so its negative left HUD and positive right HUD land naturally in
    // the 110-pixel side regions after adding origin.
    renderer.drawSprite(415, 1, origin - 102, 6);
    renderer.fillRect(origin - 98, 11,
                      20 * startup_.mainPlayerHealth(), 6,
                      {255, 0, 0, 255});
    renderer.drawSprite(415, 0, origin - 102, 6);
    drawHudHealthNumber(renderer, origin - 58, 15,
                        startup_.mainPlayerHealth(), 4);

    constexpr int gaugeX = origin + 174;
    constexpr int gaugeTop = 43;
    constexpr int gaugeBottom = 233;
    constexpr int meterLength = 184;
    const int filled = static_cast<int>(meterLength *
        (static_cast<float>(startup_.mainAmmo()) / 8.0F));
    renderer.drawSprite(416, 0, gaugeX, gaugeTop);
    if (filled > 0)
        renderer.fillRect(gaugeX + 5, gaugeBottom - filled,
                          10, filled, {255, 255, 255, 255});
    renderer.drawSprite(416, 1, gaugeX, gaugeTop);
    drawHudAmmoNumber(renderer, gaugeX + 6, 239, startup_.mainAmmo());

    constexpr int currencyX = origin + 252;
    renderer.drawSprite(376, 0, currencyX, 18);
    drawHudCurrencyNumber(renderer, currencyX - 8, 18,
                          startup_.mainCurrencyTotal());

    if (startup_.mainGemStreak() > 0) {
        const std::string streak = std::to_string(startup_.mainGemStreak());
        renderer.drawSprite(133, 0, origin + 65, 55);
        renderer.drawText(2, streak.c_str(), origin + 81, 51,
                          {255, 255, 255, 255}, TextAlign::center);
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

        const bool isPlayer = instance.objectId == 35;
        // playerDraw() suppresses the sprite while the well teleporter holds
        // goalStop, before EnterSequence scrolls the camera into the shaft.
        if (isPlayer && !startup_.menuPlayerVisible())
            continue;

        float roomX = static_cast<float>(instance.x);
        float roomY = static_cast<float>(instance.y);
        if (instance.objectId == 312) { // objBgNightsky Step: parallax view follow
            roomX = std::floor(120.0F + viewX / 1.2F);
            roomY = std::floor((viewY + logicalHeight / 2.0F) / 1.1F);
        }
        std::int32_t spriteId = isPlayer
                                    ? startup_.menuPlayerSpriteId()
                                    : (instance.spriteId >= 0
                                           ? instance.spriteId
                                           : object.spriteId);
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

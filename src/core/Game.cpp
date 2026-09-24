#include "Game.h"
#include "EnemyPresentation.h"
#include "HudNumbers.h"
#include "DeathResultsBackdrop.h"
#include "DeathResultsRows.h"
#include "DeathResultsProgressBar.h"
#include "DeathResultsProgressLabels.h"
#include "ObjectPack.h"
#include <array>
#include <cmath>
#include <string>
namespace downwell {
namespace {
void drawBorderText(Renderer& renderer, float x, float y, const char* text,
                    Color foreground) {
    const Color black{0, 0, 0, 255};
    renderer.drawText(2, text, x + 1.0F, y, black, TextAlign::center);
    renderer.drawText(2, text, x - 1.0F, y, black, TextAlign::center);
    renderer.drawText(2, text, x, y + 1.0F, black, TextAlign::center);
    renderer.drawText(2, text, x, y - 1.0F, black, TextAlign::center);
    renderer.drawText(2, text, x, y, foreground, TextAlign::center);
}

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
      objectPack_(objectPack) {
#ifdef DOWNWELL_SKIP_TO_GAME
    startup_.skipToGameForTesting();
#elif defined(DOWNWELL_SKIP_INTRO)
    startup_.skipIntroForTesting();
#endif
}

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
    bool clipReleased = false;
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
        if (stage == 3) {
            renderer.clearClipRect();
            clipReleased = true;
            drawBorderText(renderer, playfieldX + 80.0F, 130.0F,
                           "a game by\nOJIRO FUMOTO", {255, 255, 255, 255});
            drawBorderText(renderer, playfieldX + 80.0F, 150.0F,
                           "@MOPPIN_", {255, 0, 0, 255});
            drawBorderText(renderer, playfieldX + 168.0F, 200.0F,
                           "EIRIK SUHRKE", {255, 255, 255, 255});
            drawBorderText(renderer, playfieldX + 168.0F, 210.0F,
                           "@STROTCHY", {255, 0, 0, 255});
            drawBorderText(renderer, playfieldX - 8.0F, 200.0F,
                           "JOONAS TURNER", {255, 255, 255, 255});
            drawBorderText(renderer, playfieldX - 8.0F, 210.0F,
                           "@KISSAKOLME", {255, 0, 0, 255});
        }
        if (stage == 4 || stage == 5)
            playfield.drawSprite(695, 0, 80, 140); // sprControls
        if (stage == 5 || stage == 6)
            playfield.drawSpriteTiled(733, startup_.splashDitherFrame(),
                                      -300, 0); // sprDitherFade
    }
    if (!clipReleased)
        renderer.clearClipRect();
    if (startup_.inMainRoom())
        renderLandscapeHud(renderer);
    if (startup_.inMainRoom() && startup_.deathResults().active) {
        renderer.setClipRect(playfieldX, 0, 160, logicalHeight);
        struct Sink {
            Renderer& renderer;
            void sprite(int frame, int x, int y) { renderer.drawSprite(733, frame, x, y); }
            void asset(int id, unsigned frame, int x, int y) { renderer.drawSprite(id, frame, x, y); }
            void barRect(int x, int y, int w, int h, bool red) {
                renderer.fillRect(x, y, w, h, red ? Color{255,0,0,255} : Color{255,255,255,255});
            }
            void progressGlyph(unsigned code, int x, int y, unsigned pen) {
                const char digit[] = {static_cast<char>('0' + code), 0};
                const char* text = code == 11 ? "MAXED!" : code == 10 ? "/" : digit;
                const Color color = pen == 3 ? Color{0,0,0,255} :
                    pen == 2 ? Color{255,0,0,255} : Color{255,255,255,255};
                renderer.drawText(2, text, x, y, color, TextAlign::center);
            }
            void blackRect(int x, int y, int w, int h) {
                renderer.fillRect(x, y, w, h, {0, 0, 0, 255});
            }
            void title(int x, int y, bool black) {
                const std::uint8_t c = black ? 0 : 255;
                renderer.drawText(2, "GAME OVER", x, y, {c, c, c, 255}, TextAlign::center);
            }
        } sink{playfield};
        drawDeathResultsBackdrop(sink, startup_.deathResults().menu.titleOffset,
            startup_.mainCameraRenderViewX(), startup_.mainCameraRenderViewY());
        drawDeathResultsRows(sink, startup_.deathResults().menu,
            startup_.mainGameGemTotal(), startup_.mainEnemiesDefeated(), startup_.runTimeTicks());
        drawDeathResultsProgressLabels(sink, startup_.deathResults());
        DeathResultsProgressBar::from(startup_.deathResults()).draw(sink);
        renderer.clearClipRect();
    }
}

void Game::renderMain(Renderer& renderer) const {
    const InstanceRecord* player = startup_.instance(100855);
    if (player == nullptr)
        return;

    const float viewX = static_cast<float>(startup_.mainCameraRenderViewX());
    const float viewY = static_cast<float>(startup_.mainCameraRenderViewY());
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
        if (instance.objectId == 289) sprite = 550;
        if (instance.objectId == 293) sprite = 569;
        if (instance.objectId == 296) sprite = 568;
        if (instance.objectId == 109) sprite = 139;
        if (instance.objectId == 306) sprite = 127;
        const int enemySprite = enemyPresentationSprite(instance.objectId,
                                      instance.grounded, instance.auxiliaryState);
        if (enemySprite >= 0) sprite = enemySprite;
        if (sprite >= 0)
            renderer.drawSpriteTransformed(
                sprite, static_cast<std::uint32_t>(instance.imageIndex),
                instance.x - viewX, instance.y - viewY,
                instance.scaleX, instance.scaleY, instance.rotation);
    }

    if (startup_.mainPlayerVisible())
        renderer.drawSpriteTransformed(
            startup_.mainPlayerSpriteId(),
            static_cast<std::uint32_t>(startup_.mainPlayerImageIndex()),
            player->x - viewX, player->y - viewY,
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
                            gem.motion.animation.frame(),
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
        (static_cast<float>(startup_.mainAmmo()) / startup_.mainAmmoCapacity()));
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

    if (startup_.mainGemStreak() > 0 && !startup_.mainDeathSequence().active) {
        const std::string streak = std::to_string(startup_.mainGemStreak());
        renderer.drawSprite(133, 0, origin + 65, 55);
        drawGemmyTextLayers(origin + 81, 51, [&](int x, int y, Color color) {
            renderer.drawText(2, streak.c_str(), x, y, color, TextAlign::center);
        });
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

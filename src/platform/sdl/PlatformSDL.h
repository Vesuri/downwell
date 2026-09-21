#ifndef DOWNWELL_PLATFORM_SDL_H
#define DOWNWELL_PLATFORM_SDL_H

#include "core/Input.h"
#include "core/Renderer.h"
#include <SDL.h>
#include <array>
#include <string>

namespace downwell {

class SpritePack;
class FontPack;

class PlatformSDL final : public Renderer {
public:
    PlatformSDL();
    ~PlatformSDL() override;

    bool ready() const { return renderer_ != nullptr; }
    Input pollInput();
    void present();
    void waitForNextTick();
    bool loadSpriteAssets(const SpritePack& pack, const std::string& directory);
    bool loadFontAssets(const FontPack& pack);

    void clear(Color color) override;
    void setClipRect(int x, int y, int width, int height) override;
    void clearClipRect() override;
    void fillRect(int x, int y, int width, int height, Color color) override;
    void drawSpriteTiled(std::int32_t spriteId, std::uint32_t frame,
                         int x, int y) override;
    void drawSprite(std::int32_t spriteId, std::uint32_t frame,
                    int x, int y) override;
    void drawSpriteTransformed(std::int32_t spriteId, std::uint32_t frame,
                               float x, float y, float scaleX, float scaleY,
                               float rotation) override;
    void drawText(std::int32_t fontId, const char* text, float x, float y,
                  Color color, TextAlign align) override;

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    const SpritePack* spritePack_ = nullptr;
    const FontPack* fontPack_ = nullptr;
    std::array<SDL_Texture*, 8> texturePages_{};
    std::uint64_t nextTick_ = 0;
};

} // namespace downwell

#endif

#include "PlatformSDL.h"
#include "core/Game.h"
#include "core/SpritePack.h"
#include <SDL_image.h>
#include <cstdio>

namespace downwell {

PlatformSDL::PlatformSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return;
    }

    window_ = SDL_CreateWindow("Downwell port — recovered startup reference",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               480, 852, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window_) {
        std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return;
    }

    renderer_ = SDL_CreateRenderer(window_, -1,
                                   SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer_)
        renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer_) {
        std::fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
        return;
    }

    SDL_RenderSetLogicalSize(renderer_, Game::logicalWidth, Game::logicalHeight);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    nextTick_ = SDL_GetPerformanceCounter();
}

PlatformSDL::~PlatformSDL() {
    for (SDL_Texture* texture : texturePages_) {
        if (texture)
            SDL_DestroyTexture(texture);
    }
    if (renderer_)
        SDL_DestroyRenderer(renderer_);
    if (window_)
        SDL_DestroyWindow(window_);
    SDL_Quit();
}

bool PlatformSDL::loadSpriteAssets(const SpritePack& pack,
                                   const std::string& directory) {
    if (!pack.valid() || pack.texturePageCount() > texturePages_.size())
        return false;
    for (std::uint32_t page = 0; page < pack.texturePageCount(); ++page) {
        const std::string path = directory + "/" + std::to_string(page) + ".png";
        texturePages_[page] = IMG_LoadTexture(renderer_, path.c_str());
        if (texturePages_[page] == nullptr) {
            std::fprintf(stderr, "IMG_LoadTexture %s: %s\n", path.c_str(), IMG_GetError());
            return false;
        }
        SDL_SetTextureBlendMode(texturePages_[page], SDL_BLENDMODE_BLEND);
    }
    spritePack_ = &pack;
    return true;
}

Input PlatformSDL::pollInput() {
    Input input;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT)
            input.quit = true;
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
            input.quit = true;
    }

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    input.left = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
    input.right = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
    input.fire = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_Z];
    input.pause = keys[SDL_SCANCODE_P];
    return input;
}

void PlatformSDL::clear(Color color) {
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer_);
}

void PlatformSDL::fillRect(int x, int y, int width, int height, Color color) {
    SDL_Rect rect{x, y, width, height};
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer_, &rect);
}

void PlatformSDL::drawSpriteTiled(std::int32_t spriteId,
                                  std::uint32_t frame,
                                  int x,
                                  int y) {
    if (spritePack_ == nullptr || spriteId < 0)
        return;
    PackedSprite sprite;
    if (!spritePack_->sprite(static_cast<std::uint32_t>(spriteId), sprite) ||
        sprite.frameCount == 0)
        return;
    PackedTexturePageItem item;
    if (!spritePack_->spriteFrame(sprite, frame % sprite.frameCount, item))
        return;
    SDL_Texture* texture = texturePages_[static_cast<std::size_t>(item.texturePageId)];
    if (texture == nullptr || item.boundingWidth <= 0 || item.boundingHeight <= 0)
        return;
    const SDL_Rect source{item.sourceX, item.sourceY,
                          item.sourceWidth, item.sourceHeight};
    for (int tileY = y; tileY < Game::logicalHeight; tileY += item.boundingHeight) {
        for (int tileX = x; tileX < Game::logicalWidth; tileX += item.boundingWidth) {
            const SDL_Rect destination{tileX + item.targetX - sprite.originX,
                                       tileY + item.targetY - sprite.originY,
                                       item.targetWidth, item.targetHeight};
            SDL_RenderCopy(renderer_, texture, &source, &destination);
        }
    }
}

void PlatformSDL::present() {
    SDL_RenderPresent(renderer_);
}

void PlatformSDL::waitForNextTick() {
    const std::uint64_t frequency = SDL_GetPerformanceFrequency();
    const std::uint64_t tickCounters = frequency / Game::ticksPerSecond;
    nextTick_ += tickCounters;
    const std::uint64_t now = SDL_GetPerformanceCounter();
    if (nextTick_ > now) {
        const std::uint64_t remainingMilliseconds =
            (nextTick_ - now) * 1000 / frequency;
        if (remainingMilliseconds > 0)
            SDL_Delay(static_cast<Uint32>(remainingMilliseconds));
    } else if (now - nextTick_ > frequency / 4) {
        nextTick_ = now;
    }
}

} // namespace downwell

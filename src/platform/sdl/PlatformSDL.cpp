#include "PlatformSDL.h"
#include "SpriteVisibility.h"
#include "core/Game.h"
#include "core/FontPack.h"
#include "core/SpritePack.h"
#include <SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>

namespace downwell {

PlatformSDL::PlatformSDL() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return;
    }

    window_ = SDL_CreateWindow("Downwell port — recovered startup reference",
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                               760, 568, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
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
    const char* profile = std::getenv("DOWNWELL_PROFILE");
    if (profile && std::string(profile) == "1") {
        SDL_RendererInfo info{};
        if (SDL_GetRendererInfo(renderer_, &info) == 0)
            std::fprintf(stderr, "SDL profile: renderer=%s accelerated=%d vsync=%d\n",
                info.name, !!(info.flags & SDL_RENDERER_ACCELERATED),
                !!(info.flags & SDL_RENDERER_PRESENTVSYNC));
    }

    SDL_RenderSetLogicalSize(renderer_, Game::logicalWidth, Game::logicalHeight);
    // The runner rasterizes the application surface before display scaling.
    // Logical size alone scales geometry and exposes window-sized subpixels.
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    framebuffer_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET, Game::logicalWidth, Game::logicalHeight);
    if (!framebuffer_ || SDL_SetRenderTarget(renderer_, framebuffer_) != 0) {
        std::fprintf(stderr, "SDL framebuffer: %s\n", SDL_GetError());
        if (framebuffer_) SDL_DestroyTexture(framebuffer_);
        framebuffer_ = nullptr;
        return;
    }
    SDL_SetTextureBlendMode(framebuffer_, SDL_BLENDMODE_NONE);
    SDL_RenderSetLogicalSize(renderer_, Game::logicalWidth, Game::logicalHeight);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    nextTick_ = SDL_GetPerformanceCounter();
}

PlatformSDL::~PlatformSDL() {
    for(auto* texture:flashFrames_) if(texture) SDL_DestroyTexture(texture);
    if (framebuffer_) SDL_DestroyTexture(framebuffer_);
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
    // Sprite 96 uses near-white/near-black source values classified by the
    // original default shader. Prepare isolated textures once, without changing
    // shared atlas pixels or adding per-frame palette conversion.
    PackedSprite flash;
    if(pack.sprite(96,flash)) {
        if(flash.frameCount!=flashFrames_.size()) return false;
        for(unsigned frame=0;frame<flashFrames_.size();++frame) {
            PackedTexturePageItem item;
            if(!pack.spriteFrame(flash,frame,item)) return false;
            const auto path=directory+"/"+std::to_string(item.texturePageId)+".png";
            SDL_Surface* page=IMG_Load(path.c_str());
            if(!page) return false;
            SDL_Surface* crop=SDL_CreateRGBSurfaceWithFormat(0,item.sourceWidth,item.sourceHeight,32,SDL_PIXELFORMAT_RGBA32);
            if(!crop){SDL_FreeSurface(page);return false;}
            SDL_SetSurfaceBlendMode(page,SDL_BLENDMODE_NONE);
            SDL_Rect source{item.sourceX,item.sourceY,item.sourceWidth,item.sourceHeight};
            const int copied=SDL_BlitSurface(page,&source,crop,nullptr);
            SDL_FreeSurface(page);
            bool valid=copied==0;
            for(int y=0;valid && y<crop->h;++y) for(int x=0;valid && x<crop->w;++x) {
                auto* pixel=static_cast<Uint8*>(crop->pixels)+y*crop->pitch+x*4;
                if(!pixel[3]) continue;
                valid=pixel[3]==255 && pixel[0]==pixel[1] && pixel[1]==pixel[2] &&
                      (pixel[0]==254 || pixel[0]==7);
                if(valid) pixel[0]=pixel[1]=pixel[2]=pixel[0]==254?255:0;
            }
            if(valid) flashFrames_[frame]=SDL_CreateTextureFromSurface(renderer_,crop);
            SDL_FreeSurface(crop);
            if(!valid || !flashFrames_[frame]) return false;
            SDL_SetTextureBlendMode(flashFrames_[frame],SDL_BLENDMODE_BLEND);
        }
    }
    spritePack_ = &pack;
    return true;
}

bool PlatformSDL::loadFontAssets(const FontPack& pack) {
    if (!pack.valid() || spritePack_ == nullptr)
        return false;
    fontPack_ = &pack;
    return true;
}

Input PlatformSDL::pollInput() {
    Input input;
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN && !event.key.repeat &&
            event.key.keysym.sym == SDLK_F12)
            screenshotRequested_ = true;
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

void PlatformSDL::setClipRect(int x, int y, int width, int height) {
    const SDL_Rect rect{x, y, width, height};
    SDL_RenderSetClipRect(renderer_, &rect);
}

void PlatformSDL::clearClipRect() {
    SDL_RenderSetClipRect(renderer_, nullptr);
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

void PlatformSDL::drawSprite(std::int32_t spriteId,
                             std::uint32_t frame,
                             int x,
                             int y) {
    drawSpriteTransformed(spriteId, frame, static_cast<float>(x),
                          static_cast<float>(y), 1.0F, 1.0F, 0.0F);
}

void PlatformSDL::drawSpriteTransformed(std::int32_t spriteId,
                                        std::uint32_t frame,
                                        float x,
                                        float y,
                                        float scaleX,
                                        float scaleY,
                                        float rotation) {
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
    if (texture == nullptr || scaleX == 0.0F || scaleY == 0.0F)
        return;
    SDL_Rect source{item.sourceX, item.sourceY,
                          item.sourceWidth, item.sourceHeight};
    if(spriteId==96 && flashFrames_[frame%3]) {
        texture=flashFrames_[frame%3];source.x=source.y=0;
    }
    const float left = static_cast<float>(item.targetX - sprite.originX);
    const float top = static_cast<float>(item.targetY - sprite.originY);
    const float right = left + static_cast<float>(item.targetWidth);
    const float bottom = top + static_cast<float>(item.targetHeight);
    if ((spriteId == 458 || spriteId == 459) && rotation >= 267 && rotation <= 273 &&
            scaleX == 1 && scaleY == 1 && std::floor(x) == x && std::floor(y) == y) {
        SDL_RendererInfo info{};
        if (SDL_GetRendererInfo(renderer_, &info) == 0 && (info.flags & SDL_RENDERER_SOFTWARE)) {
            // The offline generator checks the entire angular interval, all
            // twelve source frames and every possible nearest-pixel sample:
            // at integer anchors these pixels equal the 270-degree variant.
            // Use that equivalent raster only for SDL's software backend,
            // whose generic rotation path differs from the GPU's sampling.
            // Keep the actual angle for GPU rendering and all other artwork.
            rotation = 270;
        }
    }
    const float transformedLeft = std::min(left * scaleX, right * scaleX);
    const float transformedTop = std::min(top * scaleY, bottom * scaleY);
    const SDL_FRect destination{x + transformedLeft,
                                y + transformedTop,
                                std::abs(scaleX) * item.targetWidth,
                                std::abs(scaleY) * item.targetHeight};
    if(sdlSpriteOutside(destination.x,destination.y,destination.w,destination.h,
                        rotation,Game::logicalWidth,Game::logicalHeight))return;
    const SDL_FPoint origin{-transformedLeft, -transformedTop};
    SDL_RendererFlip flip = SDL_FLIP_NONE;
    if (scaleX < 0.0F)
        flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_HORIZONTAL);
    if (scaleY < 0.0F)
        flip = static_cast<SDL_RendererFlip>(flip | SDL_FLIP_VERTICAL);
    SDL_RenderCopyExF(renderer_, texture, &source, &destination,
                      -static_cast<double>(rotation), &origin, flip);
}

void PlatformSDL::drawText(std::int32_t fontId,
                           const char* text,
                           float x,
                           float y,
                           Color color,
                           TextAlign align) {
    if (fontPack_ == nullptr || spritePack_ == nullptr || text == nullptr ||
        fontId < 0)
        return;
    PackedFont font;
    PackedTexturePageItem atlas;
    if (!fontPack_->font(static_cast<std::uint32_t>(fontId), font) ||
        !spritePack_->pageItem(static_cast<std::uint32_t>(font.texturePageItemId),
                               atlas))
        return;
    SDL_Texture* texture = texturePages_[static_cast<std::size_t>(atlas.texturePageId)];
    if (texture == nullptr)
        return;
    SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(texture, color.a);

    const auto lineWidth = [this, &font](const char* begin, const char* end) {
        int width = 0;
        for (const char* cursor = begin; cursor < end && *cursor; ++cursor) {
            PackedGlyph glyph;
            if (!fontPack_->findGlyph(font,
                    static_cast<unsigned char>(*cursor), glyph))
                continue;
            const std::uint32_t next = cursor + 1 < end
                                           ? static_cast<unsigned char>(cursor[1])
                                           : 0;
            width += glyph.shift + fontPack_->kerningShift(glyph, next);
        }
        return width;
    };

    const char* line = text;
    float drawY = y;
    while (true) {
        const char* end = line;
        while (*end && *end != '\n')
            ++end;
        const int width = lineWidth(line, end);
        float drawX = x;
        if (align == TextAlign::center) drawX -= width / 2.0F;
        if (align == TextAlign::right) drawX -= static_cast<float>(width);
        for (const char* cursor = line; cursor < end; ++cursor) {
            PackedGlyph glyph;
            if (!fontPack_->findGlyph(font,
                    static_cast<unsigned char>(*cursor), glyph))
                continue;
            const SDL_Rect source{atlas.sourceX + glyph.sourceX,
                                  atlas.sourceY + glyph.sourceY,
                                  glyph.sourceWidth, glyph.sourceHeight};
            const SDL_FRect destination{drawX + glyph.offset, drawY,
                                        static_cast<float>(glyph.sourceWidth),
                                        static_cast<float>(glyph.sourceHeight)};
            SDL_RenderCopyF(renderer_, texture, &source, &destination);
            const std::uint32_t next = cursor + 1 < end
                                           ? static_cast<unsigned char>(cursor[1])
                                           : 0;
            drawX += glyph.shift + fontPack_->kerningShift(glyph, next);
        }
        if (!*end)
            break;
        line = end + 1;
        drawY += 12.0F;
    }
    SDL_SetTextureColorMod(texture, 255, 255, 255);
    SDL_SetTextureAlphaMod(texture, 255);
}

void PlatformSDL::present() {
    // Capture the logical framebuffer, not the window-sized presentation.
    if (screenshotRequested_) {
        screenshotRequested_ = false;
        saveScreenshot();
    }
    SDL_SetRenderTarget(renderer_, nullptr);
    SDL_RenderSetLogicalSize(renderer_, Game::logicalWidth, Game::logicalHeight);
    SDL_RenderCopy(renderer_, framebuffer_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
    SDL_SetRenderTarget(renderer_, framebuffer_);
}

bool PlatformSDL::saveScreenshot(const std::string& directory) {
    namespace fs = std::filesystem;
    fs::path folder(directory);
    if (folder.empty()) {
        const char* home = std::getenv("HOME");
        if (!home || !*home) {
            SDL_Log("Screenshot failed: HOME is unavailable");
            return false;
        }
        folder = fs::path(home) / "Pictures" / "Screenshots";
    }
    std::error_code error;
    fs::create_directories(folder, error);
    if (error) {
        SDL_Log("Screenshot directory: %s", error.message().c_str());
        return false;
    }
    int width = 0, height = 0;
    SDL_Texture* target = SDL_GetRenderTarget(renderer_);
    if ((target ? SDL_QueryTexture(target, nullptr, nullptr, &width, &height)
                : SDL_GetRendererOutputSize(renderer_, &width, &height)) != 0)
        return false;
    SDL_Surface* pixels = SDL_CreateRGBSurfaceWithFormat(
        0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
    if (!pixels) return false;
    if (SDL_RenderReadPixels(renderer_, nullptr, pixels->format->format,
                             pixels->pixels, pixels->pitch) != 0) {
        SDL_Log("Screenshot readback: %s", SDL_GetError());
        SDL_FreeSurface(pixels);
        return false;
    }
    const auto now = std::chrono::system_clock::now();
    const std::time_t time = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_r(&time, &local);
    char timestamp[40];
    std::strftime(timestamp, sizeof(timestamp), "Downwell_%Y-%m-%d_%H-%M-%S", &local);
    fs::path path;
    int descriptor = -1;
    // Exclusive creation prevents overwriting another capture, even across processes.
    for (unsigned suffix = 0; suffix < 10000; ++suffix) {
        path = folder / (std::string(timestamp) + "_" + std::to_string(suffix) + ".png");
        descriptor = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (descriptor >= 0 || errno != EEXIST) break;
    }
    bool success = false;
    if (descriptor >= 0) {
        FILE* file = fdopen(descriptor, "wb");
        if (file) {
            SDL_RWops* output = SDL_RWFromFP(file, SDL_TRUE);
            if (output) success = IMG_SavePNG_RW(pixels, output, 1) == 0;
            else std::fclose(file);
        } else {
            ::close(descriptor);
        }
        if (!success) fs::remove(path, error);
    }
    SDL_FreeSurface(pixels);
    if (success) SDL_Log("Screenshot saved: %s", path.c_str());
    else SDL_Log("Screenshot save failed: %s", path.c_str());
    return success;
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

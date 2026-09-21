#include "PlatformSDL.h"
#include "core/Game.h"
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
    if (renderer_)
        SDL_DestroyRenderer(renderer_);
    if (window_)
        SDL_DestroyWindow(window_);
    SDL_Quit();
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

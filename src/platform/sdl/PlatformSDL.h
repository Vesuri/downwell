#ifndef DOWNWELL_PLATFORM_SDL_H
#define DOWNWELL_PLATFORM_SDL_H

#include "core/Input.h"
#include "core/Renderer.h"
#include <SDL.h>

namespace downwell {

class PlatformSDL final : public Renderer {
public:
    PlatformSDL();
    ~PlatformSDL() override;

    bool ready() const { return renderer_ != nullptr; }
    Input pollInput();
    void present();
    void waitForNextTick();

    void clear(Color color) override;
    void fillRect(int x, int y, int width, int height, Color color) override;

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    std::uint64_t nextTick_ = 0;
};

} // namespace downwell

#endif


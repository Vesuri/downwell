#include "platform/sdl/PlatformSDL.h"
#include <SDL_image.h>
#include <cassert>
#include <filesystem>
#include <iostream>

int main() {
    std::string directory;
    for (unsigned suffix = 0; ; ++suffix) {
        directory = "build/screenshot-test-" + std::to_string(suffix);
        if (std::filesystem::create_directory(directory)) break;
    }
    downwell::PlatformSDL platform;
    assert(platform.ready());
    platform.clear({255, 0, 0, 255});
    assert(platform.saveScreenshot(directory));
    platform.clear({0, 255, 0, 255});
    assert(platform.saveScreenshot(directory));
    unsigned count = 0, red = 0, green = 0;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        SDL_Surface* png = IMG_Load(entry.path().c_str());
        assert(png && png->w >= 380 && png->h >= 284);
        SDL_Surface* rgba = SDL_ConvertSurfaceFormat(png, SDL_PIXELFORMAT_RGBA32, 0);
        assert(rgba);
        const auto* pixel = static_cast<const Uint8*>(rgba->pixels) +
            (rgba->h / 2) * rgba->pitch + (rgba->w / 2) * 4;
        red += pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 0;
        green += pixel[0] == 0 && pixel[1] == 255 && pixel[2] == 0;
        ++count;
        SDL_FreeSurface(rgba);
        SDL_FreeSurface(png);
    }
    assert(count == 2 && red == 1 && green == 1);
    std::cout << "SDL PNG framebuffer capture and unique filenames passed\n";
}

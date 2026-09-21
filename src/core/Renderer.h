#ifndef DOWNWELL_CORE_RENDERER_H
#define DOWNWELL_CORE_RENDERER_H

#include <cstdint>

namespace downwell {

struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void clear(Color color) = 0;
    virtual void fillRect(int x, int y, int width, int height, Color color) = 0;
};

} // namespace downwell

#endif


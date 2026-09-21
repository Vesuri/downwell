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

enum class TextAlign { left, center, right };

class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void clear(Color color) = 0;
    virtual void setClipRect(int x, int y, int width, int height) = 0;
    virtual void clearClipRect() = 0;
    virtual void fillRect(int x, int y, int width, int height, Color color) = 0;
    virtual void drawSpriteTiled(std::int32_t spriteId,
                                 std::uint32_t frame,
                                 int x,
                                 int y) = 0;
    virtual void drawSprite(std::int32_t spriteId,
                            std::uint32_t frame,
                            int x,
                            int y) = 0;
    virtual void drawSpriteTransformed(std::int32_t spriteId,
                                       std::uint32_t frame,
                                       float x,
                                       float y,
                                       float scaleX,
                                       float scaleY,
                                       float rotation) = 0;
    virtual void drawText(std::int32_t fontId,
                          const char* text,
                          float x,
                          float y,
                          Color color,
                          TextAlign align) = 0;
};

} // namespace downwell

#endif

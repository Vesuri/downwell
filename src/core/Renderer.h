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
    // MSB-first binary effect coverage. Publish only set bits, never clear the
    // retained background; target-specific renderers may override word writes.
    virtual void drawCoverageWord(int x,int y,std::uint16_t bits,Color color) {
        for(unsigned bit=0;bit<16;){
            if(!(bits&(0x8000u>>bit))){++bit;continue;}
            const unsigned first=bit;
            do{++bit;}while(bit<16 && (bits&(0x8000u>>bit)));
            fillRect(x+int(first),y,int(bit-first),1,color);
        }
    }
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

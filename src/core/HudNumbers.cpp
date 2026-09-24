#include "HudNumbers.h"
#include <string>

namespace downwell {
namespace {

constexpr std::int32_t numberSprite = 413;

std::string digitsFor(int value) {
    return std::to_string(value < 0 ? 0 : value);
}

} // namespace

void drawHudHealthNumber(Renderer& renderer, int x, int y,
                         int health, int maximum) {
    drawHealthNumberLayers(x, y, health < 0 ? 0u : static_cast<unsigned>(health),
        maximum < 0 ? 0u : static_cast<unsigned>(maximum),
        [&renderer](unsigned sprite, unsigned frame, int px, int py) {
            renderer.drawSprite(sprite, frame, px, py);
        });
}

void drawHudCurrencyNumber(Renderer& renderer, int x, int y, int value) {
    const std::string digits = digitsFor(value);
    const int firstX = x - static_cast<int>(digits.size()) * 8;
    const int digitY = y - 5;
    for (std::size_t index = 0; index < digits.size(); ++index) {
        const std::uint32_t digit = static_cast<std::uint32_t>(digits[index] - '0');
        const int digitX = firstX + static_cast<int>(index) * 8;
        renderer.drawSprite(numberSprite, digit + 10, digitX - 1, digitY);
        renderer.drawSprite(numberSprite, digit + 10, digitX, digitY - 1);
        renderer.drawSprite(numberSprite, digit + 20, digitX + 1, digitY);
        renderer.drawSprite(numberSprite, digit + 20, digitX, digitY + 1);
    }
    for (std::size_t index = 0; index < digits.size(); ++index) {
        const std::uint32_t digit = static_cast<std::uint32_t>(digits[index] - '0');
        renderer.drawSprite(numberSprite, digit,
                            firstX + static_cast<int>(index) * 8, digitY);
    }
}

void drawHudAmmoNumber(Renderer& renderer, int x, int y, int value) {
    drawAmmoNumberLayers(x, y, static_cast<unsigned>(value < 0 ? 0 : value),
        [&renderer](unsigned frame, int px, int py) {
            renderer.drawSprite(numberSprite, frame, px, py);
        });
}

} // namespace downwell

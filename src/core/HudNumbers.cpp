#include "HudNumbers.h"
#include <string>

namespace downwell {
namespace {

constexpr std::int32_t numberSprite = 413;
constexpr std::int32_t slashSprite = 414;

std::string digitsFor(int value) {
    return std::to_string(value < 0 ? 0 : value);
}

} // namespace

void drawHudHealthNumber(Renderer& renderer, int x, int y,
                         int health, int maximum) {
    const int slashX = x - 4;
    const int digitY = y - 5;
    const std::string healthDigits = digitsFor(health);
    const int healthX = slashX - static_cast<int>(healthDigits.size()) * 8 - 1;
    for (std::size_t index = 0; index < healthDigits.size(); ++index) {
        const std::uint32_t digit = static_cast<std::uint32_t>(healthDigits[index] - '0');
        const int digitX = healthX + static_cast<int>(index) * 8;
        renderer.drawSprite(numberSprite, digit + 20, digitX + 1, digitY);
        renderer.drawSprite(numberSprite, digit + 20, digitX + 1, digitY + 1);
        renderer.drawSprite(numberSprite, digit + 20, digitX, digitY + 1);
        renderer.drawSprite(numberSprite, digit, digitX, digitY);
    }

    renderer.drawSprite(slashSprite, 2, slashX + 1, digitY);
    renderer.drawSprite(slashSprite, 2, slashX + 1, digitY + 1);
    renderer.drawSprite(slashSprite, 2, slashX, digitY + 1);
    renderer.drawSprite(slashSprite, 0, slashX, digitY);

    const std::string maximumDigits = digitsFor(maximum);
    const int maximumX = slashX + 9;
    for (std::size_t index = 0; index < maximumDigits.size(); ++index) {
        const std::uint32_t digit = static_cast<std::uint32_t>(maximumDigits[index] - '0');
        const int digitX = maximumX + static_cast<int>(index) * 8;
        renderer.drawSprite(numberSprite, digit + 20, digitX + 1, digitY);
        renderer.drawSprite(numberSprite, digit + 20, digitX + 1, digitY + 1);
        renderer.drawSprite(numberSprite, digit + 20, digitX, digitY + 1);
        renderer.drawSprite(numberSprite, digit, digitX, digitY);
    }
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

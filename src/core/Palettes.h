#ifndef DOWNWELL_CORE_PALETTES_H
#define DOWNWELL_CORE_PALETTES_H
#include <cstdint>
namespace downwell {
// One PALETTE menu entry (scrShaderListInit): the shader's replacements for
// white (colorL), red (colorM), black (colorD) and blue (colorS) as 12-bit
// Amiga colours. Generated from data.win; see tools/generate-palettes.py.
struct PaletteEntry {
    const char* name;
    int shader;
    std::uint16_t light, middle, dark, special;
    // Remap one of the build's own colours. The Amiga draws only with black,
    // white and red; anything else is returned unchanged.
    std::uint16_t remap(std::uint16_t colour) const {
        switch (colour & 0xfff) {
        case 0x000: return dark;
        case 0xfff: return light;
        case 0xf00: return middle;
        default: return colour;
        }
    }
};
}
#endif

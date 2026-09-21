#include "core/FontPack.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
}

int main(int argc, char** argv) {
    require(argc == 2, "font pack path is required");
    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    downwell::FontPack pack(bytes.data(), bytes.size());
    require(pack.valid(), "generated font pack validates");
    require(pack.fontCount() == 8 && pack.glyphCount() == 1525 &&
                pack.kerningCount() == 323,
            "recovered font resource counts are pinned");
    downwell::PackedFont font;
    require(pack.font(2, font) && font.texturePageItemId == 13 &&
                font.glyphCount == 412,
            "DOWNWELL font references its recovered atlas item");
    downwell::PackedGlyph glyph;
    require(pack.findGlyph(font, 'A', glyph) && glyph.sourceX == 56 &&
                glyph.sourceY == 122 && glyph.sourceWidth == 7 &&
                glyph.sourceHeight == 10 && glyph.shift == 8,
            "DOWNWELL capital A metrics are exact");
    require(!pack.findGlyph(font, 0x10ffff, glyph),
            "missing Unicode glyphs are rejected");
    std::cout << "font pack tests passed\n";
    return 0;
}

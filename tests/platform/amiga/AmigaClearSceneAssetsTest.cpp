#include "platform/amiga/AmigaClearSceneAssets.h"
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void require(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr, "%s\n", message); std::abort(); }
}
static std::vector<std::uint8_t> read(const char* path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}
int main(int argc, char** argv) {
    require(argc == 3, "clear artwork and mask required");
    auto art = read(argv[1]), mask = read(argv[2]);
    require(!art.empty() && (mask.size() == AmigaExitMask::bytes ||
        mask.size() == AmigaExitMask::selectionBytes), "read offline assets");
    const auto artOriginal = art, maskOriginal = mask;
    const auto layout = AmigaClearSceneAssets::partition(76800, 13940);
    require(layout.valid && layout.resultsOffset == 31104 && layout.resultsCapacity == 31756 &&
        layout.hudOffset == 62860, "mask and art coexist without moving relocated HUD");
    require(!AmigaClearSceneAssets::partition(31103, 0).valid &&
        !AmigaClearSceneAssets::partition(45043, 13940).valid,
        "short or overlapping allocations rejected");
    std::vector<std::uint8_t> memory(76800 + 32, 0xa5);
    auto* storage = memory.data() + 16;
    AmigaClearSceneAssets assets;
    const unsigned initial[] = {0, 6, 13, 100};
    require(assets.prepare(storage, 76800, 13940, art.data(), art.size(), mask.data(), mask.size(),
        initial, 4, 1, 1), "prepare simultaneous mask/art bank");
    for (unsigned sample = 0; sample < 256; ++sample) {
        unsigned offers[4];
        for (unsigned i = 0; i < 4; ++i) {
            offers[i] = (sample * (i + 1) + i * 7) % 24;
            if (offers[i] == 23) offers[i] = 100;
        }
        unsigned char owned[23];
        for (unsigned i = 0; i < 23; ++i) owned[i] = (sample + i) % 2;
        require(assets.changeOffers(storage, art.data(), art.size(), offers, 4, 1 + sample % 4, 1 + sample % 3, owned),
            "rerolled offers fit alongside resident mask");
        std::vector<std::uint8_t> expected(layout.resultsCapacity);
        const unsigned bytes = AmigaClearBank::build(art.data(), art.size(), offers, 4, expected.data(), expected.size(),
            1 + sample % 4, 1 + sample % 3, owned);
        require(bytes == assets.bankBytes && std::memcmp(storage + layout.resultsOffset, expected.data(), bytes) == 0,
            "resident selected artwork matches independently selected bank");
        require(std::memcmp(storage, mask.data(), mask.size()) == 0, "offer changes preserve every departure mask byte");
        for (unsigned i = layout.hudOffset; i < 76800; ++i)
            require(storage[i] == 0xa5, "relocated HUD artwork stays intact");
        for (unsigned i = 0; i < 16; ++i)
            require(memory[i] == 0xa5 && memory[memory.size() - 1 - i] == 0xa5, "allocation guards intact");
    }
    const auto saved = memory;
    const unsigned previousBytes = assets.bankBytes;
    const unsigned invalid[] = {999};
    require(!assets.prepare(storage, 76800, 13940, art.data(), art.size(), mask.data(), mask.size() - 1,
            initial, 4, 1, 1) &&
        !assets.prepare(storage, 31104, 0, art.data(), art.size(), mask.data(), mask.size(), initial, 4, 1, 1) &&
        !assets.changeOffers(storage, art.data(), art.size(), invalid, 1, 1, 1) &&
        assets.bankBytes == previousBytes && memory == saved,
        "failed preparation or re-selection is transactional");
    require(art == artOriginal && mask == maskOriginal, "source packs remain untouched");
    std::puts("256 clear-scene banks preserve resident mask, HUD and allocation guards");
}

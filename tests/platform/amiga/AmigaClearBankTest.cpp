#include "platform/amiga/AmigaClearBank.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>
#include <cstring>
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
int main(int argc, char** argv) {
    require(argc == 2, "clear pack argument required");
    std::ifstream file(argv[1], std::ios::binary);
    std::vector<std::uint8_t> source{std::istreambuf_iterator<char>(file), {}};
    require(source.size() > 20, "read generated clear pack");
    const auto original = source;
    std::vector<std::uint8_t> storage(32140 + 32, 0xa5);
    auto* target = storage.data() + 16;
    unsigned maximum = 0;
    for (unsigned sample = 0; sample < 256; ++sample) {
        unsigned offers[4];
        for (unsigned i = 0; i < 4; ++i) {
            const unsigned id = (sample * (i + 1) + i * 7) % 24;
            offers[i] = id == 23 ? 100 : id;
        }
        const unsigned area = 1 + sample % 4;
        const unsigned level = 1 + sample % 3;
        unsigned char owned[23];
        for (unsigned i = 0; i < 23; ++i) owned[i] = (sample + i) % 2;
        const unsigned bytes = AmigaClearBank::build(source.data(), source.size(), offers, 4, target, 32140, area, level, owned);
        require(bytes && bytes <= 13148, "selected offers, stage title and borders fit source-verified bound");
        if (bytes > maximum) maximum = bytes;
        require(std::memcmp(target + 12, source.data() + 12, 8) == 0, "input provenance prefix preserved");
        const unsigned count = AmigaClearBank::longword(target + 8);
        require(count >= 9 && count <= 18, "duplicates deduplicated by resource ID/frame");
        unsigned borders = 0;
        unsigned titles = 0;
        for (unsigned i = 0; i < count; ++i) {
            const auto* copied = target + 20 + i * 24;
            const auto key = AmigaClearBank::longword(copied);
            if (AmigaClearBank::word(copied) >= 64500 && AmigaClearBank::word(copied) < 64512) {
                require(AmigaClearBank::word(copied) == downwell::levelClearStageLabel(area, level),
                    "bank includes only this stage's composed title");
                ++titles;
            }
            require(i == 0 || AmigaClearBank::longword(copied - 24) < key,
                "selected directory is strictly ordered for renderer binary search");
            if (AmigaClearBank::word(copied + 2) != 0) {
                const unsigned id = AmigaClearBank::word(copied);
                require((id == downwell::levelBorderSprite(area) || id == downwell::levelBorderSprite(area + 1)) &&
                    (AmigaClearBank::word(copied + 2) == 7 || AmigaClearBank::word(copied + 2) == 13),
                    "only current/incoming border frames are banked");
                ++borders;
            }
            const std::uint8_t* originalRecord = nullptr;
            for (unsigned j = 0; j < AmigaClearBank::longword(source.data() + 8); ++j) {
                const auto* candidate = source.data() + 20 + j * 24;
                if (AmigaClearBank::longword(candidate) == AmigaClearBank::longword(copied)) originalRecord = candidate;
            }
            require(originalRecord && std::memcmp(copied, originalRecord, 16) == 0 &&
                AmigaClearBank::longword(copied + 20) == AmigaClearBank::longword(originalRecord + 20),
                "resource ID, frame, origins, dimensions and flags preserved");
            const unsigned offset = AmigaClearBank::longword(copied + 16);
            const unsigned length = AmigaClearBank::longword(copied + 20);
            require(offset <= bytes && length <= bytes - offset &&
                std::memcmp(target + offset, source.data() + AmigaClearBank::longword(originalRecord + 16), length) == 0,
                "all selected planar payload bytes preserved");
        }
        require(borders == 4, "both border frames from both selected areas survive sorting");
        require(titles == 1, "exactly one stage title is banked");
        for (unsigned id : offers) {
            const unsigned wanted = downwell::levelUpgradeNameLabel(id, id < 23 ? owned[id] : 0);
            unsigned found = 0;
            for (unsigned i = 0; i < count; ++i)
                found += AmigaClearBank::word(target + 20 + i * 24) == wanted;
            require(found == 1, "exact owned-name variant banked once");
        }
        for (unsigned i = 0; i < 16; ++i)
            require(storage[i] == 0xa5 && storage[storage.size() - 1 - i] == 0xa5, "adjacent staging/HUD guards intact");
    }
    const auto before = storage;
    unsigned char invalidOwned[23] = {};
    invalidOwned[0] = 2;
    const unsigned ownedOffer[] = {0};
    require(!AmigaClearBank::build(source.data(), source.size(), ownedOffer, 1, target, 32140, 1, 1, invalidOwned) &&
        storage == before, "unsupported ownership level fails instead of displaying an incorrect name");
    unsigned bad[] = {99}, fallback[] = {100, 100, 100, 100};
    require(!AmigaClearBank::build(source.data(), source.size(), bad, 1, target, 32140) &&
        !AmigaClearBank::build(source.data(), 19, fallback, 4, target, 32140) &&
        !AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 1) &&
        !AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 32140, 0) &&
        !AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 32140, 5) &&
        !AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 32140, 1, 4) &&
        !AmigaClearBank::build(source.data(), source.size(), fallback, 5, target, 32140) && storage == before,
        "rejected requests leave existing bank untouched");
    require(AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 32140) &&
        AmigaClearBank::longword(target + 8) == 9, "four fallback offers share one bundle alongside four borders");
    require(source == original, "source pack untouched by bank construction");
    const auto validBank = storage;
    for (unsigned j = 0; j < AmigaClearBank::longword(source.data() + 8); ++j) {
        auto* record = source.data() + 20 + j * 24;
        if (AmigaClearBank::word(record) == 64500) {
            AmigaClearBank::putLong(record + 16, 0xffffffffu);
            break;
        }
    }
    require(!AmigaClearBank::build(source.data(), source.size(), fallback, 4, target, 32140) && storage == validBank,
        "out-of-range payload offset is rejected before any destination write");
    std::printf("256 clear banks verified; largest sampled bank %u bytes\n", maximum);
}

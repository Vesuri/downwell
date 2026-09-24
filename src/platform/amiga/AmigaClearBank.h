#ifndef DOWNWELL_AMIGA_CLEAR_BANK_H
#define DOWNWELL_AMIGA_CLEAR_BANK_H
#include <cstdint>
#include "core/LevelUpgradeArt.h"
// Copy selected already-planar records from the Fast/host pack to a bounded
// Chip bank. Caller owns DMA/blitter retirement; source and target must not
// overlap. All validation completes before the first destination write.
struct AmigaClearBank {
    static unsigned word(const std::uint8_t* p) { return (unsigned(p[0]) << 8) | p[1]; }
    static std::uint32_t longword(const std::uint8_t* p) {
        return (std::uint32_t(word(p)) << 16) | word(p + 2);
    }
    static void putLong(std::uint8_t* p, std::uint32_t value) {
        p[0] = value >> 24; p[1] = value >> 16; p[2] = value >> 8; p[3] = value;
    }
    static unsigned build(const std::uint8_t* source, unsigned size,
            const unsigned* offers, unsigned offerCount,
            std::uint8_t* target, unsigned capacity, unsigned area = 1) {
        if (!source || !target || (offerCount && !offers) || offerCount > 4 || size < 20 ||
            longword(source) != 0x44574150u || word(source + 4) != 2 || word(source + 6) != 20)
            return 0;
        if (area < 1 || area > 4) return 0;
        const unsigned sourceCount = longword(source + 8);
        if (sourceCount > (size - 20) / 24) return 0;
        unsigned ids[18] = {64000, 64001}, frames[18] = {}, count = 2;
        for (unsigned selectedArea = area; selectedArea <= area + 1; ++selectedArea) {
            ids[count] = downwell::levelBorderSprite(selectedArea); frames[count++] = 7;
            ids[count] = downwell::levelBorderSprite(selectedArea); frames[count++] = 13;
        }
        for (unsigned i = 0; i < offerCount; ++i) {
            const unsigned sprite = downwell::levelUpgradeSprite(offers[i]);
            if (!sprite) return 0;
            const unsigned bundle[] = {sprite, 64100 + offers[i], 64300 + offers[i]};
            for (unsigned id : bundle) {
                bool found = false;
                for (unsigned j = 0; j < count; ++j) found |= ids[j] == id && frames[j] == 0;
                if (!found) ids[count++] = id;
            }
        }
        const std::uint8_t* records[18] = {};
        unsigned bytes = 20 + count * 24;
        if (bytes > capacity) return 0;
        for (unsigned i = 0; i < count; ++i) {
            for (unsigned j = 0; j < sourceCount; ++j) {
                const auto* record = source + 20 + j * 24;
                if (word(record) == ids[i] && word(record + 2) == frames[i]) {
                    if (records[i]) return 0; // ambiguous duplicate resource/frame
                    records[i] = record;
                }
            }
            const auto* record = records[i];
            if (!record || word(record + 14) != 3) return 0;
            const unsigned offset = longword(record + 16), length = longword(record + 20);
            const unsigned width = word(record + 8), height = word(record + 10), strideBits = word(record + 12);
            if (!width || !height || strideBits < width || (strideBits & 15) ||
                length != (strideBits / 8) * height * 3u ||
                offset < 20 + sourceCount * 24 || offset > size || length > size - offset ||
                length > capacity - bytes) return 0;
            bytes += length;
        }
        for (unsigned i = 0; i < 20; ++i) target[i] = source[i];
        putLong(target + 8, count);
        unsigned cursor = 20 + count * 24;
        for (unsigned i = 0; i < count; ++i) {
            const auto* record = records[i];
            auto* output = target + 20 + i * 24;
            for (unsigned j = 0; j < 24; ++j) output[j] = record[j];
            const unsigned offset = longword(record + 16), length = longword(record + 20);
            putLong(output + 16, cursor);
            for (unsigned j = 0; j < length; ++j) target[cursor + j] = source[offset + j];
            cursor += length;
        }
        return bytes;
    }
};
#endif

#ifndef DOWNWELL_AMIGA_CLEAR_BANK_H
#define DOWNWELL_AMIGA_CLEAR_BANK_H
#include <cstdint>
#include <cstring>
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
            std::uint8_t* target, unsigned capacity, unsigned area = 1, unsigned level = 1,
            const unsigned char* owned = nullptr, bool outlined = false,
            bool preparedHeadings = false) { // optional 23-entry source inventory
        if (!source || !target || (offerCount && !offers) || offerCount > 4 || size < 20 ||
            longword(source) != 0x44574150u || word(source + 4) != 2 || word(source + 6) != 20)
            return 0;
        const unsigned stageLabel = downwell::levelClearStageLabel(area, level);
        if (!stageLabel) return 0;
        const unsigned sourceCount = longword(source + 8);
        if (sourceCount > (size - 20) / 24) return 0;
        // Exported DWAP directories are strictly ordered. Validate once,
        // then merge the selected keys instead of rescanning the full pack
        // for every title, icon and text variant on a 68000.
        for (unsigned i = 1; i < sourceCount; ++i)
            if (longword(source + 20 + (i-1)*24) >= longword(source + 20 + i*24)) return 0;
        unsigned ids[30] = {stageLabel, 64001}, frames[30] = {}, count = 2;
        if (preparedHeadings) {
            if (!outlined) return 0;
            const unsigned titles[] = {stageLabel, 64001u};
            for (unsigned id : titles)
                for (unsigned frame = 2; frame <= 7; ++frame) {
                    ids[count] = id; frames[count++] = frame;
                }
        }
        for (unsigned selectedArea = area; selectedArea <= area + 1; ++selectedArea) {
            ids[count] = downwell::levelBorderSprite(selectedArea); frames[count++] = 7;
            ids[count] = downwell::levelBorderSprite(selectedArea); frames[count++] = 13;
        }
        for (unsigned i = 0; i < offerCount; ++i) {
            const unsigned sprite = downwell::levelUpgradeSprite(offers[i]);
            if (!sprite) return 0;
            const unsigned name = downwell::levelUpgradeNameLabel(offers[i],
                owned && offers[i] < 23 ? owned[offers[i]] : 0);
            if (!name) return 0;
            const unsigned bundle[] = {sprite, name, 64300 + offers[i]};
            for (unsigned id : bundle) {
                for (unsigned frame = preparedHeadings && id >= 64000 ? 1u : 0u;
                     frame < (outlined && id >= 64000 ? 2u : 1u); ++frame) {
                    bool found = false;
                    for (unsigned j = 0; j < count; ++j) found |= ids[j] == id && frames[j] == frame;
                    if (!found) { ids[count] = id; frames[count++] = frame; }
                }
            }
        }
        // Renderer lookup is binary search over (resource ID, frame), not
        // selection order. Sort the small directory before locating payloads.
        for (unsigned i = 1; i < count; ++i) {
            const unsigned id = ids[i], frame = frames[i];
            unsigned j = i;
            while (j && (ids[j - 1] > id || (ids[j - 1] == id && frames[j - 1] > frame))) {
                ids[j] = ids[j - 1]; frames[j] = frames[j - 1]; --j;
            }
            ids[j] = id; frames[j] = frame;
        }
        const std::uint8_t* records[30] = {};
        unsigned bytes = 20 + count * 24;
        if (bytes > capacity) return 0;
        unsigned sourceIndex = 0;
        for (unsigned i = 0; i < count; ++i) {
            const std::uint32_t key = (std::uint32_t(ids[i]) << 16) | frames[i];
            while (sourceIndex < sourceCount && longword(source + 20 + sourceIndex*24) < key) ++sourceIndex;
            if (sourceIndex < sourceCount && longword(source + 20 + sourceIndex*24) == key)
                records[i] = source + 20 + sourceIndex*24;
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
        std::memcpy(target, source, 20);
        putLong(target + 8, count);
        unsigned cursor = 20 + count * 24;
        for (unsigned i = 0; i < count; ++i) {
            const auto* record = records[i];
            auto* output = target + 20 + i * 24;
            std::memcpy(output, record, 24);
            const unsigned offset = longword(record + 16), length = longword(record + 20);
            putLong(output + 16, cursor);
            std::memcpy(target + cursor, source + offset, length);
            cursor += length;
        }
        return bytes;
    }
};
#endif

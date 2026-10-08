#ifndef DOWNWELL_AMIGA_CLEAR_SCENE_ASSETS_H
#define DOWNWELL_AMIGA_CLEAR_SCENE_ASSETS_H
#include "AmigaClearBank.h"
#include "AmigaExitMask.h"
#include "GameplaySceneStorage.h"

// Clear scenes retire ALL gameplay DMA first: a terrain display slot may have
// rotated into the garden staging region. Only then may its bytes become the
// immutable departure mask. Keep relocated HUD artwork untouched for reentry.
// The two world displays and pristine terrain backing live outside this bank;
// their subsequent ownership is the presenter's responsibility.
struct AmigaClearSceneAssets {
#ifdef DOWNWELL_REFERENCE_CLEAR_HEADINGS
    static constexpr bool preparedHeadings = false;
#else
    static constexpr bool preparedHeadings = true;
#endif
    GameplaySceneStorage layout;
    unsigned bankBytes = 0;
    static GameplaySceneStorage partition(unsigned retiredBytes, unsigned hudBytes) {
        return GameplaySceneStorage::partition(retiredBytes, AmigaExitMask::bytes, hudBytes);
    }
    // Source packs must not overlap retiredStorage. Caller drains the blitter
    // and retires copper/sprite DMA before entry, and before changing offers.
    // No allocation, pixel conversion, or writes on validation failure.
    bool prepare(std::uint8_t* retiredStorage, unsigned retiredBytes, unsigned hudBytes,
                 const std::uint8_t* art, unsigned artBytes,
                 const std::uint8_t* mask, unsigned maskBytes,
                 const unsigned* offers, unsigned offerCount, unsigned area, unsigned level,
                 const unsigned char* owned = nullptr) {
        const auto proposed = partition(retiredBytes, hudBytes);
        if (!retiredStorage || !proposed.valid || !mask ||
            (maskBytes != AmigaExitMask::bytes && maskBytes != AmigaExitMask::selectionBytes)) return false;
        const unsigned bytes = AmigaClearBank::build(art, artBytes, offers, offerCount,
            retiredStorage + proposed.resultsOffset, proposed.resultsCapacity, area, level, owned, true, preparedHeadings);
        if (!bytes) return false;
        for (unsigned i = 0; i < maskBytes; ++i) retiredStorage[i] = mask[i];
        layout = proposed; bankBytes = bytes;
        return true;
    }
    bool changeOffers(std::uint8_t* retiredStorage, const std::uint8_t* art, unsigned artBytes,
                      const unsigned* offers, unsigned offerCount, unsigned area, unsigned level,
                      const unsigned char* owned = nullptr) {
        if (!retiredStorage || !layout.valid || !bankBytes) return false;
        const unsigned bytes = AmigaClearBank::build(art, artBytes, offers, offerCount,
            retiredStorage + layout.resultsOffset, layout.resultsCapacity, area, level, owned, true, preparedHeadings);
        if (!bytes) return false;
        bankBytes = bytes;
        return true;
    }
};
#endif

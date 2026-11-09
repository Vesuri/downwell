#ifndef DOWNWELL_CORE_COMPACT_SHOP_SPEECH_H
#define DOWNWELL_CORE_COMPACT_SHOP_SPEECH_H
#include "CompactGuns.h"
#include "ShopFeedback.h"
#include "ShopText.h"
namespace downwell {
// scrRisingTextShop(msgx, keeper.y - 18, text): Shop Step destroys the
// shop's previous fxRisingText (212) and creates another. fxRisingText has
// no sprite, so its Collision with fxRisingText never fires; it rises with
// ysp *= 0.85, flashes after Alarm 1 (60) and Alarm 0 (90) destroys it.
// Object 212 steps before Shop 277: a new record first moves next update.
// Strings borrow the configured language table.
struct CompactShopSpeech {
    CompactItemNoticeRecord rise;
    const char* text = nullptr;
    int length = 0;
    // fxRisingText Draw: string_width (font 2: eight pixels per character)
    // centred on x, kept ten pixels inside the view.
    int left(int viewLeft, int viewWidth) const {
        const int width = 8 * length;
        int xxx = rise.x;
        const int over = rise.x + width / 2 - (viewLeft + viewWidth);
        const int under = rise.x - width / 2 - viewLeft;
        if (over > -10) xxx = rise.x - over - 10;
        else if (under < 10) xxx = rise.x - under + 10;
        return xxx;
    }
};

// scrShopAssignText's speech strings (logMaido, logWelcome/Hello/Hoho,
// the shop's logThank, logPoor, logWhat, logHuh) for one language.
struct CompactShopSpeechText {
    const char* sold = nullptr;
    const char* greeting[3]{};
    const char* bye[3]{};
    const char* poor = nullptr;
    const char* what = nullptr;
    const char* huh = nullptr;
    bool configure(const ShopLanguageEntry* entries, unsigned count, const char* language) {
        *this = {};
        if (!entries || !language) return false;
        const ShopLanguageLookup lookup{entries, count, language};
        return lookup.lookup("shopSold", sold) && lookup.lookup("shopGreet0", greeting[0]) &&
            lookup.lookup("shopGreet1", greeting[1]) && lookup.lookup("shopGreet2", greeting[2]) &&
            lookup.lookup("shopBye0", bye[0]) && lookup.lookup("shopBye1", bye[1]) &&
            lookup.lookup("shopBye2", bye[2]) && lookup.lookup("shopPoor", poor) &&
            lookup.lookup("shopWhat0", what) && lookup.lookup("shopWhat1", huh);
    }
    // The Step's literal "?" for the first sold-out reply.
    const char* text(ShopFeedback::Message message, unsigned farewell) const {
        using M = ShopFeedback::Message;
        switch (message) {
        case M::welcome: return greeting[0];
        case M::hello: return greeting[1];
        case M::hoho: return greeting[2];
        case M::maido: return sold;
        case M::poor: return poor;
        case M::question: return "?";
        case M::what: return what;
        case M::huh: return huh;
        case M::thank: return farewell < 3 ? bye[farewell] : nullptr;
        case M::none: break;
        }
        return nullptr;
    }
};
}
#endif

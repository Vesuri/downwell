#ifndef DOWNWELL_CORE_SHOP_INVENTORY_H
#define DOWNWELL_CORE_SHOP_INVENTORY_H
#include "CurrencyAccount.h"
namespace downwell {
// Shop Create/Step stock and transaction rules. IDs are original ug IDs.
// Interaction, merchant responses, case destruction and deferred upgrade
// application belong to the caller, not to a platform-specific store model.
struct ShopInventory {
    struct Item { unsigned upgrade = 100, remaining = 1, price = 0; };
    Item items[3];
    enum class Purchase { notAnItem, soldOut, insufficientFunds, bought };

    static unsigned roundedTenth(unsigned value) {
        const unsigned whole = value / 10, remainder = value % 10;
        return whole + (remainder > 5 || (remainder == 5 && (whole & 1)));
    }
    // Source domain: consumables 100..105 and area >= 1.
    static unsigned price(unsigned upgrade, unsigned area, bool hardMode,
                          unsigned style, bool member) {
        constexpr unsigned base[] = {300, 150, 500, 250, 400, 1000};
        unsigned value = base[upgrade - 100] + 200 * (area - 1);
        if (hardMode) value *= 2;
        if (style == 4) value -= roundedTenth(value) * 3;
        if (member) value -= roundedTenth(value);
        return value;
    }
    // random(lo,hi) must implement inclusive irandom_range and advance the
    // shared game stream. Do not replace rejection sampling with a shuffle.
    template<class Range>
    void create(Range random, int health, unsigned area = 1, bool hardMode = false,
                unsigned style = 0, bool member = false) {
        for (auto& item : items) item = {};
        items[0].upgrade = random(100, 101);
        if (health <= 2) items[0].upgrade = 100;
        do { items[1].upgrade = random(100, 103); }
        while (items[1].upgrade == items[0].upgrade);
        items[2].upgrade = random(102, 105); // overwritten by the source's while(true)
        do { items[2].upgrade = random(102, 105); }
        while (items[2].upgrade == items[1].upgrade || items[2].upgrade == items[0].upgrade);
        for (auto& item : items)
            item.price = price(item.upgrade, area, hardMode, style, member);
    }
    // Cursor zero is Leave. Source checks stock before affordability and runs
    // scrGainUg synchronously before charging (it may itself add currency).
    template<class Grant>
    Purchase purchase(unsigned cursor, CurrencyAccount& account, Grant grant) {
        if (cursor < 1 || cursor > 3) return Purchase::notAnItem;
        auto& item = items[cursor - 1];
        if (!item.remaining) return Purchase::soldOut;
        if (account.currency < item.price) return Purchase::insufficientFunds;
        --item.remaining;
        grant(item.upgrade);
        account.currency -= item.price;
        return Purchase::bought;
    }
};
}
#endif

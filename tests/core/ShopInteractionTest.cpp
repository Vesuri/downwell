#include "core/ShopInteraction.h"
#include "core/Consumables.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
using Menu = downwell::ShopInteraction;
using Effect = Menu::Effect;
static void require(bool ok, const char* why) {
    if (!ok) { std::fprintf(stderr, "%s\n", why); std::abort(); }
}
struct Fixture {
    Menu menu;
    Menu::Context context{true,true,true,false,false};
    downwell::ShopInventory stock;
    downwell::CurrencyAccount money;
    downwell::Consumables items;
    bool locked = false;
    int facing = 1;
    unsigned choices = 0, nextChoice = 1;
    std::vector<Effect> effects;
    Fixture() {
        stock.items[0] = {100,1,300};
        stock.items[1] = {101,1,150};
        stock.items[2] = {105,1,1000};
        money.currency = 1000;
    }
    Menu::Input step(Menu::Input input = {}) {
        effects.clear();
        menu.step(input, context, locked, facing, stock, money,
            [&] { ++choices; return nextChoice; },
            [&](unsigned id) { require(menu.thank, "thank latches before grant"); items.grant(id, money); },
            [&](Effect effect) { effects.push_back(effect); });
        return input;
    }
};
int main() {
    Fixture f;
    f.context.interactable = false;
    require(f.step({false,false,true}).confirm && !f.locked && f.choices == 0,
            "ineligible confirmation is neither consumed nor randomized");
    f.context.interactable = true;
    require(!f.step({false,false,true}).confirm && f.locked && f.menu.active &&
            f.menu.cursor == 1 && f.facing == -1 && f.choices == 1 && f.money.currency == 1000,
            "opening faces merchant, locks controls and consumes confirmation before purchase");
    require(f.effects == std::vector<Effect>{Effect::opened,Effect::welcome}, "opening effect order");
    f.step();
    require(f.effects.empty() && f.items.pending[0] == 0, "no input edge means no repeat");
    auto input = f.step({false,false,true,true});
    require(input.confirm && !input.cancel && f.menu.active && f.money.currency == 700 &&
            f.stock.items[0].remaining == 0 && f.items.pending[0] == 1 && f.choices == 1,
            "buy queues grant and suppresses simultaneous cancel without consuming confirm");
    require(f.effects == std::vector<Effect>{Effect::purchased}, "purchase feedback");
    f.nextChoice = 2;
    f.step({false,false,true});
    require(f.effects == std::vector<Effect>{Effect::what} && f.choices == 2 && f.money.currency == 700,
            "sold-out response alone draws random choice");
    f.step({true,true});
    require(f.menu.cursor == 1 && f.menu.cursorWobbleX == -6 &&
            f.effects == std::vector<Effect>{Effect::moveRight,Effect::moveLeft},
            "simultaneous directions are independent right-then-left branches");
    f.menu.cursor = 3;
    f.step({false,false,true});
    require(f.effects == std::vector<Effect>{Effect::poor} && f.choices == 2 &&
            f.stock.items[2].remaining == 1, "insufficient funds does not consume stock or RNG");
    f.step({true,false});
    require(f.menu.cursor == 0, "right wraps last item to Leave");
    f.step({false,false,true});
    require(!f.menu.active && !f.menu.speaking && !f.menu.leave && !f.locked && !f.menu.thank &&
            f.effects == std::vector<Effect>{Effect::thank}, "Leave thanks once and releases controls");
    f.context.playerOnRight = false;
    f.step({false,false,true});
    require(f.facing == 1 && !f.menu.leave && f.stock.items[0].remaining == 0,
            "reentry preserves sold stock and the source leave flag until next speaking update");
    f.step({false,false,false,true});
    require(!f.locked && f.effects == std::vector<Effect>{Effect::cancel}, "cancel without new purchase uses cancel feedback");
    Fixture legacy;
    legacy.menu.active = legacy.menu.speaking = legacy.locked = true;
    legacy.step({true,false,true});
    require(legacy.menu.menuState == 1 && legacy.menu.cursor == 2 &&
            legacy.effects == std::vector<Effect>{Effect::moveRight,Effect::browse,Effect::moveRight},
            "source state-zero transition also executes state-one direction branch");
    Fixture dead;
    dead.step({false,false,true});
    dead.context.dead = true;
    dead.step({false,false,true});
    require(dead.money.currency == 700 && !dead.menu.active && dead.menu.leave && dead.locked &&
            dead.effects == std::vector<Effect>{Effect::purchased,Effect::disableMask},
            "source death invalidation follows interaction and does not invent control release");
    Fixture criminal;
    criminal.context.criminal = true;
    criminal.step({false,false,true});
    require(!criminal.locked && criminal.choices == 0 &&
            criminal.effects == std::vector<Effect>{Effect::disableMask}, "criminal skips interaction entirely");
    std::puts("Shared shop interaction, input consumption and transaction ordering passed");
}

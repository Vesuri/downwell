#ifndef DOWNWELL_CORE_SHOP_WORLD_H
#define DOWNWELL_CORE_SHOP_WORLD_H
#include "ShopInteraction.h"
#include "ShopFeedback.h"
#include "Input.h"
namespace downwell {
// Bounded world ownership and static child masks, separate from shop rendering.
// Coordinates passed here are integer mask-query positions. Full fractional
// GameMaker collision semantics remain a separate runtime audit.
struct ShopWorld {
    static constexpr unsigned capacity = 8;
    struct Instance {
        int x = 0, y = 0;
        bool allocated = false, wallPresent = false;
        bool present = false, maskEnabled = true, interactOk = true;
        ShopInventory stock;
        ShopInteraction menu;
        ShopFeedback feedback;
    };
    Instance instances[capacity];
    unsigned allocatedCount = 0;
    unsigned fixtureRevision = 0;
    bool noControl = false, interactable = false;
    Input previous;

    template<class Range>
    Instance* create(int x, int y, Range random, int health, unsigned area,
                     bool hard = false, unsigned style = 0, bool member = false) {
        for (auto& shop : instances) if (!shop.allocated) {
            shop = {};
            shop.x = x; shop.y = y; shop.present = true;
            shop.allocated = shop.wallPresent = true;
            ++allocatedCount;
            ++fixtureRevision;
            shop.stock.create(random, health, area, hard, style, member);
            return &shop;
        }
        return nullptr; // Caller must handle exhaustion; do not consume RNG.
    }
    static bool meets(const Instance& shop, int x, int y) {
        // sprShopCounter (480), origin 12,12, margins 2..11 / 11..20;
        // sprPlayerIdle mask -4..3 / -4..7.
        return shop.present && shop.maskEnabled &&
            x + 3 >= shop.x - 10 && x - 4 <= shop.x - 1 &&
            y + 7 >= shop.y - 1 && y - 4 <= shop.y + 8;
    }
    struct Rectangle { int left, top, right, bottom; };
    template<class Visit>
    void visitPlayerSolids(Visit visit) const {
        if (!allocatedCount) return; // Ordinary first-level queries stay cheap.
        for (const auto& shop : instances) if (shop.allocated) {
            // Shop Create children, not the non-solid counter sprite:
            // table (278/481): origin 32,8; margins 2..61,8..15.
            visit(Rectangle{shop.x + 34, shop.y, shop.x + 93, shop.y + 7});
            // cases (279/482): origin 12,12; margins 3..20,3..20.
            // Buying removes contents only, so sold cases remain solid.
            for (int i = 0; i < 3; ++i) {
                const int caseX = shop.x + 45 + i * 19;
                visit(Rectangle{caseX - 9, shop.y - 18, caseX + 8, shop.y - 1});
            }
            // Invisible wall (98/46), at counter y+8, full 16x16 mask.
            if (shop.wallPresent)
                visit(Rectangle{shop.x - 8, shop.y, shop.x + 7, shop.y + 15});
        }
    }
    bool playerSolidAt(int x, int y) const {
        bool hit = false;
        visitPlayerSolids([&](Rectangle r) {
            if (x >= r.left && x <= r.right && y >= r.top && y <= r.bottom) hit = true;
        });
        return hit;
    }
    int playerLanding(int x, int minimumY, int nearest) const {
        visitPlayerSolids([&](Rectangle r) {
            const int landing = r.top - 8;
            if (landing >= minimumY && landing < nearest && x + 3 >= r.left && x - 4 <= r.right)
                nearest = landing;
        });
        return nearest;
    }
    void observePlayer(int x, int y, bool grounded, long speedX) {
        interactable = false;
        if (!grounded || speedX) return;
        for (const auto& shop : instances) if (meets(shop, x, y)) {
            interactable = shop.interactOk;
            return;
        }
    }
    template<class Choose, class Grant, class Emit>
    void step(const Input& held, int x, int y, bool criminal, bool dead,
              int& facing, CurrencyAccount& currency, Choose choose,
              Grant grant, Emit emit) {
        ShopInteraction::Input edges{held.right && !previous.right,
            held.left && !previous.left, held.fire && !previous.fire, false};
        previous = held; // Always track raw edges, including while locked.
        for (auto& shop : instances) if (shop.present) {
            shop.feedback.beginStep();
            const ShopInteraction::Context context{meets(shop, x, y),
                interactable, x > shop.x, criminal, dead};
            shop.menu.step(edges, context, noControl, facing, shop.stock,
                currency, choose, grant, [&](ShopInteraction::Effect effect) {
                    if (effect == ShopInteraction::Effect::disableMask)
                        shop.maskEnabled = false;
                    shop.feedback.apply(effect, shop.menu.cursor, shop.stock);
                    emit(shop, effect);
                });
        }
    }
    void endStep(int viewY) {
        for (auto& shop : instances) {
            if (shop.present && (shop.y < viewY - 320 || shop.y > viewY + 3000)) {
                shop.present = false;
                ++fixtureRevision;
            }
            if (shop.wallPresent && (shop.y + 8 < viewY - 160 || shop.y + 8 > viewY + 3000))
                shop.wallPresent = false;
        }
        // Table/cases have no inherited retirement Step; keep them until room
        // teardown even when the counter retires. Do not recycle their owner.
        // Source destruction does not independently release global.noControl.
    }
};
}
#endif

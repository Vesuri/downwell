#ifndef DOWNWELL_CORE_SHOP_INTERACTION_H
#define DOWNWELL_CORE_SHOP_INTERACTION_H
#include "ShopInventory.h"
namespace downwell {
// Shop Step's interaction branches. Caller supplies source input EDGES and
// collision/interactable predicates; radio, visibility and merchant animation
// remain world/presentation responsibilities. Effects are emitted in VM order.
struct ShopInteraction {
    struct Input { bool right = false, left = false, confirm = false, cancel = false; };
    struct Context {
        bool meeting = false, interactable = false, playerOnRight = false;
        bool criminal = false, dead = false;
    };
    enum class Effect {
        opened, welcome, hello, hoho, moveRight, moveLeft, browse,
        purchased, poor, question, what, huh, thank, cancel, disableMask
    };
    bool speaking = false, active = false, leave = true, thank = false;
    bool exitRequested = false;
    unsigned menuState = 0, menuSelect = 0, cursor = 1;
    int cursorWobbleX = 0;

    template<class Emit>
    void requestExit(Emit emit) {
        exitRequested = true;
        if (thank) { emit(Effect::thank); thank = false; }
        else emit(Effect::cancel);
    }
    // choose3() must implement source choose(1,2,3) on the shared RNG stream.
    // Grant is synchronous scrGainUg, normally Consumables::grant for stock IDs.
    template<class Choose, class Grant, class Emit>
    void step(Input& input, const Context& context, bool& noControl, int& facing,
              ShopInventory& stock, CurrencyAccount& currency,
              Choose choose3, Grant grant, Emit emit) {
        if (!context.criminal) {
            if (!speaking) {
                if (context.meeting && context.interactable && input.confirm) {
                    facing = context.playerOnRight ? -1 : 1;
                    speaking = active = noControl = true;
                    cursor = menuSelect = menuState = 1;
                    emit(Effect::opened); // keeper thankful=0, goodbye=0
                    const unsigned greeting = choose3();
                    emit(greeting == 1 ? Effect::welcome : greeting == 2 ? Effect::hello : Effect::hoho);
                    input.confirm = false;
                }
            } else leave = true;
            if (active) {
                if (menuState == 0) {
                    if (input.right) {
                        menuSelect = menuSelect < 1 ? menuSelect + 1 : 0;
                        cursorWobbleX = 6; emit(Effect::moveRight);
                    }
                    if (input.left) {
                        menuSelect = menuSelect > 0 ? menuSelect - 1 : 1;
                        cursorWobbleX = -6; emit(Effect::moveLeft);
                    }
                    if (input.confirm) {
                        if (menuSelect > 0) { menuState = menuSelect; emit(Effect::browse); }
                        else requestExit(emit);
                        cursor = 1;
                        input.confirm = false;
                    }
                }
                // Deliberately independent: entering state 1 above can process
                // the same direction edges again in this update, as in the VM.
                if (menuState == 1) {
                    if (input.right) {
                        cursor = cursor < 3 ? cursor + 1 : 0;
                        cursorWobbleX = 6; emit(Effect::moveRight);
                    }
                    if (input.left) {
                        cursor = cursor > 0 ? cursor - 1 : 3;
                        cursorWobbleX = -6; emit(Effect::moveLeft);
                    }
                    if (input.confirm) {
                        if (!cursor) requestExit(emit);
                        else {
                            const auto result = stock.purchase(cursor, currency, [&](unsigned id) {
                                thank = true;
                                grant(id);
                            });
                            if (result == ShopInventory::Purchase::bought) emit(Effect::purchased);
                            else if (result == ShopInventory::Purchase::insufficientFunds) emit(Effect::poor);
                            else if (result == ShopInventory::Purchase::soldOut) {
                                const unsigned reply = choose3();
                                emit(reply == 1 ? Effect::question : reply == 2 ? Effect::what : Effect::huh);
                            }
                        }
                        input.cancel = false;
                    }
                    if (input.cancel) requestExit(emit);
                }
            }
        }
        if (context.criminal || context.dead) {
            active = false;
            leave = true;
            emit(Effect::disableMask);
            // This source branch does NOT itself release global.noControl.
        }
        if (exitRequested) {
            active = noControl = leave = speaking = false;
            menuSelect = menuState = 0;
            exitRequested = false;
        }
    }
};
}
#endif

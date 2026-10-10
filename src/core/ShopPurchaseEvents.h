#ifndef DOWNWELL_CORE_SHOP_PURCHASE_EVENTS_H
#define DOWNWELL_CORE_SHOP_PURCHASE_EVENTS_H
#include "ShopInventory.h"
namespace downwell {
// Shop Step blocks 73..80, after confirmation selected an item. Invoke
// inside each source with(Shop) body, not once for a whole room.
// A rejected synchronous operation keeps its already-published prefix.
struct ShopPurchaseEvents {
    template<class Parent,class Ops>
    static bool buy(Parent& shop,CurrencyAccount& currency,Ops& ops,
                    ShopInventory::Purchase& result){
        using Purchase=ShopInventory::Purchase;
        result=Purchase::notAnItem;
        auto selected=[&]{return shop.menu.cursor>=1&&shop.menu.cursor<=3;};
        if(!selected())return true;
        if(!shop.stock.items[shop.menu.cursor-1].remaining){result=Purchase::soldOut;return true;}
        if(currency.currency<shop.stock.items[shop.menu.cursor-1].price){
            result=Purchase::insufficientFunds;return true;
        }
        shop.menu.thank=true;
        --shop.stock.items[shop.menu.cursor-1].remaining;
        if(!ops.grant(shop.stock.items[shop.menu.cursor-1].upgrade))return false;
        // scrGainUg may refund currency. Do not cache the balance, cursor,
        // price or remaining stock across that synchronous script call.
        // Negative balances are outside CurrencyAccount's admitted domain;
        // fail rather than silently wrap them into a large unsigned balance.
        if(!selected()||currency.currency<shop.stock.items[shop.menu.cursor-1].price)return false;
        currency.currency-=shop.stock.items[shop.menu.cursor-1].price;
        if(!shop.stock.items[shop.menu.cursor-1].remaining){
            if(!ops.clearCase(shop.children.cases[shop.menu.cursor-1]))return false;
            if(!selected()||!ops.destroyItem(shop.children.items[shop.menu.cursor-1]))return false;
        }
        if(!ops.thankKeeper(shop.children.keeper)||!ops.resetKeeper(shop.children.keeper)||
           !ops.maido(shop.children.keeper)||!ops.sound(104,90,"merchant")||!ops.sound(105,50,"UI"))return false;
        result=Purchase::bought;return true;
    }
};
}
#endif

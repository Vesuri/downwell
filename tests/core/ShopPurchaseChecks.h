#ifndef DOWNWELL_TEST_SHOP_PURCHASE_CHECKS_H
#define DOWNWELL_TEST_SHOP_PURCHASE_CHECKS_H
#include "core/ShopWorld.h"
#include "core/ShopPurchaseEvents.h"
inline bool shopPurchaseTextEqual(const char* a,const char* b){
    while(*a&&*a==*b){++a;++b;}return *a==*b;
}
struct ShopPurchaseProbe {
    downwell::ShopWorld::Instance& shop;downwell::CurrencyAccount& currency;
    int keeperY;unsigned calls=0;
    bool grant(unsigned id){
        if(calls++||!shop.menu.thank||shop.stock.items[0].remaining||id!=shop.stock.items[0].upgrade||
           currency.currency!=10000)return false;
        currency.currency+=100;return true;
    }
    bool maido(downwell::MotionBinary64 y){
        const auto expected=downwell::MotionBinary64::fromInteger(keeperY-18);
        return calls++==1&&y.high==expected.high&&y.low==expected.low&&
            currency.currency==10100-shop.stock.items[0].price;
    }
    bool sound(int id,int priority,int loop,int cut,const char* group){
        const bool merchant=calls==2;
        return calls++>=2&&id==(merchant?104:105)&&priority==(merchant?90:50)&&!loop&&cut==1&&
            shopPurchaseTextEqual(group,merchant?"merchant":"UI");
    }
};
inline bool checkShopPurchasePrefixes(){
    using namespace downwell;
    for(unsigned reject=0;reject<=8;++reject){
        ShopWorld::Instance shop;shop.stock.items[0]={100,1,300};
        shop.children.cases[0]=1;shop.children.items[0]=2;shop.children.keeper=3;
        CurrencyAccount account;account.currency=300;
        struct Ops {
            ShopWorld::Instance& shop;CurrencyAccount& currency;unsigned reject,calls=0;
            bool next(unsigned expected){return ++calls==expected&&calls!=reject;}
            bool grant(unsigned id){
                if(id!=100||!shop.menu.thank||shop.stock.items[0].remaining||currency.currency!=300)return false;
                currency.currency+=100;return next(1);
            }
            bool clearCase(unsigned id){return id==1&&currency.currency==100&&next(2);}
            bool destroyItem(unsigned id){return id==2&&next(3);}
            bool thankKeeper(unsigned id){return id==3&&next(4);}
            bool resetKeeper(unsigned id){return id==3&&next(5);}
            bool maido(unsigned id){return id==3&&next(6);}
            bool sound(int id,int priority,const char* group){
                const bool merchant=calls==6;
                return id==(merchant?104:105)&&priority==(merchant?90:50)&&
                    shopPurchaseTextEqual(group,merchant?"merchant":"UI")&&next(merchant?7:8);
            }
        } ops{shop,account,reject};
        ShopInventory::Purchase result;
        const bool ok=ShopPurchaseEvents::buy(shop,account,ops,result);
        if(ok!=!reject||ops.calls!=(reject?reject:8)||!shop.menu.thank||shop.stock.items[0].remaining||
           account.currency!=(reject==1?400u:100u)||
           (ok&&result!=ShopInventory::Purchase::bought))return false;
    }
    ShopWorld::Instance shop;shop.stock.items[0]={100,1,300};shop.stock.items[1]={101,1,150};
    shop.children.keeper=7;
    CurrencyAccount account;account.currency=300;
    struct Reentrant {
        ShopWorld::Instance& shop;CurrencyAccount& currency;unsigned calls=0;bool invalid=false;
        bool grant(unsigned id){
            ++calls;shop.menu.cursor=invalid?4:2;currency.currency+=100;return id==100;
        }
        bool clearCase(unsigned){return false;}
        bool destroyItem(unsigned){return false;}
        bool thankKeeper(unsigned id){++calls;return id==7&&currency.currency==250;}
        bool resetKeeper(unsigned id){++calls;return id==7;}
        bool maido(unsigned id){++calls;return id==7;}
        bool sound(int,int,const char*){++calls;return true;}
    } reentrant{shop,account};
    ShopInventory::Purchase result;
    if(!ShopPurchaseEvents::buy(shop,account,reentrant,result)||result!=ShopInventory::Purchase::bought||
       reentrant.calls!=6||shop.stock.items[0].remaining||shop.stock.items[1].remaining!=1||account.currency!=250)return false;
    // Gates do not call the grant or manufacture keeper/child effects.
    shop.menu.cursor=0;
    if(!ShopPurchaseEvents::buy(shop,account,reentrant,result)||result!=ShopInventory::Purchase::notAnItem)return false;
    shop.menu.cursor=1;account.currency=0;
    if(!ShopPurchaseEvents::buy(shop,account,reentrant,result)||result!=ShopInventory::Purchase::soldOut)return false;
    shop.menu.cursor=2;
    if(!ShopPurchaseEvents::buy(shop,account,reentrant,result)||result!=ShopInventory::Purchase::insufficientFunds||reentrant.calls!=6)return false;
    shop.menu.cursor=1;shop.stock.items[0].remaining=1;account.currency=300;reentrant.invalid=true;reentrant.calls=0;
    if(ShopPurchaseEvents::buy(shop,account,reentrant,result)||reentrant.calls!=1||account.currency!=400||shop.stock.items[0].remaining)return false;
    return true;
}
#endif

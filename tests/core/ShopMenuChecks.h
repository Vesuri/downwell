#ifndef DOWNWELL_TEST_SHOP_MENU_CHECKS_H
#define DOWNWELL_TEST_SHOP_MENU_CHECKS_H
#include "core/ShopMenuEvents.h"
struct ShopMenuProbe {
    using Message=downwell::ShopMenuEvents::Message;
    downwell::ShopWorld::Instance& shop;downwell::CurrencyAccount& currency;int keeperY;
    unsigned calls=0,reject=0,choice=1,says=0,sounds=0,grants=0;
    int facing=0,soundIds[16]{},wobbles[16]{};
    Message last=Message::question;bool locked=false,meet=true,eligible=true,onRight=true;
    bool event(){return ++calls!=reject;}
    bool meeting(bool& out){out=meet;return event();}
    bool interactable(bool& out){out=eligible;return event();}
    bool playerOnRight(bool& out){out=onRight;return event();}
    bool facePlayer(int value){facing=value;return event();}
    bool noControl(bool value){locked=value;return event();}
    bool choose3(unsigned& out){out=choice;return event();}
    bool say(Message message,downwell::MotionBinary64 y){
        last=message;++says;const auto expected=downwell::MotionBinary64::fromInteger(keeperY-18);
        return y.high==expected.high&&y.low==expected.low&&event();
    }
    bool sound(int id,int priority,int loop,int cut,const char* group){
        if(sounds==16||priority<50||loop||cut!=1||!group||!*group)return false;
        soundIds[sounds]=id;wobbles[sounds++]=shop.menu.cursorWobbleX;return event();
    }
    bool grant(unsigned id){
        ++grants;
        if(id!=shop.stock.items[shop.menu.cursor-1].upgrade||!shop.menu.thank||shop.stock.items[shop.menu.cursor-1].remaining)return false;
        currency.currency+=100;return event();
    }
};
inline bool checkShopMenuEvents(){
    using namespace downwell;
    struct Ops:ShopMenuProbe {
        int thankful=1,goodbye=1,confused=0,resets=0,purchases=0;
        ShopInventory::Purchase purchaseResult=ShopInventory::Purchase::bought;
        Ops(ShopWorld::Instance& s,CurrencyAccount& c):ShopMenuProbe{s,c,200}{}
        bool keeper(ShopMenuEvents::KeeperField f,int v){
            if(f==ShopMenuEvents::KeeperField::thankful)thankful=v;
            else if(f==ShopMenuEvents::KeeperField::goodbye)goodbye=v;
            else confused=v;
            return event();
        }
        bool keeperImageZero(){++resets;return event();}
        bool say(Message m){return ShopMenuProbe::say(m,MotionBinary64::fromInteger(182));}
        bool sound(int id,int priority,const char* group){return ShopMenuProbe::sound(id,priority,0,1,group);}
        bool purchase(ShopInventory::Purchase& result){++purchases;result=purchaseResult;return event();}
    };
    for(unsigned rejected=0;rejected<=10;++rejected){
        ShopWorld::Instance s;CurrencyAccount c;Ops ops{s,c};ops.reject=rejected;
        ShopInteraction::Input input;input.confirm=true;
        const bool ok=ShopMenuEvents::step(s.menu,input,ops);
        if(ok!=!rejected||ops.calls!=(rejected?rejected:10)||input.confirm!=bool(rejected)||
           s.menu.speaking!=(rejected==0||rejected>=5))return false;
    }
    for(unsigned choice=1;choice<=3;++choice){
        ShopWorld::Instance s;CurrencyAccount c;Ops ops{s,c};ops.choice=choice;
        ShopInteraction::Input input;input.confirm=true;
        const int greeting[]={101,95,96};
        if(!ShopMenuEvents::step(s.menu,input,ops)||input.confirm||!ops.locked||ops.facing!=-1||
           ops.thankful||ops.goodbye||ops.soundIds[0]!=greeting[choice-1]||ops.purchases)return false;
        // The same confirmation cannot open a second shop in the inner loop.
        ShopWorld::Instance next;Ops second{next,c};
        if(!ShopMenuEvents::step(next.menu,input,second)||next.menu.speaking||second.says)return false;
    }
    {
        ShopWorld::Instance s;CurrencyAccount c;Ops ops{s,c};
        s.menu.speaking=s.menu.active=true;s.menu.menuState=0;s.menu.menuSelect=0;
        ShopInteraction::Input input;input.right=input.confirm=true;
        if(!ShopMenuEvents::step(s.menu,input,ops)||s.menu.cursor!=2||input.confirm||ops.purchases||
           ops.sounds!=3||ops.soundIds[0]!=109||ops.soundIds[1]!=107||ops.soundIds[2]!=109||
           ops.wobbles[0]!=0||ops.wobbles[2]!=6)return false;
    }
    for(unsigned kind=0;kind<3;++kind){
        ShopWorld::Instance s;CurrencyAccount c;Ops ops{s,c};
        s.menu.speaking=s.menu.active=true;s.menu.menuState=1;
        ops.purchaseResult=kind==0?ShopInventory::Purchase::bought:kind==1?
            ShopInventory::Purchase::insufficientFunds:ShopInventory::Purchase::soldOut;
        ShopInteraction::Input input;input.confirm=input.cancel=true;
        if(!ShopMenuEvents::step(s.menu,input,ops)||!input.confirm||input.cancel||s.menu.exitRequested||ops.purchases!=1)return false;
        if(kind&&(!ops.confused||ops.resets!=1||ops.soundIds[0]!=(kind==1?100:98)))return false;
    }
    {
        ShopWorld::Instance s;CurrencyAccount c;Ops ops{s,c};
        s.menu.speaking=s.menu.active=s.menu.thank=true;s.menu.menuState=1;s.menu.cursor=0;
        ShopInteraction::Input input;input.confirm=true;
        if(!ShopMenuEvents::step(s.menu,input,ops)||!s.menu.exitRequested||s.menu.thank||
           !ops.goodbye||ops.resets!=1||ops.soundIds[0]!=99||!input.confirm)return false;
    }
    // A rejected navigation sound leaves the new selection, not the later jiggle.
    for(unsigned state=0;state<2;++state){
        ShopWorld::Instance s;CurrencyAccount c;Ops ops{s,c};ops.reject=1;
        s.menu.speaking=s.menu.active=true;s.menu.menuState=state;
        ShopInteraction::Input input;input.right=true;
        if(ShopMenuEvents::step(s.menu,input,ops)||s.menu.cursorWobbleX||
           (state?s.menu.cursor!=2:s.menu.menuSelect!=1))return false;
    }
    for(unsigned reject=1;reject<=4;++reject){
        ShopWorld::Instance s;CurrencyAccount c;Ops ops{s,c};ops.reject=reject;ops.goodbye=0;
        s.menu.speaking=s.menu.active=s.menu.thank=true;s.menu.menuState=1;s.menu.cursor=0;
        ShopInteraction::Input input;input.confirm=input.cancel=true;
        if(ShopMenuEvents::step(s.menu,input,ops)||ops.calls!=reject||!s.menu.exitRequested||
           s.menu.thank!=(reject<=2)||!input.confirm||!input.cancel||!s.menu.active)return false;
    }
    for(unsigned choice=1;choice<=3;++choice){
        ShopWorld::Instance s;CurrencyAccount c;Ops ops{s,c};ops.choice=choice;
        s.menu.speaking=s.menu.active=true;s.menu.menuState=1;
        ops.purchaseResult=ShopInventory::Purchase::soldOut;
        ShopInteraction::Input input;input.confirm=true;const int sounds[]={98,102,97};
        if(!ShopMenuEvents::step(s.menu,input,ops)||ops.soundIds[0]!=sounds[choice-1]||!ops.confused)return false;
    }
    {
        ShopWorld::Instance s;CurrencyAccount c;ShopInteraction::Input input;input.right=true;
        struct MutableInput:Ops {
            ShopInteraction::Input& input;
            MutableInput(ShopWorld::Instance& s,CurrencyAccount& c,ShopInteraction::Input& i):Ops(s,c),input(i){}
            bool sound(int id,int priority,const char* group){
                if(!Ops::sound(id,priority,group))return false;
                input.left=true;return true;
            }
        } ops{s,c,input};
        s.menu.speaking=s.menu.active=true;s.menu.menuState=1;
        if(!ShopMenuEvents::step(s.menu,input,ops)||s.menu.cursor!=1||s.menu.cursorWobbleX!=-6||ops.sounds!=2)return false;
    }
    return true;
}
#endif

#ifndef DOWNWELL_CORE_SHOP_MENU_EVENTS_H
#define DOWNWELL_CORE_SHOP_MENU_EVENTS_H
#include "ShopWorld.h"
namespace downwell {
// Blocks 18..95 of Shop Step, reached through the earlier !criminal test.
// Input is the shared global state, not a per-shop edge snapshot. Callers run
// the view branch and crime/exit/sign tail afterwards in the same inner body.
struct ShopMenuEvents {
    enum class Message {welcome,hello,hoho,thank,maido,poor,question,what,huh};
    enum class KeeperField {thankful,goodbye,confused};
    template<class Ops> static bool exit(ShopInteraction& menu,Ops& ops){
        menu.exitRequested=true;
        if(menu.thank){
            if(!ops.say(Message::thank)||!ops.sound(99,90,"merchant"))return false;
            menu.thank=false;
            return ops.keeper(KeeperField::goodbye,1)&&ops.keeperImageZero();
        }
        return ops.sound(108,90,"UI");
    }
    template<class Ops> static bool step(ShopInteraction& menu,ShopInteraction::Input& input,Ops& ops){
        if(!menu.speaking){
            bool meeting,interactable;
            if(!ops.meeting(meeting))return false;
            if(meeting){
                if(!ops.interactable(interactable))return false;
                if(interactable&&input.confirm){
                    bool right;
                    if(!ops.playerOnRight(right)||!ops.facePlayer(right?-1:1))return false;
                    menu.speaking=menu.active=true;
                    if(!ops.noControl(true))return false;
                    menu.cursor=menu.menuSelect=1;
                    if(!ops.keeper(KeeperField::thankful,0)||!ops.keeper(KeeperField::goodbye,0))return false;
                    menu.menuState=1;
                    unsigned choice;if(!ops.choose3(choice)||choice<1||choice>3)return false;
                    const Message messages[]={Message::welcome,Message::hello,Message::hoho};
                    const int sounds[]={101,95,96};
                    if(!ops.say(messages[choice-1])||!ops.sound(sounds[choice-1],90,"merchant"))return false;
                    input.confirm=false;
                }
            }
        }else menu.leave=true;
        if(!menu.active)return true;
        if(menu.menuState==0){
            if(input.right){
                menu.menuSelect=menu.menuSelect<1?menu.menuSelect+1:0;
                if(!ops.sound(109,90,"UI"))return false;
                menu.cursorWobbleX=6;
            }
            if(input.left){
                menu.menuSelect=menu.menuSelect>0?menu.menuSelect-1:1;
                if(!ops.sound(109,90,"UI"))return false;
                menu.cursorWobbleX=-6;
            }
            if(input.confirm){
                if(menu.menuSelect>0){
                    menu.menuState=menu.menuSelect;
                    if(!ops.sound(107,90,"UI"))return false;
                }else if(!exit(menu,ops))return false;
                menu.cursor=1;input.confirm=false;
            }
        }
        // Deliberately independent: entering this menu above reuses direction
        // inputs, but not the confirmation that was just consumed.
        if(menu.menuState==1){
            if(input.right){
                menu.cursor=menu.cursor<3?menu.cursor+1:0;
                if(!ops.sound(109,90,"UI"))return false;
                menu.cursorWobbleX=6;
            }
            if(input.left){
                menu.cursor=menu.cursor>0?menu.cursor-1:3;
                if(!ops.sound(109,90,"UI"))return false;
                menu.cursorWobbleX=-6;
            }
            if(input.confirm){
                if(!menu.cursor){if(!exit(menu,ops))return false;}
                else{
                    ShopInventory::Purchase result;
                    if(!ops.purchase(result))return false;
                    if(result==ShopInventory::Purchase::insufficientFunds){
                        if(!ops.say(Message::poor)||!ops.sound(100,90,"merchant")||
                           !ops.keeper(KeeperField::confused,1)||!ops.keeperImageZero())return false;
                    }else if(result==ShopInventory::Purchase::soldOut){
                        unsigned choice;if(!ops.choose3(choice)||choice<1||choice>3)return false;
                        const Message messages[]={Message::question,Message::what,Message::huh};
                        const int sounds[]={98,102,97};
                        if(!ops.say(messages[choice-1])||!ops.sound(sounds[choice-1],90,"merchant")||
                           !ops.keeper(KeeperField::confused,1)||!ops.keeperImageZero())return false;
                    }else if(result!=ShopInventory::Purchase::bought)return false;
                }
                input.cancel=false; // dUp intentionally remains set here.
            }
            if(input.cancel&&!exit(menu,ops))return false;
        }
        return true;
    }
};
// Retained owners supply source-ID child setters; environments supply actual
// player/global queries and synchronous text/audio/grant scripts. No feedback
// aggregation or successful placeholder callbacks belong at this boundary.
template<class Owner,class Key,class Environment> struct ShopMenuOwnerOps {
    Owner& owner;Key key;ShopWorld::Instance& shop;CurrencyAccount& currency;Environment& env;
    bool meeting(bool& out){return env.meeting(out)&&owner.valid();}
    bool interactable(bool& out){return env.interactable(out)&&owner.valid();}
    bool playerOnRight(bool& out){return env.playerOnRight(out)&&owner.valid();}
    bool facePlayer(int value){return env.facePlayer(value)&&owner.valid();}
    bool noControl(bool value){return env.noControl(value)&&owner.valid();}
    bool choose3(unsigned& out){return env.choose3(out)&&owner.valid();}
    bool keeper(ShopMenuEvents::KeeperField field,int value){return owner.menuKeeper(shop.children.keeper,field,value);}
    bool keeperImageZero(){return owner.menuKeeperImageZero(shop.children.keeper);}
    bool say(ShopMenuEvents::Message message){
        MotionBinary64 y;
        return owner.menuKeeperY(shop.children.keeper,y)&&y.add(MotionBinary64::fromInteger(-18))&&
            env.say(message,y)&&owner.valid();
    }
    bool sound(int id,int priority,const char* group){return sound(id,priority,0,1,group);}
    bool sound(int id,int priority,int loop,int cut,const char* group){return env.sound(id,priority,loop,cut,group)&&owner.valid();}
    bool grant(unsigned id){return env.grant(id)&&owner.valid();}
    bool maido(MotionBinary64 y){return env.say(ShopMenuEvents::Message::maido,y)&&owner.valid();}
    bool purchase(ShopInventory::Purchase& result){return owner.purchase(key,currency,*this,result);}
};
}
#endif

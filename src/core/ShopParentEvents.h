#ifndef DOWNWELL_CORE_SHOP_PARENT_EVENTS_H
#define DOWNWELL_CORE_SHOP_PARENT_EVENTS_H
#include "ShopWorld.h"
namespace downwell {
// One selected with(Shop) body. The enclosing Step must select the population
// again for each dispatched parent; this does not replace that traversal.
struct ShopParentEvents {
    template<class Ops> static bool body(Ops& ops){
        if(!ops.radio())return false;
        bool criminal;if(!ops.criminal(criminal))return false;
        if(!criminal&&(!ops.menu()||!ops.view()))return false;
        return ops.tail();
    }
    // Blocks 114..125. Crime/death does not release noControl. Exit does,
    // before clearing leave/speaking/menu state. The sign script runs last
    // even when the merchant is criminal, dead or no longer speaking.
    template<class Ops> static bool tail(ShopWorld::Instance& shop,Ops& ops){
        bool disabled;if(!ops.criminal(disabled))return false;
        if(!disabled&&!ops.death(disabled))return false;
        auto& menu=shop.menu;
        if(disabled){
            menu.active=false;menu.leave=true;
            if(!ops.mask(582))return false;
        }
        if(menu.exitRequested){
            menu.active=false;
            if(!ops.noControl(false))return false;
            menu.leave=menu.speaking=false;menu.menuSelect=menu.menuState=0;menu.exitRequested=false;
        }
        bool eligible;if(!ops.meeting(eligible))return false;
        if(eligible&&!ops.interactable(eligible))return false;
        return shop.prompt.step(eligible);
    }
};
template<class Owner,class Key,class Environment> struct ShopParentBodyOps {
    Owner& owner;Key key;ShopInteraction::Input& input;CurrencyAccount& currency;Environment& env;
    bool radio(){return owner.parentRadioStep(key,env);}
    bool criminal(bool& out){return env.criminal(out)&&owner.valid();}
    bool menu(){return owner.parentMenuStep(key,input,currency,env);}
    bool view(){return owner.parentViewStep(key,env);}
    bool tail(){return owner.parentTailStep(key,env);}
};
}
#endif

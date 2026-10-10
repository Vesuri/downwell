#ifndef DOWNWELL_TEST_SHOP_PARENT_CHECKS_H
#define DOWNWELL_TEST_SHOP_PARENT_CHECKS_H
#include "core/ShopParentEvents.h"
#include "core/RunnerWithSnapshot.h"
#include "ShopMenuChecks.h"
inline bool checkShopWithCapture(){
    using namespace downwell;
    for(unsigned count=0;count<=8;++count){
        RunnerWithSnapshot<8> selected;
        for(unsigned i=0;i<count;++i)if(!selected.append({i,10+i}))return false;
        // Rebinding and activation queues can contain an already selected
        // lifetime. Inline order stays unchanged; the larger list reappends.
        if(count&&!selected.append({0,10}))return false;
        if(!selected.seal()||selected.size()!=count)return false;
        unsigned seen=0;
        if(!selected.visit([&](InstanceHandle key){
            const unsigned expected=count<=2?seen:(seen?count-seen:0);
            ++seen;return key.slot==expected&&key.generation==10+expected;
        })||seen!=count)return false;
        for(unsigned stop=1;stop<=count;++stop){
            seen=0;
            if(selected.visit([&](InstanceHandle){return ++seen!=stop;})||seen!=stop)return false;
        }
    }
    RunnerWithSnapshot<3> mixed;
    // Same slot, new generation is a different lifetime, not a duplicate.
    if(!mixed.append({4,1})||!mixed.append({5,1})||!mixed.append({4,2})||
       !mixed.append({5,1})||!mixed.seal())return false;
    const InstanceHandle expected[]={{5,1},{4,2},{4,1}};unsigned seen=0;
    if(!mixed.visit([&](InstanceHandle h){
        const auto e=expected[seen++];return h.slot==e.slot&&h.generation==e.generation;
    })||seen!=3)return false;
    RunnerWithSnapshot<1> overflow;
    if(!overflow.append({0,1})||overflow.append({1,1})||overflow.seal()||
       overflow.visit([](InstanceHandle){return true;}))return false;
    RunnerWithSnapshot<1> unsealed;
    if(unsealed.visit([](InstanceHandle){return true;}))return false;
    RunnerWithSnapshot<1> invalid;
    if(invalid.append({0,0})||invalid.seal())return false;
    return true;
}
struct ShopParentProbe:ShopMenuProbe {
    bool crime=false,dead=false;unsigned criminalReads=0,deathReads=0,viewReads=0;
    ShopParentProbe(downwell::ShopWorld::Instance& shop,downwell::CurrencyAccount& currency,int y)
        :ShopMenuProbe{shop,currency,y}{}
    bool timeStop(bool& out){out=false;return true;}
    bool criminal(bool& out){++criminalReads;out=crime;return true;}
    bool death(bool& out){++deathReads;out=dead;return true;}
    bool distance(downwell::MotionBinary64,downwell::MotionBinary64,downwell::MotionBinary64&){return false;}
    bool inView(downwell::MotionBinary64,downwell::MotionBinary64,bool&){return false;}
    bool gain(int,downwell::MotionBinary64 gain,int duration){return gain.zero()&&!duration;}
    bool noBgm(bool&){return false;}
    bool paused(int,bool&){return false;}
    bool resume(int){return false;}
    bool view(unsigned field,downwell::MotionBinary64& out){
        ++viewReads;if(field>3)return false;
        const int values[]={0,0,1000,3000};out=downwell::MotionBinary64::fromInteger(values[field]);return true;
    }
    bool playerX(downwell::MotionBinary64& out){out=downwell::MotionBinary64::fromInteger(2000);return true;}
};
template<class Speak> struct ShopCachedSpeechProbe:ShopParentProbe {
    Speak& speak;
    ShopCachedSpeechProbe(downwell::ShopWorld::Instance& shop,downwell::CurrencyAccount& currency,Speak& callback)
        :ShopParentProbe{shop,currency,shop.y},speak(callback){}
    bool say(Message message,downwell::MotionBinary64 y){++says;return speak(message,y);}
};
inline bool checkShopParentEvents(){
    using namespace downwell;
    if(!checkShopWithCapture())return false;
    struct Body {
        unsigned calls=0,reject=0;char trace[6]{};bool crime=false;
        bool event(char c){trace[calls++]=c;return calls!=reject;}
        bool radio(){return event('r');}
        bool criminal(bool& out){out=crime;return event('c');}
        bool menu(){crime=true;return event('m');}
        bool view(){return event('v');}
        bool tail(){return event('t');}
    };
    for(unsigned reject=0;reject<=5;++reject){
        Body b;b.reject=reject;
        if(ShopParentEvents::body(b)!=!reject||b.calls!=(reject?reject:5))return false;
        const char* sequence="rcmvt";for(unsigned i=0;i<b.calls;++i)if(b.trace[i]!=sequence[i])return false;
    }
    Body b;b.crime=true;
    if(!ShopParentEvents::body(b)||b.calls!=3||b.trace[2]!='t')return false;
    struct Tail {
        ShopWorld::Instance& shop;bool crime=false,dead=false,meet=true,eligible=true,locked=true;
        unsigned calls=0,reject=0,deathReads=0,masks=0,unlocks=0,eligibilityReads=0;
        bool event(){return ++calls!=reject;}
        bool criminal(bool& out){out=crime;return event();}
        bool death(bool& out){++deathReads;out=dead;return event();}
        bool mask(int id){++masks;return id==582&&!shop.menu.active&&shop.menu.leave&&event();}
        bool noControl(bool value){++unlocks;locked=value;return !value&&!shop.menu.active&&shop.menu.speaking&&shop.menu.exitRequested&&event();}
        bool meeting(bool& out){out=meet;return event();}
        bool interactable(bool& out){++eligibilityReads;out=eligible;return event();}
    };
    for(unsigned crime=0;crime<2;++crime)for(unsigned dead=0;dead<2;++dead)for(unsigned exiting=0;exiting<2;++exiting){
        ShopWorld::Instance shop;shop.menu.active=shop.menu.speaking=true;shop.menu.leave=false;
        shop.menu.exitRequested=exiting;shop.menu.menuState=shop.menu.menuSelect=1;
        Tail tail{shop};tail.crime=crime;tail.dead=dead;
        if(!ShopParentEvents::tail(shop,tail)||tail.deathReads!=!crime||tail.masks!=unsigned(crime||dead)||
           tail.unlocks!=exiting||tail.locked==bool(exiting)||shop.prompt.frame()!=2||
           shop.menu.active!=!(crime||dead||exiting)||shop.menu.speaking==bool(exiting)||
           shop.menu.leave!=bool((crime||dead)&&!exiting)||shop.menu.exitRequested)return false;
    }
    for(unsigned reject=1;reject<=5;++reject){
        ShopWorld::Instance shop;shop.menu.active=shop.menu.speaking=true;shop.menu.exitRequested=true;
        Tail tail{shop};tail.crime=true;tail.reject=reject;
        if(ShopParentEvents::tail(shop,tail)||tail.calls!=reject||shop.prompt.phase.mantissa||
           shop.menu.exitRequested!=(reject<=3)||shop.menu.speaking!=(reject<=3))return false;
    }
    ShopWorld::Instance shop;Tail tail{shop};tail.meet=false;
    if(!ShopParentEvents::tail(shop,tail)||tail.eligibilityReads||shop.prompt.frame()!=0||!shop.prompt.phase.mantissa)return false;
    return true;
}
template<class Run> bool checkShopCapturedBody(const downwell::RunnerWithSnapshot<1>& selected,
    downwell::ShopWorld::Instance& shop,Run run){
    using namespace downwell;
    shop.menu={};shop.shopBgm=true;shop.prompt={};
    CurrencyAccount currency;ShopInteraction::Input input;input.confirm=true;
    ShopParentProbe probe{shop,currency,shop.y};
    unsigned visits=0;
    return selected.visit([&](InstanceHandle h){++visits;return run(h,input,currency,probe);})&&
        visits==1&&!shop.shopBgm&&!input.confirm&&shop.menu.speaking&&probe.locked&&
        probe.soundIds[0]==101&&probe.criminalReads==2&&probe.deathReads==1&&
        probe.viewReads==6&&shop.prompt.frame()==2;
}
template<class Run,class Mask> bool checkShopOwnedTail(downwell::ShopWorld::Instance& shop,Run run,Mask mask){
    using namespace downwell;
    CurrencyAccount currency;ShopInteraction::Input input;input.cancel=true;
    ShopParentProbe exit{shop,currency,shop.y};exit.locked=true;
    if(!run(input,currency,exit)||exit.locked||shop.menu.active||shop.menu.speaking||shop.menu.leave||
       shop.menu.exitRequested||shop.menu.menuState||shop.menu.menuSelect||exit.soundIds[0]!=99)return false;
    ShopParentProbe criminal{shop,currency,shop.y};criminal.crime=true;criminal.locked=true;
    shop.menu.active=shop.menu.speaking=true;input={};
    if(!run(input,currency,criminal)||!criminal.locked||shop.menu.active||!shop.menu.speaking||!shop.menu.leave||
       shop.maskEnabled||!mask()||criminal.deathReads||criminal.viewReads||criminal.criminalReads!=2)return false;
    return true;
}
#endif

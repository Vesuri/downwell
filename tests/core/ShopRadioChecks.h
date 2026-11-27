#ifndef DOWNWELL_TEST_SHOP_RADIO_CHECKS_H
#define DOWNWELL_TEST_SHOP_RADIO_CHECKS_H
#include "core/ShopRadioEvents.h"
struct ShopRadioProbe {
    bool stopped=true,dead=false,visible=true,muted=false,isPaused=true;
    int separation=159,expectedRadio=0;unsigned calls=0,failure=0;char log[32]{};
    downwell::AudioLevel lastGain{};int lastDuration=-1;
    bool put(char c){log[calls++]=c;return calls!=failure;}
    bool matches(const char* expected)const{unsigned i=0;for(;expected[i];++i)if(i>=calls||log[i]!=expected[i])return false;return i==calls;}
    bool timeStop(bool& out){out=stopped;return put('t');}
    bool death(bool& out){out=dead;return put('d');}
    bool distance(downwell::MotionBinary64& out){out=downwell::MotionBinary64::fromInteger(separation);return put('l');}
    bool distance(downwell::MotionBinary64,downwell::MotionBinary64,downwell::MotionBinary64& out){return distance(out);}
    bool inView(bool& out){out=visible;return put('v');}
    bool inView(downwell::MotionBinary64,downwell::MotionBinary64,bool& out){return inView(out);}
    bool gain(downwell::AudioLevel value,int ms){lastGain=value;lastDuration=ms;return put(value.zero()?'z':'g');}
    bool gain(int id,downwell::AudioLevel value,int ms){return id==expectedRadio&&gain(value,ms);}
    bool noBgm(bool& out){out=muted;return put('m');}
    bool paused(bool& out){out=isPaused;return put('p');}
    bool paused(int id,bool& out){return id==expectedRadio&&paused(out);}
    bool resume(){return put('r');}
    bool resume(int id){return id==expectedRadio&&resume();}
};
inline bool checkShopRadioLogic(){
    using downwell::ShopRadioEvents;
    for(unsigned failure=0;failure<=8;++failure){
        ShopRadioProbe p;p.failure=failure;bool enabled=false;
        if(ShopRadioEvents::step(enabled,p)!=!failure||p.calls!=(failure?failure:8)||enabled!=(failure==0||failure>=4))return false;
        const char* expected="tdlvgmpr";
        for(unsigned i=0;i<p.calls;++i)if(p.log[i]!=expected[i])return false;
    }
    ShopRadioProbe p;bool enabled=false;p.separation=160;
    if(!ShopRadioEvents::step(enabled,p)||enabled||!p.matches("tdl"))return false;
    p={};p.dead=true;enabled=true;
    if(!ShopRadioEvents::step(enabled,p)||!enabled||!p.matches("td"))return false;
    p={};p.stopped=false;
    if(!ShopRadioEvents::step(enabled,p)||enabled||!p.matches("tz")||!p.lastGain.zero()||p.lastDuration)return false;
    p={};p.stopped=false;
    if(!ShopRadioEvents::step(enabled,p)||enabled||!p.matches("t"))return false;
    p={};p.stopped=false;p.failure=2;enabled=true;
    if(ShopRadioEvents::step(enabled,p)||!enabled||!p.matches("tz"))return false;
    p={};p.muted=true;p.visible=false;enabled=false;
    if(!ShopRadioEvents::step(enabled,p)||!enabled||!p.matches("tdlvgmz")||!p.lastGain.zero()||p.lastDuration!=100)return false;
    p={};p.visible=false;p.isPaused=false;enabled=true;
    if(!ShopRadioEvents::step(enabled,p)||!p.matches("tdvgmp")||p.lastGain!=downwell::AudioLevel::raw(19661)||p.lastDuration!=100)return false;
    return true;
}
#endif

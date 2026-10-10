#ifndef DOWNWELL_TEST_SHOP_VIEW_CHECKS_H
#define DOWNWELL_TEST_SHOP_VIEW_CHECKS_H
#include "core/ShopViewEvents.h"
struct ShopViewProbe {
    int left=0,top=0,width=1000,height=3000,player=2000;
    unsigned calls=0,reject=0,greetings=0,angrySounds=0,reads=0;
    char trace[32]{};
    bool event(char c){trace[calls++]=c;return calls!=reject;}
    bool view(unsigned field,downwell::MotionBinary64& out){
        if(field>3)return false;
        const int values[]={left,top,width,height};out=downwell::MotionBinary64::fromInteger(values[field]);
        return event(char('0'+field));
    }
    bool playerX(downwell::MotionBinary64& out){++reads;out=downwell::MotionBinary64::fromInteger(player);return event('p');}
    bool sound(int id,int priority,int loop,int cut,const char* group){
        const char* expected="merchant";
        while(*expected&&*expected==*group){++expected;++group;}
        if(*expected!=*group||priority!=90||loop||cut!=1||(id!=103&&id!=94))return false;
        if(id==103)++greetings;else ++angrySounds;
        return event(id==103?'g':'a');
    }
};
inline bool checkShopViewEvents(){
    using namespace downwell;
    struct Ops:ShopViewProbe {
        int px=10,py=10,kx=18;
        MotionBinary64 x(){return MotionBinary64::fromInteger(px);}
        MotionBinary64 y(){return MotionBinary64::fromInteger(py);}
        MotionBinary64 keeperX(){return MotionBinary64::fromInteger(kx);}
        bool sound(int id){return ShopViewProbe::sound(id,90,0,1,"merchant");}
    };
    const char* expected="002113gpa";
    for(unsigned reject=0;reject<=9;++reject){
        Ops ops;ops.reject=reject;bool visible=false,angry=false;
        const bool ok=ShopViewEvents::step(visible,angry,ops);
        if(ok!=!reject||ops.calls!=(reject?reject:9)||visible!=(reject==0||reject>=7)||angry!=(reject==0||reject>=9))return false;
        for(unsigned i=0;i<ops.calls;++i)if(ops.trace[i]!=expected[i])return false;
    }
    // Strict source edges; outside does not clear the angry latch.
    for(unsigned edge=0;edge<4;++edge){
        Ops ops;bool visible=true,angry=true;
        if(edge<2)ops.px=edge?1000:0;else ops.py=edge==2?0:3000;
        if(!ShopViewEvents::step(visible,angry,ops)||visible||!angry||ops.reads||ops.greetings||ops.angrySounds)return false;
        const unsigned counts[]={1,3,4,6};if(ops.calls!=counts[edge])return false;
    }
    Ops ops;bool visible=false,angry=false;
    if(!ShopViewEvents::step(visible,angry,ops)||!visible||!angry)return false;
    ops.calls=0;
    if(!ShopViewEvents::step(visible,angry,ops)||ops.greetings!=1||ops.angrySounds!=1)return false;
    ops.calls=0;ops.player=18;
    if(!ShopViewEvents::step(visible,angry,ops)||angry||ops.angrySounds!=1)return false;
    ops.calls=0;ops.player=19;
    if(!ShopViewEvents::step(visible,angry,ops)||!angry||ops.angrySounds!=2)return false;
    // Each source __view_get is a fresh read, not a cached rectangle.
    struct MovingView:Ops {
        bool view(unsigned field,MotionBinary64& out){
            if(calls==1)left=20;
            return Ops::view(field,out);
        }
    } moving;
    moving.width=5;visible=false;angry=false;
    if(!ShopViewEvents::step(visible,angry,moving)||!visible)return false;
    return true;
}
#endif

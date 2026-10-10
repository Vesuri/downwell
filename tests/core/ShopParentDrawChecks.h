#ifndef DOWNWELL_TEST_SHOP_PARENT_DRAW_CHECKS_H
#define DOWNWELL_TEST_SHOP_PARENT_DRAW_CHECKS_H
#include "core/ShopParentDraw.h"
struct ShopParentDrawProbe {
    using N=downwell::MotionBinary64;
    unsigned reject=0,calls=0,money=10000,frames=0,prompts=0,sprites=0,texts=0;
    bool crime=false,redPrice=false,negativeArrow=false;
    char trace[128]{};
    bool event(char c){if(calls==128)return false;trace[calls++]=c;return calls!=reject;}
    bool self(){return event('S');}
    bool halign(unsigned v){return v==1&&event('h');}
    bool valign(unsigned v){return v<=1&&event('v');}
    bool view(unsigned axis,N& out){if(axis>1)return false;out=N::fromInteger(axis?1000:480);return event(axis?'Y':'X');}
    bool cameraHeight(N& out){out=N::fromInteger(284);return event('H');}
    bool arrowFrames(unsigned& out){++frames;out=4;return event('F');}
    bool sprite(int id,N frame,N,N,int angle){
        if(angle!=0&&angle!=90&&angle!=270)return false;
        ++sprites;if(id==484&&(frame.high&0x80000000u))negativeArrow=true;
        return event('s');
    }
    bool text(downwell::ShopParentDraw::Label label,N,N,bool red){
        if(label.format==downwell::ShopParentDraw::Label::literal&&!label.text)return false;
        ++texts;if(label.format==downwell::ShopParentDraw::Label::currency)redPrice=red;
        return event('t');
    }
    bool tableX(N& out){out=N::fromInteger(544);return event('T');}
    bool y(N& out){out=N::fromInteger(1200);return event('y');}
    bool itemSprite(unsigned id,int& out){out=152;return id>=100&&id<=105&&event('i');}
    bool currency(unsigned& out){out=money;return event('$');}
    bool criminal(bool& out){out=crime;return event('c');}
    bool prompt(){++prompts;return event('p');}
};
#endif

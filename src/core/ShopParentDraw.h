#ifndef DOWNWELL_CORE_SHOP_PARENT_DRAW_H
#define DOWNWELL_CORE_SHOP_PARENT_DRAW_H
#include "ShopWorld.h"
#include "MultiplyBinary64.h"
#include "RunnerRealComparison.h"
namespace downwell {
struct ShopParentDraw {
    using N=MotionBinary64;
    struct Label {
        enum Format {literal,number,stringNumber,currency};
        const char* text=nullptr;N value{};Format format=literal;
    };
    static bool offset(N value,int first,int second,N& result){
        if(!value.add(N::fromInteger(first))||!value.add(N::fromInteger(second)))return false;
        result=value;return true;
    }
    // Ops own live view/global reads, sprite/text primitives and the final
    // scrCheckThisDraw. They must stop on failure; completed self writes stay.
    template<class Ops> static bool draw(ShopWorld::Instance& shop,Ops& ops){
        auto& d=shop.draw;auto& m=shop.menu;auto& labels=shop.text;
        N a,b,x,y,index;int order;
        if(!ops.self()||!ops.halign(1)||!ops.view(0,d.boxX)||
           !ops.view(1,a)||!offset(a,56,1,d.boxY))return false;
        if(!ops.view(1,a)||!ops.cameraHeight(b)||!multiplyBinary64(b,{0x3fe00000u,0},b)||
           !a.add(b)||!offset(a,-85,0,d.boxY)||!offset(d.boxX,6,0,d.textX))return false;
        if(!ops.view(1,a)||!offset(a,34,0,d.nameY)||!ops.view(1,a)||!ops.cameraHeight(b)||
           !multiplyBinary64(b,{0x3fe00000u,0},b)||!a.add(b)||!offset(a,-108,0,d.nameY))return false;
        if(!compareRunnerReal(d.wobbleY,{},order))return false;
        if(order>0&&!d.wobbleY.add(N::fromInteger(-1)))return false;
        m.cursorWobbleX-=(m.cursorWobbleX>0)-(m.cursorWobbleX<0);
        if(!ops.halign(1))return false;
        if(m.active){
            // Create initializes cSp=0.25; no source event rewrites it.
            if(m.cursorPhaseQuarters==0x7fffffff)return false;
            ++m.cursorPhaseQuarters;unsigned count;
            if(!ops.arrowFrames(count)||!count||count>65535)return false;
            if(m.cursorPhaseQuarters>int((count-1)*4)){
                if(!ops.arrowFrames(count)||!count||count>65535)return false;
                m.cursorPhaseQuarters-=int(count*4);
            }
            if(!multiplyBinary64(N::fromInteger(m.cursorPhaseQuarters),{0x3fd00000u,0},index)||
               !ops.halign(1)||!ops.sprite(489,{},d.boxX,d.boxY,0))return false;
            auto arrow=[&](N ax,N ay,int angle){return ops.sprite(484,index,ax,ay,angle);};
            auto text=[&](const char* value,N tx,N ty){return value&&ops.text(Label{value},tx,ty,false);};
            auto boxY=[&](int first,int second,N& out){return offset(d.boxY,first,second,out)&&out.add(d.wobbleY);};
            auto boxX=[&](N& out){return offset(d.boxX,80,m.cursorWobbleX,out);};
            auto viewX=[&](int first,int second,N& out){N v;return ops.view(0,v)&&offset(v,first,second,out);};
            if(m.menuState==0){
                if(!ops.halign(1)||!ops.valign(1))return false;
                for(unsigned i=0;i<3;++i)if(!viewX(80,int(i)*48-48,d.menuX[i]))return false;
                if(!boxY(24,0,d.menuY)||!offset(d.menuY,24,0,y)||!text(labels.exit,d.menuX[0],y)||
                   !offset(d.menuY,24,0,y)||!text(labels.buy,d.menuX[1],y)||m.menuSelect>2||
                   !offset(d.menuY,6,0,y)||!offset(d.menuX[m.menuSelect],-m.cursorWobbleX,0,x)||!arrow(x,y,0))return false;
            }else if(m.menuState==1){
                if(m.cursor>3||!ops.tableX(a)||!offset(a,-38,int(19*m.cursor),d.arrowX)||
                   !d.arrowX.add(N::fromInteger(-m.cursorWobbleX))||!ops.y(a)||!offset(a,-28,0,y)||
                   !arrow(d.arrowX,y,m.cursor?0:270))return false;
                if(m.cursor&&(!ops.y(a)||!offset(a,-28,0,y)||!ops.sprite(498,N::fromInteger(m.cursor),d.arrowX,y,0)))return false;
                if(!ops.valign(1))return false;
                if(m.cursor&&shop.stock.items[m.cursor-1].remaining){
                    auto& item=shop.stock.items[m.cursor-1];int sprite;
                    if(!boxY(36,0,y)||!viewX(80,m.cursorWobbleX,x)||!ops.itemSprite(item.upgrade,sprite)||
                       !ops.sprite(sprite,{},x,y,0)||!ops.halign(1)||!boxY(12,0,y)||!boxX(x)||
                       !text(labels.itemName[m.cursor-1],x,y))return false;
                    unsigned money;if(!ops.currency(money)||item.price>0x7fffffffu)return false;
                    const bool red=money<item.price;
                    if(!boxY(54,0,y)||!viewX(80,m.cursorWobbleX,x)||
                       !ops.text(Label{nullptr,N::fromInteger(item.price),Label::currency},x,y,red)||
                       !ops.halign(1)||!ops.valign(1)||!boxY(72,8,y)||!boxX(x)||
                       !text(labels.itemDescription[m.cursor-1],x,y))return false;
                }else{
                    if(!ops.halign(1)||!boxY(12,24,y)||!boxX(x)||!text(m.cursor?labels.purchased:labels.back,x,y))return false;
                }
            }else if(m.menuState==2){
                if(!ops.halign(1)||!ops.valign(0))return false;
                for(unsigned i=0;i<2;++i)if(!viewX(80,int(i)*48-48,d.sellX[i]))return false;
                for(unsigned i=0;i<3;++i)if(!viewX(80,int(i)*24-24,d.iconX[i]))return false;
                if(!boxY(48,0,d.sellY)||!offset(d.sellY,-12,0,d.iconY))return false;
                for(labels.index=0;labels.index<=2;++labels.index)
                    if(!ops.sprite(495,N::fromInteger(labels.index),d.iconX[labels.index],d.iconY,0))return false;
                if(d.amount>0x7fffffffu||d.sellPrice>0x7fffffffu)return false;
                if(!offset(d.iconY,-20,0,y)||!ops.text(Label{nullptr,N::fromInteger(d.amount),Label::number},d.iconX[0],y,false)||
                   !multiplyBinary64(N::fromInteger(d.amount),N::fromInteger(d.sellPrice),a)||
                   !offset(d.iconY,-20,0,y)||!ops.text(Label{nullptr,a,Label::number},d.iconX[2],y,false)||
                   !offset(d.sellY,16,0,y)||!text(labels.back,d.sellX[0],y)||
                   !offset(d.sellY,16,0,y)||!text(labels.sell,d.sellX[1],y)||
                   !offset(d.sellY,16,10,y)||!ops.text(Label{nullptr,N::fromInteger(d.amount),Label::stringNumber},d.sellX[1],y,false))return false;
                if(m.cursor==1){
                    if(d.amount>1&&(!offset(d.sellY,30,0,y)||!offset(d.sellX[1],-20,0,x)||!arrow(x,y,270)))return false;
                    if(!offset(d.sellY,30,0,y)||!offset(d.sellX[1],20,0,x)||!arrow(x,y,90))return false;
                }
                if(m.cursor>1||!offset(d.sellY,4,0,y)||!offset(d.sellX[m.cursor],-m.cursorWobbleX,0,x)||!arrow(x,y,0))return false;
            }
        }
        bool criminal;return ops.criminal(criminal)&&(criminal||ops.prompt());
    }
};
template<class Owner,class Environment> struct ShopDrawOwnerOps {
    using N=MotionBinary64;
    Owner& owner;Environment& env;
    bool self(){return env.self()&&owner.valid();}
    bool halign(unsigned v){return env.halign(v)&&owner.valid();}
    bool valign(unsigned v){return env.valign(v)&&owner.valid();}
    bool view(unsigned axis,N& v){return env.view(axis,v)&&owner.valid();}
    bool cameraHeight(N& v){return env.cameraHeight(v)&&owner.valid();}
    bool arrowFrames(unsigned& n){return env.arrowFrames(n)&&owner.valid();}
    bool sprite(int id,N frame,N x,N y,int angle){return env.sprite(id,frame,x,y,angle)&&owner.valid();}
    bool text(ShopParentDraw::Label text,N x,N y,bool red){return env.text(text,x,y,red)&&owner.valid();}
    bool tableX(N& v){return env.tableX(v)&&owner.valid();}
    bool y(N& v){return env.y(v)&&owner.valid();}
    bool itemSprite(unsigned id,int& sprite){return env.itemSprite(id,sprite)&&owner.valid();}
    bool currency(unsigned& value){return env.currency(value)&&owner.valid();}
    bool criminal(bool& value){return env.criminal(value)&&owner.valid();}
    bool prompt(){return env.prompt()&&owner.valid();}
};
}
#endif

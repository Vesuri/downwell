#ifndef DOWNWELL_AMIGA_HEALTH_HUD_H
#define DOWNWELL_AMIGA_HEALTH_HUD_H
#include "AmigaRenderer.h"
#include "AmigaHealthArt.h"
#include "core/HudNumbers.h"

// Static art is retained as plane words. A damage frame combines only changed
// glyph/fill rectangles and publishes final words once: no clear-and-repaint.
class AmigaHealthHud {
    enum { firstY=6, rows=28, words=7 };
    using Pair=AmigaHealthPair;
    Pair background_[rows][words]{}, rim_[rows][words]{}, text_[rows][words]{};
    AmigaHudGlyph glyphs_[11]{};
    const UBYTE* pack_=nullptr;
    UBYTE* destination_=nullptr;
    unsigned health_=0,maximum_=0,pieces_=0,perHeart_=0;
    bool valid_=false,artReady_=false;
    bool preparedArt_=false,borrowedArt_=false;
    static UWORD read(const UBYTE* p){return UWORD(unsigned(p[0])*256+p[1]);}
    static void write(UBYTE* p,UWORD v){p[0]=UBYTE(v>>8);p[1]=UBYTE(v);}
    static UWORD mask(int word,int left,int right){
        left-=word*16;right-=word*16;
        if(left<0)left=0;
        if(right>16)right=16;
        return left>=right?0:UWORD((0xffffu>>left)&(0xffffu<<(16-right)));
    }
    static void over(Pair& target,Pair source){
        const UWORD alpha=source.low|source.high;
        target.low=(target.low&UWORD(~alpha))|source.low;
        target.high=(target.high&UWORD(~alpha))|source.high;
    }
    void capture(Pair (&out)[rows][words],const UBYTE* source){
        for(unsigned y=0;y<rows;++y)for(unsigned w=0;w<words;++w){
            const auto* p=source+(y+firstY)*downwellBytesPerPlaneLine+w*2;
            out[y][w]={read(p),read(p+downwellPlaneBytes)};
        }
    }
    void glyph(unsigned index,int x){
        const auto& g=glyphs_[index];x+=g.left;
        const int word=x/16,shift=x%16;
        for(unsigned row=0;row<g.height;++row){
            const int y=10+g.top+int(row)-firstY;
            if(y<0||y>=rows)continue;
            if(word>=0&&word<words)over(text_[y][word],
                {UWORD(g.low[row]>>shift),UWORD(g.high[row]>>shift)});
            if(shift&&word+1>=0&&word+1<words)over(text_[y][word+1],
                {UWORD(g.low[row]<<(16-shift)),UWORD(g.high[row]<<(16-shift))});
        }
    }
    void clearText(int left,int right){
        for(int y=10-firstY;y<19-firstY;++y)for(int w=left/16;w<(right+15)/16;++w){
            const UWORD keep=UWORD(~mask(w,left,right));
            text_[y][w].low&=keep;text_[y][w].high&=keep;
        }
    }
    void composeText(unsigned health,unsigned maximum,bool initial){
        if(initial){
            for(auto& row:text_)for(auto& word:row)word={0,0};
            glyph(10,48);
        }
        if(initial||health_!=health){
            clearText(31,48);
            if(health>=10)glyph(health/10,31);
            glyph(health%10,39);
        }
        if(initial||maximum_!=maximum){
            clearText(57,74);
            if(maximum>=10){glyph(maximum/10,57);glyph(maximum%10,65);}
            else glyph(maximum,57);
        }
    }
    void rectangle(UBYTE* hud,int left,int top,int right,int bottom,
                   unsigned fill,unsigned reserve){
        if(left<8)left=8;
        if(right>100)right=100;
        if(top<firstY)top=firstY;
        if(bottom>firstY+rows)bottom=firstY+rows;
        // Horizontal geometry is constant for this entire rectangle. Resolve
        // it once, not once per row.
        UWORD clips[words],fills[words],reserves[words];
        const int first=left/16,last=(right+15)/16;
        for(int w=first;w<last;++w){
            clips[w]=mask(w,left,right);fills[w]=mask(w,12,12+fill);
            reserves[w]=mask(w,12,12+reserve);
        }
        const auto* background=borrowedArt_?AmigaHealthArt::background:background_;
        const auto* rim=borrowedArt_?AmigaHealthArt::rim:rim_;
        for(int y=top;y<bottom;++y)for(int w=first;w<last;++w){
            Pair value=background[y-firstY][w];
            if(y>=11&&y<17){const auto m=fills[w];
                value.low&=UWORD(~m);value.high|=m;}
            if(y>=20&&y<22){const auto m=reserves[w];
                value.low|=m;value.high&=UWORD(~m);}
            over(value,rim[y-firstY][w]);over(value,text_[y-firstY][w]);
            const auto clip=clips[w];
            auto* p=hud+y*downwellBytesPerPlaneLine+w*2;
            const auto low=read(p),high=read(p+downwellPlaneBytes);
            value.low=(low&UWORD(~clip))|(value.low&clip);
            value.high=(high&UWORD(~clip))|(value.high&clip);
            if(value.low!=low){write(p,value.low);++lastWordWrites;}
            if(value.high!=high){write(p+downwellPlaneBytes,value.high);++lastWordWrites;}
        }
    }
public:
    unsigned lastWordWrites=0;
    void invalidate(){valid_=false;}
    static void reference(AmigaRenderer* r,const UBYTE* pack,const UBYTE* end,
                          unsigned health,unsigned maximum,unsigned pieces,unsigned perHeart){
        auto asset=[&](unsigned s,unsigned f,int x,int y){amigaRendererDrawAsset(r,pack,end,s,f,x,y,0);};
        amigaRendererClearClipRect(r);amigaRendererFillRect(r,8,6,92,28,3);
        asset(415,1,8,6);
        amigaRendererFillRect(r,12,11,maximum?int(health*80/maximum):0,6,2);
        downwell::drawHeartPieceMeter(8,6,pieces,perHeart,
            [&](int x,int y,int w,int h){amigaRendererFillRect(r,x,y,w,h,1);},asset);
        asset(415,0,8,6);downwell::drawHealthNumberLayers(52,15,health,maximum,asset);
    }
    void draw(AmigaRenderer* r,const UBYTE* pack,const UBYTE* end,unsigned health,
              unsigned maximum,unsigned pieces=0,unsigned perHeart=4,bool forceReference=false){
        lastWordWrites=0;
        amigaRendererClearClipRect(r);
        if(forceReference||health>=100||maximum>=100||health>maximum||pieces>perHeart){
            reference(r,pack,end,health,maximum,pieces,perHeart);invalidate();return;
        }
        if(pack_!=pack){
            preparedArt_=AmigaHealthArt::matches(pack,end);
            artReady_=true;
            for(unsigned i=0;i<11;++i)if(!amigaRendererPrepareHudGlyph(&glyphs_[i],pack,end,65011,i))artReady_=false;
            pack_=pack;invalidate();
        }
        if(!artReady_){reference(r,pack,end,health,maximum,pieces,perHeart);invalidate();return;}
        if(destination_!=r->hud||perHeart_!=perHeart)invalidate();
        const bool initial=!valid_;
        if(initial)borrowedArt_=preparedArt_&&perHeart==4;
        if(initial&&!borrowedArt_){
            amigaRendererFillRect(r,8,6,92,28,3);
            amigaRendererDrawAsset(r,pack,end,415,1,8,6,0);capture(background_,r->hud);
            amigaRendererFillRect(r,8,6,92,28,0);
            downwell::drawHeartPieceMeter(8,6,0,perHeart,[](int,int,int,int){},
                [&](unsigned s,unsigned f,int x,int y){amigaRendererDrawAsset(r,pack,end,s,f,x,y,0);});
            amigaRendererDrawAsset(r,pack,end,415,0,8,6,0);capture(rim_,r->hud);
        }
        if(initial||health_!=health||maximum_!=maximum)composeText(health,maximum,initial);
        const unsigned fill=maximum?health*80/maximum:0,reserve=perHeart?pieces*80/perHeart:0;
        if(initial)rectangle(r->hud,8,6,100,34,fill,reserve);
        else{
            if(health_!=health)rectangle(r->hud,31,10,48,19,fill,reserve);
            if(maximum_!=maximum)rectangle(r->hud,57,10,74,19,fill,reserve);
            const unsigned oldFill=maximum_?health_*80/maximum_:0;
            if(oldFill!=fill)rectangle(r->hud,12+(oldFill<fill?oldFill:fill),11,
                12+(oldFill>fill?oldFill:fill),17,fill,reserve);
            const unsigned oldReserve=perHeart?pieces_*80/perHeart:0;
            if(oldReserve!=reserve)rectangle(r->hud,12+(oldReserve<reserve?oldReserve:reserve),20,
                12+(oldReserve>reserve?oldReserve:reserve),22,fill,reserve);
        }
        health_=health;maximum_=maximum;pieces_=pieces;perHeart_=perHeart;
        destination_=r->hud;valid_=true;
    }
};
#endif

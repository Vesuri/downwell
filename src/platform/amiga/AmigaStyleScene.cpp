#include "AmigaStyleScene.h"
#include "AmigaSceneRuntime.h"
#include "AmigaProfile.h"
#include "framework/AmigaHardware.h"
#include "framework/Sprite.h"
#include "AmigaMenuScroll.h"
#include "AmigaRenderer.h"

bool AmigaStyleScene::bindIcons(){
    const auto word=[](const UBYTE* p){return unsigned(p[0])*256+p[1];};
    const auto dword=[&](const UBYTE* p){return word(p)*65536u+word(p+2);};
    if(end_-pack_<20||word(pack_+4)!=2||word(pack_+6)!=20)return false;
    const unsigned count=dword(pack_+8);
    if(count>unsigned(end_-pack_-20)/24)return false;
    for(unsigned i=0;i<14;++i){
        const unsigned id=i<8?65510:i<12?65511:i==12?65512:65513;
        const unsigned variant=i<8?i:i<12?i-8:0;
        unsigned lo=0,hi=count;const UBYTE* r=nullptr;
        while(lo<hi){const unsigned mid=lo+(hi-lo)/2;const auto* p=pack_+20+mid*24;
            const unsigned key=dword(p),wanted=id*65536u+variant;
            if(key<wanted)lo=mid+1;else if(key>wanted)hi=mid;else{r=p;break;}}
        if(!r||word(r+14)!=4||word(r+8)!=16||word(r+10)>32||dword(r+20)!=272)return false;
        const unsigned offset=dword(r+16);
        if(offset>unsigned(end_-pack_)||272>unsigned(end_-pack_)-offset)return false;
        auto& icon=icons_[i];icon.x=short(word(r+4));icon.y=short(word(r+6));icon.height=word(r+10);
        icon.data[0]=(UWORD*)(pack_+offset);icon.data[1]=(UWORD*)(pack_+offset+136);
    }
    return true;
}

bool AmigaStyleScene::prepareIcons(unsigned slot,int top,Frame& frame){
    unsigned indices[8],commands[8],used=0,locks=0;
    // Certify the whole moving layer before changing either commands or DMA
    // headers. A partially vertically clipped sprite retains the planar path.
    for(unsigned i=0;i<drawList_.count;++i){const auto& c=drawList_.commands[i];
        if(c.id<65500||c.id>65503)continue;
        if(c.id==65500&&c.frame>=8)return false;
        const unsigned index=c.id==65500?c.frame:c.id==65501?8+locks++:c.id==65502?12:13;
        if(index>=14||used==8||locks>4)return false;
        const auto& icon=icons_[index];const int y=c.y-top-icon.y;
        if(y<0&&y+icon.height>0)return false;
        if(y<284&&y+icon.height>284)return false;
        if(y+icon.height<=0||y>=284)continue;
        indices[used]=index;commands[used++]=i;
    }
    for(unsigned i=0;i<used;++i){const auto& c=drawList_.commands[commands[i]];const auto& icon=icons_[indices[i]];
        Sprite sprite(icon.data[slot],UWORD(icon.height));
        sprite.setX(UWORD(amigaMenuDisplayHStart+c.x-icon.x));
        sprite.setY(UWORD(downwellDisplayVStart+c.y-top-icon.y));
        frame.sprites[used-1-i]=icon.data[slot];
    }
    unsigned out=0;
    for(unsigned i=0;i<drawList_.count;++i){const auto& c=drawList_.commands[i];
        if(c.id>=65500&&c.id<=65503)continue;
        drawList_.commands[out++]=c;
    }
    drawList_.count=out;frame.hardware=true;return true;
}

AmigaStyleScene::AmigaStyleScene(UBYTE* cache, unsigned cacheBytes,
    UWORD* firstList, UWORD* secondList, unsigned listWords,
    UBYTE* firstFrame, UBYTE* secondFrame, unsigned frameBytes)
    : cache_(cache), cacheBytes_(cacheBytes),
      targets_{firstFrame,secondFrame},frameBytes_(frameBytes),
      lists_{DownwellStyleCopperList(firstList,listWords,AmigaSelectionLayer::planeBytes),
             DownwellStyleCopperList(secondList,listWords,AmigaSelectionLayer::planeBytes)} {}

bool AmigaStyleScene::beginPrepare(const UBYTE* pack,const UBYTE* end) {
    prepared_ = preparing_ = false;
    layers_[0].invalidate(); layers_[1].invalidate();
    built_[0] = built_[1] = false;
    if (!cache_ || cacheBytes_ < amigaStyleCacheBytes || !pack || !end ||
        !targets_[0] || !targets_[1] || targets_[0]==targets_[1] || frameBytes_<AmigaSelectionLayer::bytes) return false;
    pack_ = pack; end_ = end; preparation_ = {}; preparing_ = true;
#ifndef DOWNWELL_REFERENCE_STYLE_SPRITES
    if(!bindIcons()){preparing_=false;return false;}
#endif
    return true;
}

bool AmigaStyleScene::preparePart(){
    if(prepared_)return true;
    if(!preparing_)return false;
    if(!amigaBuildStyleBorderPart(pack_,end_,cache_+amigaStyleFrameBytes*2,preparation_)){
        preparing_=false;return false;
    }
    if(preparation_.complete()){preparing_=false;prepared_=true;}
    return true;
}

bool AmigaStyleScene::prepare(const UBYTE* pack,const UBYTE* end,bool borderPreserved){
    if(!borderPreserved||(!prepared_&&!preparing_)||pack_!=pack||end_!=end){
        if(!beginPrepare(pack,end))return false;
    }else{
        layers_[0].invalidate();layers_[1].invalidate();
        built_[0]=built_[1]=false;
    }
    while(preparing_)if(!preparePart())return false;
    return prepared_;
}

bool AmigaStyleScene::render(unsigned slot, const AmigaSceneRuntime& scenes,
    Frame& frame) {
    frame = {};
    if (!prepared_ || slot >= 2) return false;
    const int top=AmigaStyleDrawList::baseView-scenes.styleYOffset();
    if(top<0 || top>int(AmigaSelectionLayer::height-284) || !drawList_.build(scenes)) return false;
    const auto* border = cache_ + amigaStyleFrameBytes*2;
    DW_PROFILE_PHASE(profileStyleActors);
#ifndef DOWNWELL_REFERENCE_STYLE_SPRITES
    prepareIcons(slot,top,frame);
#endif
    unsigned painted=0;
    if (!layers_[slot].render(targets_[slot],pack_,end_,drawList_.commands,drawList_.count,painted)) return false;
    frame.paintedPixels = painted;
    DW_PROFILE_PHASE(profileStyleDrain);
    AmigaHardware::blitterDrain();
    frame.content = layers_[slot].view(unsigned(top));
    frame.border = border - scenes.styleTileY()*amigaStyleBytesPerRow;
    frame.mask = scenes.styleDitherY() < 684 || scenes.styleSelected()
        ? amigaStyleMask(cache_,scenes) : nullptr;
    return true;
}

bool AmigaStyleScene::updateCopper(unsigned slot, const Frame& frame,
    const Sprite& nullSprite, const Palette& palette) {
    if (!prepared_ || slot >= 2 || !frame.content || !frame.border) return false;
    const auto* sprites=frame.hardware?frame.sprites:nullptr;
    if (!(built_[slot] ? lists_[slot].update(frame.content,frame.mask,frame.border,sprites)
          : lists_[slot].build(frame.content,frame.mask,nullSprite,palette,frame.border,sprites))) return false;
    built_[slot] = true;
    return true;
}

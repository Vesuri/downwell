#include "platform/amiga/AmigaGameplayActors.h"
#include "platform/amiga/framework/Sprite.h"
#include "core/DamageNumberText.h"
#include <array>
#include <cassert>
#include <cstdio>
static UBYTE records[5][4][2];
static bool drawOK=true;
static int failedPart=-1;
static bool boundsOK=true;
int amigaRendererAssetBounds(const UBYTE*,const UBYTE*,UWORD sprite,UWORD,int,int,int,
    int* x,int* y,int* width,int* height) {
    *x=110+sprite*20;*y=10;*width=16*sprite;*height=1;return boundsOK;
}
Sprite::Sprite(UWORD* data,UWORD,bool,bool) : data_(data) {}
Sprite::~Sprite() {}
UWORD* Sprite::data() const { return data_; }
void Sprite::hide() { data_[0]=0xffff; }
int amigaRendererResolveSpritePart(AmigaResolvedSpritePart* out,const UBYTE* pack,
    const UBYTE* end,UWORD sprite,UWORD frame,unsigned part,int flip) {
    assert(frame==17 && flip==1);
    if(sprite==999 || int(part)==failedPart) return 0;
    assert(sprite>=1 && sprite<=4 && part<sprite);
    records[sprite][part][0]=sprite; records[sprite][part][1]=part;
    *out={pack,end,records[sprite][part],sprite};
    return 1;
}
int amigaRendererDrawResolvedSpritePart(const AmigaResolvedSpritePart* part,int x,int y,UWORD* target,unsigned height) {
    assert(x==-20 && y==300 && height==32); // positioning, not viewport clipping
    target[0]=part->record[0]*16+part->record[1];
    return drawOK;
}
int main() {
    UWORD data[8][4]{};
    Sprite player(data[0],32), regular[6]={{data[1],32},{data[2],32},{data[3],32},
        {data[4],32},{data[5],32},{data[6],32}}, overflow(data[7],32);
    UBYTE pack[8]{};
    {
        GameplayBob covers[]={{gameplayBlackRectMarker,160,110,0,30},
            {gameplayDitherEdge,0,190,-50,0}};
        const auto result=AmigaGameplayActors::assign(covers,2,pack,pack+8,player,regular,overflow,32);
        assert(result.ready&&result.bobCount==2); // No hardware conversion/lookup.
    }
    AmigaActorFrames::Frame prepared[5]{};
    for(unsigned s=1;s<=4;++s){
        auto& f=prepared[s];f.originX=130+s*36;f.originY=290;f.width=16*s;f.height=1;
        f.pack=pack;f.end=pack+8;f.partCount[1]=s;
        for(unsigned p=0;p<s;++p){records[s][p][0]=s;records[s][p][1]=p;f.parts[1][p]=records[s][p];}
    }
    unsigned cases=0;
    for(bool bound:{false,true})
    for(unsigned pattern=0;pattern<256;++pattern) for(unsigned limit=0;limit<=8;++limit)
    for(unsigned marker=0;marker<=4;++marker) for(unsigned cover=0;cover<16;++cover) {
        downwell::PaletteEffectSurface<> surface;
        const UBYTE opaque[]={0x80,0},zero[]={0,0};
        for(unsigned i=0;i<4;++i)if(cover&(1u<<i))
            assert(surface.add({{opaque,2,16,1,2},zero,zero},int((i+1)*20),10));
        GameplayBob actors[4];
        unsigned v=pattern;
        for(unsigned i=0;i<4;++i) {
            actors[i]={UWORD(1+v%4),17,-20,300,1}; v/=4;
            if(bound)actors[i].prepared=&prepared[actors[i].sprite];
            if(i==marker){
                actors[i].sprite=pattern&1?gameplayRayMarker:gameplayDamageTextMarker;
                actors[i].x=120+int(pattern%5)*20;actors[i].y=pattern&2?32:10;
            }
        }
        // Independent pixel oracle, not a second implementation of assignment.
        // All synthetic actor pixels occupy scanline10. Parts form contiguous
        // sixteen-pixel strips, and different sprite colours expose inversion.
        std::array<unsigned,380> expected{},actual{};
        const auto span=[](auto& pixels,int left,int right,unsigned pen){
            for(int x=left;x<right;++x)if(x>=110&&x<270)pixels[x]=pen;
        };
        const auto paint=[&](auto& pixels,const GameplayBob& a){
            if(a.sprite==gameplayRayMarker){span(pixels,110,270,7);return;}
            if(a.sprite==gameplayDamageTextMarker){
                if(a.y-1<=10&&a.y+11>10){
                    downwell::DamageNumberText text;text.formatInteger(a.flip);
                    span(pixels,a.x-int(text.length)*4-1,a.x+int(text.length)*4+1,9);
                }
                return;
            }
            span(pixels,110+a.sprite*20,110+a.sprite*20+a.sprite*16,a.sprite);
        };
        const auto overlay=[&](auto& pixels){surface.visit([&](unsigned x,unsigned y,
            std::uint16_t alpha,std::uint16_t,std::uint16_t){
            if(y==10)for(unsigned bit=0;bit<16;++bit)
                if(alpha&(0x8000u>>bit))pixels[110+x+bit]=0;
        });};
        for(const auto& a:actors)paint(expected,a);
        overlay(expected);
        const std::array<GameplayBob,4> original{{actors[0],actors[1],actors[2],actors[3]}};
        const auto result=AmigaGameplayActors::assign(actors,4,pack,pack+8,player,regular,overflow,32,limit,&surface);
        assert(result.ready&&result.bobCount<=4);
        unsigned prior=0;
        for(unsigned i=0;i<result.bobCount;++i){
            while(prior<4&&(original[prior].sprite!=actors[i].sprite||
                original[prior].x!=actors[i].x||original[prior].y!=actors[i].y))++prior;
            assert(prior<4);++prior;paint(actual,actors[i]);
        }
        overlay(actual);
        for(unsigned i=8;i;){--i;if(data[i][0]==0xffff)continue;
            const unsigned sprite=data[i][0]/16,part=data[i][0]%16;
            assert(i<limit&&sprite>=1&&sprite<=4&&part<sprite);
            span(actual,110+sprite*20+part*16,110+sprite*20+(part+1)*16,sprite);
        }
        assert(actual==expected);
        ++cases;
    }
    {
        GameplayBob crowded[20];
        for(unsigned i=0;i<3;++i)crowded[i]={1,17,-20,300,1};
        for(unsigned i=3;i<20;++i)crowded[i]={gameplayDamageTextMarker,0,130,10,-2147483647};
        auto stopped=AmigaGameplayActors::assign(crowded,20,pack,pack+8,player,regular,overflow,32);
        assert(stopped.ready&&stopped.bobCount==20); // Bounded conservative occluder overflow.
        for(const auto& slot:data)assert(slot[0]==0xffff);
    }
    {
        // Same position: ordinary actor < retained controller < promoted gem.
        // The surface must hide the former but never its own topmost suffix.
        downwell::PaletteEffectSurface<> surface;
        const UBYTE opaque[]={0xff,0xff},zero[]={0,0};
        assert(surface.add({{opaque,2,16,1,2},zero,zero},20,10));
        GameplayBob partition[]={{1,17,-20,300,1},{1,17,-20,300,1}};
        auto split=AmigaGameplayActors::assign(partition,2,pack,pack+8,player,regular,overflow,32,8,&surface,1);
        assert(split.ready&&split.bobCount==1&&partition[0].sprite==1);
        assert(data[0][0]==16);
        for(unsigned i=1;i<8;++i)assert(data[i][0]==0xffff);
        split=AmigaGameplayActors::assign(partition,2,pack,pack+8,player,regular,overflow,32,8,&surface,3);
        assert(!split.ready);
    }
    GameplayBob invalid{999,17,-20,300,1};
    auto result=AmigaGameplayActors::assign(&invalid,1,pack,pack+8,player,regular,overflow,32);
    assert(!result.ready && result.bobCount==1 && result.failedSprite==999 && result.failedFrame==17);
    for(const auto& slot:data) assert(slot[0]==0xffff);
    GameplayBob actor{4,17,-20,300,1}; drawOK=false;
    result=AmigaGameplayActors::assign(&actor,1,pack,pack+8,player,regular,overflow,32);
    assert(!result.ready && result.failedSprite==4 && result.failedFrame==17);
    drawOK=true; failedPart=1;
    result=AmigaGameplayActors::assign(&actor,1,pack,pack+8,player,regular,overflow,32);
    assert(!result.ready && result.failedSprite==4 && result.failedFrame==17);
    failedPart=-1;
    downwell::PaletteEffectSurface<> empty;
    boundsOK=false;
    result=AmigaGameplayActors::assign(&actor,1,pack,pack+8,player,regular,overflow,32,8,&empty);
    assert(!result.ready && result.bobCount==1 && result.failedSprite==4);
    boundsOK=true;
    GameplayBob separated[]={{1,17,-20,300,1},{1,17,-20,300,1},
        {gameplayDamageTextMarker,0,250,10,1},{1,17,-20,300,1}};
    result=AmigaGameplayActors::assign(separated,4,pack,pack+8,player,regular,overflow,32);
#ifdef DOWNWELL_REFERENCE_ACTOR_PREFIX
    assert(result.ready&&result.bobCount==3);
#else
    assert(result.ready&&result.bobCount==1&&separated[0].sprite==gameplayDamageTextMarker);
    assert(data[0][0]==16&&data[1][0]==16&&data[2][0]==16&&data[3][0]==0xffff);
#endif
    result=AmigaGameplayActors::assign(nullptr,0,pack,pack+8,player,regular,overflow,32);
    assert(result.ready && result.bobCount==0);
    for(const auto& slot:data) assert(slot[0]==0xffff);
    std::printf("Gameplay sprite assignment: %u multipart/capacity/ray/effect-overlap cases and failure diagnostics\n",cases);
}

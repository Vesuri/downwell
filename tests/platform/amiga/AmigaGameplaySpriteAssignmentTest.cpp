#include "platform/amiga/AmigaGameplayActors.h"
#include "platform/amiga/framework/Sprite.h"
#include <cassert>
#include <cstdio>
static UBYTE records[5][4][2];
static bool drawOK=true;
static int failedPart=-1;
static bool boundsOK=true;
int amigaRendererAssetBounds(const UBYTE*,const UBYTE*,UWORD sprite,UWORD,int,int,int,
    int* x,int* y,int* width,int* height) {
    *x=110+sprite*20;*y=10;*width=16;*height=1;return boundsOK;
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
    unsigned cases=0;
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
            if(i==marker) actors[i].sprite=pattern&1?gameplayRayMarker:gameplayDamageTextMarker;
        }
        UWORD expected[8]; for(auto& value:expected) value=0xffff;
        unsigned remaining=4, used=0;
        while(remaining) {
            const unsigned parts=actors[remaining-1].sprite;
            if(parts==gameplayRayMarker || parts==gameplayDamageTextMarker || (cover&(1u<<(parts-1))) || used+parts>limit) break;
            for(unsigned part=0;part<parts;++part) expected[used++]=parts*16+part;
            --remaining;
        }
        const auto result=AmigaGameplayActors::assign(actors,4,pack,pack+8,player,regular,overflow,32,limit,&surface);
        assert(result.ready && result.bobCount==remaining);
        for(unsigned i=0;i<8;++i) assert(data[i][0]==expected[i]);
        ++cases;
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
    result=AmigaGameplayActors::assign(nullptr,0,pack,pack+8,player,regular,overflow,32);
    assert(result.ready && result.bobCount==0);
    for(const auto& slot:data) assert(slot[0]==0xffff);
    std::printf("Gameplay sprite assignment: %u multipart/capacity/ray/effect-overlap cases and failure diagnostics\n",cases);
}

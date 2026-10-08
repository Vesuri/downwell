#include "platform/amiga/AmigaGameplayActors.h"
#include "platform/amiga/AmigaGameplayRows.h"
#include "platform/amiga/AmigaLaserSight.h"
#include "platform/amiga/framework/Bitmap.h"
#include "platform/amiga/framework/AmigaHardware.h"
#include <cassert>
#include <cstdio>
#include <vector>
void AmigaHardware::blitterDrain() {}
void AmigaHardware::setBlitterNasty(bool) {}
void Bitmap::copy(const Bitmap&,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t,uint16_t) { assert(false); }
int amigaBlitterDrawPlanarMasked(const UBYTE*,const UBYTE*,UWORD,UWORD,UWORD,UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD,int) { return 0; }
int amigaBlitterClearPlanar(UBYTE*,UWORD,UWORD) { return 0; }
int amigaBlitterSolidRect(UBYTE*,UWORD,UWORD,UWORD,UWORD,UWORD,UWORD) { return 0; }
int main() {
    std::vector<UBYTE> pack(152);
    auto word=[&](unsigned at,unsigned v) { pack[at]=v>>8; pack[at+1]=v; };
    auto dword=[&](unsigned at,unsigned v) { word(at,v>>16); word(at+2,v); };
    dword(0,0x44574150); word(4,2); word(6,20); dword(8,1);
    word(20,7); word(24,8); word(26,3); word(28,29); word(30,9); word(32,32); word(34,3);
    dword(36,44); dword(40,108);
    for(unsigned y=0;y<9;++y) for(unsigned x=0;x<29;++x) {
        const unsigned pen=(x+3*y)%4;
        for(unsigned p=0;p<3;++p) if(p==2 ? pen!=0 : (pen&(1u<<p))!=0)
            pack[44+p*36+y*4+x/8]|=0x80>>(x%8);
    }
    unsigned cases=0;
    for(int x : {70,105,110,117,150,263,275,300})
    for(int y : {-15,-2,0,40,280,290}) for(unsigned origin : {0u,310u,319u})
    for(unsigned buffer=0;buffer<2;++buffer) for(unsigned rayFirst=0;rayFirst<2;++rayFirst) {
        std::vector<UBYTE> pixels(downwellGameRingBytes+16,0xa5), expected=pixels;
        std::vector<UBYTE> backing(downwellGameRingBytes,0x39), hud(downwellLayerBytes,0x78);
        const auto savedBacking=backing, savedHud=hud;
        Bitmap display(pixels.data()+8,380,320,2,false,false,384);
        AmigaRenderer renderer; amigaRendererInit(&renderer,backing.data(),hud.data());
        renderer.blitterEnabled=0;
        AmigaGameplayRows rows;
        rows.damage(0,7,1,1u<<(1-buffer)); // unrelated pre-existing damage
        AmigaLaserSight laser;
        downwell::LaserSightLine line{105,42,276,42};
        laser.prepare(pixels.data()+8,origin,&line,true);
        GameplayBob actor{7,0,x,y,0}, marker{gameplayRayMarker,0,0,0,0};
        GameplayBob commands[2]={rayFirst ? marker : actor,rayFirst ? actor : marker};
        auto pixel=[&](int px,int py,unsigned pen) {
            if(px<110 || px>=270 || py<0 || py>=284) return;
            const unsigned row=(origin+unsigned(py))%320;
            for(unsigned p=0;p<2;++p) {
                auto& b=expected[8+p*320*48+row*48+px/8];
                const unsigned bit=0x80>>(px%8);
                b=(b&~bit)|((pen&(1u<<p)) ? bit : 0);
            }
        };
        auto drawActor=[&] {
            for(int sy=0;sy<9;++sy) for(int sx=0;sx<29;++sx) {
                const unsigned pen=(sx+3*sy)%4;
                if(pen) pixel(x-8+sx,y-3+sy,pen);
            }
        };
        auto drawRay=[&] { for(int px=105;px<=276;++px) pixel(px,42,2); };
        if(rayFirst) { drawRay(); drawActor(); } else { drawActor(); drawRay(); }
        AmigaGameplayActors::draw(renderer,display,buffer,origin,commands,2,
            pack.data(),pack.data()+pack.size(),rows,laser,&line);
        assert(pixels==expected && backing==savedBacking && hud==savedHud);
        for(unsigned row=0;row<320;++row) {
            bool damaged=false;
            if(x-8<270 && x-8+29>110) for(int py=y-3;py<y+6;++py)
                if(py>=0 && py<284 && (origin+unsigned(py))%320==row) damaged=true;
            assert(rows.dirty(row,buffer)==damaged);
            assert(rows.dirty(row,1-buffer)==(row==7));
        }
        ++cases;
    }
    for(unsigned origin:{0u,310u,319u})for(unsigned buffer:{0u,1u}){
        std::vector<UBYTE> pixels(downwellGameRingBytes,0x55),expected=pixels;
        std::vector<UBYTE> backing(downwellGameRingBytes,0x39),hud(downwellLayerBytes,0x78);
        Bitmap display(pixels.data(),380,320,2,false,false,384);
        AmigaRenderer renderer;amigaRendererInit(&renderer,backing.data(),hud.data());renderer.blitterEnabled=0;
        AmigaGameplayRows rows;AmigaLaserSight laser;
        GameplayBob cover{gameplayBlackRectMarker,159,111,1,283};
        AmigaGameplayActors::draw(renderer,display,buffer,origin,&cover,1,pack.data(),pack.data()+pack.size(),rows,laser,nullptr);
        for(unsigned y=1;y<284;++y)for(unsigned x=111;x<270;++x)for(unsigned p=0;p<2;++p)
            expected[p*320*48+((origin+y)%320)*48+x/8]|=0x80>>(x%8);
        assert(pixels==expected);
        for(unsigned y=0;y<320;++y){
            const unsigned screen=(y+320-origin)%320;
            assert(rows.dirty(y,buffer)==(screen>=1&&screen<284)&&!rows.dirty(y,1-buffer));
        }
    }
    std::printf("Gameplay actors: %u clipped/wrapped painter-order cases, backing and buffer debts preserved\n",cases);
}

#include "platform/amiga/AmigaRenderer.h"
#include "core/AmmoGaugeRows.h"
#include "core/AmmoFill.h"
#include "core/AmmoDividerLayout.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

int amigaBlitterDrawPlanarMasked(const UBYTE*, const UBYTE*, UWORD, UWORD,
    UWORD, UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD, int) { return 0; }
int amigaBlitterClearPlanar(UBYTE*, UWORD, UWORD) { return 0; }
int amigaBlitterSolidRect(UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD) { return 0; }
static void check(bool value) { if (!value) std::abort(); }
int main(int argc, char** argv) {
    check(argc == 2);
    std::ifstream file(argv[1],std::ios::binary);
    std::vector<UBYTE> pack((std::istreambuf_iterator<char>(file)),{});
    check(!pack.empty());
    std::vector<UBYTE> image(downwellLayerBytes);
    AmigaRenderer renderer = {};
    renderer.hud = renderer.target = image.data();
    downwell::AmmoDividerLayout dividers;
    unsigned random = 0x74883aa9;
    for (unsigned cap=1;cap<=16;++cap) {
        downwell::AmmoGaugeRows rows;
        for (unsigned tick=0;tick<100;++tick) {
            random=random*1664525u+1013904223u;
            const unsigned ammo = tick < 5 ? cap : (random >> 16) % (cap+1);
            rows.step(ammo,cap);
            if (ammo >= 10) continue; // Deliberate fallback in the probe.
            for (unsigned segment=0;segment<cap;++segment) {
                const auto first=downwell::ammoGaugeFilledRows(segment,cap);
                const auto last=downwell::ammoGaugeFilledRows(segment+1,cap);
                for(unsigned i=first;i<last;++i) check(rows.state[i]==rows.state[first]);
            }
            for (unsigned meter=0;meter<4;++meter) for(unsigned offset=0;offset<3;++offset) {
                std::fill(image.begin(),image.end(),0);
                amigaRendererClearClipRect(&renderer);
                auto asset = [&](unsigned sprite,unsigned frame,int x,int y) {
                    if (sprite == 421) frame %= 2;
                    amigaRendererDrawAsset(&renderer,pack.data(),pack.data()+pack.size(),sprite,frame,x,y,0);
                };
                amigaRendererFillRect(&renderer,282,40,24,216,3);
                asset(416,0,284,43+meter);
                downwell::drawAmmoFill(ammo,cap,289,[&](int x,unsigned y,unsigned w,unsigned h) {
                    amigaRendererFillRect(&renderer,x,y,w,h,1);
                });
                rows.draw(288,229+4*meter,asset);
                dividers.draw(ammo,cap,1,288,229+4*meter,asset);
                asset(416,1,284,43+meter);
                asset(65010,ammo,290+offset,239+offset);
                std::uint64_t hash=14695981039346656037ull;
                for (unsigned y=40;y<256;++y) for (unsigned plane=0;plane<2;++plane)
                    for (unsigned column=0;column<3;++column) {
                        const UBYTE* p=image.data()+plane*downwellPlaneBytes+y*48+34+column*2;
                        unsigned word=unsigned(p[0])*256+p[1];
                        if (!column) word &= 0x003f;
                        hash=(hash^(word>>8))*1099511628211ull;
                        hash=(hash^(word&255))*1099511628211ull;
                    }
                std::printf("%u %u %u %u ",cap,ammo,meter,offset);
                for(unsigned segment=0;segment<cap;++segment)
                    std::putchar('0'+std::min(unsigned(rows.state[downwell::ammoGaugeFilledRows(segment,cap)]),3u));
                std::printf(" %016llx\n",static_cast<unsigned long long>(hash));
            }
        }
    }
}

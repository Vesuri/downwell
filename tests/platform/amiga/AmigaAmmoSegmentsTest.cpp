#include "platform/amiga/AmigaRenderer.h"
#include "core/AmmoGaugeRows.h"
#include "core/AmmoFill.h"
#include "core/AmmoDividerLayout.h"
#include "../../../generated/amiga-ammo-segments-probe.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

int amigaBlitterDrawPlanarMasked(const UBYTE*, const UBYTE*, UWORD, UWORD,
    UWORD, UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD, int) { return 0; }
int amigaBlitterClearPlanar(UBYTE*, UWORD, UWORD) { return 0; }
int amigaBlitterSolidRect(UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD) { return 0; }
static void check(bool value, const char* why) {
    if (!value) { std::fprintf(stderr,"FAILED: %s\n",why); std::exit(1); }
}
static unsigned word(const UBYTE* p) { return unsigned(p[0])*256+p[1]; }
int main(int argc, char** argv) {
    check(argc == 2,"HUD pack argument");
    std::ifstream file(argv[1],std::ios::binary);
    std::vector<UBYTE> pack((std::istreambuf_iterator<char>(file)),{});
    check(!pack.empty(),"read HUD pack");
    std::vector<UBYTE> image(downwellLayerBytes);
    AmigaRenderer renderer = {};
    renderer.hud = renderer.target = image.data();
    downwell::AmmoDividerLayout dividers;
    unsigned random = 0x74883aa9, comparisons = 0;
    for (unsigned cap=1;cap<=16;++cap) {
        downwell::AmmoGaugeRows rows;
        const auto* bounds = downwell_ammo_segments::boundaries[cap-1];
        for (unsigned a=0;a<=cap;++a)
            check(bounds[a]==downwell::ammoGaugeFilledRows(a,cap),"source charge thresholds");
        for (unsigned tick=0;tick<400;++tick) {
            random=random*1664525u+1013904223u;
            const unsigned ammo = tick < 5 ? cap : (random >> 16) % (cap+1);
            rows.step(ammo,cap);
            for (unsigned segment=0;segment<cap;++segment)
                for (unsigned row=bounds[segment];row<bounds[segment+1];++row)
                    check(rows.state[row]==rows.state[bounds[segment]],"fixed-capacity segment state invariant");
            for (unsigned meter=0;meter<4;++meter) {
                std::fill(image.begin(),image.end(),255);
                amigaRendererClearClipRect(&renderer);
                auto asset = [&](unsigned sprite,unsigned frame,int x,int y) {
                    if (sprite == 421) frame %= 2; // GameMaker wraps the two-frame sprite.
                    amigaRendererDrawAsset(&renderer,pack.data(),pack.data()+pack.size(),sprite,frame,x,y,0);
                };
                downwell::drawAmmoFill(ammo,cap,289,[&](int x,unsigned y,unsigned w,unsigned h) {
                    amigaRendererFillRect(&renderer,x,y,w,h,1);
                });
                rows.draw(288,229+4*meter,asset);
                dividers.draw(ammo,cap,1,288,229+4*meter,asset);
                for (unsigned segment=0;segment<cap;++segment) {
                    unsigned own=std::min(unsigned(rows.state[bounds[segment]]),3u);
                    unsigned previous=segment ? std::min(unsigned(rows.state[bounds[segment-1]]),3u) : 0;
                    const unsigned configuration=downwell_ammo_segments::configStart[cap-1]+(ammo*4+meter)*cap+segment;
                    const unsigned page=downwell_ammo_segments::configurations[configuration];
                    const unsigned tile=downwell_ammo_segments::pages[page][own*4+previous];
                    const auto* directory=downwell_ammo_segments::tiles[tile];
                    unsigned y=229+4*meter-bounds[segment+1]+(segment==cap-1 ? 1 : 8);
                    for (unsigned i=0;i<directory[1];++i) {
                        const auto* run=downwell_ammo_segments::runs[directory[0]+i];
                        const auto* pair=downwell_ammo_segments::pairs[run[1]];
                        for (unsigned row=0;row<run[0];++row,++y) {
                            const auto* p=image.data()+y*downwellBytesPerPlaneLine+36;
                            if ((word(p)&0xfff0)!=pair[0] || (word(p+downwellPlaneBytes)&0xfff0)!=pair[1]) {
                                std::fprintf(stderr,"cap=%u ammo=%u tick=%u meter=%u segment=%u y=%u states=%u/%u\n",
                                             cap,ammo,tick,meter,segment,y,own,previous);
                                check(false,"prepared final words match ordered source drawing");
                            }
                            ++comparisons;
                        }
                    }
                    check(y==229+4*meter-bounds[segment]+8,"tile has exact segment extent");
                }
            }
        }
    }
    std::printf("%u prepared segment row comparisons and fixed-capacity state checks passed\n",comparisons);
}

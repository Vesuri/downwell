#include "platform/amiga/AmigaRenderer.h"
#include "../../../generated/amiga-game-planes.h"
#include "../../../generated/amiga-scene-banks.h"
#include "platform/amiga/AmigaActorFrames.h"
#include "platform/amiga/AmigaWallBackground.h"
#include "platform/amiga/AmigaAmmoHud.h"
#ifdef DOWNWELL_AMMO_FACTORED
volatile unsigned amigaAmmoFactoredMode=DOWNWELL_AMMO_FACTORED;
#endif
#include "platform/amiga/AmigaHealthHud.h"
#include "platform/amiga/AmigaLaserSight.h"
#include "platform/amiga/AmigaGoalField.h"
#include "platform/amiga/AmigaBufferCompare.h"
#include "platform/amiga/GameplaySceneStorage.h"
#include "platform/amiga/AmigaExitMask.h"
#include "core/ShopFixtures.h"
#include "core/DamageNumberText.h"
#include "core/EffectMaskAssets.h"
#include "platform/amiga/AmigaShopMenu.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int amigaBlitterDrawPlanarMasked(const UBYTE*, const UBYTE*, UWORD, UWORD,
                                 UWORD, UBYTE*, UWORD, UWORD, UWORD, UWORD,
                                 UWORD, UWORD, UWORD, int) {
    return 0;
}

int amigaBlitterClearPlanar(UBYTE*, UWORD, UWORD) {
    return 0;
}
int amigaBlitterSolidRect(UBYTE*, UWORD, UWORD, UWORD, UWORD, UWORD, UWORD) { return 0; }

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void putWord(UBYTE* bytes, unsigned offset, unsigned value) {
    bytes[offset] = static_cast<UBYTE>(value >> 8);
    bytes[offset + 1] = static_cast<UBYTE>(value);
}

void verifyDamageEndpoints() {
    unsigned random = 0x548abc21u;
    auto next = [&]() { random = random * 1664525u + 1013904223u; return random; };
    for (unsigned trial = 0; trial < 12000; ++trial) {
        AmmoHudDamage damage;
        downwell::AmmoGaugeRows rows;
        damage.valid = true;
        damage.ammo = 4; damage.capacity = 8;
        damage.meter = trial % 4;
        damage.previous.filled = next() % 185;
        rows.filled = trial % 2 ? damage.previous.filled : next() % 185;
        for (unsigned i = 0; i < 184; ++i) {
            damage.previous.state[i] = next() % 5;
            rows.state[i] = damage.previous.state[i];
            if (trial % 3 == 0 || i == trial % 184)
                rows.state[i] = next() % 5;
            if (trial % 7 == 0) rows.state[i] = damage.previous.state[i];
            if (trial % 11 == 0) {
                damage.previous.state[i] = 3 + (i & 1);
                rows.state[i] = 4 - (i & 1);
            }
        }
        int top = 256, bottom = 0;
        // Previous implementation, deliberately scanning every row.
        for (unsigned i = 0; i < 184; ++i) {
            const unsigned a = damage.previous.state[i] < 4 ? damage.previous.state[i] : 3;
            const unsigned b = rows.state[i] < 4 ? rows.state[i] : 3;
            if (a != b || (i < damage.previous.filled) != (i < rows.filled)) {
                const int y = 229 + 4 * damage.meter - static_cast<int>(i);
                if (y < 232) { if (y < top) top = y; if (y+8 > bottom) bottom = y+8; }
            }
        }
        damage.update(4, 8, 0, damage.meter, &rows);
        require(damage.top == top && damage.bottom == bottom,
                "endpoint damage bounds equal the complete row scan");
    }
}

void verifyVisitedDamage() {
    downwell::AmmoGaugeRows rows;
    AmmoHudDamage bounded, reference;
    unsigned random = 0x793fab12u, ammo = 8, cap = 8;
    unsigned boundedUpdates = 0, skippedUpdates = 0;
    for (unsigned tick = 0; tick < 60000; ++tick) {
        random = random * 1664525u + 1013904223u;
        if (tick % 43 == 0) cap = 1 + (random >> 24) % 64;
        if (tick % 7 == 0) ammo = (random >> 16) % (cap + 1);
        if (ammo > cap) ammo = cap;
        if (tick % 251 == 0) rows = downwell::AmmoGaugeRows{};
        rows.step(ammo, cap);
        // Renderer can omit settled/invisible updates, or miss visible ones.
        if (!rows.visuallyChanged() || tick % 5 == 0) continue;
        const auto serial = rows.visualSerial();
        if (serial == bounded.previous.visualSerial() + 1u) ++boundedUpdates;
        else ++skippedUpdates;
        const unsigned meter = (tick / 17) % 4, offset = (tick / 37) % 3;
        bounded.update(ammo, cap, offset, meter, &rows, true);
        reference.update(ammo, cap, offset, meter, &rows, false);
        require(bounded.top == reference.top && bounded.bottom == reference.bottom &&
                bounded.label == reference.label,
                "visited row bounds equal full reference through missed draws and resets");
    }
    require(boundedUpdates > 1000 && skippedUpdates > 1000,
            "natural gauge test exercises consecutive and skipped observations");
}

void putLong(UBYTE* bytes, unsigned offset, unsigned long value) {
    putWord(bytes, offset, static_cast<unsigned>(value >> 16));
    putWord(bytes, offset + 2, static_cast<unsigned>(value));
}

void buildOpaqueSpritePack(UBYTE* pack, unsigned size) {
    std::memset(pack, 0, size);
    putLong(pack, 0, 0x44574150UL);
    putWord(pack, 4, 2);
    putWord(pack, 6, 20);
    putLong(pack, 8, 1);
    putLong(pack, 20, 7UL << 16);
    putWord(pack, 24, 8);  // origin X
    putWord(pack, 26, 0);  // origin Y
    putWord(pack, 28, 16); // width
    putWord(pack, 30, 1);  // height
    putWord(pack, 32, 16); // stored row width
    putLong(pack, 36, 44);
    putLong(pack, 40, 4);
    pack[44] = 0xff;
    pack[45] = 0xff;
}

// The title fade's last frame geometry: 128x32 centred at (64,16), opaque
// over columns 8..119, so the host fallback needs seven sixteen-pixel slices.
// Targets never convert it; it only exercises multi-part strips.
std::vector<UBYTE> buildWideTitlePack() {
    const unsigned stride = 16, height = 32, payload = 44, bytes = stride * height * 3;
    std::vector<UBYTE> pack(payload + bytes, 0);
    putLong(pack.data(), 0, 0x44574150UL);
    putWord(pack.data(), 4, 2);
    putWord(pack.data(), 6, 20);
    putLong(pack.data(), 8, 1);
    putLong(pack.data(), 20, (654UL << 16) | 9);
    putWord(pack.data(), 24, 64);
    putWord(pack.data(), 26, 16);
    putWord(pack.data(), 28, 128);
    putWord(pack.data(), 30, height);
    putWord(pack.data(), 32, 128);
    putWord(pack.data(), 34, 3);
    putLong(pack.data(), 36, payload);
    putLong(pack.data(), 40, bytes);
    for (unsigned y = 0; y < height; ++y)
        for (unsigned x = 8; x < 120; ++x) {
            const UBYTE bit = static_cast<UBYTE>(0x80u >> (x & 7));
            pack[payload + y * stride + x / 8] |= bit;
            pack[payload + 2 * stride * height + y * stride + x / 8] |= bit;
        }
    return pack;
}

void requireRuntimeSprite(const std::vector<UBYTE>& pack,
                          unsigned spriteId, unsigned frameCount) {
    UWORD sprite[2 + 2 * 256 + 2] = {};
    for (unsigned frame = 0; frame < frameCount; ++frame) {
        const int result = amigaRendererBuildHardwareSprite(
            pack.data(), pack.data() + pack.size(),
            static_cast<UWORD>(spriteId), static_cast<UWORD>(frame),
            downwellPlayfieldX + downwellPlayfieldWidth / 2, 0, 0,
            sprite, 256);
        if (result == 0) {
            std::cerr << "FAILED: Amiga game pack is missing runtime sprite "
                      << spriteId << ':' << frame << '\n';
            std::exit(1);
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    {
        downwell::WallBackgroundTiles<1> tiles;
        require(tiles.append(downwell::MotionBinary64::fromInteger(108),
            downwell::MotionBinary64::fromInteger(508)),"retained tile fixture");
        for(unsigned origin:{0u,319u}){
            std::vector<UBYTE> game(downwellGameRingBytes,255),hud(downwellLayerBytes,255);
            AmigaRenderer renderer;amigaRendererInit(&renderer,game.data(),hud.data());
            amigaRendererSetGameOrigin(&renderer,origin);
            amigaRendererSetClipRect(&renderer,110,0,160,284);
            require(amigaPaintWallBackground(&renderer,tiles,100,500,110,12,160,4),"paint only exposed wall-tile strip");
            for(unsigned p=0;p<2;++p)for(unsigned y=0;y<320;++y)for(unsigned x=0;x<384;++x){
                const unsigned logical=(y+320-origin)%320;
                const bool black=x>=118&&x<134&&logical>=12&&logical<16;
                const auto byte=game[p*downwellGameRingPlaneBytes+y*48+x/8];
                require(bool(byte&(128u>>(x%8)))==!black,"native patch clears both planes only inside strip across ring wrap");
            }
            for(auto byte:hud)require(byte==255,"wall tile cannot touch static HUD");
        }
    }
    {
        unsigned lines=0;
        for(int x0=-8;x0<=8;++x0)for(int y0=-8;y0<=8;++y0)
          for(int x1=-8;x1<=8;++x1)for(int y1=-8;y1<=8;++y1){
            downwell::LaserSightLine line{x0,y0,x1,y1};
            std::vector<int> expected,actual;
            // Independent integer-ratio construction: do not duplicate the
            // rasterizer's evolving error accumulator. Ties advance the minor
            // axis away from the starting point, including reversed lines.
            const int dx=std::abs(x1-x0),dy=std::abs(y1-y0);
            const int major=std::max(dx,dy),minor=std::min(dx,dy);
            for(int step=0;step<=major;++step){
                const int small=major?(2*step*minor+major)/(2*major):0;
                expected.push_back(x0+(x0<x1?1:-1)*(dx>=dy?step:small));
                expected.push_back(y0+(y0<y1?1:-1)*(dx>=dy?small:step));
            }
            line.draw([&](int x,int y){actual.push_back(x);actual.push_back(y);});
            require(actual==expected,"line raster matches independent ordered pixels in every octant");
            ++lines;
        }
        std::cout<<lines<<" independent line-raster comparisons passed\n";
    }
    {
        // Independent pre-optimization vertex formula and complete ordered
        // pixel stream, not just a final bitmap that could hide overdraw.
        constexpr int cosine[24]={32768,31651,28378,23170,16384,8481,0,-8481,-16384,-23170,-28378,-31651,
            -32768,-31651,-28378,-23170,-16384,-8481,0,8481,16384,23170,28378,31651};
        for(int minimum:{24,128}) for(int fraction=0;fraction<512;++fraction){
            const int x=-140+(fraction*17)%580,y=-150+(fraction*31)%600;
            downwell::GoalFieldOutline actual,reference;
            actual.setRasterCircle(x,y,minimum*256+fraction);reference.visible=true;
            for(unsigned ring=0;ring<3;++ring){
                const int radius=minimum*256+fraction+(ring==1?-256:ring==2?256:0);
                auto component=[&](int unit){
                    const unsigned long product=static_cast<unsigned short>(radius)*
                        static_cast<unsigned short>(unit<0?-unit:unit);
                    const int rounded=int((product+(1L<<22))>>23);
                    return unit<0?-rounded:rounded;
                };
                for(unsigned i=0;i<24;++i)
                    reference.points[ring][i]={x+component(cosine[i]),y+component(cosine[(i+18)%24])};
            }
            require(actual.same(reference),"symmetric outline preserves every original rounded vertex");
            for(unsigned clip=0;clip<4;++clip){
                const int left=clip==0?downwellPlayfieldX:clip==1?x:clip==3?x-132:x+200;
                const int top=clip==0?0:clip==3?y-132:y;
                const int right=left+(clip==0?160:clip==1?1:clip==3?265:0);
                const int bottom=top+(clip==0?284:clip==3?265:1);
                std::vector<int> expected,got;
                reference.draw([&](int px,int py,int pen){
                    if(px>=left&&px<right&&py>=top&&py<bottom){expected.push_back(px);expected.push_back(py);expected.push_back(pen);}
                });
                actual.drawClipped(left,top,right,bottom,[&](int px,int py,int pen){
                    got.push_back(px);got.push_back(py);got.push_back(pen);
                });
                require(got==expected,"clipped outline preserves ordered raster, endpoints and pens");
                std::vector<int> pixels((right-left)*(bottom-top)),spans(pixels.size());
                for(unsigned i=0;i<expected.size();i+=3)
                    pixels[(expected[i+1]-top)*(right-left)+expected[i]-left]=expected[i+2];
                actual.drawClippedSpans(left,top,right,bottom,[&](int first,int end,int row,int pen){
                    require(first>=left&&first<end&&end<=right&&row>=top&&row<bottom,
                        "outline spans are nonempty and clipped");
                    for(int px=first;px<end;++px)spans[(row-top)*(right-left)+px-left]=pen;
                });
                require(spans==pixels,"coalesced spans preserve final coverage and pen priority");
                std::fill(spans.begin(),spans.end(),0);
                downwell::TimeFieldOutlines cached;
                cached.visible=true;cached.count=1;
                cached.circles[0]={x,y,minimum*256+fraction};
                amigaFieldSpans(cached,left,top,right,bottom,[&](int first,int end,int row,int pen){
                    require(first>=left&&first<end&&end<=right&&row>=top&&row<bottom,
                        "cached field runs are nonempty and clipped");
                    for(int px=first;px<end;++px)spans[(row-top)*(right-left)+px-left]=pen;
                });
                require(spans==pixels,"offline field rows preserve final coverage and pen priority");
                for(int phase=0;phase<(clip==3?16:1);++phase){
                    std::fill(spans.begin(),spans.end(),0);
                    cached.circles[0].x=x+phase;
                    amigaFieldWords(cached,left+phase,top,right+phase,bottom,
                        [&](int wx,int wy,unsigned mask,unsigned low,unsigned high){
                            require(!(wx&15)&&mask&&!(mask&~0xffffu)&&!(low&~mask)&&!(high&~mask),
                                "cached words have aligned coordinates and bounded planar masks");
                            for(unsigned bit=0;bit<16;++bit)if(mask&(0x8000u>>bit)){
                                const int px=wx+int(bit)-phase;
                                require(px>=left&&px<right&&wy>=top&&wy<bottom,"cached words obey partial-word clipping");
                                spans[(wy-top)*(right-left)+px-left]=
                                    ((low&(0x8000u>>bit))?1:0)|((high&(0x8000u>>bit))?2:0);
                            }
                        });
                    require(spans==pixels,"offline planar words preserve all radii, shifts and final pens");
                }
            }
        }
        std::cout << "1024 outline radii, 4096 span clips and 19456 planar-word shift/clip comparisons passed\n";
    }
    {
        std::vector<UBYTE> base(downwellGameRingBytes), actual(base.size()), expected(base.size());
        for (unsigned i = 0; i < base.size(); ++i) base[i] = i * 19 + 7;
        actual = base;
        AmigaTimeFields retained;
        downwell::OrdinaryTimeFields<64> fields;
        for (unsigned i = 0; i < 64; ++i)
            require(fields.adoptCreate(160 + (i % 8) * 22, 4000 + (i / 8) * 38,
                i * 67108863u), "dense field fixture capacity");
        downwell::GoalTimeField goal;
        goal.x = 240; goal.y = 4140;
        goal.radius = downwell::HudReal::integer(129);
        for (unsigned trial = 0; trial <= 64; ++trial) {
            const unsigned origin = trial * 29 % downwellGameRingHeight;
            const int viewX = trial % 16 == 15 ? 80 : 160;
            const int viewY = 3970 + int(trial) * 3;
            downwell::TimeFieldOutlines shape;
            shape.set(fields, goal, true, trial, viewX, viewY, downwellPlayfieldX);
            require(retained.prepare(actual.data(), origin, shape, false, [](unsigned) { return false; }),
                "all 64 fields and exit fit viewport word storage");
            retained.draw();
            expected = base;
            AmigaRenderer oracle;
            amigaRendererInit(&oracle, expected.data(), nullptr);
            amigaRendererSetGameOrigin(&oracle, origin);
            amigaRendererSetClipRect(&oracle, downwellPlayfieldX, 0, downwellPlayfieldWidth, downwellDisplayHeight);
            auto circle = [&](int x, int y, downwell::HudReal radius) {
                radius.exponent += 8;
                downwell::GoalFieldOutline outline;
                outline.setRasterCircle(x-viewX+downwellPlayfieldX, y-viewY, int(radius.floor()));
                outline.draw([&](int px, int py, int pen) { amigaRendererFillRect(&oracle, px, py, 1, 1, pen); });
            };
            if (viewX != 80) for (unsigned i = 64; i; --i) {
                if (i == trial) circle(goal.x, goal.y, goal.radius);
                const auto& field = *fields.at(i-1);
                circle(field.x, field.y, field.radius);
            }
            if (viewX != 80 && trial == 0) circle(goal.x, goal.y, goal.radius);
            require(actual == expected, "merged fields match independent ordered composition and side-camera hiding");
            require(retained.prepare(actual.data(), origin, shape, false, [](unsigned) { return false; }),
                "retained multi-field snapshot");
            retained.draw();
            require(retained.lastWordWrites == 0, "unchanged field collection writes no pixels");
            require(retained.prepare(actual.data(), origin, shape, true, [](unsigned) { return false; }),
                "forced coverage rebuild retains unchanged underlay");
            retained.draw();
            require(retained.lastWordWrites == 0 && actual == expected,
                "coverage rebuild suppresses every unchanged planar write");
        }
        downwell::TimeFieldOutlines absent;
        require(retained.prepare(actual.data(), 0, absent, false, [](unsigned) { return false; }), "field teardown");
        require(actual == base, "all fields restore terrain on retirement");
        std::cout << "65 dense ordinary/exit field retention and painter-order comparisons passed\n";
    }
    {
        std::vector<UBYTE> base(downwellGameRingBytes),actual(base.size()),expected(base.size());
        for(unsigned i=0;i<base.size();++i)base[i]=i*13+9;
        actual=base;
        AmigaTimeFields retained;
        unsigned previous=0xffffu,reused=0;
        for(int minimum:{24,128})for(int phase=0;phase<512;++phase){
            downwell::TimeFieldOutlines shape;
            shape.visible=true;shape.count=1;
            shape.circles[0]={190,140,minimum*256+phase};
            const auto raster=amigaFieldRasterId(shape.circles[0].radius);
            require(retained.matches(actual.data(),270,shape)==(raster==previous),
                "radius retention follows exact cached raster identity");
            unsigned inspected=0;
            require(retained.prepare(actual.data(),270,shape,false,[&](unsigned){++inspected;return false;}),
                "prepare radius equivalence fixture");
            retained.draw();
            if(raster==previous){require(!inspected&&!retained.lastWordWrites,
                "equivalent radii do no restoration or word publication");++reused;}
            expected=base;
            AmigaTimeFields fresh;
            require(fresh.prepare(expected.data(),270,shape,false,[](unsigned){return false;}),"fresh raster oracle");
            fresh.draw();
            require(actual==expected,"radius retention preserves ring-wrapped pixels and underlay");
            auto moved=shape;++moved.circles[0].x;
            require(!retained.matches(actual.data(),270,moved)&&!retained.matches(actual.data(),271,shape)&&
                !retained.matches(expected.data(),270,shape),"position, ring origin and target remain cache keys");
            previous=raster;
        }
        require(reused==982,"all 1024 radius phases collapse to 42 raster changes");
        downwell::TimeFieldOutlines a,b;a.count=b.count=1;
        a.circles[0].radius=1;b.circles[0].radius=2;
        require(!amigaFieldSameRaster(a,b),"unsupported radii never alias through sentinel");
        b=a;require(amigaFieldSameRaster(a,b),"identical unsupported radius retains reference behavior");
        std::cout<<"1024 retained radius phases: 982 exact-raster reuses, ring wrap and invalidation passed\n";
    }
    {
        std::vector<UBYTE> base(downwellGameRingBytes),actual(base.size()),expected(base.size());
        for(unsigned i=0;i<base.size();++i)base[i]=i*31+5;
        actual=base;AmigaTimeFields retained;unsigned reused=0,clipped=0;
        for(unsigned frame=0;frame<322;++frame){
            const unsigned scroll=frame<=160?frame:321-frame;
            const unsigned origin=(270+scroll)%downwellGameRingHeight;
            downwell::TimeFieldOutlines shape;shape.visible=true;shape.count=2;
            shape.circles[0]={190,180-int(scroll),24*256+int(frame%20)};
            shape.circles[1]={200,205-int(scroll),24*256};
            const bool same=retained.matches(actual.data(),origin,shape);
            unsigned inspected=0;
            require(retained.prepare(actual.data(),origin,shape,false,[&](unsigned){++inspected;return false;}),
                "prepare ring-stationary circles during bidirectional scrolling");
            retained.draw();
            if(same){++reused;require(!inspected&&!retained.lastWordWrites,
                "hardware scrolling reuses identical physical field words");}
            else if(frame)++clipped;
            expected=base;AmigaTimeFields fresh;
            require(fresh.prepare(expected.data(),origin,shape,false,[](unsigned){return false;}),"fresh scrolling oracle");
            fresh.draw();require(actual==expected,"scroll reuse matches fresh clipping, painter order and ring wrap");
            if(same&&frame==80){
                require(retained.prepare(actual.data(),origin,shape,true,[&](unsigned){++inspected;return false;}),
                    "underlying change forces preparation even with identical physical coverage");
                retained.draw();require(inspected&&actual==expected,"dirty-backing dependency remains observed");
            }
        }
        require(reused>280&&clipped>0,"scroll fixture covers retained and boundary-clipped cases");
        std::cout<<"322 bidirectional scrolling field frames: "<<reused<<" physical-raster reuses passed\n";
    }
    for(unsigned offset=0;offset<downwellGameRingPlaneBytes;offset+=2)
        require(AmigaTimeFields::rowOf(offset)==offset/downwellBytesPerPlaneLine,"exact field row reciprocal");
    {
        // Two retained banks and one work bank may rotate in either order.
        // Compare against the copying implementation, including pending draws
        // surviving preparation for the other display and target replacement.
        AmigaTimeFieldPair exchanged;
        AmigaTimeFields reference[2];
        std::vector<UBYTE> actual[2],expected[2],replacement[2],replacementReference[2];
        for(unsigned b=0;b<2;++b){
            actual[b].resize(downwellGameRingBytes);
            for(unsigned i=0;i<actual[b].size();++i)actual[b][i]=i*19+b*53;
            expected[b]=actual[b];replacement[b]=actual[b];replacementReference[b]=actual[b];
        }
        for(unsigned frame=0;frame<256;++frame){
            const unsigned first=frame&1;
            for(unsigned n=0;n<2;++n){
                const unsigned b=first^n;
                downwell::TimeFieldOutlines shape;
                shape.count=frame%13?2:0;shape.visible=shape.count!=0;
                shape.circles[0]={190+int(frame%17)-8,130-int(frame%71),128*256+int(frame%512)};
                shape.circles[1]={120+int(frame%113),240-int(frame%89),24*256+int(frame*7%512)};
                const unsigned origin=frame*11%downwellGameRingHeight;
                const bool changed=frame%7==0;
                auto dirty=[&](unsigned row){return changed&&row%11==frame%11;};
                auto* a=frame<128?actual[b].data():replacement[b].data();
                auto* e=frame<128?expected[b].data():replacementReference[b].data();
                require(exchanged[b].prepare(a,origin,shape,changed,dirty),"exchange preparation");
                require(reference[b].prepare(e,origin,shape,changed,dirty),"reference preparation");
            }
            for(unsigned n=0;n<2;++n){
                const unsigned b=first^n^1;
                exchanged[b].draw();reference[b].draw();
                require(actual[b]==expected[b]&&replacement[b]==replacementReference[b],
                    "bank exchange preserves pending geometry, underlay and target replacement");
                require(exchanged[b].lastWordWrites==reference[b].lastWordWrites,"same planar writes");
            }
            if(frame%31==0)for(unsigned b=0;b<2;++b){exchanged[b].discard();reference[b].discard();}
        }
        std::cout << "512 exchanged field banks match copying reference with reverse draw and target changes\n";
    }
    {
        std::vector<UBYTE> base(downwellGameRingBytes,0x69),actual=base,expected=base;
        AmigaTimeFields retained;
        constexpr unsigned origin=319;
        for(unsigned trial=0;trial<1024;++trial){
            downwell::TimeFieldOutlines shape;
            shape.visible=true;shape.count=1;
            shape.circles[0]={190,140,int((trial<512?24:128)*256+trial%512)};
            require(retained.prepare(actual.data(),origin,shape,false,[](unsigned){return false;}),"stationary field prepare");
            retained.draw();
            expected=base;
            bool covered[320]{};
            AmigaRenderer oracle;amigaRendererInit(&oracle,expected.data(),nullptr);
            amigaRendererSetGameOrigin(&oracle,origin);
            amigaRendererSetClipRect(&oracle,110,0,160,284);
            shape.draw([&](int x,int y,int pen){
                if(x>=110&&x<270&&y>=0&&y<284)covered[(origin+unsigned(y))%320]=true;
                amigaRendererFillRect(&oracle,x,y,1,1,pen);
            });
            require(actual==expected,"stationary field matches fresh source-radius pixel oracle");
            const unsigned dirty=trial*13%320;
            require(retained.touchesDirtyRows([&](unsigned row){return row==dirty;})==covered[dirty],
                "retained physical footprint matches pixel-raster dirty-row detection");
            if(trial%64==0)for(unsigned checkRow=0;checkRow<320;++checkRow)
                require(retained.touchesDirtyRows([&](unsigned row){return row==checkRow;})==covered[checkRow],
                    "every physical row agrees across ring wrap and both circle sizes");
            require(retained.prepare(actual.data(),origin,shape,covered[dirty],[&](unsigned row){return row==dirty;}),
                "dirty backing overrides unchanged snapshot reuse");
            for(unsigned p=0;p<2;++p)for(unsigned x=0;x<48;++x){
                const unsigned at=p*downwellGameRingPlaneBytes+dirty*48+x;
                actual[at]=base[at]^=0x53;
            }
            retained.draw();expected=base;
            shape.draw([&](int x,int y,int pen){amigaRendererFillRect(&oracle,x,y,1,1,pen);});
            require(actual==expected,"dirty underlay is refreshed under stationary field coverage");
        }
        downwell::TimeFieldOutlines absent;
        require(retained.prepare(actual.data(),origin,absent,false,[](unsigned){return false;})&&actual==base,
            "retiring stationary field restores the latest changed backing");
        std::cout<<"1024 stationary-field and changing-backing comparisons passed\n";
    }
    {
        std::vector<UBYTE> base(downwellGameRingBytes), actual(base.size()), expected(base.size());
        for (unsigned i = 0; i < base.size(); ++i) base[i] = i * 71 + 13;
        AmigaGoalField retained;
        downwell::GoalTimeField field;
        field.x = 240; field.y = 4000;
        actual = base;
        for (unsigned trial = 0; trial < 512; ++trial) {
            field.radius = downwell::HudReal::ratio(128 * 256 + trial, 256);
            const unsigned origin = (trial * 23) % downwellGameRingHeight;
            downwell::GoalFieldOutline shape;
            shape.set(field, true, 81 + (trial % 159), 3800 + (trial % 320), downwellPlayfieldX);
            require(retained.prepare(actual.data(), origin, shape, false, [](unsigned) { return false; }), "field fits sparse storage");
            retained.draw();
            expected = base;
            AmigaRenderer oracle;
            amigaRendererInit(&oracle, expected.data(), nullptr);
            amigaRendererSetGameOrigin(&oracle, origin);
            amigaRendererSetClipRect(&oracle, downwellPlayfieldX, 0, downwellPlayfieldWidth, downwellDisplayHeight);
            shape.draw([&](int x, int y, int pen) { amigaRendererFillRect(&oracle, x, y, 1, 1, pen); });
            require(actual == expected, "retained field equals fresh scalar circle composition through ring wrap");
            require(retained.prepare(actual.data(), origin, shape, false, [](unsigned) { return false; }), "unchanged field fits");
            retained.draw();
            require(retained.lastWordWrites == 0 && actual == expected,
                    "unchanged circle geometry performs no plane writes");
            const unsigned dirty = (origin + 140) % downwellGameRingHeight;
            require(retained.prepare(actual.data(), origin, shape, true,
                    [dirty](unsigned row) { return row == dirty; }), "dirty-row field fits");
            for (unsigned plane = 0; plane < 2; ++plane)
                for (unsigned x = 0; x < downwellBytesPerPlaneLine; ++x) {
                    const unsigned at = plane * downwellGameRingPlaneBytes + dirty * downwellBytesPerPlaneLine + x;
                    base[at] ^= 0x51;
                    actual[at] = base[at];
                }
            retained.draw();
            expected = base;
            shape.draw([&](int x, int y, int pen) { amigaRendererFillRect(&oracle, x, y, 1, 1, pen); });
            require(actual == expected, "terrain repair beneath a retained field refreshes only affected rows");
            actual[0] ^= 1; base[0] ^= 1;
            if ((trial & 15) == 15) {
                downwell::GoalFieldOutline absent;
                require(retained.prepare(actual.data(), origin, absent, false, [](unsigned) { return false; }), "field retirement fits");
                require(actual == base, "circle retirement restores only its changed bits, including outline overlaps");
            }
        }
        std::cout << "512 retained goal-field radius/ring comparisons passed\n";
    }
    {
        alignas(4) unsigned char a[68], b[68];
        for (unsigned offset : {0u, 2u}) for (unsigned length = 0; length <= 64; ++length) {
            for (unsigned i = 0; i < 68; ++i) a[i] = b[i] = i*37;
            require(!amigaBufferDifferences(a+offset,b+offset,length), "equal aligned buffers compare exactly");
            b[offset+length] ^= 255;
            require(!amigaBufferDifferences(a+offset,b+offset,length), "comparison ignores bytes beyond the requested range");
            b[offset+length] ^= 255;
            for (unsigned i = 0; i < length; ++i) {
                for (unsigned bit = 0; bit < 8; ++bit) {
                    b[offset+i] ^= 1u << bit;
                    require(amigaBufferDifferences(a+offset,b+offset,length) == 1,
                            "word comparison detects every byte and bit including tail bytes");
                    b[offset+i] ^= 1u << bit;
                }
            }
            for (unsigned i = 0; i < length; ++i) b[offset+i] ^= 255;
            require(amigaBufferDifferences(a+offset,b+offset,length) == length,
                    "word comparison preserves the exact mismatching-byte count");
        }
    }
    {
        std::vector<UBYTE> pixels(downwellGameRingBytes), pristine;
        for (unsigned i = 0; i < pixels.size(); ++i) pixels[i] = (i*37u + i/11u) & 255u;
        pristine = pixels;
        AmigaLaserSight overlay;
        for (unsigned pen = 0; pen < 4; ++pen) {
            std::vector<UBYTE> solid(downwellGameRingBytes, (pen & 1) ? 255 : 0);
            std::fill(solid.begin()+downwellGameRingPlaneBytes, solid.end(), (pen & 2) ? 255 : 0);
            const auto before = solid;
            const downwell::LaserSightLine point{191,283,191,283};
            AmigaLaserSight single;
            single.prepare(solid.data(), 319, &point, false);
            single.draw();
            const unsigned changedPlanes = (pen & 1 ? 1 : 0) + (pen & 2 ? 0 : 1);
            require(single.lastByteWrites == changedPlanes, "red pixel writes only planes whose bits change");
            single.prepare(solid.data(), 319, nullptr, false);
            require(single.lastByteWrites == changedPlanes && solid == before,
                    "each source pen restores with exactly its changed-plane writes");
            const downwell::LaserSightLine diagonal{110,0,269,283};
            single.prepare(solid.data(), 319, &diagonal, false); single.draw();
            require(single.lastByteWrites == 284*changedPlanes,
                    "full-height ray saves both changed planes without truncation");
            const auto painted = solid;
            single.draw();
            require(single.lastByteWrites == 284*changedPlanes && solid == painted,
                    "repeated draw without prepare neither republishes nor duplicates saved bytes");
            single.prepare(solid.data(), 319, nullptr, false);
            require(single.lastByteWrites == 284*changedPlanes && solid == before,
                    "full-height two-plane restoration preserves every original byte");
        }
        for (unsigned origin = 0; origin < 320; ++origin)
            for (unsigned y = 0; y < 284; ++y)
                require(AmigaLaserSight::physicalRow(origin,y) == (origin+y)%320,
                        "bounded ring addressing matches modulo for every visible row and origin");
        for (unsigned origin : {0u, 319u, 103u}) {
            for (const auto& line : {downwell::LaserSightLine{190,-10,190,300},
                 downwell::LaserSightLine{80,0,300,283},
                 downwell::LaserSightLine{300,283,80,0},
                 downwell::LaserSightLine{100,150,280,150},
                 downwell::LaserSightLine{190,100,190,100}}) {
                require(overlay.prepare(pixels.data(), origin, &line, false), "new ray requires drawing");
                overlay.draw();
                auto expected = pristine;
                require(overlay.matches(pixels.data(), origin, line), "drawn ray is eligible for retention");
                require(!overlay.matches(pixels.data(), (origin+1)%320, line), "scroll changes physical coverage");
                auto moved = line; ++moved.x0;
                require(!overlay.matches(pixels.data(), origin, moved), "moved endpoint cannot retain old coverage");
                line.draw([&](int x, int y) {
                    if (x < 110 || x >= 270 || y < 0 || y >= 284) return;
                    const unsigned offset = ((origin+y)%320)*48 + x/8;
                    const unsigned mask = 128u >> (x%8);
                    expected[offset] &= ~mask;
                    expected[offset+15360] |= mask;
                });
                require(pixels == expected, "sparse ray equals clipped planar red-line composition across ring wrap");
                require(!overlay.prepare(pixels.data(), origin, &line, false), "unchanged ray is retained");
                overlay.draw();
                require(overlay.lastByteWrites == 0 && pixels == expected,
                        "unchanged ray performs no clearing or drawing writes");
                overlay.prepare(pixels.data(), origin, nullptr, false);
                require(pixels == pristine, "ray removal restores both planes without damaging neighbors");
            }
        }
        const downwell::LaserSightLine line{190,0,190,283};
        overlay.prepare(pixels.data(), 319, &line, false);
        require(overlay.present() && !overlay.retainsPixels(),
                "pending ray grants underlying rectangle ownership before draw");
        overlay.draw();
        require(overlay.retainsPixels(), "drawn ray retains bitmap pixels");
        overlay.prepare(pixels.data(), 319, &line, false);
        require(overlay.retainsPixels(), "unchanged ray retains rectangle ownership");
        overlay.prepare(pixels.data(), 319, &line, true);
        require(overlay.present() && !overlay.retainsPixels(),
                "changed underlying pixels retire the ray before field composition");
        require(pixels == pristine, "underlying updates see the original pixels before painting");
        // Simulate new terrain arriving after restoration, before ray draw.
        pixels[319*48+190/8] ^= 0xff;
        pristine = pixels;
        overlay.draw();
        overlay.prepare(pixels.data(), 319, nullptr, false);
        require(pixels == pristine, "removing ray preserves the newly painted terrain, not stale backing");
        std::vector<UBYTE> red(downwellGameRingBytes, 0);
        std::fill(red.begin()+downwellGameRingPlaneBytes, red.end(), 255);
        overlay.prepare(red.data(), 0, &line, false); overlay.draw();
        require(overlay.lastByteWrites == 0, "already-red pixels are never rewritten");
        overlay.prepare(red.data(), 0, nullptr, false);
        require(overlay.lastByteWrites == 0, "already-red pixels need no restoration");
    }
    verifyDamageEndpoints();
    verifyVisitedDamage();
    {
        UWORD overlay[downwellDisplayHeight*2];
        for (unsigned i = 0; i < downwellDisplayHeight*2; ++i)
            overlay[i] = (i % 7) ? (i*173u + 0x1257u) : 0;
        for (unsigned word : {0u, 18u, 22u})
            for (int shift : {-12, 0, 12})
                for (int top : {0, 41, 280})
                    for (unsigned width : {8u, 12u, 16u}) {
                        const int bottom = top+44 < 284 ? top+44 : 284;
                        std::vector<UBYTE> working(downwellLayerBytes), actual(downwellLayerBytes, 0x96);
                        for (unsigned i = 0; i < working.size(); ++i) working[i] = i*73u + i/31u;
                        auto expectedWorking = working, expected = actual;
                        AmigaRenderer oracle;
                        amigaRendererInit(&oracle, nullptr, expectedWorking.data());
                        amigaRendererSetClipRect(&oracle, word*16, top, width, bottom-top);
                        amigaRendererApplyHudColumns(&oracle, overlay, word, 1, shift, top, bottom, 1);
                        const unsigned writes = amigaRendererCopyHudWords(expected.data(),
                            expectedWorking.data(), word*16, top, word*16+width, bottom, 1);
                        const UWORD clip = static_cast<UWORD>(0xffffu << (16-width));
                        require(amigaRendererPublishHudColumn(actual.data(), working.data(), overlay,
                            word, top, bottom, shift, clip) == writes,
                            "fused publication preserves changed-plane write counts");
                        require(actual == expected && working == expectedWorking,
                            "fused publication equals separate masked composition and copy");
                        require(amigaRendererPublishHudColumn(actual.data(), working.data(), overlay,
                            word, top, bottom, shift, clip) == 0,
                            "repeated fused publication writes neither display plane");
                    }
    }
    {
        std::vector<UBYTE> source(downwellLayerBytes), actual(downwellLayerBytes), expected;
        for (unsigned i = 0; i < source.size(); ++i) source[i] = (i*73u + i/31u) & 255;
        for (int word = 0; word < 24; ++word)
            for (unsigned planes = 0; planes < 4; ++planes) {
                actual = source;
                for (unsigned plane = 0; plane < 2; ++plane)
                    if (planes & (1u << plane))
                        for (unsigned y = 0; y < 284; y += 3)
                            actual[plane*downwellPlaneBytes+y*downwellBytesPerPlaneLine+word*2] ^= 0xa5;
                expected = actual;
                unsigned count = 0;
                for (unsigned plane = 0; plane < 2; ++plane)
                    for (unsigned y = 7; y < 279; ++y) {
                        const unsigned offset = plane*downwellPlaneBytes+y*downwellBytesPerPlaneLine+word*2;
                        if (expected[offset] != source[offset] || expected[offset+1] != source[offset+1]) ++count;
                        expected[offset] = source[offset]; expected[offset+1] = source[offset+1];
                    }
                require(amigaRendererCopyHudWords(actual.data(), source.data(), word*16+1, 7,
                            word*16+11, 279, 1) == count,
                        "one-word publication counts only changed planes");
                require(actual == expected, "one-word publication preserves other rows and columns");
                require(amigaRendererCopyHudWords(actual.data(), source.data(), word*16+1, 7,
                            word*16+11, 279, 1) == 0, "repeated publication performs no writes");
            }
    }
    {
        std::vector<UBYTE> actual(downwellLayerBytes, 0x5a), expected = actual;
        for (unsigned plane = 0; plane < 2; ++plane)
            for (unsigned y = 3; y < 281; ++y) {
                const unsigned offset = plane*downwellPlaneBytes + y*downwellBytesPerPlaneLine + 36;
                putWord(expected.data(), offset, (0x5a5a & ~0xfff0) | (plane ? 0x8010 : 0xfff0));
            }
        amigaRendererFillHudWordColumn(actual.data(), 18, 3, 281, 0xfff0, 0xffff, 0x801f);
        require(actual == expected, "word column preserves adjacent pixels, rows and planes");
        amigaRendererFillHudWordColumn(actual.data(), 24, 0, 284, 0xffff, 0, 0);
        amigaRendererFillHudWordColumn(actual.data(), 0, 0, 285, 0xffff, 0, 0);
        amigaRendererFillHudWordColumn(actual.data(), 0, 12, 12, 0xffff, 0, 0);
        require(actual == expected, "invalid or empty word columns do not write");
    }
    const auto storage = GameplaySceneStorage::partition(76800, 30720, 13940);
    require(storage.valid && storage.resultsOffset == 30720 &&
                storage.resultsCapacity == 32140 && storage.hudOffset == 62860,
            "results pack fits between live staging ring and relocated HUD");
    require(storage.resultsCapacity >= 25820, "current offline results pack fits without allocation");
    require(AmigaExitMask::bytes == 31104 && storage.resultsCapacity >= AmigaExitMask::bytes,
        "offline exit mask fits retired intro bank without touching staging or HUD");
    for (int top = -100; top <= 300; ++top) {
        const unsigned offset = AmigaExitMask::offset(top);
        require(offset % 48 == 0 && offset + 284 * 48 <= AmigaExitMask::bytes,
            "every exit wipe DMA window stays within immutable mask bank");
    }
    require(AmigaExitMask::offset(-100) == 364 * 48 && AmigaExitMask::offset(284) == 0,
        "exit DMA endpoints select transparent and solid shaft coverage");
    require(!GameplaySceneStorage::partition(100, 101, 0).valid &&
                !GameplaySceneStorage::partition(100, 80, 21).valid &&
                !GameplaySceneStorage::partition(100, 99, 1).valid,
            "invalid and alignment-overlapping partitions rejected");
    require(GameplaySceneStorage::partition(100, 80, 17).hudOffset == 80,
            "unaligned asset sizes reserve padding without touching staging");
    UBYTE pack[48];
    UWORD sprite[6] = {};
    buildOpaqueSpritePack(pack, sizeof(pack));

    int result = amigaRendererBuildHardwareSprite(
        pack, pack + sizeof(pack), 7, 0, downwellPlayfieldX + 4, 10, 0,
        sprite, 1);
    require(result == 1 && sprite[2] == 0x0fff,
            "left-edge hardware sprite is masked to the game playfield");

    result = amigaRendererBuildHardwareSprite(
        pack, pack + sizeof(pack), 7, 0,
        downwellPlayfieldX + downwellPlayfieldWidth - 4, 10, 0, sprite, 1);
    require(result == 1 && sprite[2] == 0xfff0,
            "right-edge hardware sprite is masked to the game playfield");

    result = amigaRendererBuildHardwareSprite(
        pack, pack + sizeof(pack), 7, 0, downwellPlayfieldX - 20, 10, 0,
        sprite, 1);
    require(result == 2 && sprite[0] == 0 && sprite[1] == 0,
            "fully clipped hardware sprite produces a hidden record");

    require(argc == 3, "Amiga game and HUD pack paths are supplied");
    AmigaResolvedSpritePart missing{};
    UWORD untouched[4] = {1, 2, 3, 4};
    require(!amigaRendererResolveSpritePart(&missing, pack, pack+sizeof pack, 7, 0, 0, 0) &&
        !amigaRendererDrawResolvedSpritePart(&missing, 100, 10, untouched, 0) &&
        untouched[0] == 1 && untouched[1] == 2 && untouched[2] == 3 && untouched[3] == 4,
        "resolved API rejects missing offline artwork without modifying destination");
    int bx, by, bw, bh;
    require(amigaRendererAssetBounds(pack, pack + sizeof pack, 7, 0, 20, 10, 0, &bx, &by, &bw, &bh),
        "asset lookup primes same-address bank cache");
    putLong(pack, 20, 8UL << 16);
    require(!amigaRendererAssetBounds(pack, pack + sizeof pack, 7, 0, 20, 10, 0, &bx, &by, &bw, &bh) &&
        amigaRendererAssetBounds(pack, pack + sizeof pack, 8, 0, 20, 10, 0, &bx, &by, &bw, &bh),
        "same-size bank replacement cannot return a cached different resource");
    putLong(pack, 8, 0);
    require(!amigaRendererAssetBounds(pack, pack + sizeof pack, 8, 0, 20, 10, 0, &bx, &by, &bw, &bh),
        "shrunk directory cannot return an old cached record");
    buildOpaqueSpritePack(pack, sizeof pack);
    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<UBYTE> gamePack{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()};
    require(!gamePack.empty(), "Amiga game pack can be read");
    const unsigned runtimeSprites[][2] = {
        {0, 2}, {1, 8}, {3, 4}, {12, 8}, {27, 8}, {31, 4}, {37, 5},
        {127, 1}, {133, 4}, {134, 4}, {139, 2}, {153, 4}, {154, 1}, {157, 4}, {162, 4},
        {163, 4}, {164, 1}, {166, 4}, {168, 2}, {170, 4}, {173, 4},
        {177, 4}, {178, 1}, {186, 4}, {187, 1}, {188, 6}, {278, 2},
        {459, 1}, {485, 4}, {486, 5}, {487, 2}, {496, 4}, {497, 2}, {707, 6},
        {550, 17}, {568, 3}, {569, 4}
    };
    for (const auto& requirement : runtimeSprites)
        requireRuntimeSprite(gamePack, requirement[0], requirement[1]);
    for (unsigned sprite : {480u, 481u, 482u}) {
        int left, top, width, height;
        require(amigaRendererAssetBounds(gamePack.data(), gamePack.data() + gamePack.size(),
            sprite, 0, 197, 93, 0, &left, &top, &width, &height) && width > 0 && height > 0,
            "offline game pack contains every static shop fixture");
    }
    for (unsigned sprite : {143u, 147u, 149u, 150u, 151u, 152u}) {
        int left, top, width, height;
        require(amigaRendererAssetBounds(gamePack.data(), gamePack.data() + gamePack.size(),
            sprite, 1, 197, 93, 0, &left, &top, &width, &height) && width > 0 && height > 0,
            "offline pack contains frame one of all six cased consumables");
    }

    require(amigaRendererHardwareSpritePartCount(
                gamePack.data(), gamePack.data() + gamePack.size(),
                163, 0) == 1,
            "sixteen-pixel snake artwork keeps one hardware sprite");
    require(amigaRendererHardwareSpritePartCount(
                gamePack.data(), gamePack.data() + gamePack.size(),
                186, 0) == 2,
            "wide turtle artwork requests two hardware sprites");
    const auto titlePack = buildWideTitlePack();
    require(amigaRendererHardwareSpritePartCount(
                titlePack.data(), titlePack.data() + titlePack.size(),
                654, 9) == 7,
            "Downwell title artwork requests its seven opaque slices");
    UWORD turtleLeft[2 + 2 * 32 + 2] = {};
    UWORD turtleRight[2 + 2 * 32 + 2] = {};
    require(amigaRendererBuildHardwareSpritePart(
                gamePack.data(), gamePack.data() + gamePack.size(),
                186, 0, 0, downwellPlayfieldX + 80, 40, 0,
                turtleLeft, 32) == 1 &&
            amigaRendererBuildHardwareSpritePart(
                gamePack.data(), gamePack.data() + gamePack.size(),
                186, 0, 1, downwellPlayfieldX + 80, 40, 0,
                turtleRight, 32) == 1 &&
            reinterpret_cast<UBYTE*>(turtleRight)[0] ==
                reinterpret_cast<UBYTE*>(turtleLeft)[0] &&
            reinterpret_cast<UBYTE*>(turtleRight)[1] ==
                reinterpret_cast<UBYTE*>(turtleLeft)[1] + 8,
            "wide turtle halves occupy adjacent sixteen-pixel slices");

    UWORD titleLeft[2 + 2 * 32 + 2] = {};
    UWORD titleRight[2 + 2 * 32 + 2] = {};
    require(amigaRendererBuildHardwareSpritePart(
                titlePack.data(), titlePack.data() + titlePack.size(),
                654, 9, 0, downwellPlayfieldX + 80, 40, 0,
                titleLeft, 32) == 1 &&
            amigaRendererBuildHardwareSpritePart(
                titlePack.data(), titlePack.data() + titlePack.size(),
                654, 9, 6, downwellPlayfieldX + 80, 40, 0,
                titleRight, 32) == 1 &&
            reinterpret_cast<UBYTE*>(titleRight)[1] ==
                reinterpret_cast<UBYTE*>(titleLeft)[1] + 48,
            "seven-part title spans consecutive hardware-sprite slices");

    UWORD clippedTitle[2 + 2 * 32 + 2] = {};
    require(amigaRendererBuildHardwareSpritePart(
                titlePack.data(), titlePack.data() + titlePack.size(),
                654, 9, 0, downwellPlayfieldX + 80, 8, 0,
                clippedTitle, 32) == 1 &&
            reinterpret_cast<UBYTE*>(clippedTitle)[0] ==
                downwellDisplayVStart &&
            reinterpret_cast<UBYTE*>(clippedTitle)[2] ==
                downwellDisplayVStart + 24,
            "hardware-sprite strips clip cleanly through the display top");

    // Remove only the prepared DMA directory entries, retaining original payload
    // offsets, to run the old pixel converter as an independent oracle.
    auto oraclePack = gamePack;
    auto word = [](const UBYTE* p) { return unsigned(p[0]) * 256 + p[1]; };
    const unsigned records = (word(gamePack.data() + 8) << 16) |
                             word(gamePack.data() + 10);
    {
        auto longword=[&](const UBYTE* p){return (word(p)<<16)|word(p+2);};
        const auto bytes=amigaRendererChipPackBytes(gamePack.data(),gamePack.data()+gamePack.size());
        require(bytes>20&&bytes<gamePack.size(),"compact resident pack size");
        std::vector<UBYTE> compact(bytes,0x5a);
        require(!amigaRendererCopyChipPack(gamePack.data(),gamePack.data()+gamePack.size(),compact.data(),bytes-1),
            "short destination rejects before writing");
        for(auto value:compact)require(value==0x5a,"failed resident copy leaves destination untouched");
        auto malformed=gamePack;
        putLong(malformed.data(),8,0xffffffffu);
        require(amigaRendererChipPackBytes(malformed.data(),malformed.data()+malformed.size())==0&&
                !amigaRendererCopyChipPack(malformed.data(),malformed.data()+malformed.size(),compact.data(),bytes),
                "invalid directory count rejects without overflow");
        malformed=gamePack;putLong(malformed.data(),36,0xffffffffu);
        require(!amigaRendererCopyChipPack(malformed.data(),malformed.data()+malformed.size(),compact.data(),bytes),
                "invalid payload offset rejects before copying");
        for(auto value:compact)require(value==0x5a,"malformed pack leaves destination untouched");
        require(amigaRendererCopyChipPack(gamePack.data(),gamePack.data()+gamePack.size(),compact.data(),bytes),
            "compact resident copy");
        unsigned keptRecords=0,omitted=0;
        for(unsigned i=0;i<records;++i){
            const auto* source=gamePack.data()+20+i*24;const unsigned flags=word(source+14);
            if((flags&4)&&(flags>>8)){++omitted;continue;}
            const auto* target=compact.data()+20+keptRecords++*24;
            require(std::memcmp(source,target,16)==0&&longword(source+20)==longword(target+20),
                "resident metadata preserves IDs, frames, origins, dimensions and flags");
            const auto offset=longword(target+16),length=longword(target+20);
            require(offset<=bytes&&length<=bytes-offset&&!(offset&1),"resident payload bounds/alignment");
            require(std::memcmp(gamePack.data()+longword(source+16),compact.data()+offset,length)==0,
                "every resident planar/garden payload remains byte-identical");
        }
        require(omitted>0&&keptRecords==longword(compact.data()+8),"only gameplay DMA directory omitted");
        require(amigaRendererChipPackBytes(compact.data(),compact.data()+bytes)==bytes,"resident copy is idempotent");
        downwell::EffectMaskAssets effectAssets;
        require(effectAssets.bind(compact.data(),compact.size()),"effect masks resolve compact resident directory");
        std::cout<<"Resident directory: "<<omitted<<" CPU-only records removed, "<<omitted*24<<" Chip bytes saved\n";
    }
    unsigned kept = 0;
    for (unsigned i = 0; i < records; ++i) {
        const auto* record = gamePack.data() + 20 + i * 24;
        if (word(record) >= 62000 && word(record) < 64000) continue;
        std::memcpy(oraclePack.data() + 20 + kept++ * 24, record, 24);
    }
    putLong(oraclePack.data(), 8, kept);
    unsigned comparisons = 0;
    for (unsigned i = 0; i < records; ++i) {
        const auto* record = gamePack.data() + 20 + i * 24;
        const unsigned id = word(record), variant = word(record + 2);
        if (id < 63000 || id >= 64000) continue;
        for (int x : {100, 110, 115, 190, 263, 270, 285})
            for (int y : {-35, -3, 0, 40, 275, 284, 310}) {
                UWORD actual[68] = {}, expected[68] = {};
                const int a = amigaRendererBuildHardwareSpritePart(
                    gamePack.data(), gamePack.data() + gamePack.size(),
                    id - 63000, variant / 8, variant % 4,
                    x, y, (variant % 8) / 4, actual, 32);
                AmigaResolvedSpritePart resolved;
                UWORD resolvedPixels[68] = {};
                require(amigaRendererResolveSpritePart(&resolved, gamePack.data(),
                    gamePack.data()+gamePack.size(), id-63000, variant/8,
                    variant%4, (variant%8)/4), "offline part resolves for direct drawing");
                require(resolved.partCount == amigaRendererHardwareSpritePartCount(
                    gamePack.data(), gamePack.data()+gamePack.size(), id-63000, variant/8),
                    "resolved orientation preserves hardware channel requirements");
                require(amigaRendererDrawResolvedSpritePart(&resolved, x, y, resolvedPixels, 32) == a &&
                    !std::memcmp(actual, resolvedPixels, sizeof actual),
                    "resolved hardware part matches lookup path at every tested edge");
                const int b = amigaRendererBuildHardwareSpritePart(
                    oraclePack.data(), oraclePack.data() + oraclePack.size(),
                    id - 63000, variant / 8, variant % 4,
                    190, y, (variant % 8) / 4, expected, 32);
                // Horizontal edges must preserve the entire sprite body: the
                // opaque HUD playfield, not a runtime pixel mask, covers it.
                if (a == 2) {
                    require(actual[0] == 0 && actual[1] == 0,
                            "offscreen actor is hidden");
                } else {
                    std::memcpy(expected, actual, 4); // position differs
                    require(a == b && !std::memcmp(actual, expected, sizeof(actual)),
                            "offline DMA body matches oracle without horizontal clipping");
                }
                ++comparisons;
            }
    }
    require(comparisons > 1000, "gameplay pack contains offline sprite variants");
    std::cout << comparisons << " offline sprite oracle comparisons\n";
    std::vector<UBYTE> actualGame(downwellGameRingBytes), expectedGame(downwellGameRingBytes);
    std::vector<UBYTE> actualHud(downwellLayerBytes), expectedHud(downwellLayerBytes);
    AmigaRenderer actualRenderer, expectedRenderer;
    for(unsigned origin:{0u,311u,319u})for(int shift=0;shift<16;++shift)for(int edge:{110,190,266}){
        std::fill(actualGame.begin(),actualGame.end(),0x5a);expectedGame=actualGame;
        amigaRendererInit(&actualRenderer,actualGame.data(),actualHud.data());
        amigaRendererSetGameOrigin(&actualRenderer,origin);
        amigaRendererSetClipRect(&actualRenderer,110,0,160,284);
        downwell::DamageNumberText label;label.formatInteger(12);
        const int anchor=edge+shift,anchorY=3;
        label.draw(anchor,anchorY,[&](unsigned glyph,int x,int y,unsigned pen){
            amigaRendererDrawAssetPen(&actualRenderer,gamePack.data(),gamePack.data()+gamePack.size(),glyph,0,x,y,0,pen);
        });
        const unsigned glyphs[]={65110,65101,65102};
        const int dx[]={1,-1,0,0,0},dy[]={0,0,1,-1,0};
        for(unsigned pass=0;pass<5;++pass)for(unsigned g=0;g<3;++g){
            const UBYTE* record=nullptr;
            for(unsigned i=0;i<records;++i){const auto* r=gamePack.data()+20+i*24;if(word(r)==glyphs[g]&&word(r+2)==0){record=r;break;}}
            require(record!=nullptr,"damage text glyph exists");
            const int stride=word(record+12)/8,height=word(record+10),width=word(record+8);
            const auto* data=gamePack.data()+word(record+16)*65536u+word(record+18);
            for(int sy=0;sy<height;++sy)for(int sx=0;sx<width;++sx){
                const unsigned bit=128u>>(sx&7),offset=sy*stride+sx/8;
                if(!((data[offset]|data[offset+stride*height])&bit))continue;
                const int x=anchor-12+int(g)*8+dx[pass]+sx,y=anchorY+dy[pass]+sy;
                if(x<110||x>=270||y<0||y>=284)continue;
                for(unsigned plane=0;plane<2;++plane){
                    auto& byte=expectedGame[plane*downwellGameRingPlaneBytes+((origin+y)%320)*48+x/8];
                    const unsigned mask=128u>>(x&7),pen=pass==4?2:0;
                    byte=(byte&~mask)|((pen&(1u<<plane))?mask:0);
                }
            }
        }
        require(actualGame==expectedGame,"damage text CPU glyph writes match scalar outline, clipping and circular-buffer oracle");
    }
    unsigned tileComparisons = 0;
    for (unsigned i = 0; i < records; ++i) {
        const auto* r = gamePack.data() + 20 + i * 24;
        if (word(r) != 51) continue;
        const int ox = static_cast<short>(word(r+4)), oy = static_cast<short>(word(r+6));
        const int width = word(r+8), height = word(r+10), stride = word(r+12)/8;
        const auto* data = gamePack.data() + (word(r+16)*65536u + word(r+18));
        for (unsigned origin : {0u, 311u, downwellGameRingHeight-1u})
            for (int shift = 0; shift < 16; ++shift)
                for (int clipped : {0, 1, 2}) {
                    std::fill(actualGame.begin(), actualGame.end(), 0x5a);
                    expectedGame = actualGame;
                    amigaRendererInit(&actualRenderer, actualGame.data(), actualHud.data());
                    amigaRendererSetGameOrigin(&actualRenderer, origin);
                    const int x = 110 + shift, y = 8;
                    const int cx = clipped ? x-3 : 0, cy = clipped == 2 ? 7 : 0;
                    const int cw = clipped ? 9 : downwellVisibleWidth;
                    const int ch = clipped == 2 ? 3 : downwellDisplayHeight;
                    amigaRendererSetClipRect(&actualRenderer, cx, cy, cw, ch);
                    amigaRendererDrawAsset(&actualRenderer, gamePack.data(), gamePack.data()+gamePack.size(),
                        51, word(r+2), x, y, 0);
                    for (int sy = 0; sy < height; ++sy)
                        for (int sx = 0; sx < width; ++sx) {
                            const int dx = x-ox+sx, dy = y-oy+sy;
                            if (dx < cx || dx >= cx+cw || dy < cy || dy >= cy+ch ||
                                dx < 0 || dx >= downwellVisibleWidth || dy < 0 || dy >= downwellDisplayHeight) continue;
                            const unsigned bit = 0x80u >> (sx&7);
                            const unsigned offset = sy*stride + sx/8;
                            const unsigned pen = ((data[offset]&bit) ? 1 : 0) |
                                ((data[offset+stride*height]&bit) ? 2 : 0);
                            if (!pen) continue;
                            for (unsigned plane = 0; plane < 2; ++plane) {
                                auto& dest = expectedGame[plane*downwellGameRingPlaneBytes +
                                    ((origin+dy)%downwellGameRingHeight)*downwellBytesPerPlaneLine + dx/8];
                                const unsigned mask = 0x80u >> (dx&7);
                                dest = (dest & ~mask) | ((pen&(1u<<plane)) ? mask : 0);
                            }
                        }
                    require(actualGame == expectedGame, "tile word composition matches scalar clipping and ring-wrap oracle");
                    ++tileComparisons;
                }
    }
    require(tileComparisons > 1000, "terrain tile oracle exercises all frames and word shifts");
    std::cout << tileComparisons << " terrain tile scalar comparisons\n";
    std::fill(actualGame.begin(), actualGame.end(), 0);
    std::fill(expectedGame.begin(), expectedGame.end(), 0);
    amigaRendererInit(&actualRenderer, actualGame.data(), actualHud.data());
    amigaRendererInit(&expectedRenderer, expectedGame.data(), expectedHud.data());
    amigaRendererSetGameOrigin(&actualRenderer, 311);
    amigaRendererSetGameOrigin(&expectedRenderer, 311);
    amigaRendererSetClipRect(&actualRenderer, 110, 0, 160, 284);
    amigaRendererSetClipRect(&expectedRenderer, 110, 0, 160, 284);
    const auto ringPixel = [](const std::vector<UBYTE>& pixels, int x, int y) {
        unsigned pen = 0;
        for (unsigned plane = 0; plane < 2; ++plane) {
            const unsigned offset = plane * downwellGameRingPlaneBytes +
                ((311 + y) % downwellGameRingHeight) * downwellBytesPerPlaneLine + x / 8;
            if (pixels[offset] & (0x80 >> (x & 7))) pen |= 1u << plane;
        }
        return pen;
    };
    {
        downwell::ShopWorld shopWorld;
        unsigned rolls = 0;
        auto* shop = shopWorld.create(0, 20, [&](unsigned lo, unsigned) {
            return lo + (++rolls == 2 ? 1 : 0);
        }, [] { return 0u; }, 4, 1);
        const auto draw = [&](AmigaRenderer& target) {
            downwell::drawShopFixtures(shopWorld, [&](int sprite, int frame, int x, int y) {
                amigaRendererDrawAsset(&target, gamePack.data(), gamePack.data() + gamePack.size(),
                    sprite, frame, x + 120, y, 0);
            });
        };
        draw(actualRenderer);
        const auto before = actualGame;
        shop->stock.items[0].remaining = 0;
        shop->feedback.apply(downwell::ShopInteraction::Effect::purchased, 1, shop->stock);
        draw(expectedRenderer); // Fresh post-purchase composition.
        amigaRendererClearGameRows(&actualRenderer, 0, 32, 0);
        amigaRendererSetClipRect(&actualRenderer, 110, 0, 160, 32);
        draw(actualRenderer); // Only the intersecting cached shop rows.
        require(actualGame == expectedGame && actualGame != before,
            "purchase row refresh matches fresh planar composition across ring wrap");
        for (int y = 0; y < 284; ++y) for (int x = 110; x < 270; ++x)
            if (x < 153 || x >= 177 || y >= 24)
                require(ringPixel(actualGame, x, y) == ringPixel(before, x, y),
                    "purchase changes only sold item pixels, retaining other cases and contents");
        amigaRendererSetClipRect(&actualRenderer, 110, 0, 160, 284);
    }
    for (unsigned frame = 0; frame < 2; ++frame) for (unsigned turn = 1; turn < 4; ++turn) {
        std::fill(actualGame.begin(), actualGame.end(), 0);
        std::fill(expectedGame.begin(), expectedGame.end(), 0);
        amigaRendererDrawAsset(&actualRenderer, gamePack.data(), gamePack.data() + gamePack.size(),
            278, turn * 256 + frame, 190, 100, 0);
        amigaRendererDrawAsset(&expectedRenderer, gamePack.data(), gamePack.data() + gamePack.size(),
            278, frame, 190, 100, 0);
        for (int y = 40; y < 160; ++y) for (int x = 130; x < 250; ++x) {
            int sx = x, sy = y;
            for (unsigned i = 0; i < turn; ++i) {
                const int nextX = 190 - (sy - 100) - 1;
                sy = 100 + (sx - 190);
                sx = nextX;
            }
            require(ringPixel(actualGame, x, y) == ringPixel(expectedGame, sx, sy),
                    "offline crawler quarter turn preserves source anchor through ring wrapping");
        }
    }
    for (unsigned sprite : {177u, 178u})
      for (unsigned frame = 0; frame < (sprite == 177 ? 4u : 1u); ++frame) for (int flipX : {0, 1}) {
        std::fill(actualGame.begin(), actualGame.end(), 0);
        std::fill(expectedGame.begin(), expectedGame.end(), 0);
        amigaRendererDrawAsset(&actualRenderer, gamePack.data(), gamePack.data() + gamePack.size(),
            sprite, 256 + frame, 190, 100, flipX);
        amigaRendererDrawAsset(&expectedRenderer, gamePack.data(), gamePack.data() + gamePack.size(),
            sprite, frame, 190, 100, flipX);
        for (unsigned plane = 0; plane < 2; ++plane)
            for (unsigned y = 0; y < downwellDisplayHeight; ++y)
                for (unsigned x = 0; x < downwellBytesPerPlaneLine; ++x) {
                    const unsigned row = (311 + y) % downwellGameRingHeight;
                    const unsigned mirror = (311 + 199 - y) % downwellGameRingHeight;
                    const UBYTE expected = y < 200 ? expectedGame[
                        plane * downwellGameRingPlaneBytes + mirror * downwellBytesPerPlaneLine + x] : 0;
                    require(actualGame[plane * downwellGameRingPlaneBytes +
                        row * downwellBytesPerPlaneLine + x] == expected,
                        "offline snail Y flip preserves the source anchor and either X facing");
                }
    }
    for (unsigned i = 0; i < records; ++i) {
        const auto* record = gamePack.data() + 20 + i * 24;
        const unsigned id = word(record), frame = word(record + 2);
        if (id < 62000 || id >= 63000) continue;
        for (int x : {110, 117, 190, 263}) for (int y : {2, 90, 278}) {
            std::fill(actualGame.begin(), actualGame.end(), 0x55);
            std::fill(expectedGame.begin(), expectedGame.end(), 0x55);
            amigaRendererDrawAsset(&actualRenderer, gamePack.data(), gamePack.data() + gamePack.size(),
                id - 62000, frame, x, y, 1);
            amigaRendererDrawAsset(&expectedRenderer, oraclePack.data(), oraclePack.data() + oraclePack.size(),
                id - 62000, frame, x, y, 1);
            require(actualGame == expectedGame, "offline mirrored scenery matches scalar clipped/ring oracle");
            int left, top, width, height;
            require(amigaRendererAssetBounds(gamePack.data(), gamePack.data() + gamePack.size(),
                id - 62000, frame, x, y, 1, &left, &top, &width, &height),
                "overflow actor bounds exist");
            // Every changed row must be covered by the restoration interval,
            // including assets straddling the circular-buffer boundary.
            for (unsigned row = 0; row < downwellGameRingHeight; ++row) {
                const unsigned screenY = (row + downwellGameRingHeight - 311) % downwellGameRingHeight;
                if ((int)screenY >= top && (int)screenY < top + height) continue;
                for (unsigned plane = 0; plane < 2; ++plane)
                    for (unsigned byte = 0; byte < downwellBytesPerPlaneLine; ++byte)
                        require(actualGame[plane * downwellGameRingPlaneBytes +
                            row * downwellBytesPerPlaneLine + byte] == 0x55,
                            "overflow restoration bounds cover every modified row");
            }
        }
    }
    std::ifstream hudInput(argv[2], std::ios::binary);
    const std::vector<UBYTE> hudPack{std::istreambuf_iterator<char>(hudInput), {}};
    {
        auto reference=hudPack,narrowed=hudPack;
        auto word=[&](unsigned at){return unsigned(reference[at])*256+reference[at+1];};
        const unsigned count=word(8)*65536+word(10),directory=word(6);
        bool found=false;unsigned originalWidth=0,mirrorAt=0;
        for(unsigned i=0;i<count;++i){
            const unsigned at=directory+i*24;
            if(word(at)==408)originalWidth=word(at+8);
            if(word(at)==62408)mirrorAt=at;
        }
        require(originalWidth&&mirrorAt,"both tablet border records are present");
        require(word(mirrorAt+8)==word(mirrorAt+12)&&word(mirrorAt+8)>originalWidth,
            "mirrored border exposes its transparent DMA extent");
        putWord(narrowed.data(),mirrorAt+8,originalWidth);
        const unsigned stride=word(mirrorAt+12)/8,height=word(mirrorAt+10);
        const unsigned payload=word(mirrorAt+16)*65536+word(mirrorAt+18);
        for(unsigned plane=0;plane<3;++plane)for(unsigned y=0;y<height;++y)
        for(unsigned x=originalWidth;x<stride*8;++x)
            require(!(hudPack[payload+(plane*height+y)*stride+x/8]&(128u>>(x&7))),
                "exposed border padding has zero colour and alpha bits");
        for(unsigned i=0;i<count;++i){
            const unsigned at=directory+i*24;
            if(word(at)==62408){
                // Keep directory ordering while forcing the original reflection.
                putWord(reference.data(),at,62409);found=true;
            }
        }
        require(found,"offline tablet-border mirror is present");
        for(int x : {-3,0,108,272,379,384})for(int y : {-17,0,23})
        for(unsigned background : {0u,0x5au,0xffu})for(unsigned clip=0;clip<3;++clip){
            std::vector<UBYTE> actual(downwellLayerBytes,background),expected(actual),oldExtent(actual);
            AmigaRenderer a,b,c;
            amigaRendererInit(&a,nullptr,actual.data());
            amigaRendererInit(&b,nullptr,expected.data());
            amigaRendererInit(&c,nullptr,oldExtent.data());
            if(clip)for(auto* renderer : {&a,&b,&c}) {
                amigaRendererSetClipRect(renderer,clip==1?x-2:0,clip==1?y+3:0,
                    clip==1?5:270,clip==1?271:190);
                renderer->target=renderer->hud;
            }
            amigaRendererDrawAsset(&a,hudPack.data(),hudPack.data()+hudPack.size(),408,0,x,y,1);
            amigaRendererDrawAsset(&b,reference.data(),reference.data()+reference.size(),408,0,x,y,1);
            amigaRendererDrawAsset(&c,narrowed.data(),narrowed.data()+narrowed.size(),408,0,x,y,1);
            require(actual==expected,"offline border equals runtime reflection, clipped and over existing pixels");
            require(actual==oldExtent,"transparent extent preserves the original mirrored logical rectangle");
        }
    }
#ifdef DOWNWELL_PREPARED_AMMO_DIVIDERS
    {
        AmigaWordStamp stamps[2];
        for (unsigned frame = 0; frame < 2; ++frame)
            require(amigaRendererPrepareWordStamp(stamps + frame, hudPack.data(),
                hudPack.data() + hudPack.size(), 421, frame), "prepare divider oracle stamps");
        require(amigaPreparedDividersMatch(stamps), "prepared bank accepts exact source artwork");
        stamps[0].mask[2] ^= 1;
        require(!amigaPreparedDividersMatch(stamps), "prepared bank rejects changed coverage");
        stamps[0].mask[2] ^= 1;
        UWORD actual[568] = {};
        // Sweep empty/short/tall compositions, including their transparent
        // rows and every dictionary-block boundary, in both directions.
        for (unsigned pass = 0; pass < 2; ++pass)
            for (unsigned c = 0; c <= 16; ++c) {
                const unsigned cap = pass ? 16-c : c;
                for (unsigned a = 0; a <= cap; ++a) {
                    const unsigned ammo = pass ? cap-a : a;
                    UWORD expected[568] = {};
                    downwell::drawAmmoDividers(ammo, cap, 1, 288, 229,
                        [&](unsigned, unsigned frame, int, unsigned y) {
                            const auto& stamp = stamps[frame];
                            for (unsigned row = stamp.first; row < stamp.last; ++row) {
                                if (y+row >= 284) continue;
                                const UWORD mask = stamp.mask[row];
                                auto* target = expected + (y+row)*2;
                                target[0] = (target[0] & UWORD(~mask)) | (stamp.low[row] & mask);
                                target[1] = (target[1] & UWORD(~mask)) | (stamp.high[row] & mask);
                            }
                        });
                    const auto prepared = amigaPreparedDividers(ammo, cap);
                    for (unsigned row = 0; row < 284; ++row) {
                        const auto* source = prepared.blocks + prepared.indices[row/8]*16 + (row%8)*2;
                        actual[row*2] = source[0]; actual[row*2+1] = source[1];
                    }
                    require(std::memcmp(actual, expected, sizeof(actual)) == 0,
                            "offline divider composition equals source binary64 ordered draws");
                }
            }
    }
#endif
    require(!hudPack.empty(), "HUD pack can be read");
    // Force the established byte-span path for the oracle by giving its
    // destination neither the HUD nor game identity (ordinary planar stride).
    // Compare every narrow HUD frame, clipped edges and replacement pens.
    actualRenderer.target = actualRenderer.hud;
    expectedRenderer.target = expectedRenderer.hud;
    expectedRenderer.hud = nullptr;
    for (unsigned sprite : {418u, 419u, 421u})
      for (unsigned frame = 0; frame < (sprite == 421 ? 2u : 4u); ++frame)
        for (int y : {-4, 0, 100, 280}) for (int inset : {0, 1, 5, 11})
          for (int pen : {-1, 0, 1, 2, 3}) {
            std::fill(actualHud.begin(), actualHud.end(), 0x5a);
            std::fill(expectedHud.begin(), expectedHud.end(), 0x5a);
            amigaRendererSetClipRect(&actualRenderer, 288 + inset, 0, 12 - inset, 284);
            amigaRendererSetClipRect(&expectedRenderer, 288 + inset, 0, 12 - inset, 284);
            actualRenderer.target = actualHud.data();
            expectedRenderer.target = expectedHud.data();
            for (auto* renderer : {&actualRenderer, &expectedRenderer}) {
                if (pen < 0) amigaRendererDrawAsset(renderer, hudPack.data(), hudPack.data() + hudPack.size(),
                    sprite, frame, 288, y, 0);
                else amigaRendererDrawAssetPen(renderer, hudPack.data(), hudPack.data() + hudPack.size(),
                    sprite, frame, 288, y, 0, pen);
            }
            require(actualHud == expectedHud, "aligned HUD word path matches clipped byte-span oracle");
        }
    expectedRenderer.hud = expectedHud.data();
    expectedRenderer.hud = nullptr;
    for (unsigned sprite : {413u, 65010u, 418u, 419u, 421u})
      for (unsigned frame = 0; frame < (sprite == 413 ? 30u : sprite == 65010 ? 10u : sprite == 421 ? 2u : 4u); ++frame)
        for (int shift = 0; shift < 16; ++shift) for (int y : {-4, 100, 280}) {
            std::fill(actualHud.begin(), actualHud.end(), 0x5a);
            std::fill(expectedHud.begin(), expectedHud.end(), 0x5a);
            for (auto* renderer : {&actualRenderer, &expectedRenderer}) {
                renderer->clipX = 283; renderer->clipY = 1;
                renderer->clipWidth = 28; renderer->clipHeight = 281;
                renderer->target = renderer == &actualRenderer ? actualHud.data() : expectedHud.data();
                amigaRendererDrawAsset(renderer, hudPack.data(), hudPack.data()+hudPack.size(),
                    sprite, frame, 280+shift, y, 0);
                amigaRendererFillRect(renderer, 281+shift, y+2, 19, 6, shift%4);
            }
            require(actualHud == expectedHud, "shifted small HUD words/clears match byte-span oracle");
        }
    expectedRenderer.hud = expectedHud.data();
    {
        for (unsigned sprite : {418u, 419u, 421u})
            for (unsigned frame = 0; frame < (sprite == 421 ? 2u : 4u); ++frame) {
                AmigaWordStamp stamp;
                require(amigaRendererPrepareWordStamp(&stamp, hudPack.data(), hudPack.data()+hudPack.size(), sprite, frame),
                    "prepare repeated row artwork");
                for (unsigned count : {0u, 1u, 7u, 8u, 9u, 17u, 184u})
                    for (int y : {0, 100, 283}) for (int inset : {0, 3}) {
                        std::fill(actualHud.begin(), actualHud.end(), 0x5a);
                        std::fill(expectedHud.begin(), expectedHud.end(), 0x5a);
                        for (auto* renderer : {&actualRenderer, &expectedRenderer}) {
                            amigaRendererClearClipRect(renderer);
                            renderer->clipX = 288+inset; renderer->clipWidth = 12-inset;
                            renderer->clipY = inset; renderer->clipHeight = 284-2*inset;
                        }
                        require(amigaRendererDrawWordStampRun(&actualRenderer, &stamp, 288, y, count),
                            "repeated word column draw supported");
                        for (unsigned i = 0; i < count; ++i)
                            amigaRendererDrawAsset(&expectedRenderer, hudPack.data(), hudPack.data()+hudPack.size(),
                                sprite, frame, 288, y-static_cast<int>(i), 0);
                        require(actualHud == expectedHud, "constant run middle and clipped ends match ordered sprite draws");
                    }
            }
    }
    {
        AmigaAmmoHud retained;
#ifdef DOWNWELL_AMMO_FACTORED
        retained.sharedRowProducer(true);
#endif
        downwell::AmmoGaugeRows rows;
        unsigned previousDigits = 1;
        auto fullRedraw = [&](unsigned ammo, unsigned cap, unsigned offset, unsigned meter) {
            auto* renderer = &expectedRenderer;
            amigaRendererClearClipRect(renderer);
            auto asset = [&](unsigned sprite, unsigned frame, int x, int y) {
                amigaRendererDrawAsset(renderer, hudPack.data(), hudPack.data()+hudPack.size(), sprite, frame, x, y, 0);
            };
            amigaRendererFillRect(renderer, 282, 40, 24, 216, 3);
            unsigned digits = 1;
            for (unsigned value = ammo; value >= 10; value /= 10) ++digits;
            const unsigned clearDigits = digits > previousDigits ? digits : previousDigits;
            previousDigits = digits;
            if (clearDigits == 1) amigaRendererFillRect(renderer, 282, 232, 24, 24, 3);
            else amigaRendererFillRect(renderer, 282-(int(clearDigits)-1)*4, 231,
                34+(clearDigits-1)*8, 26+clearDigits-1, 0);
            asset(416, 0, 284, 43+meter);
            downwell::drawAmmoFill(ammo, cap, 289, [&](int x, unsigned y, unsigned w, unsigned h) {
                amigaRendererFillRect(renderer, x, y, w, h, 1);
            });
            rows.draw(288, 229+4*meter, [&](unsigned sprite, unsigned frame, int x, int y) {
                asset(sprite, sprite == 421 ? frame%2 : frame, x, y);
            });
            downwell::drawAmmoDividers(ammo, cap, 1, 288, 229+4*meter, asset);
            asset(416, 1, 284, 43+meter);
            if (ammo < 10) asset(65010, ammo, 290+offset, 239+offset);
            else downwell::drawAmmoNumberLayers(290+offset, 239+offset, ammo,
                [&](unsigned frame, int x, int y) { asset(413, frame, x, y); });
        };
        std::fill(actualHud.begin(), actualHud.end(), 0x5a);
        std::fill(expectedHud.begin(), expectedHud.end(), 0x5a);
        unsigned oldAmmo = ~0u, skipped = 0;
        for (unsigned tick = 0; tick < 1000; ++tick) {
            const unsigned ammo = tick % 31 < 12 ? 8 : 7;
            rows.step(ammo, 8);
            if (ammo != oldAmmo || rows.visuallyChanged())
                retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(), ammo, 8, 0, &rows, 0);
            else ++skipped;
            oldAmmo = ammo;
            fullRedraw(ammo, 8, 0, 0);
            require(actualHud == expectedHud, "skipping invisible row-state changes preserves all HUD pixels");
        }
        require(skipped > 500, "stable and invisible animation updates bypass retained HUD composition");
        // The white fill is fixed at y=53 when full, while row artwork moves
        // by four times the jiggle. An ammo change at the same nonzero jiggle
        // must retire that unshifted fill too (the next case empties it).
        rows.step(8,8);
        retained.draw(&actualRenderer,hudPack.data(),hudPack.data()+hudPack.size(),8,8,0,&rows,3);
        fullRedraw(8,8,0,3);
        require(actualHud==expectedHud,"full gauge at nonzero jiggle matches source artwork");
        for(unsigned cap : {1u,8u,15u,64u,127u})
          for(unsigned meter=0;meter<4;++meter)
            for(unsigned before=0;before<=(cap<8?cap:8);++before)
                for(unsigned after=0;after<=(cap<8?cap:8);++after){
                    rows.step(before,cap);
                    retained.draw(&actualRenderer,hudPack.data(),hudPack.data()+hudPack.size(),before,cap,0,&rows,meter);
                    fullRedraw(before,cap,0,meter);
                    require(actualHud==expectedHud,"fixed-fill transition starts at source pixels");
                    rows.step(after,cap);
                    retained.draw(&actualRenderer,hudPack.data(),hudPack.data()+hudPack.size(),after,cap,0,&rows,meter);
                    fullRedraw(after,cap,0,meter);
                    require(actualHud==expectedHud,"same-jiggle ammo changes retire unshifted fill pixels");
                }
        for (unsigned cap : {8u, 15u, 1u, 64u, 127u}) {
            for (unsigned tick = 0; tick < (cap+1)*10; ++tick) {
                const unsigned ammo = (tick / 5) % (cap + 1);
                const unsigned meter = tick % 19 < 4 ? 3 - tick % 19 : 0;
                rows.step(ammo, cap);
                retained.draw(&actualRenderer, hudPack.data(), hudPack.data() + hudPack.size(),
                    ammo, cap, tick % 3, &rows, meter);
                fullRedraw(ammo, cap, tick%3, meter);
                if (actualHud != expectedHud) {
                    std::cerr << "HUD mismatch cap=" << cap << " ammo=" << ammo << " tick=" << tick
                              << " meter=" << meter << '\n';
                    for (unsigned byte = 0; byte < actualHud.size(); ++byte) if (actualHud[byte] != expectedHud[byte]) {
                        std::cerr << "first byte=" << byte << " actual=" << unsigned(actualHud[byte])
                                  << " expected=" << unsigned(expectedHud[byte]) << '\n'; break;
                    }
                }
                require(actualHud == expectedHud, "retained ammo strips match complete reconstruction");
                retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(),
                    ammo, cap, tick%3, &rows, meter);
                require(retained.lastWordWrites == 0, "unchanged gauge does not write either visible plane");
            }
        }
        // Every cached divider capacity, charge boundary and jiggle offset.
        // Compare against ordered original draws, not against another cache.
        for (unsigned cap = 1; cap <= 64; ++cap)
            for (unsigned ammo = 0; ammo <= cap; ++ammo)
                for (unsigned meter = 0; meter < 4; ++meter) {
                    rows.step(ammo, cap);
                    retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(),
                        ammo, cap, 0, &rows, meter);
                    fullRedraw(ammo, cap, 0, meter);
                    require(actualHud == expectedHud, "composed divider cache preserves all charge and jiggle states");
                }
        // Isolate every artwork transition, including the special bottom row
        // and both sides of the fill boundary. Adjacent overlays can conceal
        // changed pixels, but damage must never omit a surviving difference.
#ifdef DOWNWELL_AMMO_FACTORED
        require(retained.factoredDraws>1000,"prepared complete gauge is exercised, not only fallback");
        // Capacity changes hidden between rendered frames must not inherit an
        // obsolete uniform-group certificate when the old capacity returns.
        rows=downwell::AmmoGaugeRows{};
        retained.invalidate();
        rows.step(7,8);
        retained.draw(&actualRenderer,hudPack.data(),hudPack.data()+hudPack.size(),7,8,0,&rows,0);
        fullRedraw(7,8,0,0);
        rows.step(4,15);rows.step(6,8);
        retained.draw(&actualRenderer,hudPack.data(),hudPack.data()+hudPack.size(),6,8,2,&rows,3);
        fullRedraw(6,8,2,3);
        require(actualHud==expectedHud,"missed capacity changes recertify prepared groups");
        const auto beforeRecovery=retained.factoredDraws;
        for(unsigned tick=0;tick<8;++tick){
            rows.step(8,8);
            retained.draw(&actualRenderer,hudPack.data(),hudPack.data()+hudPack.size(),8,8,tick%3,&rows,tick%4);
            fullRedraw(8,8,tick%3,tick%4);
            require(actualHud==expectedHud,"prepared path recovers after nonuniform fallback");
        }
        require(retained.factoredDraws>beforeRecovery,"settled groups reenter prepared path");
        {
            AmigaAmmoFactored probe;
            std::vector<UBYTE> altered=hudPack, scratch(downwellLayerBytes);
            // Change one byte of relevant art without changing its metadata.
            auto read16=[&](unsigned at){return unsigned(altered[at])*256+altered[at+1];};
            auto read32=[&](unsigned at){return read16(at)*65536+read16(at+2);};
            bool alteredAsset=false;
            const auto count=read32(8),directory=read16(6);
            for(unsigned i=0;i<count;++i){
                const auto record=directory+i*24;
                if(read16(record)==416){altered[read32(record+16)]^=1;alteredAsset=true;break;}
            }
            require(alteredAsset,"source artwork mutation fixture exists");
            unsigned writes=0;
            require(!probe.draw(&actualRenderer,scratch.data(),altered.data(),altered.data()+altered.size(),8,8,0,&rows,0,writes),
                    "changed source bytes use original renderer rather than prepared art");
        }
        retained.sharedRowProducer(false); // These cases edit rows outside the shared producer.
#endif
        rows.filled = 92;
        // Settle the three-digit-to-one-digit backing transition from the
        // preceding capacity-127 sweep before isolating row-only changes.
        for (unsigned ammo : {4u, 5u}) {
            retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(), ammo, 8, 0, &rows, 0);
            fullRedraw(ammo, 8, 0, 0);
        }
        for (unsigned index : {0u, 1u, 91u, 92u, 183u})
            for (unsigned from = 0; from < 5; ++from)
                for (unsigned to = 0; to < 5; ++to) {
                    for (auto& state : rows.state) state = 3;
                    rows.state[index] = from;
                    retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(), 4, 8, 0, &rows, 0);
                    fullRedraw(4, 8, 0, 0);
                    require(actualHud == expectedHud, "isolated row transition starting image");
                    rows.state[index] = to;
                    retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(), 4, 8, 0, &rows, 0);
                    fullRedraw(4, 8, 0, 0);
                    if (actualHud != expectedHud) {
                        std::cerr << "row=" << index << " from=" << from << " to=" << to << '\n';
                        for (unsigned byte=0; byte<actualHud.size(); ++byte) if (actualHud[byte]!=expectedHud[byte]) {
                            std::cerr << "byte=" << byte << " actual=" << unsigned(actualHud[byte])
                                << " expected=" << unsigned(expectedHud[byte]) << '\n'; break;
                        }
                    }
                    require(actualHud == expectedHud, "isolated row damage matches full redraw");
                }
        for (unsigned ammo = 0; ammo <= 8; ++ammo)
            for (unsigned from = 0; from < 4; ++from)
                for (unsigned to = 0; to < 4; ++to) {
                    rows.step(ammo, 8);
                    retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(), ammo, 8, 0, &rows, from);
                    fullRedraw(ammo, 8, 0, from);
                    require(actualHud == expectedHud, "gauge jiggle starting image");
                    retained.draw(&actualRenderer, hudPack.data(), hudPack.data()+hudPack.size(), ammo, 8, 0, &rows, to);
                    fullRedraw(ammo, 8, 0, to);
                    require(actualHud == expectedHud, "all gauge jiggle pairs preserve full reconstruction");
                }
    }
    amigaRendererClearClipRect(&actualRenderer);
    amigaRendererClearClipRect(&expectedRenderer);
    const int offsets[][2] = {{-1,0},{0,-1},{1,0},{0,1},{-1,1},{0,0},
        {1,1},{0,2},{0,0},{1,-1},{2,0},{1,1},{0,1},{1,0}};
    for (unsigned digit = 0; digit < 10; ++digit) {
        std::fill(actualHud.begin(), actualHud.end(), 0x55);
        std::fill(expectedHud.begin(), expectedHud.end(), 0x55);
        amigaRendererDrawAsset(&actualRenderer, hudPack.data(), hudPack.data() + hudPack.size(),
            65010, digit, 290, 239, 0);
        for (unsigned layer = 0; layer < 14; ++layer)
            amigaRendererDrawAsset(&expectedRenderer, hudPack.data(), hudPack.data() + hudPack.size(),
                413, digit + (layer < 12 ? 20 : 10), 290 + offsets[layer][0], 239 + offsets[layer][1], 0);
        amigaRendererDrawAsset(&expectedRenderer, hudPack.data(), hudPack.data() + hudPack.size(),
            413, digit, 290, 239, 0);
        require(actualHud == expectedHud, "offline ammo digits match all fifteen original layers");
    }
    {
        AmigaHealthHud health;
        for(unsigned index=0;index<11;++index){
            AmigaHudGlyph prepared{};
            require(amigaRendererPrepareHudGlyph(&prepared,hudPack.data(),hudPack.data()+hudPack.size(),65011,index),
                "prepare glyph oracle");
            const auto& offline=AmigaHealthArt::glyphs[index];
            require(prepared.left==offline.left&&prepared.top==offline.top&&
                prepared.width==offline.width&&prepared.height==offline.height,
                "offline glyph metadata equals renderer preparation");
            for(unsigned row=0;row<16;++row)
                require(prepared.low[row]==offline.low[row]&&prepared.high[row]==offline.high[row],
                    "offline health glyph planes equal runtime preparation");
        }
        auto compare=[&](unsigned hp,unsigned maximum,unsigned pieces,unsigned perHeart){
            health.draw(&actualRenderer,hudPack.data(),hudPack.data()+hudPack.size(),hp,maximum,pieces,perHeart);
            AmigaHealthHud::reference(&expectedRenderer,hudPack.data(),hudPack.data()+hudPack.size(),hp,maximum,pieces,perHeart);
            require(actualHud==expectedHud,"retained health HUD matches complete original layered rendering");
        };
        for(unsigned glyph=0;glyph<11;++glyph)for(int offset=0;offset<16;++offset){
            std::fill(actualHud.begin(),actualHud.end(),0x55);
            std::fill(expectedHud.begin(),expectedHud.end(),0x55);
            amigaRendererDrawAsset(&actualRenderer,hudPack.data(),hudPack.data()+hudPack.size(),65011,glyph,32+offset,10,0);
            const unsigned sprite=glyph<10?413:414,foreground=glyph<10?glyph:0,shadow=glyph<10?glyph+20:2;
            auto asset=[&](unsigned frame,int x,int y){amigaRendererDrawAsset(&expectedRenderer,
                hudPack.data(),hudPack.data()+hudPack.size(),sprite,frame,x,y,0);};
            asset(shadow,33+offset,10);asset(shadow,33+offset,11);asset(shadow,32+offset,11);asset(foreground,32+offset,10);
            require(actualHud==expectedHud,"offline health glyphs retain all four layers at every word offset");
        }
        std::fill(actualHud.begin(),actualHud.end(),0x55);
        std::fill(expectedHud.begin(),expectedHud.end(),0x55);
        for(unsigned capacity : {0u,4u,8u,10u,16u,99u}){
            for(unsigned hp=capacity;hp>0;--hp)compare(hp,capacity,0,4);
            compare(0,capacity,0,4);
            for(unsigned hp=0;hp<=capacity;++hp)compare(hp,capacity,hp%4,4);
        }
        for(unsigned perHeart : {0u,1u,2u,3u,4u,5u,8u})
            for(unsigned pieces=0;pieces<=perHeart;++pieces)compare(4,4,pieces,perHeart);
        compare(4,4,0,4);compare(4,4,0,4);
        require(health.lastWordWrites==0,"unchanged health does not rewrite any words");
        compare(3,4,0,4);
        require(health.lastWordWrites<80,"single damage updates glyph and lost fill only");
        health.invalidate();compare(3,4,0,4);
        require(AmigaHealthArt::matches(hudPack.data(),hudPack.data()+hudPack.size()),
            "original health art admits offline words");
        require(!AmigaHealthArt::matches(hudPack.data(),hudPack.data()+19),
            "truncated health art is not admitted");
        auto changedPack=hudPack;
        // Mutate source payload, not glyph data: the offline rim/background
        // must fall back even though resource IDs and font are unchanged.
        const auto& sourceSpan=AmigaHealthArt::spans[2];
        changedPack[sourceSpan.offset]^=0x80;
        require(!AmigaHealthArt::matches(changedPack.data(),changedPack.data()+changedPack.size()),
            "changed health source art rejects offline words");
        const std::vector<UBYTE>* reboundPacks[]={&changedPack,&hudPack};
        for(const auto* pack : reboundPacks){
            for(unsigned i=0;i<actualHud.size();++i)
                actualHud[i]=expectedHud[i]=UBYTE(i*37+19);
            health.draw(&actualRenderer,pack->data(),pack->data()+pack->size(),2,4,1,4);
            AmigaHealthHud::reference(&expectedRenderer,pack->data(),pack->data()+pack->size(),2,4,1,4);
            require(actualHud==expectedHud,"health art rebind/fallback preserves full HUD including outside pixels");
        }
    }
    {
        std::ifstream file("generated/amiga-shop.dwap", std::ios::binary);
        std::vector<UBYTE> shopPack((std::istreambuf_iterator<char>(file)), {});
        require(!shopPack.empty(), "offline shop bank exists");
        std::vector<UBYTE> world(downwellGameRingBytes, 0x5a);
        std::vector<UBYTE> retainedHud(downwellLayerBytes), freshHud(downwellLayerBytes);
        AmigaRenderer retainedRenderer, freshRenderer;
        amigaRendererInit(&retainedRenderer, world.data(), retainedHud.data());
        amigaRendererInit(&freshRenderer, world.data(), freshHud.data());
        AmigaShopMenu retained;
        downwell::ShopWorld::Instance shop;
        shop.present = shop.menu.active = true;
        shop.menu.menuState = 1; shop.x = 240; shop.y = 240;
        for (unsigned i = 0; i < 3; ++i) shop.stock.items[i] = {100+i,1,150+i*100};
        for (unsigned tick = 0; tick < 64; ++tick) {
            shop.menu.cursor = (tick / 8) % 4;
            shop.menu.cursorWobbleX = int(tick % 11) - 5;
            shop.menu.advancePresentation();
            if (tick == 24) shop.stock.items[2].remaining = 0;
            if (tick == 56) shop.menu.active = false;
            require(retained.draw(&retainedRenderer, shopPack.data(), shopPack.data()+shopPack.size(),
                &shop, 160, 34+tick/4, 200), "retained shop draw resolves every offline asset");
            std::fill(freshHud.begin(), freshHud.end(), 0);
            AmigaShopMenu fresh;
            require(fresh.draw(&freshRenderer, shopPack.data(), shopPack.data()+shopPack.size(),
                &shop, 160, 34+tick/4, 200), "fresh shop composition");
            require(retainedHud == freshHud, "shop dirty bounds match fresh composition across navigation and closing");
        }
        for (auto byte : world) require(byte == 0x5a, "shop overlay never repaints the terrain ring");
        for (auto byte : retainedHud) require(byte == 0, "closed shop leaves no stale overlay pixels");
        auto overlay = [](AmigaRenderer* renderer, void*) {
            // Include the outer digits of a full-width streak, not just
            // the old two-digit region in the middle of the panel.
            amigaRendererFillRect(renderer, 150, 60, 84, 4, 1);
        };
        shop.menu.active = true;
        for (unsigned phase = 0; phase < 3; ++phase) {
            // Simulate another HUD component clearing pixels in its bounds.
            amigaRendererClearClipRect(&retainedRenderer);
            amigaRendererFillRect(&retainedRenderer, 148, 44, 88, 24, 0);
            retained.invalidateStreak();
            if (phase == 2) shop.menu.active = false;
            require(retained.draw(&retainedRenderer, shopPack.data(), shopPack.data()+shopPack.size(),
                &shop, 160, 34, 200, overlay), "externally invalidated shop region recomposes");
            std::fill(freshHud.begin(), freshHud.end(), 0);
            AmigaShopMenu fresh;
            fresh.invalidateStreak();
            require(fresh.draw(&freshRenderer, shopPack.data(), shopPack.data()+shopPack.size(),
                &shop, 160, 34, 200, overlay), "fresh HUD-over-shop composition");
            require(retainedHud == freshHud,
                "unchanged and closing shop restores foreground HUD without erasing underlying panel");
        }
    }
    {
        static AmigaActorFrames prepared;
        const auto* pack=gamePack.data();const auto* end=pack+gamePack.size();
        require(prepared.prepare(pack,end),"prepare immutable gameplay actor bindings");
        prepared.invalidate();
        require(!prepared.find(92,0),"released gameplay bank exposes no borrowed frame");
        require(prepared.prepare(pack,end),"rebind gameplay frames after re-entry");
        for(unsigned frame=0;frame<8;++frame){
            const auto* particle=prepared.find(92,frame);
            require(particle&&particle->width==2&&particle->height==2,
                "all consumption particle frames have offline planar bindings");
        }
        const auto word=[](const UBYTE* p){return unsigned(p[0])*256+p[1];};
        const unsigned count=word(pack+8)*65536+word(pack+10);
        unsigned checked=0;
        for(unsigned i=0;i<count;++i){
            const auto* r=pack+20+i*24;const auto sprite=word(r),frame=word(r+2);
            if(sprite>=741)continue;
            const auto* f=prepared.find(sprite,frame);require(f,"every original actor frame indexed");
            for(int flip:{0,1})for(int x:{-20,110,270})for(int y:{-32,0,284}){
                int l,t,w,h,rl,rt,rw,rh;f->bounds(x,y,flip,l,t,w,h);
                require(amigaRendererAssetBounds(pack,end,sprite,frame,x,y,flip,&rl,&rt,&rw,&rh)&&
                    l==rl&&t==rt&&w==rw&&h==rh,"prepared actor bounds equal scalar pack decoder");
                for(unsigned p=0;p<5;++p){
                    AmigaResolvedSpritePart actual{},expected{};
                    const bool a=f->part(p,flip,actual);
                    const bool b=p<4&&amigaRendererResolveSpritePart(&expected,pack,end,sprite,frame,p,flip);
                    require(a==b,"prepared parts retain absent variants and exact multipart count");
                    if(a)require(actual.pack==expected.pack&&actual.end==expected.end&&
                        actual.record==expected.record&&actual.partCount==expected.partCount,
                        "prepared part binds the original immutable DMA payload");
                }
                ++checked;
            }
        }
        require(!prepared.find(741,0)&&!prepared.find(1,1024)&&!prepared.find(143,0),
            "missing sprite, page and sparse frame remain absent");
        require(!prepared.prepare(pack,pack+19)&&!prepared.find(1,0),"failed preparation invalidates old bindings");
        require(prepared.prepare(pack,end),"bindings can be prepared again at ownership boundary");
        std::cout<<"Prepared actor frame cases: "<<checked<<'\n';
    }
    std::cout << "Amiga renderer pixel comparisons passed\n";
    // The old per-asset upload sum is not the shipping Chip allocation: the
    // offline atlas shares planes. Enforce the actual banks' fixed ceilings.
    require(gamePack.size()==DOWNWELL_GAME_BANK_BYTES&&gamePack.size()<=240u*1024u,
            "CPU gameplay art stays within its fixed scene lease budget");
    require(DOWNWELL_GAME_PLANE_BYTES<=112u*1024u,
            "DMA gameplay atlas stays within its fixed Chip lease budget");
    std::cout << "Amiga renderer tests passed\n";
    return 0;
}

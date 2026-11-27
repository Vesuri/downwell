#include "core/SmokeComposition.h"
#include "core/CurrencySmokeComposition.h"
#include "core/CurrencyDraw.h"
#include "core/SmokeEffects.h"
#include "core/GameMakerRandom.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
using namespace downwell;
#ifdef DOWNWELL_PROBES
static unsigned activePhase=profileControllerComposition;
static unsigned phaseCalls[profilePhaseCount]{};
unsigned downwellProfileEnter(unsigned phase){
    const auto previous=activePhase;activePhase=phase;++phaseCalls[phase];return previous;
}
void downwellProfilePhase(unsigned phase){activePhase=phase;}
#endif
static MotionBinary64 number(double d){std::uint64_t n;std::memcpy(&n,&d,8);return {std::uint32_t(n>>32),std::uint32_t(n)};}
static Fixed16 q(double d){return Fixed16::raw(std::int32_t(std::lround(d*65536.0)));}
static double value(Fixed16 n){return n.v/65536.0;}
static double value(MotionBinary64 n){const std::uint64_t bits=(std::uint64_t(n.high)<<32)|n.low;double d;std::memcpy(&d,&bits,8);return d;}
int main(int argc,char**argv){
    assert(argc==2);std::ifstream file(argv[1],std::ios::binary);
    const std::vector<std::uint8_t> pack{std::istreambuf_iterator<char>(file),{}};
    EffectMaskAssets assets;assert(assets.bind(pack.data(),pack.size()));
    for(unsigned sprite:{133u,134u})for(unsigned frame=0;frame<4;++frame)for(unsigned flags=0;flags<8;++flags){
        CurrencyDraw::Command command;RunnerImageValue index;assert(index.assign(number(frame+0.5)));
        EffectSurfacePlacement::View view{q(160),q(400),q(160),q(284)};
        assert(CurrencyDraw::prepare(sprite,index,q(240.25),q(512.5),view,flags&1,flags&2,flags&4,command));
        const auto target=(flags&3)?EffectSurfacePlacement::direct:(flags&4)?EffectSurfacePlacement::effects:EffectSurfacePlacement::hidden;
        assert(command.placement.target==target);unsigned emitted=0;
        const bool ok=CurrencyDraw::surface(command,assets,[&](const auto& asset,auto x,auto y){
            ++emitted;assert(asset.low==assets.currency(sprite==134,frame)->low&&value(x)==80.25&&value(y)==112.5);return true;
        });
        assert(ok==(target!=EffectSurfacePlacement::direct)&&emitted==(target==EffectSurfacePlacement::effects?1u:0u));
        if(target==EffectSurfacePlacement::effects)assert(!CurrencyDraw::surface(command,assets,[](const auto&,auto,auto){return false;}));
        if(target==EffectSurfacePlacement::direct)assert(value(command.placement.x)==240.25&&value(command.placement.y)==512.5);
    }
    for(unsigned sprite:{172u,206u})for(bool mirrored:{false,true}){
        EffectMaskAssets::Asset corpse;assert(assets.corpse(sprite,mirrored,corpse));
        assert(corpse.low&&corpse.high&&corpse.mask.data);
    }
    for(unsigned sprite:{91u,115u,116u})for(unsigned frame=0;frame<(sprite==91?6u:4u);++frame){
        EffectMaskAssets::Asset debris;
        assert(assets.debris(sprite,frame,debris));
        assert(debris.mask.width && debris.mask.height && debris.mask.bytes);
        assert(debris.high==debris.low+debris.mask.bytes);
        assert(debris.mask.data==debris.high+debris.mask.bytes);
    }
    SmokeEffects<32> smoke;GameMakerRandom random(225);
    for(unsigned i=0;i<24;++i)assert(smoke.create(q(int(i%6)*29-3),q(int(i/6)*75-2),random));
    SmokeComposition composition;
    CurrencySmokeComposition colored;
    unsigned frames=0,directFrames=0,controllerFrames=0,hiddenFrames=0,shadeVisits=0;
    const std::array<std::array<double,2>,4> lights{{{3,1},{63,77},{121,151},{159,283}}};
    // Sampling is intentionally supplied by the test, not claimed to establish
    // Windows raster semantics. Fractional route/snap arithmetic is independent.
    auto raster=[](Fixed16 n,std::int32_t& out){out=int(std::floor(value(n)));return true;};
    for(unsigned tick=0;tick<100;++tick){
        for(unsigned flags=0;flags<32;++flags){
            SmokeComposition::Settings settings{bool(flags&1),bool(flags&2),bool(flags&4),bool(flags&8),bool(flags&16)};
            const double vx=double(int(tick%9)-4)*0.25,vy=double(int(tick%7)-3)*0.5;
            EffectSurfacePlacement::View view{q(vx),q(vy),q(160),q(284)};
            const auto saved=random;
            assert(composition.prepare(smoke,assets,view,settings,[&](auto emit){
                ++shadeVisits;for(auto light:lights)emit(q(light[0]),q(light[1]));
            },raster));
            auto gems=[&](auto emit){
                for(unsigned i=0;i<12;++i)
                    emit(bool(i&1),(tick+i)%4,q(int(i%6)*29-3),q(int(i/6)*75+1));
            };
            // Overlapping frames exercise masked replacement, including black
            // pixels, followed by shade subtraction. Fractional anchors must
            // reach the supplied sampling policy unchanged.
            auto debris=[&](auto emit){
                for(unsigned sprite:{91u,115u,116u})
                    for(unsigned frame=0;frame<(sprite==91?6u:4u);++frame){
                        EffectMaskAssets::Asset asset;assert(assets.debris(sprite,frame,asset));
                        if(!emit(asset,q(62.25+frame),q(77.5)))return false;
                    }
                return true;
            };
            assert(colored.prepareWithLateEffects(gems,smoke,assets,view,settings,debris,[&](auto emit){
                for(auto light:lights)emit(q(light[0]),q(light[1]));
            },raster));
            assert(random.drawCount()==saved.drawCount() && random.stateIndex()==saved.stateIndex());
            for(unsigned i=0;i<16;++i)assert(random.stateWord(i)==saved.stateWord(i));
            std::array<bool,160*284> expected{},actual{};
            auto stamp=[&](const EffectMaskAssets::Asset& a,double x,double y,bool subtract){
                const int left=int(std::floor(x))-a.originX,top=int(std::floor(y))-a.originY;
                for(unsigned sy=0;sy<a.mask.height;++sy)for(unsigned sx=0;sx<a.mask.width;++sx){
                    const int dx=left+int(sx),dy=top+int(sy);
                    if(dx>=0 && dx<160 && dy>=0 && dy<284 &&
                       (a.mask.data[sy*a.mask.stride+sx/8]&(0x80u>>(sx%8))))expected[dy*160+dx]=!subtract;
                }
            };
            auto visible=[&](double x,double y){return x-vx>1e-5 && vx+160-x>1e-5 && y-vy>1e-5 && vy+284-y>1e-5;};
            const bool direct=settings.death||settings.lowSpec;
            // Independent scalar color painter, using original offline pixels.
            // -1 is transparent; opaque black remains a covered pixel.
            std::array<int,160*284> colorExpected,colorActual;
            colorExpected.fill(-1);colorActual.fill(-1);
            auto paint=[&](const EffectMaskAssets::Asset& a,double x,double y,bool erase){
                const int left=int(std::floor(x))-a.originX,top=int(std::floor(y))-a.originY;
                for(unsigned sy=0;sy<a.mask.height;++sy)for(unsigned sx=0;sx<a.mask.width;++sx){
                    const int dx=left+int(sx),dy=top+int(sy);
                    const unsigned at=sy*a.mask.stride+sx/8,bit=0x80u>>(sx%8);
                    if(dx>=0 && dx<160 && dy>=0 && dy<284 && (a.mask.data[at]&bit))
                        colorExpected[dy*160+dx]=erase?-1:int(bool(a.low[at]&bit))+2*int(bool(a.high[at]&bit));
                }
            };
            if(!direct && settings.effectsExist){
                gems([&](bool large,unsigned frame,Fixed16 x,Fixed16 y){
                    if(visible(value(x),value(y)))paint(*assets.currency(large,frame),value(x)-vx,value(y)-vy,false);
                });
                smoke.visitNewest([&](auto,const auto& e){
                    const double x=value(smokeRealX(e)),y=value(smokeRealY(e));
                    if(!e.destroyRequested && visible(x,y))paint(*assets.smoke(e.drawFrame()),x-vx,y-vy,false);
                });
                assert(debris([&](const auto& asset,auto x,auto y){
                    if(visible(value(x),value(y)))paint(asset,value(x)-vx,value(y)-vy,false);
                    return true;
                }));
                if(settings.shadeExists && !settings.noControl)
                    for(auto light:lights)if(visible(light[0],light[1]))
                        paint(*assets.shade(),std::nearbyint(light[0]/2)*2-vx,std::nearbyint(light[1]/2)*2-vy,true);
            }
            colored.surface().visit([&](unsigned x,unsigned y,std::uint16_t a,std::uint16_t lo,std::uint16_t hi){
                for(unsigned i=0;i<16;++i){const unsigned bit=0x8000u>>i;
                    if(a&bit)colorActual[y*160+x+i]=int(bool(lo&bit))+2*int(bool(hi&bit));
                }
            });
            bool colorVisible=false;for(int pen:colorExpected)colorVisible|=pen>=0;
            assert(colorActual==colorExpected && colored.visible()==colorVisible && colored.directRequired()==direct);
            smoke.visitNewest([&](auto,const auto& e){
                const double x=value(smokeRealX(e)),y=value(smokeRealY(e));
                if(!e.destroyRequested && (direct || (settings.effectsExist && visible(x,y))))
                    stamp(*assets.smoke(e.drawFrame()),x-vx,y-vy,false);
            });
            if(!direct && settings.effectsExist && settings.shadeExists && !settings.noControl)
                for(auto light:lights)if(visible(light[0],light[1]))
                    stamp(*assets.shade(),std::nearbyint(light[0]/2)*2-vx,std::nearbyint(light[1]/2)*2-vy,true);
            bool any=false;for(bool b:expected)any|=b;
            composition.coverage().visit([&](unsigned x,unsigned y,std::uint16_t bits){
                for(unsigned i=0;i<16;++i)actual[y*160+x+i]=bool(bits&(0x8000u>>i));
            });
            assert(actual==expected && composition.visible()==any);
            const auto wanted=!any?SmokeComposition::Layer::none:direct?SmokeComposition::Layer::direct:SmokeComposition::Layer::controller;
            assert(composition.layer()==wanted);
            if(wanted==SmokeComposition::Layer::direct)++directFrames;
            else if(wanted==SmokeComposition::Layer::controller)++controllerFrames;
            else ++hiddenFrames;
            ++frames;
        }
        const auto phase=smoke.beginPhase();assert(smoke.advanceAnimation(phase));assert(smoke.step(smoke.beginPhase(),random));
    }
    assert(smoke.size()==0 && directFrames && controllerFrames && hiddenFrames && shadeVisits);
    assert(smoke.create(q(80),q(140),random));
    const EffectSurfacePlacement::View view{{},{},q(160),q(284)};
    auto noShades=[](auto){};
    assert(composition.prepare(smoke,assets,view,{},noShades,raster) && composition.visible());
    assert(!composition.prepare(smoke,assets,view,{},noShades,[](auto,std::int32_t&){return false;}));
    assert(!composition.visible() && composition.layer()==SmokeComposition::Layer::none);
    unsigned words=0;composition.coverage().visit([&](auto,auto,auto){++words;});assert(!words);
    smoke.clear();EffectMaskAssets missing;
    assert(composition.prepare(smoke,missing,view,{},[](auto){assert(false);},raster));
    assert(smoke.create(q(80),q(140),random));
    assert(!composition.prepare(smoke,missing,view,{},noShades,raster));
    auto oneGem=[](auto emit){emit(false,0,q(80),q(140));};
    assert(colored.prepare(oneGem,smoke,assets,view,{},noShades,raster) && colored.visible());
    assert(!colored.prepare(oneGem,smoke,assets,view,{},noShades,[](auto,std::int32_t&){return false;}));
    assert(!colored.visible());
    words=0;colored.surface().visit([&](auto,auto,auto,auto,auto){++words;});assert(!words);
    assert(!colored.prepare(oneGem,smoke,missing,view,{},noShades,raster));
    unsigned lateVisits=0;
    auto failingLate=[&](auto emit){
        ++lateVisits;EffectMaskAssets::Asset asset;assert(assets.debris(115,0,asset));
        assert(emit(asset,q(80.25),q(140.5)));return false;
    };
    assert(!colored.prepareWithLateEffects(oneGem,smoke,assets,view,{},failingLate,noShades,raster));
    assert(lateVisits==1&&!colored.visible());
    words=0;colored.surface().visit([&](auto,auto,auto,auto,auto){++words;});assert(!words);
    for(unsigned mode=0;mode<3;++mode){
        SmokeComposition::Settings settings;
        settings.death=mode==0;settings.lowSpec=mode==1;settings.effectsExist=mode!=2;
        assert(colored.prepareWithLateEffects(oneGem,smoke,assets,view,settings,
            [&](auto){assert(false);return false;},noShades,raster));
        assert(!colored.visible());
    }
    // drawOnSurface must keep an offscreen origin whose bitmap overlaps the
    // viewport. Black pixels cover prior effects; transparent pixels preserve them.
    const std::uint8_t full[]={255,255},edge[]={0xaa,0x55},black[]={0,0};
    const EffectMaskAssets::Asset whiteAsset{{full,2,16,1,2},0,0,full,black};
    const EffectMaskAssets::Asset blackAsset{{edge,2,16,1,2},0,300,black,black};
    smoke.clear();
    auto emptyCurrency=[](auto){};
    for(unsigned mode=0;mode<4;++mode){
        SmokeComposition::Settings settings;
        settings.death=mode==1;settings.lowSpec=mode==2;settings.effectsExist=mode!=3;
        settings.shadeExists=false;
        unsigned surfaceCalls=0;
        auto ordinary=[&](auto emit){return emit(whiteAsset,q(80),q(1));};
        auto cover=[&](auto emit){++surfaceCalls;return emit(blackAsset,q(80),q(301));};
        assert(colored.prepareWithSurfaceProducers(emptyCurrency,smoke,assets,view,settings,
            ordinary,cover,noShades,raster));
        assert(surfaceCalls==(mode==3?0u:1u));
        assert(colored.visible()==(mode!=3));
        unsigned visited=0;
        colored.surface().visit([&](unsigned x,unsigned y,std::uint16_t alpha,std::uint16_t lo,std::uint16_t hi){
            ++visited;assert(x==80&&y==1&&hi==0);
            assert(alpha==(mode==0?0xffff:0xaa55));
            assert(lo==(mode==0?0x55aa:0));
        });
        assert(visited==(mode==3?0u:1u));
    }
    // Black cover alpha is subtracted too. The no-control guard, not death,
    // suppresses shade holes; a hardware cover is invalid in the shaded case.
    for(unsigned mode=0;mode<3;++mode){
        SmokeComposition::Settings settings;settings.noControl=mode==1;settings.death=mode==2;
        const auto* shade=assets.shade();assert(shade);
        auto blackCover=*shade;blackCover.low=blackCover.high=shade->mask.data;
        assert(colored.prepareWithSurfaceProducers(emptyCurrency,smoke,assets,view,settings,
            [](auto){return true;},[&](auto emit){return emit(blackCover,q(80),q(140));},
            [&](auto emit){emit(q(80),q(140));},raster));
        assert(colored.visible()==(mode==1));
    }
    assert(!colored.prepareWithSurfaceProducers(emptyCurrency,smoke,assets,view,{},
        [](auto){return true;},[&](auto emit){assert(emit(blackAsset,q(80),q(301)));return false;},
        noShades,raster));
    assert(!colored.visible());
    colored.surface().visit([](auto,auto,auto,auto,auto){assert(false);});
    // Whole-pixel producers bypass binary64 placement and the raster. They
    // must compose exactly as the same positions emitted as binary64 with the
    // Amiga's round-to-nearest raster: view edges, odd shade snaps, fractional
    // views and magnitudes beyond the pixel range (taken through Q16).
    unsigned pixelChecks=0,pixelStamps=0;
    {
        CurrencySmokeComposition pixel;
        auto rounded=[](Fixed16 n,std::int32_t& out){return n.roundPixel(out);};
        std::uint32_t seed=0x2545f491u;
        auto next=[&]{seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
        smoke.clear();
        for(unsigned trial=0;trial<4000;++trial){
            const bool fractional=trial%8==7,far=trial%16==3;
            const int vx=far?(1<<14)+int(next()%8):int(next()%64)-32;
            const int vy=far?-(1<<14)-int(next()%8):int(next()%64)-32;
            const EffectSurfacePlacement::View pixelView{q(vx+(fractional?0.5:0)),q(vy),q(160),q(284)};
            auto pick=[&](int origin,int size){
                switch(next()%8){
                case 0:return std::int32_t(origin);
                case 1:return std::int32_t(origin+size);
                case 2:return std::int32_t(origin+1+int(next()%2));
                case 3:return std::int32_t(origin+size-1-int(next()%2));
                case 4:return std::int32_t((next()&1?1:-1)*((1<<14)-2+int(next()%5)));
                default:return std::int32_t(origin-24+int(next()%unsigned(size+48)));
                }
            };
            std::int32_t gx[6],gy[6],sx[4],sy[4];
            for(unsigned i=0;i<6;++i){gx[i]=pick(vx,160);gy[i]=pick(vy,284);}
            for(unsigned i=0;i<4;++i){sx[i]=pick(vx,160);sy[i]=pick(vy,284);}
            const unsigned phase=trial%4;
            auto gemsPixel=[&](auto emit){for(unsigned i=0;i<6;++i)emit(bool(i&1),(phase+i)%4,gx[i],gy[i]);};
            auto gemsReal=[&](auto emit){for(unsigned i=0;i<6;++i)
                emit(bool(i&1),(phase+i)%4,Fixed16::fromInteger(gx[i]),Fixed16::fromInteger(gy[i]));};
            auto shadesPixel=[&](auto emit){for(unsigned i=0;i<4;++i)emit(sx[i],sy[i]);};
            auto shadesReal=[&](auto emit){for(unsigned i=0;i<4;++i)
                emit(Fixed16::fromInteger(sx[i]),Fixed16::fromInteger(sy[i]));};
            assert(pixel.prepare(gemsPixel,smoke,assets,pixelView,{},shadesPixel,rounded));
            assert(colored.prepare(gemsReal,smoke,assets,pixelView,{},shadesReal,rounded));
            assert(pixel.surface().samePixels(colored.surface())&&pixel.visible()==colored.visible());
            ++pixelChecks;pixelStamps+=pixel.visible();
        }
    }
    assert(pixelChecks==4000&&pixelStamps>1000);
#ifdef DOWNWELL_PROBES
    assert(activePhase==profileControllerComposition);
    assert(phaseCalls[profileEffectPlacement] && phaseCalls[profileEffectRaster] &&
           phaseCalls[profileEffectStamp]);
#endif
    std::printf("Currency/smoke composition: %u joint color/shade frames match scalar painter\n",frames);
    std::printf("Currency/smoke whole-pixel producers: %u frames (%u visible) match binary64 placement\n",pixelChecks,pixelStamps);
    std::printf("Smoke composition: %u lifecycle/flag/camera frames match scalar masks; direct=%u controller=%u hidden=%u\n",frames,directFrames,controllerFrames,hiddenFrames);
}

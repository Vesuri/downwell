#include "core/SmokeComposition.h"
#include "core/CurrencySmokeComposition.h"
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
static MotionBinary64 number(double d){std::uint64_t n;std::memcpy(&n,&d,8);return {std::uint32_t(n>>32),std::uint32_t(n)};}
static double value(MotionBinary64 n){const std::uint64_t bits=(std::uint64_t(n.high)<<32)|n.low;double d;std::memcpy(&d,&bits,8);return d;}
int main(int argc,char**argv){
    assert(argc==2);std::ifstream file(argv[1],std::ios::binary);
    const std::vector<std::uint8_t> pack{std::istreambuf_iterator<char>(file),{}};
    EffectMaskAssets assets;assert(assets.bind(pack.data(),pack.size()));
    SmokeEffects<32> smoke;GameMakerRandom random(225);
    for(unsigned i=0;i<24;++i)assert(smoke.create(number(int(i%6)*29-3),number(int(i/6)*75-2),random));
    SmokeComposition composition;
    CurrencySmokeComposition colored;
    unsigned frames=0,directFrames=0,controllerFrames=0,hiddenFrames=0,shadeVisits=0;
    const std::array<std::array<double,2>,4> lights{{{3,1},{63,77},{121,151},{159,283}}};
    // Sampling is intentionally supplied by the test, not claimed to establish
    // Windows raster semantics. Fractional route/snap arithmetic is independent.
    auto raster=[](MotionBinary64 n,std::int32_t& out){out=int(std::floor(value(n)));return true;};
    for(unsigned tick=0;tick<100;++tick){
        for(unsigned flags=0;flags<32;++flags){
            SmokeComposition::Settings settings{bool(flags&1),bool(flags&2),bool(flags&4),bool(flags&8),bool(flags&16)};
            const double vx=double(int(tick%9)-4)*0.25,vy=double(int(tick%7)-3)*0.5;
            EffectSurfacePlacement::View view{number(vx),number(vy),number(160),number(284)};
            const auto saved=random;
            assert(composition.prepare(smoke,assets,view,settings,[&](auto emit){
                ++shadeVisits;for(auto light:lights)emit(number(light[0]),number(light[1]));
            },raster));
            auto gems=[&](auto emit){
                for(unsigned i=0;i<12;++i)
                    emit(bool(i&1),(tick+i)%4,number(int(i%6)*29-3),number(int(i/6)*75+1));
            };
            assert(colored.prepare(gems,smoke,assets,view,settings,[&](auto emit){
                for(auto light:lights)emit(number(light[0]),number(light[1]));
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
                gems([&](bool large,unsigned frame,MotionBinary64 x,MotionBinary64 y){
                    if(visible(value(x),value(y)))paint(*assets.currency(large,frame),value(x)-vx,value(y)-vy,false);
                });
                smoke.visitNewest([&](auto,const SmokeEffect& e){
                    const double x=value(e.x.promoted()),y=value(e.y.promoted());
                    if(!e.destroyRequested && visible(x,y))paint(*assets.smoke(e.drawFrame()),x-vx,y-vy,false);
                });
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
            smoke.visitNewest([&](auto,const SmokeEffect& e){
                const double x=value(e.x.promoted()),y=value(e.y.promoted());
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
    assert(smoke.create(number(80),number(140),random));
    const EffectSurfacePlacement::View view{{},{},number(160),number(284)};
    auto noShades=[](auto){};
    assert(composition.prepare(smoke,assets,view,{},noShades,raster) && composition.visible());
    assert(!composition.prepare(smoke,assets,view,{},noShades,[](auto,std::int32_t&){return false;}));
    assert(!composition.visible() && composition.layer()==SmokeComposition::Layer::none);
    unsigned words=0;composition.coverage().visit([&](auto,auto,auto){++words;});assert(!words);
    smoke.clear();EffectMaskAssets missing;
    assert(composition.prepare(smoke,missing,view,{},[](auto){assert(false);},raster));
    assert(smoke.create(number(80),number(140),random));
    assert(!composition.prepare(smoke,missing,view,{},noShades,raster));
    auto oneGem=[](auto emit){emit(false,0,number(80),number(140));};
    assert(colored.prepare(oneGem,smoke,assets,view,{},noShades,raster) && colored.visible());
    assert(!colored.prepare(oneGem,smoke,assets,view,{},noShades,[](auto,std::int32_t&){return false;}));
    assert(!colored.visible());
    words=0;colored.surface().visit([&](auto,auto,auto,auto,auto){++words;});assert(!words);
    assert(!colored.prepare(oneGem,smoke,missing,view,{},noShades,raster));
    std::printf("Currency/smoke composition: %u joint color/shade frames match scalar painter\n",frames);
    std::printf("Smoke composition: %u lifecycle/flag/camera frames match scalar masks; direct=%u controller=%u hidden=%u\n",frames,directFrames,controllerFrames,hiddenFrames);
}

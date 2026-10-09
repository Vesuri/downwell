#include "core/BinaryEffectSurface.h"
#include "core/EffectMaskAssets.h"
#include "core/CorpseDraw.h"
#include <array>
#include <vector>
#include <fstream>
#include <iterator>
#include <cstdlib>
#include <cstdio>
#include <climits>
using Surface=downwell::BinaryEffectSurface<>;
static_assert(sizeof(Surface)<=6820,"effect scratch must remain ordinary-RAM bounded");
static void require(bool b,int line){if(!b){std::fprintf(stderr,"Binary effect check failed at line %d\n",line);std::abort();}}
#define check(b) require((b),__LINE__)
static unsigned word(const unsigned char* p){return unsigned(p[0])*256+p[1];}
static unsigned longword(const unsigned char* p){return word(p)*65536u+word(p+2);}
int main(int argc,char**argv){
    check(argc==2);std::ifstream input(argv[1],std::ios::binary);
    std::vector<unsigned char> pack{std::istreambuf_iterator<char>(input),{}};
    check(pack.size()>20);
    struct Asset {Surface::Mask mask;int ox,oy;};
    auto asset=[&](unsigned id,unsigned frame){
        for(unsigned i=0;i<longword(pack.data()+8);++i){
            const auto* r=pack.data()+word(pack.data()+6)+i*24;
            if(word(r)!=id || word(r+2)!=frame)continue;
            const unsigned w=word(r+8),h=word(r+10),stride=word(r+12)/8;
            const unsigned offset=longword(r+16),length=longword(r+20);
            check(word(r+14)==3 && offset+length<=pack.size() && length==stride*h*3);
            return Asset{{pack.data()+offset+2*stride*h,stride*h,w,h,stride},
                         int(short(word(r+4))),int(short(word(r+6)))};
        }
        std::abort();
    };
    Surface surface;std::array<bool,160*284> expected{};
    downwell::EffectMaskAssets shared;
    check(shared.bind(pack.data(),pack.size()));
    {
        using Real=downwell::MotionBinary64;
        const downwell::EffectSurfacePlacement::View view{Real::fromInteger(160),Real::fromInteger(34),
            Real::fromInteger(160),Real::fromInteger(284)};
        for(unsigned sprite:{172u,206u})for(bool mirrored:{false,true}){
            downwell::CorpseAnimation corpse;corpse.spriteId=int(sprite);
            corpse.x={0x43784000u};corpse.y={0x43008000u}; // 248.25, 128.5
            corpse.imageXscale={mirrored?0xbf800000u:0x3f800000u};corpse.imageIndex={0x3f400000u};
            unsigned emitted=0;
            auto emit=[&](const auto& actual,Real x,Real y){
                const auto expectedAsset=asset(sprite+(mirrored?62000:0),0);
                check(actual.mask.data==expectedAsset.mask.data&&actual.originX==expectedAsset.ox&&actual.originY==expectedAsset.oy);
                check(x.high==corpse.x.promoted().high&&x.low==corpse.x.promoted().low&&
                    y.high==corpse.y.promoted().high&&y.low==corpse.y.promoted().low);
                ++emitted;return true;
            };
            check(downwell::CorpseDraw::surface(corpse,shared,view,emit)&&emitted==1);
            check(!downwell::CorpseDraw::surface(corpse,shared,view,[](const auto&,auto,auto){return false;}));
            corpse.flash=1;check(downwell::CorpseDraw::surface(corpse,shared,view,emit)&&emitted==1);
            corpse.flash=0;corpse.destroyRequested=true;
            check(downwell::CorpseDraw::surface(corpse,shared,view,emit)&&emitted==1);
            corpse.destroyRequested=false;corpse.x={0x43200000u}; // Strict left edge: hidden.
            check(downwell::CorpseDraw::surface(corpse,shared,view,emit)&&emitted==1);
            corpse.x={0x43784000u};corpse.imageXscale={};
            check(downwell::CorpseDraw::surface(corpse,shared,view,emit)&&emitted==1);
            corpse.imageXscale={0x40000000u};check(!downwell::CorpseDraw::surface(corpse,shared,view,emit));
            corpse.imageXscale={0x3f800000u};corpse.imageIndex={0x7fc00000u};
            check(!downwell::CorpseDraw::surface(corpse,shared,view,emit)&&emitted==1);
        }
        std::puts("Corpse surface adapter: four offline variants, fractional anchors, visibility and failure gates passed");
    }
    // Keep the independent directory walker above as an oracle for the shared
    // descriptor's borrowed bytes and signed cropped-frame origins.
    for(unsigned frame=0;frame<9;++frame){
        const auto reference=asset(84,frame);const auto* actual=shared.smoke(frame);
        check(actual && actual->originX==reference.ox && actual->originY==reference.oy &&
              actual->mask.data==reference.mask.data && actual->mask.bytes==reference.mask.bytes &&
              actual->mask.width==reference.mask.width && actual->mask.height==reference.mask.height &&
              actual->mask.stride==reference.mask.stride);
    }
    check(!shared.smoke(9));
    auto stamp=[&](const Asset& a,int ax,int ay,bool subtract){
        const int x=ax-a.ox,y=ay-a.oy;
        check(subtract?surface.subtract(a.mask,x,y):surface.add(a.mask,x,y));
        for(unsigned sy=0;sy<a.mask.height;++sy)for(unsigned sx=0;sx<a.mask.width;++sx){
            const int dx=x+int(sx),dy=y+int(sy);
            if(dx<0||dx>=160||dy<0||dy>=284)continue;
            if(a.mask.data[sy*a.mask.stride+sx/8]&(0x80>>(sx%8)))expected[dy*160+dx]=!subtract;
        }
    };
    auto verify=[&](){
        std::array<bool,160*284> actual{};
        surface.visit([&](unsigned x,unsigned y,std::uint16_t bits){
            check(x%16==0 && x<160 && y<284);
            for(unsigned b=0;b<16;++b)actual[y*160+x+b]=(bits&(0x8000>>b))!=0;
        });check(actual==expected);
    };
    const auto* resolvedShade=shared.shade();const auto referenceShade=asset(117,0);
    check(resolvedShade && resolvedShade->mask.data==referenceShade.mask.data &&
          resolvedShade->originX==referenceShade.ox && resolvedShade->originY==referenceShade.oy);
    const Asset shade{resolvedShade->mask,resolvedShade->originX,resolvedShade->originY};
    for(unsigned tick=0;tick<256;++tick){
        surface.clear();expected.fill(false);verify();
        for(unsigned i=0;i<9;++i){const auto* a=shared.smoke(i);
            stamp({a->mask,a->originX,a->originY},int((tick*7+i*31)%220)-30,int((tick*13+i*47)%340)-28,false);}
        verify();
        for(unsigned i=0;i<5;++i){stamp(shade,int((tick*11+i*29)%220)-30,int((tick*17+i*59)%340)-28,true);verify();}
    }
    const unsigned char oddMask[]={0xff,0xff,0xff};
    Asset odd{{oddMask,3,17,1,3},0,0};
    for(int x=-20;x<165;++x){surface.clear();expected.fill(false);stamp(odd,x,0,false);verify();}
    check(surface.add(odd.mask,INT_MIN,INT_MAX));verify();
    auto invalid=odd.mask;invalid.bytes=2;check(!surface.add(invalid,0,0));verify();
    std::puts("Binary effects: 256 source-art compositions, overlap/clipping, all X shifts, dirty clear and invalid input passed");
}

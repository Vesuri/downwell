#include "core/BinaryEffectSurface.h"
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
    const auto shade=asset(117,0);
    for(unsigned tick=0;tick<256;++tick){
        surface.clear();expected.fill(false);verify();
        for(unsigned i=0;i<9;++i)stamp(asset(84,i),int((tick*7+i*31)%220)-30,int((tick*13+i*47)%340)-28,false);
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

#include "core/PaletteEffectSurface.h"
#include "core/EffectMaskAssets.h"
#include <array>
#include <vector>
#include <fstream>
#include <iterator>
#include <cassert>
#include <cstdio>
#include <climits>
using Surface=downwell::PaletteEffectSurface<>;
static_assert(sizeof(Surface)<=18300,"portable scratch and backend bindings must stay outside the task stack");
static unsigned word(const unsigned char* p){return unsigned(p[0])*256+p[1];}
static unsigned dword(const unsigned char* p){return word(p)*65536u+word(p+2);}
struct Asset {Surface::Asset image;int ox,oy;};
static unsigned waitCalls;
int main(int argc,char**argv){
    assert(argc==2||argc==3);std::ifstream file(argv[1],std::ios::binary);
    const std::vector<unsigned char> pack{std::istreambuf_iterator<char>(file),{}};
    downwell::EffectMaskAssets shared;
    assert(shared.bind(pack.data(),pack.size()));
    auto find=[&](unsigned id,unsigned frame){
        for(unsigned i=0;i<dword(pack.data()+8);++i){
            const auto* r=pack.data()+word(pack.data()+6)+i*24;
            if(word(r)!=id || word(r+2)!=frame)continue;
            const unsigned stride=word(r+12)/8,plane=stride*word(r+10),offset=dword(r+16);
            assert(word(r+14)==3 && dword(r+20)==3*plane && offset+3*plane<=pack.size());
            const auto* cached=id==84?shared.smoke(frame):id==117?shared.shade():shared.currency(id==134,frame);
            assert(cached && cached->low==pack.data()+offset && cached->high==pack.data()+offset+plane &&
                   cached->mask.data==pack.data()+offset+2*plane && cached->mask.bytes==plane &&
                   cached->mask.width==word(r+8) && cached->mask.height==word(r+10) && cached->mask.stride==stride &&
                   cached->originX==int(short(word(r+4))) && cached->originY==int(short(word(r+6))));
            return Asset{{{pack.data()+offset+2*plane,plane,word(r+8),word(r+10),stride},
                           pack.data()+offset,pack.data()+offset+plane},
                         int(short(word(r+4))),int(short(word(r+6)))};
        }
        std::abort();
    };
    Surface surface;
    // Borrowed padded storage: declined acceleration still uses the exact CPU
    // path and must respect guards. Native tests exercise successful DMA.
    struct Storage {std::array<std::uint16_t,12*284*3> data;unsigned stride,plane;};
    Storage storage{{},argc==3?36u:12u,argc==3?12u:12u*284};
    auto& external=storage.data;external.fill(0x5a5a);
    assert(surface.bindStorage(external.data()+1,external.data()+storage.plane+1,
        external.data()+storage.plane*2+1,storage.stride,{&storage,
            [](void*,const Surface::Mask&,const std::uint8_t*,const std::uint8_t*,int,int,const Surface::Clip&){return false;},
            [](void* p,unsigned left,unsigned top,unsigned words,unsigned rows){auto& s=*static_cast<Storage*>(p);
                assert(words&&rows&&left+words<=10&&top+rows<=284);
                for(unsigned plane=0;plane<3;++plane)for(unsigned y=top;y<top+rows;++y)
                    for(unsigned x=1+left;x<=left+words;++x)s.data[plane*s.plane+y*s.stride+x]=0;
            },[](void*){++waitCalls;}}));
    const auto boundWaits=waitCalls;
    assert(!surface.intersects(0,0,160,284)&&waitCalls==boundWaits);
    // -1 is transparency; 0 is opaque black and must not be confused with it.
    std::array<int,160*284> expected{},actual{};
    // The backend clears only the touched rectangles: every stored word
    // must be zero afterwards, and the guards untouched.
    auto clear=[&](){
        surface.clear();expected.fill(-1);
        for(unsigned plane=0;plane<3;++plane)for(unsigned y=0;y<284;++y)for(unsigned x=0;x<12;++x)
            assert(external[plane*storage.plane+y*storage.stride+x]==(x&&x<=10?0:0x5a5a));
    };
    auto verify=[&](){
        std::array<bool,160*284> covered{};
        unsigned previousEnd=0;
        surface.visitRectangles([&](unsigned x,unsigned y,unsigned width,unsigned height){
            assert(width&&height&&x+width<=160&&y+height<=284&&y>=previousEnd);
            assert(!(x%16)&&!(width%16));previousEnd=y+height;
            for(unsigned row=y;row<y+height;++row)for(unsigned col=x;col<x+width;++col)
                covered[row*160+col]=true;
        });
        for(unsigned i=0;i<expected.size();++i)assert(expected[i]<0||covered[i]);
        actual.fill(-1);
        surface.visit([&](unsigned x,unsigned y,std::uint16_t mask,std::uint16_t low,std::uint16_t high){
            assert(!(low&~mask) && !(high&~mask));
            for(unsigned bit=0;bit<16;++bit)if(mask&(0x8000u>>bit))
                actual[y*160+x+bit]=unsigned(bool(low&(0x8000u>>bit)))+2*unsigned(bool(high&(0x8000u>>bit)));
        });assert(actual==expected);
        static unsigned serial=0;
        for(unsigned n=0;n<24;++n,++serial){
            const int left=int((serial*29)%210)-25,top=int((serial*17)%334)-25;
            const int right=left+int(serial%35),bottom=top+int(serial%21);
            bool hit=false;
            for(int y=top;y<bottom;++y)for(int x=left;x<right;++x)
                if(x>=0 && x<160 && y>=0 && y<284 && expected[y*160+x]>=0)hit=true;
            assert(surface.intersects(left,top,right,bottom)==hit);
        }
        const auto beforeEmptyQueries=waitCalls;
        assert(!surface.intersects(INT_MIN,INT_MIN,0,0));
        assert(!surface.intersects(160,284,INT_MAX,INT_MAX));
        assert(waitCalls==beforeEmptyQueries);
        bool any=false;for(int pen:expected)any|=pen>=0;
        assert(surface.intersects(INT_MIN,INT_MIN,INT_MAX,INT_MAX)==any);
    };
    auto stamp=[&](const Asset& a,int anchorX,int anchorY,bool shade){
        const int x=anchorX-a.ox,y=anchorY-a.oy;const auto& mask=a.image.alpha;
        assert(shade?surface.subtract(mask,x,y):surface.add(a.image,x,y));
        for(unsigned sy=0;sy<mask.height;++sy)for(unsigned sx=0;sx<mask.width;++sx){
            const int dx=x+int(sx),dy=y+int(sy);const unsigned at=sy*mask.stride+sx/8,bit=0x80u>>(sx%8);
            if(dx<0||dx>=160||dy<0||dy>=284||!(mask.data[at]&bit))continue;
            expected[dy*160+dx]=shade?-1:int(bool(a.image.low[at]&bit))+2*int(bool(a.image.high[at]&bit));
        }
    };
    const auto shade=find(117,0);
    for(unsigned tick=0;tick<256;++tick){
        clear();verify();
        // Currency at depth 10000 precedes smoke at 1000 on the same surface.
        for(unsigned i=0;i<8;++i)stamp(find(i%2?133:134,(tick+i)%4),int((tick*7+i*19)%190)-15,int((tick*3+i*23)%310)-13,false);
        verify();
        for(unsigned i=0;i<9;++i)stamp(find(84,i),int((tick*7+i*17)%190)-15,int((tick*3+i*21)%310)-13,false);
        verify();
        stamp(shade,int(tick%160),int(tick%284),true);verify();
        stamp(shade,80,142,true);verify();
    }
    // All source pens, unaligned overwrite and exact shade holes. Deliberate
    // color bits outside alpha must never become visible, even after another draw.
    const unsigned char alpha[]={0xa5,0x5a,0x80},ones[]={0xff,0xff,0xff},zeros[]={0,0,0};
    for(int x=-20;x<170;++x)for(unsigned pen=0;pen<4;++pen){
        clear();Asset a{{{alpha,3,17,1,3},pen&1?ones:zeros,pen&2?ones:zeros},0,0};
        Asset under{{{ones,3,17,1,3},ones,ones},0,0};
        stamp(under,x,5,false);stamp(a,x,5,false);verify();
        stamp(a,x+1,5,true);verify();
    }
    auto invalid=shade.image;invalid.alpha.bytes=0;
    assert(!surface.add(invalid,0,0));verify();
    assert(surface.add(shade.image,INT_MIN,INT_MAX));verify();
    clear();verify();
    // Jump between the first and last row, erase all coverage, and clear an
    // already empty surface. Row-range retention must not resurrect color.
    const unsigned char dot[]={0x80,0},black[]={0,0};
    const Asset point{{{dot,2,1,1,2},black,black},0,0};
    for(unsigned cycle=0;cycle<64;++cycle){
        clear();verify();
        stamp(point,cycle%160,cycle%2?0:283,false);verify();
        stamp(point,cycle%160,cycle%2?0:283,true);verify();
        clear();clear();verify();
    }
    std::puts("Palette effects: 256 source gem/smoke/shade frames plus 760 shifted all-pen overwrite cases passed");
    for(unsigned plane=0;plane<3;++plane)for(unsigned y=0;y<284;++y)
        assert(external[plane*storage.plane+y*storage.stride]==0x5a5a&&external[plane*storage.plane+y*storage.stride+11]==0x5a5a);
}

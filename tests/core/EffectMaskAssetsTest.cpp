#include "core/EffectMaskAssets.h"
#include <array>
#include <cassert>
#include <cstdio>
using Assets=downwell::EffectMaskAssets;
int main(){
    std::array<std::uint8_t,458> pack{};
    auto word=[&](unsigned at,unsigned n){pack[at]=n>>8;pack[at+1]=n;};
    auto dword=[&](unsigned at,unsigned n){word(at,n>>16);word(at+2,n);};
    dword(0,0x44574150u);word(4,2);word(6,20);dword(8,18);
    const std::uint8_t hash[]={0xc0,0x7d,0xaf,0x5f,0xbe,0x94,0x0a,0xe0};
    for(unsigned i=0;i<8;++i)pack[12+i]=hash[i];
    for(unsigned i=0;i<18;++i){
        const unsigned at=20+i*24;
        word(at,i<9?84:i==9?117:133+(i-10)/4);word(at+2,i<9?i:i==9?0:(i-10)%4);
        word(at+4,65533);word(at+6,7); // Signed origin, no recentering.
        word(at+8,1);word(at+10,1);word(at+12,16);word(at+14,3);
        dword(at+16,452);dword(at+20,6); // Immutable payload sharing is legal.
    }
    pack[456]=0x80;
    const auto good=pack;
    Assets assets;
    assert(!assets.ready() && !assets.smoke(0) && !assets.shade());
    auto bind=[&](){return assets.bind(pack.data(),pack.size());};
    assert(bind());
    for(unsigned i=0;i<9;++i){
        const auto* a=assets.smoke(i);
        assert(a && a->originX==-3 && a->originY==7 && a->mask.data==pack.data()+456 &&
               a->mask.bytes==2 && a->mask.stride==2 && a->mask.width==1 && a->mask.height==1);
        assert(a->low==pack.data()+452 && a->high==pack.data()+454);
    }
    assert(assets.shade() && !assets.smoke(9) && !assets.smoke(~0u));
    for(unsigned large=0;large<2;++large)for(unsigned frame=0;frame<4;++frame){
        const auto* a=assets.currency(large,frame);
        assert(a && a->mask.data==pack.data()+456 && a->low==pack.data()+452 &&
               a->high==pack.data()+454 && a->originX==-3 && a->originY==7);
    }
    assert(!assets.currency(false,4) && !assets.currency(true,~0u));
    for(unsigned size=0;size<pack.size();++size){
        assert(bind());
        assert(!assets.bind(pack.data(),size));
        assert(!assets.ready() && !assets.smoke(0) && !assets.shade() && !assets.currency(false,0));
    }
    assert(!assets.bind(nullptr,pack.size()));
    unsigned invalid=0;
    auto reject=[&](){assert(!bind());assert(!assets.smoke(0)&&!assets.shade()&&!assets.currency(true,0));pack=good;++invalid;};
    for(unsigned at:{0u,4u,6u,12u,19u}){pack[at]^=1;reject();}
    for(unsigned count:{0u,10u,17u,19u,0xffffffffu}){dword(8,count);reject();}
    // Directory duplicate, disorder, missing frame and missing shade.
    word(20+24+2,0);reject();word(20+24,83);reject();word(20,85);reject();word(20+9*24,118);reject();
    for(unsigned i=0;i<18;++i){
        const unsigned at=20+24*i;
        word(at+8,0);reject();word(at+10,0);reject();
        word(at+8,17);reject();word(at+12,15);reject();word(at+14,4);reject();
        dword(at+16,450);reject();dword(at+16,453);reject();
        dword(at+16,0xffffffffu);reject();dword(at+20,5);reject();dword(at+20,0xffffffffu);reject();
    }
    assert(bind() && pack==good);
    Assets::Asset debris{};
    assert(!assets.debris(91,0,debris)); // Older pack has no optional debris.
    assert(!assets.corpse(172,false,debris)&&!assets.corpse(206,true,debris));
    assert(!assets.corpse(65536+172,false,debris));
    std::array<std::uint8_t,794> expanded{};
    for(unsigned i=0;i<20;++i)expanded[i]=good[i];
    expanded[11]=32;
    unsigned row=0;
    for(unsigned id:{84u,91u,115u,116u,117u,133u,134u}){
        const unsigned frames=id==84?9:id==91?6:id==117?1:4;
        for(unsigned frame=0;frame<frames;++frame){
            auto* record=expanded.data()+20+24*row++;
            for(unsigned i=0;i<24;++i)record[i]=good[20+i];
            record[0]=id>>8;record[1]=id;record[2]=0;record[3]=frame;
            record[16]=record[17]=0;record[18]=3;record[19]=20; // payload 788
        }
    }
    assert(row==32);
    expanded[788]=0x80;expanded[792]=0x80;
    assert(assets.bind(expanded.data(),expanded.size()));
    for(unsigned id:{91u,115u,116u}){
        const unsigned frames=id==91?6:4;
        for(unsigned frame=0;frame<frames;++frame){
            assert(assets.debris(id,frame,debris));
            assert(debris.low==expanded.data()+788 && debris.high==expanded.data()+790 &&
                   debris.mask.data==expanded.data()+792 && debris.originX==-3 && debris.originY==7);
        }
        assert(!assets.debris(id,frames,debris));
        assert(!assets.debris(id,~0u,debris));
    }
    assert(!assets.debris(84,0,debris) && !assets.debris(65536+91,0,debris));
    // Optional malformed payload is rejected at lookup, without changing output.
    expanded[20+9*24+14]=1;
    assert(assets.bind(expanded.data(),expanded.size()));
    assert(!assets.debris(91,0,debris) && debris.low==expanded.data()+788);
    assert(!assets.bind(nullptr,0));
    assert(!assets.debris(115,0,debris));
    assert(!assets.corpse(206,false,debris));
    std::printf("Effect assets: shared color/mask payloads, signed origins, 458 truncations and %u malformed records rejected\n",invalid);
}

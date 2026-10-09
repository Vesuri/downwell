#include "platform/amiga/AmigaEffectStampBank.h"
#include <fstream>
#include <iterator>
#include <vector>
#include <cassert>
#include <cstdio>
static std::vector<unsigned char> read(const char* path){
    std::ifstream file(path,std::ios::binary);assert(file);
    return {std::istreambuf_iterator<char>(file),{}};
}
int main(){
    const auto pack=read("generated/amiga-game.dwap"),data=read("generated/amiga-effect-stamps.dwef");
    downwell::EffectMaskAssets assets;assert(assets.bind(pack.data(),pack.size()));
    AmigaEffectStampBank bank;
    std::vector<unsigned char> payload(data.begin()+AmigaEffectStampBank::payloadOffset,data.end());
    for(bool split:{false,true}){
    assert(split?bank.bindPayload(data.data(),data.size(),payload.data(),payload.size(),assets):
        bank.bind(data.data(),data.size(),assets));
    for(unsigned i=0;i<17;++i){
        const auto* asset=i<9?assets.smoke(i):assets.currency(i>=13,(i-9)%4);
        const auto& a=asset->mask;
        downwell::PaletteEffectSurface<>::Mask mask{a.data,a.bytes,a.width,a.height,a.stride};
        const auto* entry=bank.find(mask,asset->low,asset->high);assert(entry);
        const auto* base=split?payload.data():data.data();
        const auto size=split?payload.size():data.size();
        assert(entry->pixels>=base&&entry->mask+3*a.stride*a.height<=base+size);
        for(unsigned y=0;y<a.height;++y)for(unsigned x=0;x<a.stride;++x){
            const unsigned source=y*a.stride+x;
            assert(entry->pixels[(y*3)*a.stride+x]==a.data[source]);
            assert(entry->pixels[(y*3+1)*a.stride+x]==asset->low[source]);
            assert(entry->pixels[(y*3+2)*a.stride+x]==asset->high[source]);
            for(unsigned p=0;p<3;++p)assert(entry->mask[(y*3+p)*a.stride+x]==a.data[source]);
        }
        ++mask.width;assert(!bank.find(mask,asset->low,asset->high));--mask.width;
        assert(!bank.find(mask,nullptr,nullptr));
    }
    const auto& shade=assets.shade()->mask;
    assert(!bank.find({shade.data,shade.bytes,shade.width,shade.height,shade.stride},nullptr,nullptr));
    }
    auto bad=data;bad[48]=0xff;assert(!bank.bind(bad.data(),bad.size(),assets));
    const auto* first=assets.smoke(0);const auto& m=first->mask;
    assert(!bank.find({m.data,m.bytes,m.width,m.height,m.stride},first->low,first->high));
    assert(!bank.bind(data.data(),data.size()-1,assets));
    assert(!bank.bindPayload(data.data(),data.size(),payload.data(),payload.size()-1,assets));
    assert(!bank.bindPayload(data.data(),data.size(),nullptr,payload.size(),assets));
    assert(bank.bind(data.data(),data.size(),assets));
    std::puts("Effect stamp bank: 17 identities, exact streams, fallback guards and failed-rebind invalidation pass");
}

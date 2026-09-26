#include "core/SpriteMaskPack.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>
using Pack=downwell::SpriteMaskPack;
int main(int argc,char** argv){
    assert(argc==2);
    std::ifstream input(argv[1],std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
    Pack pack(bytes.data(),bytes.size());assert(pack.valid()&&pack.count()==4);
    const unsigned ids[]={577,597,613,616},widths[]={48,48,72,128},heights[]={40,48,72,128};
    const unsigned sourceHits[]={1044,1741,3969,12645};
    unsigned checked=0;
    for(unsigned i=0;i<4;++i){
        Pack::Mask mask;assert(pack.mask(ids[i],0,mask));
        assert(mask.width==widths[i]&&mask.height==heights[i]);
        unsigned hits=0;
        for(int y=-1;y<=int(mask.height);++y)for(int x=-1;x<=int(mask.width);++x){
            bool expected=false;
            if(x>=0&&y>=0&&x<int(mask.width)&&y<int(mask.height))
                expected=((mask.bytes[y*mask.stride+x/8]>>(7-x%8))&1)!=0;
            assert(mask.contains(x,y)==expected);hits+=mask.contains(x,y);++checked;
        }
        assert(hits==sourceHits[i]);
        const auto* saved=mask.bytes;
        assert(!pack.mask(ids[i],1,mask)&&mask.bytes==saved);
        assert(!pack.mask(9999,0,mask)&&mask.bytes==saved);
    }
    assert(!Pack(nullptr,0).valid());
    for(std::size_t size=0;size<bytes.size();++size)assert(!Pack(bytes.data(),size).valid());
    for(unsigned offset:{0u,4u,8u,16u,20u,32u,36u,40u}){
        auto bad=bytes;for(unsigned j=0;j<4;++j)bad[offset+j]=255;
        assert(!Pack(bad.data(),bad.size()).valid());
    }
    auto duplicate=bytes;for(unsigned i=0;i<8;++i)duplicate[44+i]=duplicate[24+i];
    assert(!Pack(duplicate.data(),duplicate.size()).valid());
    auto trailing=bytes;trailing.push_back(0);assert(!Pack(trailing.data(),trailing.size()).valid());
    std::printf("Precise mask pack: %u pixel/boundary checks, all truncations and malformed metadata passed\n",checked);
}

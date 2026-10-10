#include "platform/amiga/AmigaPlaneBank.h"
#include "../../../generated/amiga-game-planes.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>
static unsigned u32(const unsigned char* p){return (unsigned(p[0])<<24)|(unsigned(p[1])<<16)|(unsigned(p[2])<<8)|p[3];}
static unsigned u16(const unsigned char* p){return (unsigned(p[0])<<8)|p[1];}
static std::vector<unsigned char> load(const char* path){
    std::ifstream in(path,std::ios::binary);assert(in);
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
int main(){
    const auto source=load("generated/amiga-game.dwap");auto bank=load("generated/amiga-game.planes");
    AmigaPlaneBank atlas;
    assert(bank.size()==DOWNWELL_GAME_PLANE_BYTES);
    assert(atlas.bind(source.data(),source.size(),bank.data(),bank.size(),downwellGamePlaneSourceHash));
    unsigned checked=0;
    for(unsigned i=0;i<u32(source.data()+8);++i){
        const auto* r=source.data()+20+i*24;
        if(u16(r+14)!=3)continue;
        const unsigned size=u16(r+12)/8*u16(r+10);
        const auto* data=source.data()+u32(r+16);
        const unsigned char *low,*high,*mask;
        assert(atlas.find(data,size,low,high,mask));
        assert(!std::memcmp(low,data,size)&&!std::memcmp(high,data+size,size)&&!std::memcmp(mask,data+2*size,size));
        assert(!atlas.find(data,source.size()+2,low,high,mask));
        ++checked;
    }
    // Invalid rebinding drops the old certificate, not just the new request.
    bank[16]^=1;
    assert(!atlas.bind(source.data(),source.size(),bank.data(),bank.size(),downwellGamePlaneSourceHash));
    assert(!atlas.contains(source.data()));bank[16]^=1;
    auto broken=bank;broken[48+8]|=0x80; // DMA pointer beyond the resident bank.
    assert(!atlas.bind(source.data(),source.size(),broken.data(),broken.size(),downwellGamePlaneSourceHash));
    assert(!atlas.bind(source.data(),source.size()-1,bank.data(),bank.size(),downwellGamePlaneSourceHash));
    assert(!atlas.bind(source.data(),source.size(),bank.data(),49,downwellGamePlaneSourceHash));
    std::printf("Plane atlas: %u asset views, all three planes byte-exact; invalid banks rejected\n",checked);
}

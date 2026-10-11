#include "platform/amiga/AmigaPlaneBank.h"
#include "platform/amiga/AmigaGameplayAssetAdmission.h"
#include "../../../generated/amiga-game-planes.h"
#include "../../../generated/amiga-cavern-planes.h"
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
static void check(const char* sourcePath,const char* bankPath,unsigned expectedBytes,const unsigned char* hash){
    const auto source=load(sourcePath);auto bank=load(bankPath);
    AmigaPlaneBank atlas;
    assert(bank.size()==expectedBytes);
    assert(atlas.bind(source.data(),source.size(),bank.data(),bank.size(),hash));
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
    assert(!atlas.bind(source.data(),source.size(),bank.data(),bank.size(),hash));
    assert(!atlas.contains(source.data()));bank[16]^=1;
    auto broken=bank;broken[48+8]|=0x80; // DMA pointer beyond the resident bank.
    assert(!atlas.bind(source.data(),source.size(),broken.data(),broken.size(),hash));
    assert(!atlas.bind(source.data(),source.size()-1,bank.data(),bank.size(),hash));
    assert(!atlas.bind(source.data(),source.size(),bank.data(),49,hash));
    std::printf("Plane atlas: %u asset views, all three planes byte-exact; invalid banks rejected\n",checked);
}
int main(){
    for(int area=-10;area<10000;++area){
        assert(AmigaGameplayAssetAdmission::production(area)==(area==0||area==1));
        assert(AmigaGameplayAssetAdmission::referenceOracle(area)==(area>=0&&area<=5));
    }
    check("generated/amiga-game.dwap","generated/amiga-game.planes",DOWNWELL_GAME_PLANE_BYTES,downwellGamePlaneSourceHash);
    check("generated/amiga-cavern.dwap","generated/amiga-cavern.planes",DOWNWELL_CAVERN_PLANE_BYTES,downwellCavernPlaneSourceHash);
    auto full=load("generated/amiga-game.dwap"),cavern=load("generated/amiga-cavern.dwap");
    auto fullPlanes=load("generated/amiga-game.planes"),cavernPlanes=load("generated/amiga-cavern.planes");
    AmigaPlaneBank atlas;
    assert(!atlas.bind(cavern.data(),cavern.size(),fullPlanes.data(),fullPlanes.size(),downwellCavernPlaneSourceHash));
    assert(!atlas.bind(full.data(),full.size(),cavernPlanes.data(),cavernPlanes.size(),downwellGamePlaneSourceHash));
}
